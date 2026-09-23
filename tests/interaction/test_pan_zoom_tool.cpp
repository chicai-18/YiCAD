/// @file test_pan_zoom_tool.cpp
/// @brief PanZoomTool 的单测
///
/// 覆盖阶段2验收标准里的关键行为：
///   - 一次完整的按下-移动-释放手势产生 zoomPan 调用；
///   - 位移小于阈值时不触发 zoomPan（避免手抖误触发）；
///   - 释放的按钮与开始平移的按钮不一致时不结束平移；
///   - 光标只在平移进行中偏好 ClosedHandCursor，其余时候不表达偏好
///     （不会覆盖旧版 Action 体系自行设置的光标，见 ViewToolControl.h）。

#include <gtest/gtest.h>

#include <QMouseEvent>
#include <QPointF>

#include "support/FakeDocumentView.h"
#include "PanZoomTool.h"

namespace
{
QMouseEvent makeEvent(QEvent::Type type, int x, int y, Qt::MouseButton button)
{
    return QMouseEvent(type, QPointF(x, y), button, button, Qt::NoModifier);
}
}  // namespace

TEST(PanZoomToolTest, 初始状态不在平移中且无光标偏好)
{
    FakeDocumentView view;
    PanZoomTool tool(&view);
    EXPECT_FALSE(tool.isPanning());
    EXPECT_FALSE(tool.getCursor().has_value());
}

TEST(PanZoomToolTest, 按下即进入平移状态并给出ClosedHand光标)
{
    FakeDocumentView view;
    PanZoomTool tool(&view);

    QMouseEvent press = makeEvent(QEvent::MouseButtonPress, 100, 100, Qt::MiddleButton);
    EXPECT_EQ(tool.mousePressEvent(&press), ViewToolResult::Handled);

    EXPECT_TRUE(tool.isPanning());
    ASSERT_TRUE(tool.getCursor().has_value());
    EXPECT_EQ(*tool.getCursor(), DM::ClosedHandCursor);
}

TEST(PanZoomToolTest, 超过阈值的移动触发zoomPan)
{
    FakeDocumentView view;
    PanZoomTool tool(&view);

    QMouseEvent press = makeEvent(QEvent::MouseButtonPress, 100, 100, Qt::MiddleButton);
    tool.mousePressEvent(&press);

    // 位移 (20, 0)，平方距离 400 > 阈值 64，应该触发一次 zoomPan
    QMouseEvent move = makeEvent(QEvent::MouseMove, 120, 100, Qt::NoButton);
    EXPECT_EQ(tool.mouseMoveEvent(&move), ViewToolResult::Handled);

    EXPECT_EQ(view.zoomPanCount, 1);
    EXPECT_EQ(view.lastPanDx, 20);
    EXPECT_EQ(view.lastPanDy, 0);
}

TEST(PanZoomToolTest, 微小移动不触发zoomPan)
{
    FakeDocumentView view;
    PanZoomTool tool(&view);

    QMouseEvent press = makeEvent(QEvent::MouseButtonPress, 100, 100, Qt::MiddleButton);
    tool.mousePressEvent(&press);

    // 位移 (2, 2)，平方距离 8 < 阈值 64，不应触发
    QMouseEvent move = makeEvent(QEvent::MouseMove, 102, 102, Qt::NoButton);
    tool.mouseMoveEvent(&move);

    EXPECT_EQ(view.zoomPanCount, 0);
}

TEST(PanZoomToolTest, 未平移时的移动不被处理)
{
    FakeDocumentView view;
    PanZoomTool tool(&view);

    QMouseEvent move = makeEvent(QEvent::MouseMove, 120, 100, Qt::NoButton);
    EXPECT_EQ(tool.mouseMoveEvent(&move), ViewToolResult::NotHandled);
    EXPECT_EQ(view.zoomPanCount, 0);
}

TEST(PanZoomToolTest, 释放起始按钮结束平移)
{
    FakeDocumentView view;
    PanZoomTool tool(&view);

    QMouseEvent press = makeEvent(QEvent::MouseButtonPress, 100, 100, Qt::MiddleButton);
    tool.mousePressEvent(&press);

    QMouseEvent release = makeEvent(QEvent::MouseButtonRelease, 120, 100, Qt::MiddleButton);
    EXPECT_EQ(tool.mouseReleaseEvent(&release), ViewToolResult::Handled);

    EXPECT_FALSE(tool.isPanning());
    EXPECT_FALSE(tool.getCursor().has_value());
    EXPECT_EQ(view.redrawCount, 1);
}

TEST(PanZoomToolTest, 释放不相干的按钮不结束平移)
{
    FakeDocumentView view;
    PanZoomTool tool(&view);

    // 由 Ctrl+左键触发的平移
    QMouseEvent press = makeEvent(QEvent::MouseButtonPress, 100, 100, Qt::LeftButton);
    tool.mousePressEvent(&press);

    // 平移过程中右键抬起（比如与其它手势混合的边缘情况），不应结束左键平移
    QMouseEvent release = makeEvent(QEvent::MouseButtonRelease, 100, 100, Qt::RightButton);
    EXPECT_EQ(tool.mouseReleaseEvent(&release), ViewToolResult::NotHandled);
    EXPECT_TRUE(tool.isPanning());

    // 真正释放左键才结束
    QMouseEvent releaseLeft = makeEvent(QEvent::MouseButtonRelease, 100, 100, Qt::LeftButton);
    EXPECT_EQ(tool.mouseReleaseEvent(&releaseLeft), ViewToolResult::Handled);
    EXPECT_FALSE(tool.isPanning());
}
