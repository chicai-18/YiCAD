/// @file test_exclusive_command_bus.cpp
/// @brief 命令总线 ExclusiveCommandBus 的单元测试
///
/// 覆盖 doc/COMMAND_TOOL_MIGRATION_PLAN.md 第二步第 6 项的前两组：
///   - 生命周期：启动、替换、结束、激活失败、激活期间就完成、延迟销毁；
///   - 结束前回调（5.1 节）：三种原因、否决与不否决、ViewClosing 忽略否决、
///     回调期间的重入请求被忽略；
/// 以及过渡期的挂起/恢复、选择阶段约束由总线清除、捕捉设置同步。

#include <gtest/gtest.h>

#include <deque>
#include <functional>
#include <memory>
#include <vector>

#include <QCoreApplication>
#include <QMouseEvent>

#include "BaseExclusiveCommand.h"
#include "DmDocument.h"
#include "ExclusiveCommandBus.h"
#include "IViewTool.h"
#include "PanZoomTool.h"
#include "Preview.h"
#include "SelectTool.h"
#include "Snapper.h"
#include "ViewToolControl.h"
#include "support/FakeDocumentView.h"

namespace
{
/// @brief 探针命令的记录；命令由总线销毁，记录放在命令之外
struct CommandLog
{
    int activated = 0;
    int deactivated = 0;
    int suspended = 0;
    int resumed = 0;
    int destroyed = 0;
    std::vector<CommandEndReason> endRequests;
};

/// @brief 记录生命周期回调的探针命令
class ProbeCommand : public BaseExclusiveCommand
{
public:
    explicit ProbeCommand(CommandLog& log)
        : m_log(log)
    {
    }
    ~ProbeCommand() override { ++m_log.destroyed; }

    bool veto = false;                   ///< onEndRequested() 是否否决
    bool activateResult = true;          ///< onActivate() 的返回值
    bool finishOnActivate = false;       ///< 激活期间就请求结束
    bool selectOnActivate = false;       ///< 激活时进入选择阶段
    std::function<void()> duringEndRequest; ///< 在 onEndRequested() 里做的事
    std::unique_ptr<Snapper> snapper;    ///< snapService() 返回它

    bool onEndRequested(CommandEndReason reason) override
    {
        m_log.endRequests.push_back(reason);
        if (duringEndRequest)
        {
            duringEndRequest();
        }
        return !veto;
    }
    void suspend() override { ++m_log.suspended; }
    void resume() override { ++m_log.resumed; }
    ISnapService* snapService() const override { return snapper.get(); }

protected:
    bool onActivate() override
    {
        ++m_log.activated;
        if (selectOnActivate)
        {
            enterSelectionPhase();
        }
        if (finishOnActivate)
        {
            finish();
        }
        return activateResult;
    }
    void onDeactivate() override { ++m_log.deactivated; }

private:
    CommandLog& m_log;
};

/// @brief 与 UIView 相同的装配：导航层、选择层、工具控制器与总线
struct BusFixture : ::testing::Test
{
    /// @brief 探针命令的记录。放在夹具里、声明在总线之前：用例结束时仍活动的命令
    ///        由夹具析构总线时销毁，那时用例体里的局部变量已经不在了
    std::deque<CommandLog> logs;
    DmDocument doc;
    FakeDocumentView view;
    Preview preview{&doc, &view};
    Snapper snapper{&doc, &view};
    PanZoomTool panTool{&view};
    SelectTool selectTool{&doc, &view, &snapper, &preview, &panTool};
    ViewToolControl control{&view};
    ExclusiveCommandBus bus{&doc, &view, &control, &selectTool};

    BusFixture()
    {
        control.setNavigationTool(&panTool);
        control.setSelectionTool(&selectTool);
    }

    /// @brief 新建一份探针命令的记录
    CommandLog& newLog() { return logs.emplace_back(); }

    /// @brief 构造一个探针命令
    std::unique_ptr<ProbeCommand> makeCommand(CommandLog& log, const char* id = "test.bus.probe")
    {
        auto command = std::make_unique<ProbeCommand>(log);
        command->setCommandId(QString::fromLatin1(id));
        return command;
    }
};
}  // namespace

TEST_F(BusFixture, 启动后成为活动命令)
{
    CommandLog& log = newLog();
    auto command = makeCommand(log);
    ProbeCommand* raw = command.get();

    EXPECT_TRUE(bus.start(std::move(command)));
    EXPECT_EQ(bus.activeCommand(), raw);
    EXPECT_EQ(bus.activeCommandId(), QStringLiteral("test.bus.probe"));
    EXPECT_TRUE(raw->isActive());
    EXPECT_EQ(log.activated, 1);
}

