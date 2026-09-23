/// @file test_legacy_action_tool.cpp
/// @brief LegacyActionTool 的单测
///
/// LegacyActionTool 是阶段2第6项的业务工具适配器：把 GuiEventHandler
/// 包成一个 IViewTool，只在"中键"和"没有业务 Action 活动时的 Ctrl/Meta+
/// 左键"两种情况下声明 NotHandled，把事件让给导航层；平移进行中时对
/// 移动/释放也主动让路。这里用一个没有挂任何 Action 的空 GuiEventHandler
/// 验证这套"要不要转发"的判断本身，不关心转发之后 Action 内部做了什么
/// （那是 ActionInterface/SelectTool 各自的测试范围）。

#include <gtest/gtest.h>

#include <QMouseEvent>
#include <QPointF>

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

struct LegacyActionToolFixture : ::testing::Test
{
    FakeDocumentView view;
    GuiEventHandler handler{nullptr};
    PanZoomTool panTool{&view};
    LegacyActionTool tool{&handler, &panTool};
};
}  // namespace

TEST_F(LegacyActionToolFixture, 中键按下总是让给导航层)
{
    QMouseEvent e = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::MiddleButton);
    EXPECT_EQ(tool.mousePressEvent(&e), ViewToolResult::NotHandled);
}

TEST_F(LegacyActionToolFixture, 无业务Action活动时CtrlLeft按下让给导航层)
{
    ASSERT_FALSE(handler.hasAction());
    QMouseEvent e = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::LeftButton, Qt::ControlModifier);
    EXPECT_EQ(tool.mousePressEvent(&e), ViewToolResult::NotHandled);
}

TEST_F(LegacyActionToolFixture, 普通左键按下转发给业务层)
{
    QMouseEvent e = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::LeftButton);
    EXPECT_EQ(tool.mousePressEvent(&e), ViewToolResult::Handled);
}

TEST_F(LegacyActionToolFixture, 右键按下不受Ctrl规则影响照常转发)
{
    // 右键从不属于"平移手势"，无论是否按 Ctrl 都应转发给业务层
    QMouseEvent e = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::RightButton, Qt::ControlModifier);
    EXPECT_EQ(tool.mousePressEvent(&e), ViewToolResult::Handled);
}

TEST_F(LegacyActionToolFixture, 未平移时移动和释放转发给业务层)
{
    ASSERT_FALSE(panTool.isPanning());
    QMouseEvent move = makeMouse(QEvent::MouseMove, 20, 20, Qt::NoButton);
    EXPECT_EQ(tool.mouseMoveEvent(&move), ViewToolResult::Handled);

    QMouseEvent release = makeMouse(QEvent::MouseButtonRelease, 20, 20, Qt::LeftButton);
    EXPECT_EQ(tool.mouseReleaseEvent(&release), ViewToolResult::Handled);
}

TEST_F(LegacyActionToolFixture, 平移中移动和释放业务层主动让路)
{
    QMouseEvent panPress = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::MiddleButton);
    panTool.mousePressEvent(&panPress);
    ASSERT_TRUE(panTool.isPanning());

    QMouseEvent move = makeMouse(QEvent::MouseMove, 30, 30, Qt::NoButton);
    EXPECT_EQ(tool.mouseMoveEvent(&move), ViewToolResult::NotHandled);

    QMouseEvent release = makeMouse(QEvent::MouseButtonRelease, 30, 30, Qt::MiddleButton);
    EXPECT_EQ(tool.mouseReleaseEvent(&release), ViewToolResult::NotHandled);
}

TEST_F(LegacyActionToolFixture, 双击和键盘事件总是转发)
{
    QMouseEvent dbl = makeMouse(QEvent::MouseButtonDblClick, 10, 10, Qt::LeftButton);
    EXPECT_EQ(tool.mouseDoubleClickEvent(&dbl), ViewToolResult::Handled);

    QKeyEvent keyDown(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    EXPECT_EQ(tool.keyPressEvent(&keyDown), ViewToolResult::Handled);

    QKeyEvent keyUp(QEvent::KeyRelease, Qt::Key_Escape, Qt::NoModifier);
    EXPECT_EQ(tool.keyReleaseEvent(&keyUp), ViewToolResult::Handled);
}
