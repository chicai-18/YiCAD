/// @file test_select_tool.cpp
/// @brief SelectTool 的单测
///
/// SelectTool 从 ActionDefault 抽出（阶段2第5.4节第3项），不再依赖
/// QObject/ActionInterface，因此可以直接用一个空的 DmDocument + 真实
/// Snapper + FakeDocumentView 构造，脱离 GuiDocumentView 单独验证其
/// 点选/框选状态机。覆盖的是没有实体的边界情况——完整的实体拾取/
/// 几何相交语义已由其它子系统的测试覆盖，这里只锁住 SelectTool 自己
/// 负责的状态转换和视图侧调用（overlay、redraw、cursor）。

#include <gtest/gtest.h>

#include <QMouseEvent>
#include <QPointF>

#include "ActionInterface.h"
#include "DmDocument.h"
#include "GuiEventHandler.h"
#include "PanZoomTool.h"
#include "Preview.h"
#include "SelectTool.h"
#include "Snapper.h"
#include "support/FakeDocumentView.h"

namespace
{
QMouseEvent makeMouse(QEvent::Type type, int x, int y, Qt::MouseButton button, Qt::KeyboardModifiers mods = Qt::NoModifier)
{
    return QMouseEvent(type, QPointF(x, y), button, button, mods);
}

/// @brief 测试夹具：把 SelectTool 依赖的一整套对象串起来
///
/// 注意 Preview 用 nullptr 构造，不是 &doc：Preview::Preview(DmDocument*)
/// 一旦文档指针非空就无条件调用 `pDocument->getDocumentView()->
/// getPreviewContainer()`，而 `DmDocument::setDocumentView` 只接受具体的
/// `GuiDocumentView*`（阶段1就已经记录的既有限制，见
/// doc/ARCHITECTURE_EVOLUTION_PLAN.md 阶段1 4.6节"保留具体类型的例外"），
/// 没有真实 GuiDocumentView 就没法把这层关联接上，传 &doc 会在构造期直接
/// 空指针崩溃。这里的测试只覆盖 Neutral/Dragging/SetCorner2 与
/// 键盘处理——它们都不触碰 m_preview；Moving/MovingRef（拖拽实体/夹点）
/// 会调用 preview->addSelectionFromDocument() 等方法，这些需要一个真正
/// 关联了文档的 Preview，本轮未覆盖，是本次测试的已知边界。
///
/// panTool 用真实的 PanZoomTool 构造（而非默认的 nullptr）：SelectTool
/// 现在需要在导航层平移中时让路，这里的多数用例仍然从不触发平移，
/// panTool.isPanning() 始终为 false，行为与旧版完全一致。
struct SelectToolFixture : ::testing::Test
{
    DmDocument doc;
    FakeDocumentView view;
    Preview preview{nullptr};
    Snapper snapper{&doc, &view};
    PanZoomTool panTool{&view};
    SelectTool tool{&doc, &view, &snapper, &preview, &panTool};
};
}  // namespace

TEST_F(SelectToolFixture, 初始状态是Neutral且光标为箭头)
{
    EXPECT_EQ(tool.getStatus(), SelectTool::Neutral);
    ASSERT_TRUE(tool.getCursor().has_value());
    EXPECT_EQ(*tool.getCursor(), DM::ArrowCursor);
}

TEST_F(SelectToolFixture, init后回到Neutral)
{
    QMouseEvent press = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::LeftButton);
    tool.mousePressEvent(&press);
    ASSERT_EQ(tool.getStatus(), SelectTool::Dragging);

    tool.init();
    EXPECT_EQ(tool.getStatus(), SelectTool::Neutral);
}

TEST_F(SelectToolFixture, 左键按下进入Dragging状态)
{
    QMouseEvent press = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::LeftButton);
    EXPECT_EQ(tool.mousePressEvent(&press), ViewToolResult::Handled);
    EXPECT_EQ(tool.getStatus(), SelectTool::Dragging);
    // Dragging 状态没有专属光标偏好
    EXPECT_FALSE(tool.getCursor().has_value());
}

TEST_F(SelectToolFixture, 空文档上点选未命中实体转入框选)
{
    QMouseEvent press = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::LeftButton);
    tool.mousePressEvent(&press);

    // 原地释放：空文档下 catchEntity 找不到实体，应转入 SetCorner2（框选）
    QMouseEvent release = makeMouse(QEvent::MouseButtonRelease, 10, 10, Qt::LeftButton);
    tool.mouseReleaseEvent(&release);
    EXPECT_EQ(tool.getStatus(), SelectTool::SetCorner2);
}

TEST_F(SelectToolFixture, 完整框选流程结束后回到Neutral并重新给出箭头光标)
{
    QMouseEvent press = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::LeftButton);
    tool.mousePressEvent(&press);

    QMouseEvent release1 = makeMouse(QEvent::MouseButtonRelease, 10, 10, Qt::LeftButton);
    tool.mouseReleaseEvent(&release1);
    ASSERT_EQ(tool.getStatus(), SelectTool::SetCorner2);

    QMouseEvent move = makeMouse(QEvent::MouseMove, 60, 60, Qt::NoButton);
    tool.mouseMoveEvent(&move);
    EXPECT_GE(view.redrawCount, 1);

    QMouseEvent release2 = makeMouse(QEvent::MouseButtonRelease, 60, 60, Qt::LeftButton);
    tool.mouseReleaseEvent(&release2);

    EXPECT_EQ(tool.getStatus(), SelectTool::Neutral);
    ASSERT_TRUE(tool.getCursor().has_value());
    EXPECT_EQ(*tool.getCursor(), DM::ArrowCursor);
}

