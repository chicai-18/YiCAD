/// @file test_command_registry.cpp
/// @brief CommandRegistry 的单测
///
/// 覆盖阶段4第一部分（doc/ARCHITECTURE_EVOLUTION_PLAN.md 7.4节任务①）的核心
/// 行为：字符串 ID 注册、legacy ActionType 桥接、重复注册被拒绝、
/// makeSelectFirstFactory 的两个分支。
///
/// CommandRegistry 是进程范围的单例，同一个测试二进制内的所有用例共享同一份
/// 注册表状态，且 gtest 不保证跨用例的严格声明顺序（如加 --gtest_shuffle）。
/// 因此每个用例都用互不相同的字符串 ID；涉及 DM::ActionType 的用例只用原
/// 153-case switch 从未处理过的枚举值（ActionScriptOpenIDE/ActionScriptRun/
/// ActionViewDraft）——阶段4后续几批迁移真实 Action 时不会用到它们，不会跟
/// 这里的注册撞车。"确认没有注册"类断言统一用 DM::ActionNone，它是枚举自带
/// 的"无效"哨兵，不会被任何真实命令注册。

#include <gtest/gtest.h>

#include "ActionInterface.h"
#include "ActionSelect.h"
#include "CommandRegistry.h"
#include "DmDocument.h"
#include "DmPoint.h"
#include "EntityTable.h"
#include "UIActionHandler.h"
#include "support/FakeDocumentView.h"

namespace
{
/// @brief 最小的 ActionInterface 测试替身，只用来验证工厂被正确调用。
class TestAction : public ActionInterface
{
public:
    TestAction(DmDocument* doc, IDocumentView* docView)
        : ActionInterface("TestAction", doc, docView)
    {
    }
};
}  // namespace

TEST(CommandRegistryTest, 按字符串ID注册并创建)
{
    DmDocument doc;
    FakeDocumentView view;
    ASSERT_TRUE(CommandRegistry::instance().registerCommand(
        "test.cr.plain",
        [](const CommandContext& ctx) -> ActionInterface*
        { return new TestAction(ctx.document, ctx.view); }));

    CommandContext ctx{&doc, &view, nullptr, nullptr};
    ActionInterface* a = CommandRegistry::instance().create(QStringLiteral("test.cr.plain"), ctx);
    ASSERT_NE(a, nullptr);
    EXPECT_NE(dynamic_cast<TestAction*>(a), nullptr);
    delete a;

    // 没有关联 legacy ActionType，也没有任何东西注册到 ActionNone。
    EXPECT_FALSE(CommandRegistry::instance().hasLegacyMapping(DM::ActionNone));
    EXPECT_EQ(CommandRegistry::instance().create(DM::ActionNone, ctx), nullptr);
}

TEST(CommandRegistryTest, legacy桥接按ActionType和字符串ID都能创建)
{
    DmDocument doc;
    FakeDocumentView view;
    ASSERT_TRUE(CommandRegistry::instance().registerLegacyCommand(
        DM::ActionScriptOpenIDE, "test.cr.legacy",
        [](const CommandContext& ctx) -> ActionInterface*
        { return new TestAction(ctx.document, ctx.view); }));

    EXPECT_TRUE(CommandRegistry::instance().hasLegacyMapping(DM::ActionScriptOpenIDE));

    CommandContext ctx{&doc, &view, nullptr, nullptr};
    ActionInterface* a = CommandRegistry::instance().create(DM::ActionScriptOpenIDE, ctx);
    ASSERT_NE(a, nullptr);
    EXPECT_NE(dynamic_cast<TestAction*>(a), nullptr);
    delete a;

    ActionInterface* b = CommandRegistry::instance().create(QStringLiteral("test.cr.legacy"), ctx);
    ASSERT_NE(b, nullptr);
    delete b;
}

TEST(CommandRegistryTest, 重复注册同一字符串ID被拒绝)
{
    ASSERT_TRUE(CommandRegistry::instance().registerCommand(
        "test.cr.dup_id",
        [](const CommandContext&) -> ActionInterface* { return nullptr; }));
    EXPECT_FALSE(CommandRegistry::instance().registerCommand(
        "test.cr.dup_id",
        [](const CommandContext&) -> ActionInterface* { return nullptr; }));
}

TEST(CommandRegistryTest, 重复注册同一legacyActionType被拒绝且不留半成品)
{
    ASSERT_TRUE(CommandRegistry::instance().registerLegacyCommand(
        DM::ActionScriptRun, "test.cr.dup_legacy_1",
        [](const CommandContext&) -> ActionInterface* { return nullptr; }));
    EXPECT_FALSE(CommandRegistry::instance().registerLegacyCommand(
        DM::ActionScriptRun, "test.cr.dup_legacy_2",
        [](const CommandContext&) -> ActionInterface* { return nullptr; }));

    // 第二次调用在校验 legacyType 冲突时就应该短路，不该把
    // "test.cr.dup_legacy_2" 也注册进字符串表。
    DmDocument doc;
    FakeDocumentView view;
    CommandContext ctx{&doc, &view, nullptr, nullptr};
    EXPECT_EQ(CommandRegistry::instance().create(QStringLiteral("test.cr.dup_legacy_2"), ctx), nullptr);
}

TEST(CommandRegistryTest, makeSelectFirstFactory_未选中时建ActionSelect)
{
    DmDocument doc;  // 空文档，必然没有选中实体
    FakeDocumentView view;
    UIActionHandler handler(nullptr);

    CommandFactory factory = makeSelectFirstFactory(
        DM::ActionViewDraft,  // 占位的"选择完成后"动作类型，本用例不关心具体值
        [](const CommandContext& ctx) -> ActionInterface*
        { return new TestAction(ctx.document, ctx.view); });

    CommandContext ctx{&doc, &view, &handler, nullptr};
    ActionInterface* a = factory(ctx);
    ASSERT_NE(a, nullptr);
    EXPECT_NE(dynamic_cast<ActionSelect*>(a), nullptr);
    EXPECT_EQ(a->getEntityType(), DM::ActionSelect);
    delete a;
}

TEST(CommandRegistryTest, makeSelectFirstFactory_已选中时建真正Action)
{
    DmDocument doc;
    FakeDocumentView view;
    UIActionHandler handler(nullptr);

    // add_direct 绕开撤销/重做的 Cmd 机制，直接把实体放进表——一个裸的默认
    // 构造 DmDocument 没有完整的应用上下文，走 add() 的 Cmd 路径会崩溃。
    DmPoint* p = new DmPoint(nullptr, PointData(DmVector(0.0, 0.0)));
    ASSERT_TRUE(doc.getEntityTable()->add_direct(p));
    p->setSelected(true);
    ASSERT_TRUE(doc.getEntityTable()->hasSelect());

    CommandFactory factory = makeSelectFirstFactory(
        DM::ActionViewDraft,
        [](const CommandContext& ctx) -> ActionInterface*
        { return new TestAction(ctx.document, ctx.view); });

    CommandContext ctx{&doc, &view, &handler, nullptr};
    ActionInterface* a = factory(ctx);
    ASSERT_NE(a, nullptr);
    EXPECT_NE(dynamic_cast<TestAction*>(a), nullptr);
    delete a;
}
