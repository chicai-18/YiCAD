/// @file test_command_registry.cpp
/// @brief CommandRegistry 的单测
///
/// 覆盖阶段4第一部分（doc/ARCHITECTURE_EVOLUTION_PLAN.md 7.4节任务①）的核心
/// 行为：字符串 ID 注册、legacy ActionType 桥接、重复注册被拒绝；以及业务
/// 工具化第二步新增的交互命令、即时命令两类注册（doc/COMMAND_TOOL_MIGRATION_PLAN.md
/// 第二步第 2 项），第三步新增的临时视图工具注册与即时命令的打断策略。
/// makeSelectFirstFactory 随先选后建命令的迁移删除。
///
/// CommandRegistry 是进程范围的单例，同一个测试二进制内的所有用例共享同一份
/// 注册表状态，且 gtest 不保证跨用例的严格声明顺序（如加 --gtest_shuffle）。
/// 因此每个用例都用互不相同的字符串 ID；涉及 DM::ActionType 的用例只用原
/// 153-case switch 从未处理过的枚举值（ActionScriptOpenIDE/ActionScriptRun/
/// ActionViewDraft）——阶段4后续几批迁移真实 Action 时不会用到它们，不会跟
/// 这里的注册撞车。"确认没有注册"类断言统一用 DM::ActionNone，它是枚举自带
/// 的"无效"哨兵，不会被任何真实命令注册。

#include <gtest/gtest.h>

#include <memory>
#include <utility>

#include "ActionInterface.h"
#include "BaseExclusiveCommand.h"
#include "CommandRegistry.h"
#include "DmDocument.h"
#include "DmPoint.h"
#include "EntityTable.h"
#include "TransientViewTool.h"
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

/// @brief 最小的交互命令测试替身
class TestCommand : public BaseExclusiveCommand
{
protected:
    bool onActivate() override { return true; }
    void onDeactivate() override {}
};

ExclusiveCommandFactory testCommandFactory()
{
    return [](const CommandContext&) -> std::unique_ptr<IExclusiveCommand> { return std::make_unique<TestCommand>(); };
}

/// @brief 最小的临时视图工具测试替身
class TestViewTool : public TransientViewTool
{
};

ViewToolFactory testViewToolFactory()
{
    return [](const CommandContext&) -> std::unique_ptr<TransientViewTool> { return std::make_unique<TestViewTool>(); };
}
}  // namespace

TEST(CommandRegistryTest, 按字符串ID注册并创建)
{
    DmDocument doc;
    FakeDocumentView view;
    ASSERT_TRUE(CommandRegistry::instance().registerCommand(
        "test.cr.plain",
        [](const CommandContext& ctx) -> ActionInterface*
        { return new TestAction(ctx.document, ctx.view); }));

    CommandContext ctx{&doc, &view, nullptr};
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

    CommandContext ctx{&doc, &view, nullptr};
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
    CommandContext ctx{&doc, &view, nullptr};
    EXPECT_EQ(CommandRegistry::instance().create(QStringLiteral("test.cr.dup_legacy_2"), ctx), nullptr);
}

TEST(CommandRegistryTest, 按字符串ID创建的Action记录命令ID)
{
    DmDocument doc;
    FakeDocumentView view;
    ASSERT_TRUE(CommandRegistry::instance().registerCommand(
        "test.cr.command_id",
        [](const CommandContext& ctx) -> ActionInterface*
        { return new TestAction(ctx.document, ctx.view); }));

    CommandContext ctx{&doc, &view, nullptr};
    ActionInterface* a = CommandRegistry::instance().create(QStringLiteral("test.cr.command_id"), ctx);
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(a->getCommandId(), QStringLiteral("test.cr.command_id"));
    delete a;

    // 直接 new 出来的 Action 没有命令 ID。
    TestAction direct(&doc, &view);
    EXPECT_TRUE(direct.getCommandId().isEmpty());
}