TEST_F(BusFixture, 启动新命令时先请当前命令让位再结束它)
{
    CommandLog& first = newLog();
    CommandLog& second = newLog();
    ASSERT_TRUE(bus.start(makeCommand(first)));

    auto next = makeCommand(second, "test.bus.next");
    ProbeCommand* raw = next.get();
    EXPECT_TRUE(bus.start(std::move(next)));

    ASSERT_EQ(first.endRequests.size(), 1u);
    EXPECT_EQ(first.endRequests[0], CommandEndReason::Replaced);
    EXPECT_EQ(first.deactivated, 1);
    EXPECT_EQ(first.destroyed, 1);
    EXPECT_EQ(bus.activeCommand(), raw);
    EXPECT_EQ(second.activated, 1);
}

TEST_F(BusFixture, 当前命令否决替换时新命令直接销毁不激活)
{
    CommandLog& first = newLog();
    CommandLog& second = newLog();
    auto command = makeCommand(first);
    command->veto = true;
    ProbeCommand* raw = command.get();
    ASSERT_TRUE(bus.start(std::move(command)));

    EXPECT_FALSE(bus.start(makeCommand(second, "test.bus.next")));

    EXPECT_EQ(bus.activeCommand(), raw);
    EXPECT_EQ(first.deactivated, 0);
    EXPECT_EQ(second.activated, 0);
    EXPECT_EQ(second.destroyed, 1);
}

TEST_F(BusFixture, 结束全部命令可被否决)
{
    CommandLog& log = newLog();
    auto command = makeCommand(log);
    command->veto = true;
    ProbeCommand* raw = command.get();
    ASSERT_TRUE(bus.start(std::move(command)));

    EXPECT_FALSE(bus.approveEnd(CommandEndReason::Cancelled));
    EXPECT_EQ(bus.activeCommand(), raw);

    raw->veto = false;
    EXPECT_TRUE(bus.approveEnd(CommandEndReason::Cancelled));
    // approveEnd 只问不改：命令仍然活动，由调用方 end()
    EXPECT_EQ(bus.activeCommand(), raw);
    bus.end();
    EXPECT_FALSE(bus.hasActiveCommand());
    EXPECT_EQ(log.deactivated, 1);
    EXPECT_EQ(log.destroyed, 1);
    ASSERT_EQ(log.endRequests.size(), 2u);
    EXPECT_EQ(log.endRequests[1], CommandEndReason::Cancelled);
}

TEST_F(BusFixture, 视图关闭忽略否决)
{
    CommandLog& log = newLog();
    auto command = makeCommand(log);
    command->veto = true;
    ASSERT_TRUE(bus.start(std::move(command)));

    EXPECT_TRUE(bus.approveEnd(CommandEndReason::ViewClosing));
    ASSERT_EQ(log.endRequests.size(), 1u);
    EXPECT_EQ(log.endRequests[0], CommandEndReason::ViewClosing);
}

TEST_F(BusFixture, 没有活动命令时结束请求直接同意)
{
    EXPECT_TRUE(bus.approveEnd(CommandEndReason::Cancelled));
    bus.end();
    EXPECT_FALSE(bus.hasActiveCommand());
}

TEST_F(BusFixture, 回调期间的启动与结束请求被忽略)
{
    CommandLog& log = newLog();
    CommandLog& intruder = newLog();
    auto command = makeCommand(log);
    ProbeCommand* raw = command.get();
    bool inCallback = false;
    bool startResult = true;
    bool approveResult = true;
    raw->duringEndRequest = [&]()
    {
        // 模拟回调里弹出的对话框的事件循环中又点了 Ribbon、又按了"结束全部"
        inCallback = bus.isInCallback();
        startResult = bus.start(makeCommand(intruder, "test.bus.intruder"));
        approveResult = bus.approveEnd(CommandEndReason::Cancelled);
    };
    ASSERT_TRUE(bus.start(std::move(command)));

    EXPECT_TRUE(bus.approveEnd(CommandEndReason::Cancelled));

    EXPECT_TRUE(inCallback);
    EXPECT_FALSE(startResult);
    EXPECT_FALSE(approveResult);
    EXPECT_EQ(intruder.activated, 0);
    EXPECT_EQ(intruder.destroyed, 1);
    EXPECT_EQ(log.endRequests.size(), 1u);
    EXPECT_EQ(bus.activeCommand(), raw);
    EXPECT_FALSE(bus.isInCallback());
}

TEST_F(BusFixture, 分发范围内请求结束延迟到范围结束)
{
    CommandLog& log = newLog();
    auto command = makeCommand(log);
    ProbeCommand* raw = command.get();
    ASSERT_TRUE(bus.start(std::move(command)));

    {
        ExclusiveCommandBus::DispatchScope scope(&bus);
        raw->finish();
        // 命令还在自己工具的事件处理中：不结束、不销毁
        EXPECT_EQ(bus.activeCommand(), raw);
        EXPECT_EQ(log.deactivated, 0);
    }
    EXPECT_FALSE(bus.hasActiveCommand());
    EXPECT_EQ(log.deactivated, 1);
    EXPECT_EQ(log.destroyed, 1);
}

