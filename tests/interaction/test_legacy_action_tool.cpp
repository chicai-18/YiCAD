/// @file test_legacy_action_tool.cpp
/// @brief LegacyActionTool 的单测
///
/// LegacyActionTool 是阶段2第6项的业务工具适配器：把 GuiEventHandler 的旧版
/// Action 栈包成一个 IViewTool。第一步（doc/COMMAND_TOOL_MIGRATION_PLAN.md）
/// 之后它的规则是：
///   - 没有业务 Action：全部事件返回 NotHandled，交给选择层；
///   - 有业务 Action：转发并返回 Handled；例外是中键按下、平移中的移动/释放
///     （让给导航层），以及栈顶 Action 的 passesToSelection() 为真的事件
///     （转发后返回 NotHandled，继续交给选择层）。
/// 这里用一个记录收到事件的探针 Action 验证"转不转发、返回什么"，不关心
/// Action 内部做了什么（那是各 Action 与 SelectTool 自己的测试范围）。

#include <gtest/gtest.h>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPointF>
#include <QStringList>

#include "ActionInterface.h"
#include "DmDocument.h"
#include "GuiEventHandler.h"
#include "LegacyActionTool.h"
#include "PanZoomTool.h"
#include "support/FakeDocumentView.h"

namespace
{
QMouseEvent makeMouse(QEvent::Type type, int x, int y, Qt::MouseButton button, Qt::KeyboardModifiers mods = Qt::NoModifier)
{
    return QMouseEvent(type, QPointF(x, y), button, button, mods);
}

/// @brief 记录收到的事件；GuiEventHandler 拥有并删除 Action，记录放在 Action 之外
class ProbeAction : public ActionInterface
{
public:
    ProbeAction(DmDocument* doc, IDocumentView* docView, QStringList* log)
        : ActionInterface("probe", doc, docView)
        , m_log(log)
    {
    }

    /// @brief 该类型的事件在本 Action 处理后继续交给选择层
    QEvent::Type passType = QEvent::None;
    /// @brief 收到左键释放时结束自己（GuiEventHandler 随后 cleanUp() 删除本对象）
    bool finishOnRelease = false;

    bool passesToSelection(const QEvent* e) override
    {
        m_log->append(QStringLiteral("ask"));
        return e->type() == passType;
    }

    void mousePressEvent(QMouseEvent*) override { m_log->append(QStringLiteral("press")); }
    void mouseMoveEvent(QMouseEvent*) override { m_log->append(QStringLiteral("move")); }
    void mouseDoubleClickEvent(QMouseEvent*) override { m_log->append(QStringLiteral("dblclick")); }
    void mouseReleaseEvent(QMouseEvent*) override
    {
        m_log->append(QStringLiteral("release"));
        if (finishOnRelease)
        {
            finish();
        }
    }
    void keyPressEvent(QKeyEvent*) override { m_log->append(QStringLiteral("keypress")); }
    void keyReleaseEvent(QKeyEvent*) override { m_log->append(QStringLiteral("keyrelease")); }
    void suspend() override { m_log->append(QStringLiteral("suspend")); }
    void resume() override { m_log->append(QStringLiteral("resume")); }

private:
    QStringList* m_log = nullptr;
};

struct LegacyActionToolFixture : ::testing::Test
{
    QStringList log;
    DmDocument doc;
    FakeDocumentView view;
    GuiEventHandler handler{nullptr};
    PanZoomTool panTool{&view};
    LegacyActionTool tool{&handler, &panTool};

    /// @brief 压入一个探针 Action，并清掉启动过程（init/resume）留下的记录
    ProbeAction* startProbe()
    {
        auto* probe = new ProbeAction(&doc, &view, &log);
        handler.setCurrentAction(probe);
        log.clear();
        return probe;
    }
};
}  // namespace

TEST_F(LegacyActionToolFixture, 无业务Action时全部事件整体让路)
{
    ASSERT_FALSE(handler.hasAction());

    QMouseEvent left = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::LeftButton);
    EXPECT_EQ(tool.mousePressEvent(&left), ViewToolResult::NotHandled);
    QMouseEvent right = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::RightButton);
    EXPECT_EQ(tool.mousePressEvent(&right), ViewToolResult::NotHandled);
    QMouseEvent move = makeMouse(QEvent::MouseMove, 20, 20, Qt::NoButton);
    EXPECT_EQ(tool.mouseMoveEvent(&move), ViewToolResult::NotHandled);
    QMouseEvent release = makeMouse(QEvent::MouseButtonRelease, 20, 20, Qt::LeftButton);
    EXPECT_EQ(tool.mouseReleaseEvent(&release), ViewToolResult::NotHandled);
    QMouseEvent dbl = makeMouse(QEvent::MouseButtonDblClick, 10, 10, Qt::LeftButton);
    EXPECT_EQ(tool.mouseDoubleClickEvent(&dbl), ViewToolResult::NotHandled);

    QKeyEvent keyDown(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    EXPECT_EQ(tool.keyPressEvent(&keyDown), ViewToolResult::NotHandled);
    QKeyEvent keyUp(QEvent::KeyRelease, Qt::Key_Escape, Qt::NoModifier);
    EXPECT_EQ(tool.keyReleaseEvent(&keyUp), ViewToolResult::NotHandled);
}