TEST(CommandRegistryTest, 别名大小写不敏感并随说明与选项条一起登记)
{
    CommandInfo info;
    info.description = QStringLiteral("Alias test");
    info.aliases = QStringList{QStringLiteral(" CrFoo "), QStringLiteral("crbar"), QStringLiteral("crbar"), QString()};
    info.optionsFactory = [](QWidget*, ActionInterface*, bool) -> QWidget* { return nullptr; };
    ASSERT_TRUE(CommandRegistry::instance().registerCommand(
        "test.cr.alias", [](const CommandContext&) -> ActionInterface* { return nullptr; }, info));

    EXPECT_EQ(CommandRegistry::instance().commandForAlias("crfoo"), QStringLiteral("test.cr.alias"));
    EXPECT_EQ(CommandRegistry::instance().commandForAlias("CRBAR"), QStringLiteral("test.cr.alias"));
    EXPECT_TRUE(CommandRegistry::instance().commandForAlias("crbaz").isEmpty());
    EXPECT_TRUE(CommandRegistry::instance().aliases().contains(QStringLiteral("crfoo")));
    EXPECT_EQ(CommandRegistry::instance().description("test.cr.alias"), QStringLiteral("Alias test"));
    EXPECT_TRUE(static_cast<bool>(CommandRegistry::instance().optionsFactory("test.cr.alias")));
    EXPECT_FALSE(static_cast<bool>(CommandRegistry::instance().optionsFactory("test.cr.plain")));
}

TEST(CommandRegistryTest, 别名冲突时整条命令被拒绝且不留半成品)
{
    ASSERT_TRUE(CommandRegistry::instance().registerCommand(
        "test.cr.alias_owner", [](const CommandContext&) -> ActionInterface* { return nullptr; },
        {.aliases = {"crtaken"}}));
    EXPECT_FALSE(CommandRegistry::instance().registerCommand(
        "test.cr.alias_thief", [](const CommandContext&) -> ActionInterface* { return nullptr; },
        {.aliases = {"crfree", "CRTAKEN"}}));

    EXPECT_FALSE(CommandRegistry::instance().hasCommand("test.cr.alias_thief"));
    EXPECT_TRUE(CommandRegistry::instance().commandForAlias("crfree").isEmpty());
    EXPECT_EQ(CommandRegistry::instance().commandForAlias("crtaken"), QStringLiteral("test.cr.alias_owner"));
}

TEST(CommandRegistryTest, 注销清除命令别名与legacy桥接)
{
    // ActionViewStatusBar 从未被任何内置命令注册（原 switch 里没有它的 case）。
    ASSERT_TRUE(CommandRegistry::instance().registerLegacyCommand(
        DM::ActionViewStatusBar, "test.cr.unregister",
        [](const CommandContext&) -> ActionInterface* { return nullptr; }));
    ASSERT_EQ(CommandRegistry::instance().commandId(DM::ActionViewStatusBar), QStringLiteral("test.cr.unregister"));

    EXPECT_TRUE(CommandRegistry::instance().unregisterCommand("test.cr.unregister"));
    EXPECT_FALSE(CommandRegistry::instance().hasCommand("test.cr.unregister"));
    EXPECT_FALSE(CommandRegistry::instance().hasLegacyMapping(DM::ActionViewStatusBar));
    EXPECT_TRUE(CommandRegistry::instance().commandId(DM::ActionViewStatusBar).isEmpty());
    EXPECT_FALSE(CommandRegistry::instance().unregisterCommand("test.cr.unregister"));

    ASSERT_TRUE(CommandRegistry::instance().registerCommand(
        "test.cr.unregister_alias", [](const CommandContext&) -> ActionInterface* { return nullptr; },
        {.aliases = {"crgone"}}));
    EXPECT_TRUE(CommandRegistry::instance().unregisterCommand("test.cr.unregister_alias"));
    EXPECT_TRUE(CommandRegistry::instance().commandForAlias("crgone").isEmpty());
}

