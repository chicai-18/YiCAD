/// @file test_selection_set.cpp
/// @brief 文档的选择集 SelectionSet（doc/SELECTION_SET_PLAN.md 第 3、4 步）
///
/// 第 3 步锁住的语义，第 4 步改存 DmId 后照样成立：框选可反选（原在 test_geometry_spatial_query，
/// 随 Selection 移来）、锁定图层上的实体选不中、不可见的实体不算选中但记录保留、entities() 按
/// 实体表的顺序。第 4 步新增：只记当前实体表里的实体、取消全部连不可见的一起清掉、已删除的实体
/// 掉出（删除后撤销回来是未选中）、被撤销修改的实体留下、实体释放后不留悬空记录、进出块编辑清空。
/// 修改后发 changed() 而不通知文档，见 test_document_listener.cpp。
///
/// 实体用 EntityTable::add_direct 放进表；删除、修改的用例走事务与撤销（先开事务再改实体在
/// 默认构造的 DmDocument 上可以运行，见 test_modify_commands 的粘贴用例），块编辑直接执行
/// BlockEditEnterCmd/BlockEditExitCmd（同 test_document_listener）。

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "BlockEditCmd.h"
#include "DmBlock.h"
#include "DmBlockTable.h"
#include "DmDocument.h"
#include "DmLayer.h"
#include "DmLayerTable.h"
#include "DmLine.h"
#include "EntityTable.h"
#include "Modification.h"
#include "SelectionSet.h"
#include "Transaction.h"

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

TEST_F(SelectionSetFixture, 只记当前实体表里的实体)
{
    // 不在表里的实体、克隆出来的实体 id 无效，不能记录，否则它们会彼此算作选中
    DmLine loose(DmVector(0.0, 0.0), DmVector(1.0, 0.0));
    selection.add(&loose);
    EXPECT_FALSE(selection.contains(&loose));

    DmLine* line = addLine(DmVector(0.0, 0.0), DmVector(10.0, 0.0));
    selection.add(line);
    std::unique_ptr<DmEntity> clone(line->clone());
    EXPECT_TRUE(selection.contains(line));
    EXPECT_FALSE(selection.contains(clone.get()));
    EXPECT_EQ(selection.count(), 1);
}

TEST_F(SelectionSetFixture, 取消全部连不可见的实体一起清掉)
{
    // 原 selectAll(false) 只取消可见实体，关掉图层再取消全部、重新打开后实体仍选中（方案第 5 节）
    DmLine* line = addLine(DmVector(0.0, 0.0), DmVector(10.0, 0.0));
    selection.add(line);
    line->setVisible(false);

    selection.clear();
    line->setVisible(true);
    EXPECT_FALSE(selection.contains(line));
}

TEST_F(SelectionSetFixture, 已删除的实体掉出选择集撤销删除后回来是未选中)
{
    DmLine* line = addLine(DmVector(0.0, 0.0), DmVector(10.0, 0.0));
    DmLine* other = addLine(DmVector(0.0, 5.0), DmVector(10.0, 5.0));
    selection.add(line);
    selection.add(other);
    int changed = 0;
    QObject::connect(&selection, &SelectionSet::changed, [&changed]() { ++changed; });

    Transaction t("Delete", &doc);
    t.start();
    doc.getEntityTable()->remove(line);
    t.commit();
    ASSERT_TRUE(line->isErased());
    // 提交时文档通知修改，选择集剔除已删除的实体并通知一次
    EXPECT_EQ(selection.entities(), std::vector<DmEntity*>{other});
    EXPECT_EQ(changed, 1);

    doc.undo();
    ASSERT_FALSE(line->isErased());
    EXPECT_FALSE(selection.contains(line));
    EXPECT_TRUE(selection.contains(other));
}

TEST_F(SelectionSetFixture, 被撤销修改的实体仍然选中)
{
    // 原先修改的撤销命令构造时取消选中，撤销后实体总是未选中（方案第 5 节，D2）
    DmLine* line = addLine(DmVector(0.0, 0.0), DmVector(10.0, 0.0));
    selection.add(line);

    Modification modification(&doc);
    modification.move({line}, DmVector(5.0, 0.0));
    ASSERT_EQ(line->getStartpoint(), DmVector(5.0, 0.0));
    EXPECT_TRUE(selection.contains(line));

    doc.undo();
    ASSERT_EQ(line->getStartpoint(), DmVector(0.0, 0.0));
    EXPECT_TRUE(selection.contains(line));
    doc.redo();
    EXPECT_TRUE(selection.contains(line));
}

TEST_F(SelectionSetFixture, 实体释放后不留悬空记录)
{
    DmLine* line = addLine(DmVector(0.0, 0.0), DmVector(10.0, 0.0));
    selection.add(line);

    // 撤销栈清理时实体表真正释放实体，不经文档通知
    ASSERT_TRUE(doc.getEntityTable()->remove_direct(line));
    EXPECT_EQ(selection.count(), 0);
    EXPECT_TRUE(selection.isEmpty());
    EXPECT_TRUE(selection.entities().empty());
    EXPECT_FALSE(selection.nearestRef(DmVector(0.0, 0.0)).valid);

    // 新实体即使分配在同一地址也不算选中：选择集按 id 记录
    DmLine* another = addLine(DmVector(0.0, 0.0), DmVector(10.0, 0.0));
    EXPECT_FALSE(selection.contains(another));
}

TEST_F(SelectionSetFixture, 进出块编辑时清空)
{
    DmLine* line = addLine(DmVector(0.0, 0.0), DmVector(10.0, 0.0));
    selection.add(line);
    DmBlockData data;
    data.name = QStringLiteral("B");
    doc.getBlockTable()->add_direct(new DmBlock(&doc, data));
    int changed = 0;
    QObject::connect(&selection, &SelectionSet::changed, [&changed]() { ++changed; });

    BlockEditEnterCmd enter(&doc, QStringLiteral("B"));
    enter.execute();
    EXPECT_TRUE(selection.isEmpty());
    EXPECT_EQ(changed, 1);

    // 块编辑时当前实体表是块的，选的是块里的实体
    DmLine* inner = addLine(DmVector(0.0, 0.0), DmVector(1.0, 1.0));
    selection.add(inner);
    EXPECT_EQ(selection.entities(), std::vector<DmEntity*>{inner});

    BlockEditExitCmd exit(&doc, QStringLiteral("B"), false);
    exit.execute();
    EXPECT_TRUE(selection.isEmpty());
    EXPECT_FALSE(selection.contains(line));
    EXPECT_EQ(changed, 3);
}