TEST_F(SelectToolFixture, 右键在任意状态下都清除回Neutral)
{
    QMouseEvent press = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::LeftButton);
    tool.mousePressEvent(&press);
    ASSERT_EQ(tool.getStatus(), SelectTool::Dragging);

    QMouseEvent rightPress = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::RightButton);
    tool.mousePressEvent(&rightPress);
    EXPECT_EQ(tool.getStatus(), SelectTool::Neutral);
}

TEST_F(SelectToolFixture, Escape键在空文档上安全地清空选择并回到Neutral)
{
    QMouseEvent press = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::LeftButton);
    tool.mousePressEvent(&press);
    ASSERT_EQ(tool.getStatus(), SelectTool::Dragging);

    QKeyEvent esc(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    EXPECT_EQ(tool.keyPressEvent(&esc), ViewToolResult::Handled);
    EXPECT_EQ(tool.getStatus(), SelectTool::Neutral);
}

TEST_F(SelectToolFixture, Shift按下再释放不崩溃且事件被标记为已处理)
{
    // 注意：Snapper::setSnapRestriction() 本身是空实现（Snapper.cpp 里
    // 从未给 snapMode.restriction 赋值，只有 SnapMode::fromInt/clear 会），
    // 这是原 ActionDefault 就有的既有行为——Shift 键理论上应该临时切换到
    // 正交捕捉，但从 Snapper 的实现看这条路径本就不生效。这是从
    // ActionDefault 原样搬过来的既有缺陷，不在阶段2范围内修复，这里只验证
    // SelectTool 转发到 ISnapService 的机械正确性（事件被接管、不崩溃），
    // 不对 restriction 的最终取值做断言。
    QKeyEvent shiftDown(QEvent::KeyPress, Qt::Key_Shift, Qt::NoModifier);
    EXPECT_EQ(tool.keyPressEvent(&shiftDown), ViewToolResult::Handled);

    QKeyEvent shiftUp(QEvent::KeyRelease, Qt::Key_Shift, Qt::NoModifier);
    EXPECT_EQ(tool.keyReleaseEvent(&shiftUp), ViewToolResult::Handled);
}

TEST_F(SelectToolFixture, 未知按键返回NotHandled)
{
    QKeyEvent other(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);
    EXPECT_EQ(tool.keyPressEvent(&other), ViewToolResult::NotHandled);
}

TEST_F(SelectToolFixture, Ctrl左键从Neutral按下时让给导航层)
{
    // 阶段2第6项落地、SelectTool 注册为选择层之后新增的判断：Neutral
    // 状态下的 Ctrl/Meta+左键是导航层的平移手势（见 PanZoomTool），
    // 选择层不应该抢先当成框选/点选的起点。
    QMouseEvent e = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::LeftButton, Qt::ControlModifier);
    EXPECT_EQ(tool.mousePressEvent(&e), ViewToolResult::NotHandled);
    EXPECT_EQ(tool.getStatus(), SelectTool::Neutral);
}

TEST_F(SelectToolFixture, 导航层平移中时移动和释放主动让路)
{
    QMouseEvent panPress = makeMouse(QEvent::MouseButtonPress, 0, 0, Qt::MiddleButton);
    panTool.mousePressEvent(&panPress);
    ASSERT_TRUE(panTool.isPanning());

    QMouseEvent move = makeMouse(QEvent::MouseMove, 30, 30, Qt::NoButton);
    EXPECT_EQ(tool.mouseMoveEvent(&move), ViewToolResult::NotHandled);

    QMouseEvent release = makeMouse(QEvent::MouseButtonRelease, 30, 30, Qt::MiddleButton);
    EXPECT_EQ(tool.mouseReleaseEvent(&release), ViewToolResult::NotHandled);
}

TEST_F(SelectToolFixture, 有其它业务Action活动时getCursor保持沉默)
{
    // SelectTool 注册为选择层后，若有其它业务 Action（画线、修改等）正在
    // 活动，它们仍然通过 updateMouseCursor() 直接调用 setMouseCursor()
    // （105 个未改造），选择层不能用自己在 Neutral 下的 Arrow 偏好覆盖
    // 它们，否则每次仲裁都会把光标错误地重置成箭头。
    GuiEventHandler handler(nullptr);
    view.eventHandler = &handler;

    // GuiEventHandler 的业务栈拥有并负责删除压入的 Action。
    handler.setCurrentAction(new ActionInterface("test-business-action", &doc, &view));
    ASSERT_TRUE(handler.hasAction());

    EXPECT_EQ(tool.getStatus(), SelectTool::Neutral);
    EXPECT_FALSE(tool.getCursor().has_value());
}
