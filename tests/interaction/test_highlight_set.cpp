/// @file test_highlight_set.cpp
/// @brief 视图的高亮集 HighlightSet（doc/HIGHLIGHT_SET_PLAN.md 第 1 步）
///
/// 锁住 3.3 节的语义：只接受当前实体表里的实体（不在表里的、克隆出来的、拾取到的子实体、已删除的
/// 都不加入）、内容不变时不发 changed()、已删除与不可见的实体不交出、entities() 按实体表的顺序、
/// 实体被实体表释放后查询不出错，以及命令结束时由视图清空（D2）。
///
/// 实体用 EntityTable::add_direct 放进表，删除走事务（同 test_selection_set）。UIView 不在单测里构造，
/// 命令结束时清空经 TestCommandHost（与 UIView 相同的做法）验证，同 test_exclusive_command_bus。

#include <gtest/gtest.h>

#include <list>
#include <memory>
#include <vector>

#include "BaseExclusiveCommand.h"
#include "DmDocument.h"
#include "DmLine.h"
#include "DmPolyline.h"
#include "EditTool.h"
#include "EntityTable.h"
#include "ExclusiveCommandBus.h"
#include "HighlightSet.h"
#include "PanZoomTool.h"
#include "PolylineData.h"
#include "Preview.h"
#include "SelectTool.h"
#include "SelectionSet.h"
#include "Snapper.h"
#include "Transaction.h"
#include "ViewToolControl.h"
#include "support/FakeDocumentView.h"
#include "support/TestCommandHost.h"

namespace
{
/// @brief 在文档的实体表里加一条直线
DmLine* addLine(DmDocument& doc, const DmVector& a, const DmVector& b)
{
    auto* line = new DmLine(a, b);
    line->setDocument(&doc);
    line->calculateBorders();
    EXPECT_TRUE(doc.getEntityTable()->add_direct(line));
    return line;
}

/// @brief 一份空文档与一个高亮集，数着 changed() 的次数
struct HighlightSetFixture : ::testing::Test
{
    DmDocument doc;
    HighlightSet highlight{doc};
    int changed = 0;

    HighlightSetFixture()
    {
        QObject::connect(&highlight, &HighlightSet::changed, [this]() { ++changed; });
    }

    DmLine* addLine(const DmVector& a, const DmVector& b) { return ::addLine(doc, a, b); }
};

/// @brief 激活时高亮一个实体的命令，像放置工具那样经 BaseExclusiveCommand::highlight() 取高亮集
class HighlightOnActivateCommand : public BaseExclusiveCommand
{
public:
    explicit HighlightOnActivateCommand(DmEntity* entity)
        : m_entity(entity)
    {
        setCommandId(QStringLiteral("test.highlight.probe"));
    }

protected:
    bool onActivate() override
    {
        highlight()->add(m_entity);
        return true;
    }
    void onDeactivate() override {}

private:
    DmEntity* m_entity;
};

/// @brief 与 UIView 相同的装配（同 test_exclusive_command_bus 的 BusFixture），高亮集由宿主持有
struct HighlightCommandFixture : ::testing::Test
{
    DmDocument doc;
    SelectionSet selection{doc};
    FakeDocumentView view;
    Preview preview{&selection, &view};
    Snapper snapper{&doc, &view};
    PanZoomTool panTool{&view};
    SelectTool selectTool{&doc, &selection, &view, &snapper, &preview, &panTool};
    EditTool editTool{&doc, &selection, &view, &snapper, &preview, &panTool};
    ViewToolControl control{&view};
    yicad_test::TestCommandHost host{doc, selection, view, control, selectTool, &editTool};
    ExclusiveCommandBus bus{host};

    HighlightCommandFixture()
    {
        control.setNavigationTool(&panTool);
        control.setSelectionTool(&selectTool);
        host.attach(bus);
    }
};
}  // namespace

TEST_F(HighlightSetFixture, 只接受当前实体表里的实体)
{
    highlight.add(nullptr);
    // 不在表里的实体、预览里的克隆 id 无效（"0"），不能记录，否则它们会彼此算作高亮
    DmLine loose(DmVector(0.0, 0.0), DmVector(1.0, 0.0));
    highlight.add(&loose);
    DmLine* line = addLine(DmVector(0.0, 0.0), DmVector(10.0, 0.0));
    std::unique_ptr<DmEntity> clone(line->clone());
    highlight.add(clone.get());
    EXPECT_FALSE(highlight.contains(&loose));
    EXPECT_FALSE(highlight.contains(clone.get()));
    EXPECT_TRUE(highlight.entities().empty());
    EXPECT_EQ(changed, 0);

    // 以 DM::ResolveAll 拾取到的子实体（这里是多段线的一段）不在实体表里，画笔也只看顶层实体
    const std::vector<DmVector> pts{DmVector(0.0, 5.0), DmVector(10.0, 5.0), DmVector(10.0, 9.0)};
    std::vector<double> weights(4, 0.0);
    auto* poly = new DmPolyline(nullptr, PolylineData(pts, std::vector<double>(2, 0.0), weights, false));
    poly->update();
    ASSERT_TRUE(doc.getEntityTable()->add_direct(poly));
    const std::list<DmEntity*> segments = poly->getSubEntities();
    ASSERT_FALSE(segments.empty());
    highlight.add(segments.front());
    EXPECT_FALSE(highlight.contains(segments.front()));
    EXPECT_FALSE(highlight.contains(poly));
    EXPECT_EQ(changed, 0);

    highlight.add(line);
    EXPECT_TRUE(highlight.contains(line));
    EXPECT_EQ(highlight.entities(), std::vector<DmEntity*>{line});
    EXPECT_EQ(changed, 1);
}