TEST_F(LegacyActionToolFixture, 有业务Action时事件转发给它并到此为止)
{
    startProbe();

    QMouseEvent press = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::LeftButton);
    EXPECT_EQ(tool.mousePressEvent(&press), ViewToolResult::Handled);
    QMouseEvent move = makeMouse(QEvent::MouseMove, 20, 20, Qt::NoButton);
    EXPECT_EQ(tool.mouseMoveEvent(&move), ViewToolResult::Handled);
    QMouseEvent release = makeMouse(QEvent::MouseButtonRelease, 20, 20, Qt::LeftButton);
    EXPECT_EQ(tool.mouseReleaseEvent(&release), ViewToolResult::Handled);
    QMouseEvent dbl = makeMouse(QEvent::MouseButtonDblClick, 10, 10, Qt::LeftButton);
    EXPECT_EQ(tool.mouseDoubleClickEvent(&dbl), ViewToolResult::Handled);
    QKeyEvent keyDown(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    EXPECT_EQ(tool.keyPressEvent(&keyDown), ViewToolResult::Handled);
    QKeyEvent keyUp(QEvent::KeyRelease, Qt::Key_Escape, Qt::NoModifier);
    EXPECT_EQ(tool.keyReleaseEvent(&keyUp), ViewToolResult::Handled);

    EXPECT_TRUE(log.contains(QStringLiteral("press")));
    EXPECT_TRUE(log.contains(QStringLiteral("move")));
    EXPECT_TRUE(log.contains(QStringLiteral("release")));
    EXPECT_TRUE(log.contains(QStringLiteral("dblclick")));
    EXPECT_TRUE(log.contains(QStringLiteral("keypress")));
    EXPECT_TRUE(log.contains(QStringLiteral("keyrelease")));
}

TEST_F(LegacyActionToolFixture, 中键按下总是让给导航层)
{
    QMouseEvent e = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::MiddleButton);
    EXPECT_EQ(tool.mousePressEvent(&e), ViewToolResult::NotHandled);

    startProbe();
    EXPECT_EQ(tool.mousePressEvent(&e), ViewToolResult::NotHandled);
    EXPECT_FALSE(log.contains(QStringLiteral("press")));
}

TEST_F(LegacyActionToolFixture, 有业务Action时CtrlLeft按下转发给它)
{
    // 平移手势只在空闲态成立（由选择层在 Neutral 状态下让给导航层）；
    // 业务 Action 活动时 Ctrl+左键属于它。
    startProbe();
    QMouseEvent e = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::LeftButton, Qt::ControlModifier);
    EXPECT_EQ(tool.mousePressEvent(&e), ViewToolResult::Handled);
    EXPECT_TRUE(log.contains(QStringLiteral("press")));
}

TEST_F(LegacyActionToolFixture, 平移中移动和释放业务层主动让路)
{
    startProbe();
    QMouseEvent panPress = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::MiddleButton);
    panTool.mousePressEvent(&panPress);
    ASSERT_TRUE(panTool.isPanning());

    QMouseEvent move = makeMouse(QEvent::MouseMove, 30, 30, Qt::NoButton);
    EXPECT_EQ(tool.mouseMoveEvent(&move), ViewToolResult::NotHandled);

    QMouseEvent release = makeMouse(QEvent::MouseButtonRelease, 30, 30, Qt::MiddleButton);
    EXPECT_EQ(tool.mouseReleaseEvent(&release), ViewToolResult::NotHandled);

    EXPECT_FALSE(log.contains(QStringLiteral("move")));
    EXPECT_FALSE(log.contains(QStringLiteral("release")));
}

TEST_F(LegacyActionToolFixture, passesToSelection为真时转发后交给选择层)
{
    ProbeAction* probe = startProbe();
    probe->passType = QEvent::MouseButtonDblClick;

    QMouseEvent dbl = makeMouse(QEvent::MouseButtonDblClick, 10, 10, Qt::LeftButton);
    EXPECT_EQ(tool.mouseDoubleClickEvent(&dbl), ViewToolResult::NotHandled);
    EXPECT_TRUE(log.contains(QStringLiteral("dblclick")));

    // 其它事件不受影响
    QMouseEvent press = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::LeftButton);
    EXPECT_EQ(tool.mousePressEvent(&press), ViewToolResult::Handled);
}

TEST_F(LegacyActionToolFixture, 转发前询问passesToSelection释放后Action被删除也不再访问它)
{
    // GuiEventHandler 转发释放后会 cleanUp()，结束了的 Action 在那时被删除；
    // 所以去向必须在转发前问好。这里 Action 在释放中结束自己。
    ProbeAction* probe = startProbe();
    probe->passType = QEvent::MouseButtonRelease;
    probe->finishOnRelease = true;

    QMouseEvent release = makeMouse(QEvent::MouseButtonRelease, 10, 10, Qt::LeftButton);
    EXPECT_EQ(tool.mouseReleaseEvent(&release), ViewToolResult::NotHandled);

    EXPECT_EQ(log, (QStringList{QStringLiteral("ask"), QStringLiteral("release")}));
    EXPECT_FALSE(handler.hasAction());
    EXPECT_EQ(handler.getCurrentAction(), nullptr);
}

TEST_F(LegacyActionToolFixture, 进入离开画布转给栈顶业务Action)
{
    startProbe();
    tool.leaveEvent();
    tool.enterEvent();
    EXPECT_EQ(log, (QStringList{QStringLiteral("suspend"), QStringLiteral("resume")}));
}
