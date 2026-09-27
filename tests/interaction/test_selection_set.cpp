/// @file test_selection_set.cpp
/// @brief 文档的选择集 SelectionSet（选择集移出 Model 第 3 步）
///
/// 第 3 步把 model/edit/Selection 并入 Application 层的 SelectionSet，选中状态暂时仍存在实体的
/// FlagSelected 位上（doc/SELECTION_SET_PLAN.md 第 4 节）。这里锁住现在的语义，第 4 步改存 DmId 后
/// 应当照样通过：框选可反选（原在 test_geometry_spatial_query，随 Selection 移来）、锁定图层上的实体
/// 选不中、不可见的实体不算选中但记录保留、entities() 按实体表的顺序。修改后的通知见
/// test_document_listener.cpp。
///
/// 实体用 EntityTable::add_direct 放进表，不经事务。

#include <gtest/gtest.h>

#include <vector>

#include "DmDocument.h"
#include "DmLayer.h"
#include "DmLayerTable.h"
#include "DmLine.h"
#include "EntityTable.h"
#include "SelectionSet.h"

namespace
{
/// @brief 一份空文档与它的选择集
struct SelectionSetFixture : ::testing::Test
{
    DmDocument doc;
    SelectionSet selection{doc};

    /// @brief 在 "0" 图层上加一条直线
    DmLine* addLine(const DmVector& a, const DmVector& b)
    {
        auto* line = new DmLine(a, b);
        line->setDocument(&doc);
        line->setLayer(QStringLiteral("0"));
        line->calculateBorders();
        EXPECT_TRUE(doc.getEntityTable()->add_direct(line));
        return line;
    }
};
}  // namespace

TEST_F(SelectionSetFixture, 框选可反选)
{
    DmLine* line = addLine(DmVector(1.0, 1.0), DmVector(2.0, 2.0));

    selection.selectWindow(DmVector(0.0, 0.0), DmVector(5.0, 5.0), true, false);
    EXPECT_EQ(selection.entities(), std::vector<DmEntity*>{line});

    selection.selectWindow(DmVector(0.0, 0.0), DmVector(5.0, 5.0), false, false);
    EXPECT_TRUE(selection.isEmpty());
}

TEST_F(SelectionSetFixture, 锁定图层上的实体选不中)
{
    DmLine* line = addLine(DmVector(0.0, 0.0), DmVector(10.0, 0.0));
    DmLayer* layer = doc.getLayerTable()->find(QStringLiteral("0"));
    ASSERT_NE(layer, nullptr);
    layer->lock(true);

    selection.add(line);
    EXPECT_FALSE(selection.contains(line));
    selection.toggle(line);
    EXPECT_FALSE(selection.contains(line));
    selection.selectAll();
    EXPECT_FALSE(selection.contains(line));
    selection.selectWindow(DmVector(-1.0, -1.0), DmVector(11.0, 1.0));
    EXPECT_FALSE(selection.contains(line));
    EXPECT_EQ(selection.count(), 0);
}

TEST_F(SelectionSetFixture, 不可见的实体不算选中但重新可见后仍选中)
{
    // 关掉图层后重新打开，实体仍显示为选中（方案第 5 节）
    DmLine* line = addLine(DmVector(0.0, 0.0), DmVector(10.0, 0.0));
    selection.add(line);

    line->setVisible(false);
    EXPECT_FALSE(selection.contains(line));
    EXPECT_TRUE(selection.isEmpty());
    EXPECT_TRUE(selection.entities().empty());

    line->setVisible(true);
    EXPECT_TRUE(selection.contains(line));
    EXPECT_EQ(selection.count(), 1);
}

TEST_F(SelectionSetFixture, 选中的实体按实体表的顺序给出)
{
    DmLine* first = addLine(DmVector(0.0, 0.0), DmVector(10.0, 0.0));
    DmLine* second = addLine(DmVector(0.0, 5.0), DmVector(10.0, 5.0));
    DmLine* third = addLine(DmVector(0.0, 9.0), DmVector(10.0, 9.0));

    selection.add(third);
    selection.add(first);
    EXPECT_EQ(selection.entities(), (std::vector<DmEntity*>{first, third}));
    EXPECT_EQ(selection.count(), 2);
    EXPECT_FALSE(selection.contains(second));

    selection.remove(first);
    EXPECT_EQ(selection.entities(), std::vector<DmEntity*>{third});
    selection.clear();
    EXPECT_TRUE(selection.isEmpty());
}
