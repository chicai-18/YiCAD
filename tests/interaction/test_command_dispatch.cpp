/// @file test_command_dispatch.cpp
/// @brief UIActionHandler 按命令 ID / 别名 / legacy 枚举启动命令的单测
///
/// 覆盖阶段4 字符串命令 ID 的入口（doc/ARCHITECTURE_EVOLUTION_PLAN.md
/// 7.10 节）：activateCommand 按 ID 构造并触发 Action、触发源透传为
/// CommandContext::sender；keycode() 在 keyconfig.xml 查不到时按注册表别名
/// 启动；setCurrentAction(DM::ActionType) 经 legacy 桥接走同一条路径。
///
/// 没有打开文档（UIActionHandler 没有视图）时，activateCommand 构造出的
/// Action 会被立即 trigger() 再删除——测试据此观察命令是否真的被启动。

#include <gtest/gtest.h>

#include <QObject>

#include "ActionInterface.h"
#include "CommandRegistry.h"
#include "UIActionHandler.h"

namespace
{
/// @brief 被触发时记录自己的命令 ID 的测试替身。
class RecordingAction : public ActionInterface
{
public:
    RecordingAction(QString* triggeredCommandId)
        : ActionInterface("RecordingAction", nullptr, nullptr), m_triggeredCommandId(triggeredCommandId)
    {
    }

    void trigger() override { *m_triggeredCommandId = getCommandId(); }

private:
    QString* m_triggeredCommandId;
};
}  // namespace

TEST(CommandDispatchTest, activateCommand按ID构造触发并透传触发源)
{
    static QString triggered;
    static QObject* receivedSender = nullptr;
    triggered.clear();
    ASSERT_TRUE(CommandRegistry::instance().registerCommand(
        "test.dispatch.activate",
        [](const CommandContext& ctx) -> ActionInterface*
        {
            receivedSender = ctx.sender;
            return new RecordingAction(&triggered);
        }));

    UIActionHandler handler(nullptr);
    QObject source;
    // 没有视图：构造后立即 trigger() 并删除，返回 nullptr。
    EXPECT_EQ(handler.activateCommand("test.dispatch.activate", &source), nullptr);
    EXPECT_EQ(triggered, QStringLiteral("test.dispatch.activate"));
    EXPECT_EQ(receivedSender, &source);

    // 未注册的命令什么也不做。
    triggered.clear();
    EXPECT_EQ(handler.activateCommand("test.dispatch.missing"), nullptr);
    EXPECT_TRUE(triggered.isEmpty());
}

TEST(CommandDispatchTest, keycode在keyconfig之外按注册表别名启动)
{
    static QString triggered;
    triggered.clear();
    ASSERT_TRUE(CommandRegistry::instance().registerCommand(
        "test.dispatch.alias",
        [](const CommandContext&) -> ActionInterface* { return new RecordingAction(&triggered); },
        {.aliases = {"tdalias"}}));

    UIActionHandler handler(nullptr);
    EXPECT_TRUE(handler.keycode("TDALIAS"));
    EXPECT_EQ(triggered, QStringLiteral("test.dispatch.alias"));

    triggered.clear();
    EXPECT_FALSE(handler.keycode("tdunknown"));
    EXPECT_TRUE(triggered.isEmpty());
}

TEST(CommandDispatchTest, setCurrentAction经legacy桥接走同一条路径)
{
    static QString triggered;
    triggered.clear();
    // ActionViewLayerTable 从未被任何内置命令注册（原 switch 里没有它的 case）。
    ASSERT_TRUE(CommandRegistry::instance().registerLegacyCommand(
        DM::ActionViewLayerTable, "test.dispatch.legacy",
        [](const CommandContext&) -> ActionInterface* { return new RecordingAction(&triggered); }));

    UIActionHandler handler(nullptr);
    EXPECT_EQ(handler.setCurrentAction(DM::ActionViewLayerTable), nullptr);
    EXPECT_EQ(triggered, QStringLiteral("test.dispatch.legacy"));
}