TEST_F(BusFixture, 分发范围内被外部结束的命令在范围结束时才销毁)
{
    CommandLog& log = newLog();
    ASSERT_TRUE(bus.start(makeCommand(log)));

    {
        ExclusiveCommandBus::DispatchScope scope(&bus);
        bus.end();
        EXPECT_FALSE(bus.hasActiveCommand());
        EXPECT_EQ(log.deactivated, 1);
        // 它的工具可能还在这次分发的调用栈上
        EXPECT_EQ(log.destroyed, 0);
    }
    EXPECT_EQ(log.destroyed, 1);
}

TEST_F(BusFixture, 分发范围外请求结束经事件循环)
{
    CommandLog& log = newLog();
    auto command = makeCommand(log);
    ProbeCommand* raw = command.get();
    ASSERT_TRUE(bus.start(std::move(command)));

    raw->finish();
    // 调用方就是命令自己：不能当场销毁
    EXPECT_EQ(bus.activeCommand(), raw);

    for (int i = 0; i < 5 && bus.hasActiveCommand(); ++i)
    {
        QCoreApplication::processEvents();
    }
    EXPECT_FALSE(bus.hasActiveCommand());
    EXPECT_EQ(log.destroyed, 1);
}

TEST_F(BusFixture, 激活期间就完成的命令在激活返回后结束)
{
    CommandLog& log = newLog();
    auto command = makeCommand(log);
    command->finishOnActivate = true;

    EXPECT_TRUE(bus.start(std::move(command)));
    EXPECT_FALSE(bus.hasActiveCommand());
    EXPECT_EQ(log.activated, 1);
    EXPECT_EQ(log.deactivated, 1);
    EXPECT_EQ(log.destroyed, 1);
}

TEST_F(BusFixture, 激活失败的命令被销毁且不调用onDeactivate)
{
    CommandLog& log = newLog();
    auto command = makeCommand(log);
    command->activateResult = false;

    EXPECT_FALSE(bus.start(std::move(command)));
    EXPECT_FALSE(bus.hasActiveCommand());
    EXPECT_EQ(log.activated, 1);
    EXPECT_EQ(log.deactivated, 0);
    EXPECT_EQ(log.destroyed, 1);
}

TEST_F(BusFixture, 挂起与恢复各只转发一次)
{
    CommandLog& log = newLog();
    ASSERT_TRUE(bus.start(makeCommand(log)));

    bus.suspend();
    bus.suspend();
    EXPECT_TRUE(bus.isSuspended());
    EXPECT_EQ(log.suspended, 1);

    bus.resume();
    bus.resume();
    EXPECT_FALSE(bus.isSuspended());
    EXPECT_EQ(log.resumed, 1);
}

TEST_F(BusFixture, 命令结束时总线清除选择阶段约束)
{
    CommandLog& log = newLog();
    auto command = makeCommand(log);
    command->selectOnActivate = true;
    ASSERT_TRUE(bus.start(std::move(command)));
    ASSERT_TRUE(selectTool.inSelectionPhase());

    ASSERT_TRUE(bus.approveEnd(CommandEndReason::Cancelled));
    bus.end();
    EXPECT_FALSE(selectTool.inSelectionPhase());
    EXPECT_EQ(selectTool.getStatus(), SelectTool::Neutral);
}

TEST_F(BusFixture, 析构时结束活动命令且不回调)
{
    CommandLog& log = newLog();
    {
        ExclusiveCommandBus local{&doc, &view, &control, &selectTool};
        ASSERT_TRUE(local.start(makeCommand(log)));
    }
    EXPECT_TRUE(log.endRequests.empty());
    EXPECT_EQ(log.deactivated, 1);
    EXPECT_EQ(log.destroyed, 1);
}

TEST_F(BusFixture, 捕捉设置同步给活动命令的捕捉器)
{
    CommandLog& log = newLog();
    auto command = makeCommand(log);
    command->snapper = std::make_unique<Snapper>(&doc, &view);
    Snapper* commandSnapper = command->snapper.get();
    ASSERT_TRUE(bus.start(std::move(command)));

    // 捕捉限制随 SnapMode::restriction 一起下发；Snapper::setSnapRestriction()
    // 本身是空实现，这里只验证 setSnapMode 的同步。
    SnapMode mode;
    mode.snapEndpoint = true;
    mode.restriction = DM::RestrictOrthogonal;
    bus.setSnapMode(mode);
    EXPECT_TRUE(commandSnapper->getSnapMode()->snapEndpoint);
    EXPECT_EQ(commandSnapper->getSnapMode()->restriction, DM::RestrictOrthogonal);
}
