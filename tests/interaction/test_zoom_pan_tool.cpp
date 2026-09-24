/// @file test_zoom_pan_tool.cpp
/// @brief 平移模式工具 ZoomPanTool 的单测
///
/// 逐项对照原 ActionZoomPan 经 LegacyActionTool 转发时的行为
/// （doc/COMMAND_TOOL_MIGRATION_PLAN.md 第三步第 1 项）：
///   - 左键拖动超过 7 像素才平移，平移后以当前位置为新的起点；
///   - 右键释放退出（请求结束自己并重绘），左键释放只结束一次拖动；
///   - 中键与中键平移中的移动让给导航层；其余鼠标事件、双击、按键到此为止；
///   - 按键不接受（Esc/空格随后由主窗口结束全部命令）；
///   - 命令行坐标被丢弃，文本不接受（随后被当作新命令），都不交给其下的命令；
///   - 光标：等待时张开的手，拖动中握紧的手。

#include <gtest/gtest.h>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPointF>

#include "DmVector.h"
#include "GuiCommandEvent.h"
#include "ZoomPanTool.h"
#include "support/FakeDocumentView.h"

namespace
{
QMouseEvent makeEvent(QEvent::Type type, int x, int y, Qt::MouseButton button,
                      Qt::MouseButtons buttons = Qt::NoButton)
{
    return QMouseEvent(type, QPointF(x, y), button, buttons | button, Qt::NoModifier);
}

/// @brief 构造工具并记录它请求结束的次数
struct ZoomPanFixture : ::testing::Test
{
    FakeDocumentView view;
    ZoomPanTool tool{&view};
    int finishRequests = 0;

    ZoomPanFixture()
    {
        tool.setFinishHandler([this]() { ++finishRequests; });
    }
};
}  // namespace

TEST_F(ZoomPanFixture, 左键拖动超过阈值才平移)
{
    QMouseEvent press = makeEvent(QEvent::MouseButtonPress, 100, 100, Qt::LeftButton);
    EXPECT_EQ(tool.mousePressEvent(&press), ViewToolResult::Handled);
    EXPECT_EQ(tool.status(), ZoomPanTool::SetPanning);

    // 位移 7 像素：不超过阈值
    QMouseEvent small = makeEvent(QEvent::MouseMove, 107, 100, Qt::NoButton, Qt::LeftButton);
    EXPECT_EQ(tool.mouseMoveEvent(&small), ViewToolResult::Handled);
    EXPECT_EQ(view.zoomPanCount, 0);

    QMouseEvent move = makeEvent(QEvent::MouseMove, 120, 95, Qt::NoButton, Qt::LeftButton);
    tool.mouseMoveEvent(&move);
    EXPECT_EQ(view.zoomPanCount, 1);
    EXPECT_EQ(view.lastPanDx, 20);
    EXPECT_EQ(view.lastPanDy, -5);

    // 下一次以上次平移的位置为起点
    QMouseEvent next = makeEvent(QEvent::MouseMove, 130, 95, Qt::NoButton, Qt::LeftButton);
    tool.mouseMoveEvent(&next);
    EXPECT_EQ(view.zoomPanCount, 2);
    EXPECT_EQ(view.lastPanDx, 10);
    EXPECT_EQ(view.lastPanDy, 0);
}

TEST_F(ZoomPanFixture, 没按下左键时移动不平移)
{
    QMouseEvent move = makeEvent(QEvent::MouseMove, 300, 300, Qt::NoButton);
    EXPECT_EQ(tool.mouseMoveEvent(&move), ViewToolResult::Handled);
    EXPECT_EQ(view.zoomPanCount, 0);
}

TEST_F(ZoomPanFixture, 左键释放结束一次拖动但不退出)
{
    QMouseEvent press = makeEvent(QEvent::MouseButtonPress, 100, 100, Qt::LeftButton);
    tool.mousePressEvent(&press);
    QMouseEvent release = makeEvent(QEvent::MouseButtonRelease, 150, 100, Qt::LeftButton);
    EXPECT_EQ(tool.mouseReleaseEvent(&release), ViewToolResult::Handled);

    EXPECT_EQ(tool.status(), ZoomPanTool::SetPanStart);
    EXPECT_EQ(finishRequests, 0);

    QMouseEvent move = makeEvent(QEvent::MouseMove, 200, 100, Qt::NoButton);
    tool.mouseMoveEvent(&move);
    EXPECT_EQ(view.zoomPanCount, 0);
}