TEST(CommandRegistryTest, 交互命令按ID创建并记录命令ID)
{
    ASSERT_TRUE(CommandRegistry::instance().registerExclusiveCommand("test.cr.exclusive", testCommandFactory()));
    EXPECT_EQ(CommandRegistry::instance().kind("test.cr.exclusive"), CommandKind::Exclusive);

    CommandContext ctx{};
    std::unique_ptr<IExclusiveCommand> command = CommandRegistry::instance().createCommand("test.cr.exclusive", ctx);
    ASSERT_NE(command, nullptr);
    EXPECT_EQ(command->commandId(), QStringLiteral("test.cr.exclusive"));

    // 类型不对的入口都返回空：交互命令不能当旧版 Action 创建，也不能当即时命令执行
    EXPECT_EQ(CommandRegistry::instance().create(QStringLiteral("test.cr.exclusive"), ctx), nullptr);
    EXPECT_FALSE(CommandRegistry::instance().runInstant("test.cr.exclusive", ctx));
}

TEST(CommandRegistryTest, 即时命令按ID执行)
{
    static int runs = 0;
    runs = 0;
    ASSERT_TRUE(CommandRegistry::instance().registerInstantCommand(
        "test.cr.instant", [](const CommandContext&) { ++runs; }));
    EXPECT_EQ(CommandRegistry::instance().kind("test.cr.instant"), CommandKind::Instant);

    CommandContext ctx{};
    EXPECT_TRUE(CommandRegistry::instance().runInstant("test.cr.instant", ctx));
    EXPECT_EQ(runs, 1);
    EXPECT_EQ(CommandRegistry::instance().createCommand("test.cr.instant", ctx), nullptr);
    EXPECT_FALSE(CommandRegistry::instance().runInstant("test.cr.instant_missing", ctx));
    EXPECT_EQ(CommandRegistry::instance().kind("test.cr.instant_missing"), CommandKind::None);
}

TEST(CommandRegistryTest, 空工厂与空函数被拒绝)
{
    EXPECT_FALSE(CommandRegistry::instance().registerExclusiveCommand("test.cr.null_exclusive", nullptr));
    EXPECT_FALSE(CommandRegistry::instance().registerInstantCommand("test.cr.null_instant", nullptr));
    EXPECT_FALSE(CommandRegistry::instance().hasCommand("test.cr.null_exclusive"));
    EXPECT_FALSE(CommandRegistry::instance().hasCommand("test.cr.null_instant"));
}

TEST(CommandRegistryTest, 三类命令共用ID与别名空间)
{
    ASSERT_TRUE(CommandRegistry::instance().registerExclusiveCommand(
        "test.cr.shared", testCommandFactory(), {.aliases = {"crshared"}}));
    // 同一 ID 不能再以另一类注册
    EXPECT_FALSE(CommandRegistry::instance().registerInstantCommand("test.cr.shared", [](const CommandContext&) {}));
    // 别名冲突时整条拒绝
    EXPECT_FALSE(CommandRegistry::instance().registerInstantCommand(
        "test.cr.shared_alias", [](const CommandContext&) {}, {.aliases = {"CRSHARED"}}));
    EXPECT_FALSE(CommandRegistry::instance().hasCommand("test.cr.shared_alias"));
    EXPECT_EQ(CommandRegistry::instance().commandForAlias("crshared"), QStringLiteral("test.cr.shared"));
}

