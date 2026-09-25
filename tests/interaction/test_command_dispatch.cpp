/// @file test_command_dispatch.cpp
/// @brief UIActionHandler 按命令 ID / 别名启动命令的单测
///
/// 覆盖阶段4 字符串命令 ID 的入口（doc/ARCHITECTURE_EVOLUTION_PLAN.md
/// 7.10 节）：activateCommand 按 ID 启动命令、触发源透传为
/// CommandContext::sender；keycode() 先按 keyconfig.xml（以命令 ID 为键，业务工具化
/// 第四步）查，查不到时按注册表别名启动。
///
/// 没有打开文档（UIActionHandler 没有视图）时即时命令照常执行——测试用即时
/// 命令观察命令是否真的被启动。

#include <gtest/gtest.h>

#include <QObject>

#include <memory>

#include "BaseExclusiveCommand.h"
#include "CommandRegistry.h"
#include "Commands.h"
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

TEST(CommandDispatchTest, keycode按keyconfig的命令ID启动且优先于注册表别名)
{
    static int configuredRuns = 0;
    static int aliasRuns = 0;
    configuredRuns = 0;
    aliasRuns = 0;
    ASSERT_TRUE(CommandRegistry::instance().registerInstantCommand(
        "test.dispatch.configured", [](const CommandContext&) { ++configuredRuns; }));
    // 注册表别名与 keyconfig.xml 的别名重名：keyconfig.xml 优先
    ASSERT_TRUE(CommandRegistry::instance().registerInstantCommand(
        "test.dispatch.shadowed", [](const CommandContext&) { ++aliasRuns; }, {.aliases = {"tdconfigured"}}));
    COMMANDS->loadFromData({CommandKeys{"test.dispatch.configured", "Configured", {"tdconfigured"}}}, false);

    UIActionHandler handler(nullptr);
    EXPECT_TRUE(handler.keycode("tdconfigured"));
    EXPECT_EQ(configuredRuns, 1);
    EXPECT_EQ(aliasRuns, 0);
    EXPECT_EQ(COMMANDS->description("test.dispatch.configured"), QStringLiteral("Configured"));
}

TEST(CommandDispatchTest, keyconfig认领但没有实现的命令按已识别处理)
{
    // 如对应的扩展没有加载：命令 ID 不在注册表里，按键被认领、什么也不做
    COMMANDS->loadFromData({CommandKeys{"test.dispatch.unregistered", "", {"tdunregistered"}}}, false);

    UIActionHandler handler(nullptr);
    EXPECT_TRUE(handler.keycode("tdunregistered"));
    EXPECT_FALSE(CommandRegistry::instance().hasCommand("test.dispatch.unregistered"));
}

TEST(CommandDispatchTest, 结束全部命令是宿主处理的内置命令)
{
    // 原 DM::ActionEditKillAllActions：不进注册表，没有视图时什么也不做
    COMMANDS->loadFromData({CommandKeys{"edit.kill_all", "", {"tdkillall"}}}, false);

    UIActionHandler handler(nullptr);
    EXPECT_FALSE(CommandRegistry::instance().hasCommand("edit.kill_all"));
    EXPECT_TRUE(handler.keycode("tdkillall"));
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
