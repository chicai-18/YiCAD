/// @file test_select_tool.cpp
/// @brief SelectTool 的单测
///
/// SelectTool 从 ActionDefault 抽出（阶段2第5.4节第3项），不再依赖
/// QObject/ActionInterface，因此可以直接用一个空的 DmDocument + 真实
/// Snapper + FakeDocumentView 构造，脱离 GuiDocumentView 单独验证其
/// 点选/框选状态机。覆盖的是没有实体的边界情况——完整的实体拾取/
/// 几何相交语义已由其它子系统的测试覆盖，这里只锁住 SelectTool 自己
/// 负责的状态转换和视图侧调用（overlay、redraw、cursor）。
///
/// 后半部分按 UIView 的装配（业务层 LegacyActionTool、选择层、
/// 导航层 PanZoomTool）经 ViewToolControl 分发，锁住第一步
/// （doc/COMMAND_TOOL_MIGRATION_PLAN.md）之后空闲态事件直接落到选择层的路径。

#include <gtest/gtest.h>

#include <QMouseEvent>
#include <QPointF>

#include "ActionInterface.h"
#include "DmDocument.h"
#include "GuiDialogFactory.h"
#include "GuiDialogFactoryAdapter.h"
#include "GuiEventHandler.h"
#include "LegacyActionTool.h"
#include "PanZoomTool.h"
#include "Preview.h"
#include "SelectTool.h"
#include "Snapper.h"
#include "ViewToolControl.h"
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

TEST_F(SelectToolFixture, 中键按下让给导航层)
{
    QMouseEvent e = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::MiddleButton);
    EXPECT_EQ(tool.mousePressEvent(&e), ViewToolResult::NotHandled);
    EXPECT_EQ(tool.getStatus(), SelectTool::Neutral);
}

TEST_F(SelectToolFixture, 单点拾取在空文档上未命中返回空)
{
    EXPECT_EQ(tool.pickAt(10, 10), nullptr);
}

namespace
{
/// @brief 记录按键提示更新次数的对话框工厂
class HintRecorder : public GuiDialogFactoryAdapter
{
public:
    int hintUpdates = 0;
    void updateMouseWidget(const QString&, const QString&) override { ++hintUpdates; }
};

/// @brief 在用例期间把 HintRecorder 装进 GUIDIALOGFACTORY
struct HintRecorderScope
{
    HintRecorder recorder;
    HintRecorderScope() { GuiDialogFactory::instance()->setFactoryObject(&recorder); }
    ~HintRecorderScope() { GuiDialogFactory::instance()->setFactoryObject(nullptr); }
};
}  // namespace

TEST_F(SelectToolFixture, 有业务Action活动时不更新按键提示)
{
    // 块编辑时选择层经 passesToSelection 收到事件，提示归块编辑 Action 管；
    // 选择层若照常更新，会把"Edit block entities"提示清空。
    HintRecorderScope hints;

    QMouseEvent press = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::LeftButton);
    tool.mousePressEvent(&press);
    EXPECT_EQ(hints.recorder.hintUpdates, 1);  // 空闲态：Neutral -> Dragging 更新一次
    tool.init();

    GuiEventHandler handler(nullptr);
    view.eventHandler = &handler;
    handler.setCurrentAction(new ActionInterface("test-business-action", &doc, &view));
    hints.recorder.hintUpdates = 0;

    tool.mousePressEvent(&press);
    EXPECT_EQ(tool.getStatus(), SelectTool::Dragging);
    EXPECT_EQ(hints.recorder.hintUpdates, 0);
}

TEST_F(SelectToolFixture, 进入离开画布只在空闲态挂起与恢复)
{
    // 空闲态：进入画布时恢复，重绘捕捉点与预览
    const int before = view.redrawCount;
    tool.enterEvent();
    EXPECT_GT(view.redrawCount, before);

    // 有业务 Action 时由它自己挂起/恢复（经 LegacyActionTool），选择层不动
    GuiEventHandler handler(nullptr);
    view.eventHandler = &handler;
    handler.setCurrentAction(new ActionInterface("test-business-action", &doc, &view));
    const int withAction = view.redrawCount;
    tool.enterEvent();
    EXPECT_EQ(view.redrawCount, withAction);
}