TEST_F(HighlightSetFixture, 内容不变时不发changed)
{
    // 悬停时光标在同一实体上移动，工具反复加入同一实体，不能每次都让视图重建缓存
    DmLine* line = addLine(DmVector(0.0, 0.0), DmVector(10.0, 0.0));
    DmLine* other = addLine(DmVector(0.0, 5.0), DmVector(10.0, 5.0));

    highlight.add(line);
    highlight.add(line);
    EXPECT_EQ(changed, 1);

    // 空指针、不在集合里的实体什么也不做，调用处不用先判断
    highlight.remove(nullptr);
    highlight.remove(other);
    EXPECT_EQ(changed, 1);

    highlight.remove(line);
    EXPECT_EQ(changed, 2);
    highlight.remove(line);
    EXPECT_EQ(changed, 2);

    highlight.clear();
    EXPECT_EQ(changed, 2);
    highlight.add(other);
    highlight.clear();
    EXPECT_EQ(changed, 4);
    EXPECT_TRUE(highlight.entities().empty());
}

TEST_F(HighlightSetFixture, 已删除与不可见的实体不交出)
{
    DmLine* erased = addLine(DmVector(0.0, 0.0), DmVector(10.0, 0.0));
    DmLine* hidden = addLine(DmVector(0.0, 5.0), DmVector(10.0, 5.0));
    DmLine* shown = addLine(DmVector(0.0, 9.0), DmVector(10.0, 9.0));
    highlight.add(erased);
    highlight.add(hidden);
    highlight.add(shown);

    // 高亮集不监听文档，已删除的实体留在集合里，查询时过滤
    Transaction t("Delete", &doc);
    t.start();
    doc.getEntityTable()->remove(erased);
    t.commit();
    ASSERT_TRUE(erased->isErased());
    hidden->setVisible(false);

    EXPECT_FALSE(highlight.contains(erased));
    EXPECT_FALSE(highlight.contains(hidden));
    EXPECT_TRUE(highlight.contains(shown));
    EXPECT_EQ(highlight.entities(), std::vector<DmEntity*>{shown});
    EXPECT_EQ(highlight.highlightedEntities(), std::vector<DmEntity*>{shown});

    // 不可见的实体记录保留，重新可见后照样高亮
    hidden->setVisible(true);
    EXPECT_TRUE(highlight.contains(hidden));
    EXPECT_EQ(highlight.highlightedEntities(), (std::vector<DmEntity*>{hidden, shown}));

    // 已删除的实体不加入
    highlight.clear();
    changed = 0;
    highlight.add(erased);
    EXPECT_FALSE(highlight.contains(erased));
    EXPECT_EQ(changed, 0);
}

TEST_F(HighlightSetFixture, 高亮的实体按实体表的顺序给出)
{
    DmLine* first = addLine(DmVector(0.0, 0.0), DmVector(10.0, 0.0));
    DmLine* second = addLine(DmVector(0.0, 5.0), DmVector(10.0, 5.0));
    DmLine* third = addLine(DmVector(0.0, 9.0), DmVector(10.0, 9.0));

    highlight.add(third);
    highlight.add(first);
    EXPECT_EQ(highlight.entities(), (std::vector<DmEntity*>{first, third}));
    EXPECT_FALSE(highlight.contains(second));
    highlight.add(second);
    EXPECT_EQ(highlight.highlightedEntities(), (std::vector<DmEntity*>{first, second, third}));
}

TEST_F(HighlightSetFixture, 实体被实体表释放后查询不出错)
{
    DmLine* line = addLine(DmVector(0.0, 0.0), DmVector(10.0, 0.0));
    DmLine* other = addLine(DmVector(0.0, 5.0), DmVector(10.0, 5.0));
    highlight.add(line);
    highlight.add(other);

    // 撤销栈清理时实体表真正释放实体，不经文档通知；集合里留下的是 id，不是悬空的指针
    ASSERT_TRUE(doc.getEntityTable()->remove_direct(line));
    EXPECT_EQ(highlight.entities(), std::vector<DmEntity*>{other});
    EXPECT_EQ(highlight.highlightedEntities(), std::vector<DmEntity*>{other});
    EXPECT_TRUE(highlight.contains(other));

    // 新实体即使分配在同一地址也不算高亮：按 id 记录
    DmLine* another = addLine(DmVector(0.0, 0.0), DmVector(10.0, 0.0));
    EXPECT_FALSE(highlight.contains(another));

    changed = 0;
    highlight.clear();
    EXPECT_TRUE(highlight.entities().empty());
    EXPECT_EQ(changed, 1);
}

TEST_F(HighlightCommandFixture, 命令结束时视图清空高亮集)
{
    DmLine* line = addLine(doc, DmVector(0.0, 0.0), DmVector(10.0, 0.0));
    HighlightSet* highlight = host.highlight();
    int changed = 0;
    QObject::connect(highlight, &HighlightSet::changed, [&changed]() { ++changed; });

    ASSERT_TRUE(bus.start(std::make_unique<HighlightOnActivateCommand>(line)));
    // 命令经 BaseExclusiveCommand::highlight() 取到的是宿主（视图）的高亮集
    EXPECT_TRUE(highlight->contains(line));
    EXPECT_EQ(changed, 1);

    // 命令自己没有取消高亮，结束时由视图清空（D2）
    ASSERT_TRUE(bus.endCommand(CommandEndReason::Cancelled));
    EXPECT_FALSE(bus.hasActiveCommand());
    EXPECT_TRUE(highlight->entities().empty());
    EXPECT_EQ(changed, 2);
}
