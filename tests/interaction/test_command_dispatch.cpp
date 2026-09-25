/// @file test_command_dispatch.cpp
/// @brief UIActionHandler 按命令 ID / 别名 / legacy 枚举启动命令的单测
///
/// 覆盖阶段4 字符串命令 ID 的入口（doc/ARCHITECTURE_EVOLUTION_PLAN.md
/// 7.10 节）：activateCommand 按 ID 启动命令、触发源透传为
/// CommandContext::sender；keycode() 在 keyconfig.xml 查不到时按注册表别名
/// 启动；setCurrentAction(DM::ActionType) 经 legacy 桥接走同一条路径。
///
/// 没有打开文档（UIActionHandler 没有视图）时即时命令照常执行——测试用即时
/// 命令观察命令是否真的被启动。

#include <gtest/gtest.h>

#include <QObject>

#include <memory>

#include "BaseExclusiveCommand.h"
#include "CommandRegistry.h"
#include "UIActionHandler.h"

TEST(CommandDispatchTest, activateCommand按ID执行并透传触发源)
{
    static int runs = 0;
    static QObject* receivedSender = nullptr;
    runs = 0;
    ASSERT_TRUE(CommandRegistry::instance().registerInstantCommand(
        "test.dispatch.activate",
        [](const CommandContext& ctx)
        {
            ++runs;
            receivedSender = ctx.sender;
        }));

    UIActionHandler handler(nullptr);
    QObject source;
    handler.activateCommand("test.dispatch.activate", &source);
    EXPECT_EQ(runs, 1);
    EXPECT_EQ(receivedSender, &source);

    // 未注册的命令什么也不做。
    handler.activateCommand("test.dispatch.missing");
    EXPECT_EQ(runs, 1);
}

TEST(CommandDispatchTest, keycode在keyconfig之外按注册表别名启动)
{
    static int runs = 0;
    runs = 0;
    ASSERT_TRUE(CommandRegistry::instance().registerInstantCommand(
        "test.dispatch.alias", [](const CommandContext&) { ++runs; }, {.aliases = {"tdalias"}}));

    UIActionHandler handler(nullptr);
    EXPECT_TRUE(handler.keycode("TDALIAS"));
    EXPECT_EQ(runs, 1);

    EXPECT_FALSE(handler.keycode("tdunknown"));
    EXPECT_EQ(runs, 1);
}

TEST(CommandDispatchTest, setCurrentAction经legacy桥接走同一条路径)
{
    static int runs = 0;
    runs = 0;
    // ActionViewLayerTable 从未被任何内置命令注册（原 switch 里没有它的 case）。
    ASSERT_TRUE(CommandRegistry::instance().registerInstantCommand(
        DM::ActionViewLayerTable, "test.dispatch.legacy", [](const CommandContext&) { ++runs; }));

    UIActionHandler handler(nullptr);
    handler.setCurrentAction(DM::ActionViewLayerTable);
    EXPECT_EQ(runs, 1);
}

TEST(CommandDispatchTest, 没有视图时即时命令照常执行交互命令不启动)
{
    static int instantRuns = 0;
    static int factoryCalls = 0;
    static QObject* receivedSender = nullptr;
    instantRuns = 0;
    factoryCalls = 0;
    ASSERT_TRUE(CommandRegistry::instance().registerInstantCommand(
        "test.dispatch.instant",
        [](const CommandContext& ctx)
        {
            ++instantRuns;
            receivedSender = ctx.sender;
            EXPECT_EQ(ctx.view, nullptr);
        }));
    ASSERT_TRUE(CommandRegistry::instance().registerExclusiveCommand(
        "test.dispatch.exclusive",
        [](const CommandContext&) -> std::unique_ptr<IExclusiveCommand>
        {
            ++factoryCalls;
            return nullptr;
        }));

    UIActionHandler handler(nullptr);
    QObject source;
    handler.activateCommand("test.dispatch.instant", &source);
    EXPECT_EQ(instantRuns, 1);
    EXPECT_EQ(receivedSender, &source);

    // 交互命令由视图的命令总线运行：没有视图时连命令对象都不构造
    handler.activateCommand("test.dispatch.exclusive");
    EXPECT_EQ(factoryCalls, 0);
}