namespace
{
/// @brief 注册进 ViewToolControl 用的 SelectTool
///
/// 注册与注销会调 onActivate()/onDeactivate() 绘制与清除预览；夹具里的
/// Preview 没有预览容器（见 SelectToolFixture 的说明），清除时会解引用空
/// 容器。这里只验证分发路径，跳过这两处预览操作。
class DispatchSelectTool : public SelectTool
{
public:
    using SelectTool::SelectTool;
    void onActivate() override {}
    void onDeactivate() override {}
};

/// @brief 与 UIView 相同的三层装配
struct IdleDispatchFixture : ::testing::Test
{
    DmDocument doc;
    FakeDocumentView view;
    Preview preview{nullptr};
    Snapper snapper{&doc, &view};
    PanZoomTool panTool{&view};
    DispatchSelectTool tool{&doc, &view, &snapper, &preview, &panTool};
    GuiEventHandler handler{nullptr};
    LegacyActionTool legacyTool{&handler, &panTool};
    ViewToolControl control{&view};

    IdleDispatchFixture()
    {
        view.eventHandler = &handler;
        handler.setSelectTool(&tool);
        control.setNavigationTool(&panTool);
        control.setSelectionTool(&tool);
        control.activate(&legacyTool);
    }

    /// @brief 压入一个不结束的业务 Action（GuiEventHandler 拥有并删除它）
    void startBusinessAction()
    {
        handler.setCurrentAction(new ActionInterface("test-business-action", &doc, &view));
        ASSERT_TRUE(handler.hasAction());
    }
};
}  // namespace

TEST_F(IdleDispatchFixture, 空闲态左键经ViewToolControl落到选择层)
{
    QMouseEvent press = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::LeftButton);
    EXPECT_EQ(control.mousePressEvent(&press), ViewToolResult::Handled);
    EXPECT_EQ(tool.getStatus(), SelectTool::Dragging);
}

TEST_F(IdleDispatchFixture, 空闲态中键由导航层平移)
{
    // 回归：选择层曾对中键返回 Handled，把 LegacyActionTool 让出的中键吞掉，
    // 导航层收不到按下，中键平移失效。
    QMouseEvent press = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::MiddleButton);
    EXPECT_EQ(control.mousePressEvent(&press), ViewToolResult::Handled);
    EXPECT_TRUE(panTool.isPanning());
    EXPECT_EQ(tool.getStatus(), SelectTool::Neutral);

    QMouseEvent release = makeMouse(QEvent::MouseButtonRelease, 40, 10, Qt::MiddleButton);
    EXPECT_EQ(control.mouseReleaseEvent(&release), ViewToolResult::Handled);
    EXPECT_FALSE(panTool.isPanning());
}

TEST_F(IdleDispatchFixture, 有业务Action时中键仍由导航层平移)
{
    startBusinessAction();
    QMouseEvent press = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::MiddleButton);
    EXPECT_EQ(control.mousePressEvent(&press), ViewToolResult::Handled);
    EXPECT_TRUE(panTool.isPanning());
}

TEST_F(IdleDispatchFixture, 空闲态Ctrl左键由导航层平移)
{
    QMouseEvent press = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::LeftButton, Qt::ControlModifier);
    EXPECT_EQ(control.mousePressEvent(&press), ViewToolResult::Handled);
    EXPECT_TRUE(panTool.isPanning());
    EXPECT_EQ(tool.getStatus(), SelectTool::Neutral);
}

TEST_F(IdleDispatchFixture, 空闲态Esc由选择层接受)
{
    QMouseEvent press = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::LeftButton);
    control.mousePressEvent(&press);
    ASSERT_EQ(tool.getStatus(), SelectTool::Dragging);

    QKeyEvent esc(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    esc.ignore();  // 新构造的事件默认已接受，先清掉，才能看出是谁接受的
    EXPECT_EQ(control.keyPressEvent(&esc), ViewToolResult::Handled);
    EXPECT_TRUE(esc.isAccepted());
    EXPECT_EQ(tool.getStatus(), SelectTool::Neutral);
}

TEST_F(IdleDispatchFixture, 空闲态空格无人接受)
{
    // 主窗口据此结束全部命令并清空选择（ApplicationWindow::keyPressEvent）
    QKeyEvent space(QEvent::KeyPress, Qt::Key_Space, Qt::NoModifier);
    EXPECT_EQ(control.keyPressEvent(&space), ViewToolResult::NotHandled);
    EXPECT_FALSE(space.isAccepted());
}

TEST_F(IdleDispatchFixture, 有业务Action时鼠标事件不落到选择层)
{
    startBusinessAction();
    QMouseEvent press = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::LeftButton);
    EXPECT_EQ(control.mousePressEvent(&press), ViewToolResult::Handled);
    EXPECT_EQ(tool.getStatus(), SelectTool::Neutral);
}

TEST_F(IdleDispatchFixture, 结束全部命令时复位选择层)
{
    QMouseEvent press = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::LeftButton);
    control.mousePressEvent(&press);
    ASSERT_EQ(tool.getStatus(), SelectTool::Dragging);

    handler.killAllActions();
    EXPECT_EQ(tool.getStatus(), SelectTool::Neutral);
}