TEST_F(ZoomPanFixture, 右键释放退出并重绘)
{
    QMouseEvent press = makeEvent(QEvent::MouseButtonPress, 100, 100, Qt::RightButton);
    EXPECT_EQ(tool.mousePressEvent(&press), ViewToolResult::Handled);
    EXPECT_EQ(tool.status(), ZoomPanTool::SetPanStart);

    const int redraws = view.redrawCount;
    QMouseEvent release = makeEvent(QEvent::MouseButtonRelease, 100, 100, Qt::RightButton);
    EXPECT_EQ(tool.mouseReleaseEvent(&release), ViewToolResult::Handled);
    EXPECT_EQ(finishRequests, 1);
    EXPECT_EQ(view.redrawCount, redraws + 1);
}

TEST_F(ZoomPanFixture, 中键与中键平移中的移动让给导航层)
{
    QMouseEvent press = makeEvent(QEvent::MouseButtonPress, 100, 100, Qt::MiddleButton);
    EXPECT_EQ(tool.mousePressEvent(&press), ViewToolResult::NotHandled);
    EXPECT_EQ(tool.status(), ZoomPanTool::SetPanStart);

    QMouseEvent move = makeEvent(QEvent::MouseMove, 150, 100, Qt::NoButton, Qt::MiddleButton);
    EXPECT_EQ(tool.mouseMoveEvent(&move), ViewToolResult::NotHandled);

    QMouseEvent release = makeEvent(QEvent::MouseButtonRelease, 150, 100, Qt::MiddleButton);
    EXPECT_EQ(tool.mouseReleaseEvent(&release), ViewToolResult::NotHandled);
    EXPECT_EQ(finishRequests, 0);
    EXPECT_EQ(view.zoomPanCount, 0);
}

TEST_F(ZoomPanFixture, 双击与按键到此为止且按键不接受)
{
    QMouseEvent dbl = makeEvent(QEvent::MouseButtonDblClick, 100, 100, Qt::LeftButton);
    EXPECT_EQ(tool.mouseDoubleClickEvent(&dbl), ViewToolResult::Handled);

    QKeyEvent esc(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    esc.accept();
    EXPECT_EQ(tool.keyPressEvent(&esc), ViewToolResult::Handled);
    EXPECT_FALSE(esc.isAccepted());

    QKeyEvent release(QEvent::KeyRelease, Qt::Key_Escape, Qt::NoModifier);
    release.accept();
    EXPECT_EQ(tool.keyReleaseEvent(&release), ViewToolResult::Handled);
    EXPECT_FALSE(release.isAccepted());
}

TEST_F(ZoomPanFixture, 命令行输入不交给其下的命令)
{
    EXPECT_EQ(tool.coordinateEvent(DmVector(1.0, 2.0)), ViewToolResult::Handled);

    GuiCommandEvent command(QStringLiteral("line"));
    EXPECT_EQ(tool.commandEvent(&command), ViewToolResult::Handled);
    // 不接受：文本随后被当作新命令
    EXPECT_FALSE(command.isAccepted());
}

TEST_F(ZoomPanFixture, 光标随拖动在张开与握紧之间切换)
{
    ASSERT_TRUE(tool.getCursor().has_value());
    EXPECT_EQ(*tool.getCursor(), DM::OpenHandCursor);

    QMouseEvent press = makeEvent(QEvent::MouseButtonPress, 100, 100, Qt::LeftButton);
    tool.mousePressEvent(&press);
    EXPECT_EQ(*tool.getCursor(), DM::ClosedHandCursor);

    QMouseEvent release = makeEvent(QEvent::MouseButtonRelease, 100, 100, Qt::LeftButton);
    tool.mouseReleaseEvent(&release);
    EXPECT_EQ(*tool.getCursor(), DM::OpenHandCursor);
}