TEST(CommandRegistryTest, 为交互命令建立legacy桥接并可反查)
{
    ASSERT_TRUE(CommandRegistry::instance().registerExclusiveCommand("test.cr.bind", testCommandFactory()));
    ASSERT_TRUE(CommandRegistry::instance().bindLegacyType(DM::ActionViewLibrary, "test.cr.bind"));
    EXPECT_EQ(CommandRegistry::instance().commandId(DM::ActionViewLibrary), QStringLiteral("test.cr.bind"));
    EXPECT_EQ(CommandRegistry::instance().legacyType("test.cr.bind"), DM::ActionViewLibrary);

    // 已桥接的枚举、未注册的 ID 都被拒绝
    EXPECT_FALSE(CommandRegistry::instance().bindLegacyType(DM::ActionViewLibrary, "test.cr.bind"));
    EXPECT_FALSE(CommandRegistry::instance().bindLegacyType(DM::ActionViewPenToolbar, "test.cr.bind_missing"));
    EXPECT_EQ(CommandRegistry::instance().legacyType("test.cr.bind_missing"), DM::ActionNone);

    // 旧版入口按枚举创建时不构造交互命令
    CommandContext ctx{};
    EXPECT_EQ(CommandRegistry::instance().create(DM::ActionViewLibrary, ctx), nullptr);
}

TEST(CommandRegistryTest, 迁移后的先选后建命令注册为新类型)
{
    // keyconfig.xml 仍以枚举为键，桥接保留到第四步
    const std::pair<DM::ActionType, const char*> migrated[] = {
        {DM::ActionModifyMove, "modify.move"},       {DM::ActionModifyCopy, "modify.copy"},
        {DM::ActionModifyRotate, "modify.rotate"},   {DM::ActionModifyScale, "modify.scale"},
        {DM::ActionModifyMirror, "modify.mirror"},   {DM::ActionModifyExplode, "modify.explode"},
        {DM::ActionModifyReverse, "modify.reverse"}, {DM::ActionModifyDelete, "modify.delete"},
        {DM::ActionEditCopy, "edit.copy"},           {DM::ActionEditCut, "edit.cut"},
        {DM::ActionCopyToLayer, "modify.copy_to_layer"}, {DM::ActionInfoTotalLength, "info.total_length"},
    };
    for (const auto& [type, id] : migrated)
    {
        SCOPED_TRACE(id);
        EXPECT_EQ(CommandRegistry::instance().kind(id), CommandKind::Exclusive);
        EXPECT_EQ(CommandRegistry::instance().commandId(type), QString::fromLatin1(id));
        // 原先只供 ActionSelect 选择完成后使用的 _no_select 入口随之删除（删除的除外，见下）
        if (type != DM::ActionModifyDelete)
        {
            EXPECT_FALSE(CommandRegistry::instance().hasCommand(QString::fromLatin1(id) + "_no_select"));
        }
    }
    // Delete 键与手写板橡皮擦用的直接删除保留为即时命令
    EXPECT_EQ(CommandRegistry::instance().kind("modify.delete_no_select"), CommandKind::Instant);
}

TEST(CommandRegistryTest, 临时视图工具按ID创建并记录命令ID)
{
    ASSERT_TRUE(CommandRegistry::instance().registerViewTool("test.cr.view_tool", testViewToolFactory()));
    EXPECT_EQ(CommandRegistry::instance().kind("test.cr.view_tool"), CommandKind::ViewTool);

    CommandContext ctx{};
    std::unique_ptr<TransientViewTool> tool = CommandRegistry::instance().createViewTool("test.cr.view_tool", ctx);
    ASSERT_NE(tool, nullptr);
    EXPECT_EQ(tool->commandId(), QStringLiteral("test.cr.view_tool"));

    // 类型不对的入口都返回空
    EXPECT_EQ(CommandRegistry::instance().create(QStringLiteral("test.cr.view_tool"), ctx), nullptr);
    EXPECT_EQ(CommandRegistry::instance().createCommand("test.cr.view_tool", ctx), nullptr);
    EXPECT_FALSE(CommandRegistry::instance().runInstant("test.cr.view_tool", ctx));
    EXPECT_EQ(CommandRegistry::instance().createViewTool("test.cr.exclusive_missing", ctx), nullptr);
    EXPECT_FALSE(CommandRegistry::instance().registerViewTool("test.cr.null_view_tool", nullptr));
}

TEST(CommandRegistryTest, 带legacy桥接的即时命令与临时视图工具)
{
    ASSERT_TRUE(CommandRegistry::instance().registerInstantCommand(
        DM::ActionViewBlockList, "test.cr.bridged_instant", [](const CommandContext&) {}));
    ASSERT_TRUE(CommandRegistry::instance().registerViewTool(
        DM::ActionViewCommandLine, "test.cr.bridged_view_tool", testViewToolFactory()));
    EXPECT_EQ(CommandRegistry::instance().commandId(DM::ActionViewBlockList), QStringLiteral("test.cr.bridged_instant"));
    EXPECT_EQ(CommandRegistry::instance().commandId(DM::ActionViewCommandLine),
              QStringLiteral("test.cr.bridged_view_tool"));

    // 已桥接的枚举被拒绝，且不留下注册
    EXPECT_FALSE(CommandRegistry::instance().registerInstantCommand(
        DM::ActionViewBlockList, "test.cr.bridged_instant_dup", [](const CommandContext&) {}));
    EXPECT_FALSE(CommandRegistry::instance().hasCommand("test.cr.bridged_instant_dup"));
    EXPECT_FALSE(CommandRegistry::instance().registerViewTool(
        DM::ActionViewCommandLine, "test.cr.bridged_view_tool_dup", testViewToolFactory()));
    EXPECT_FALSE(CommandRegistry::instance().hasCommand("test.cr.bridged_view_tool_dup"));
}

TEST(CommandRegistryTest, 即时命令的打断策略随注册登记)
{
    ASSERT_TRUE(CommandRegistry::instance().registerInstantCommand(
        "test.cr.keep_all", [](const CommandContext&) {}, {.instantInterrupt = InstantInterrupt::KeepAll}));
    ASSERT_TRUE(CommandRegistry::instance().registerInstantCommand("test.cr.default_interrupt",
                                                                   [](const CommandContext&) {}));
    EXPECT_EQ(CommandRegistry::instance().instantInterrupt("test.cr.keep_all"), InstantInterrupt::KeepAll);
    EXPECT_EQ(CommandRegistry::instance().instantInterrupt("test.cr.default_interrupt"),
              InstantInterrupt::EndUninterruptible);
    EXPECT_EQ(CommandRegistry::instance().instantInterrupt("test.cr.interrupt_missing"),
              InstantInterrupt::EndUninterruptible);
}

TEST(CommandRegistryTest, 第三步迁移的视图与即时命令注册为新类型)
{
    // 平移模式是临时视图工具，不占命令总线
    EXPECT_EQ(CommandRegistry::instance().kind("zoom.pan"), CommandKind::ViewTool);
    EXPECT_EQ(CommandRegistry::instance().commandId(DM::ActionZoomPan), QStringLiteral("zoom.pan"));

    const std::pair<DM::ActionType, const char*> instants[] = {
        {DM::ActionZoomIn, "zoom.in"},     {DM::ActionZoomOut, "zoom.out"},
        {DM::ActionEditUndo, "edit.undo"}, {DM::ActionEditRedo, "edit.redo"},
        {DM::ActionInfoSelected, "info.selected"},
    };
    for (const auto& [type, id] : instants)
    {
        SCOPED_TRACE(id);
        EXPECT_EQ(CommandRegistry::instance().kind(id), CommandKind::Instant);
        EXPECT_EQ(CommandRegistry::instance().commandId(type), QString::fromLatin1(id));
    }
    // 原视图 Action 不打断任何命令（多行文字编辑中缩放不结束它）；撤销等照旧结束不可打断的
    EXPECT_EQ(CommandRegistry::instance().instantInterrupt("zoom.in"), InstantInterrupt::KeepAll);
    EXPECT_EQ(CommandRegistry::instance().instantInterrupt("zoom.out"), InstantInterrupt::KeepAll);
    EXPECT_EQ(CommandRegistry::instance().instantInterrupt("edit.undo"), InstantInterrupt::EndUninterruptible);
}
