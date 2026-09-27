/// @file test_edit_tool.cpp
/// @brief 夹点编辑工具 EditTool 的单测
///
/// EditTool 从 SelectTool 拆出，夹点改为单击激活（doc/COMMAND_TOOL_MIGRATION_PLAN.md 9.6 节）。
/// 按 UIView 没有命令时的装配（业务栈上只有 EditTool，其下是选择层、导航层）经 ViewToolControl
/// 分发，锁住两者按按下位置分工的规则：按在选中实体的夹点上归 EditTool，选择层收不到；其余归
/// 选择层，包括在选中实体的线身上按住拖动（空闲态不再拖动整个实体）。它随命令启停移出、放回
/// 业务栈由 test_exclusive_command_bus 覆盖，选择阶段（有命令在运行）不激活夹点由
/// test_select_first_commands 覆盖。
///
/// 落位经 Modification::moveRef 走撤销事务。先开事务再改实体在默认构造的 DmDocument 上
/// 可以运行（test_modify_commands 的粘贴用例），落位用例照此执行真正的提交与撤销。
///
/// FakeDocumentView 的预览容器与捕捉标记容器是同一个，选择层清除捕捉标记（Esc、鼠标
/// 离开画布）时也会清空预览；要断言 EditTool 自己清除了预览，用例只走选择层不参与的路径。

#include <gtest/gtest.h>

#include <QKeyEvent>
#include <QMouseEvent>

#include "CmdManager.h"
#include "DmDocument.h"
#include "DmLine.h"
#include "EditTool.h"
#include "EntityTable.h"
#include "PanZoomTool.h"
#include "Preview.h"
#include "SelectionSet.h"
#include "SelectTool.h"
#include "Snapper.h"
#include "ViewToolControl.h"
#include "support/FakeDocumentView.h"

namespace
{
QMouseEvent makeMouse(QEvent::Type type, int x, int y, Qt::MouseButton button,
                      Qt::KeyboardModifiers mods = Qt::NoModifier)
{
    return QMouseEvent(type, QPointF(x, y), QPointF(x, y), button, button, mods);
}

/// @brief 与 UIView 没有命令时相同的装配：业务栈上只有 EditTool，其下是选择层与导航层
struct EditToolFixture : ::testing::Test
{
    DmDocument doc;
    SelectionSet selection{doc};
    FakeDocumentView view;
    Preview preview{&selection, &view};
    Snapper snapper{&doc, &view};
    PanZoomTool panTool{&view};
    SelectTool selectTool{&doc, &selection, &view, &snapper, &preview, &panTool};
    EditTool editTool{&doc, &selection, &view, &snapper, &preview, &panTool};
    ViewToolControl control{&view};

    EditToolFixture()
    {
        control.setNavigationTool(&panTool);
        control.setSelectionTool(&selectTool);
        // 与 UIView 构造时相同，直接放上业务栈（这里没有命令，不接总线）
        control.activate(&editTool);
        editTool.setEnabledQuery([this]() { return selectTool.getStatus() == SelectTool::Neutral; });
    }

    DmLine* addLine(const DmVector& a, const DmVector& b, bool selected)
    {
        auto* line = new DmLine(a, b);
        line->calculateBorders();
        EXPECT_TRUE(doc.getEntityTable()->add_direct(line));
        if (selected)
        {
            selection.add(line);
        }
        return line;
    }

    /// @brief 选中的直线 (10,10)-(50,10)，两端是夹点
    DmLine* addSelectedLine() { return addLine(DmVector(10.0, 10.0), DmVector(50.0, 10.0), true); }

    ViewToolResult press(int x, int y, Qt::MouseButton button = Qt::LeftButton,
                         Qt::KeyboardModifiers mods = Qt::NoModifier)
    {
        QMouseEvent e = makeMouse(QEvent::MouseButtonPress, x, y, button, mods);
        return control.mousePressEvent(&e);
    }

    ViewToolResult release(int x, int y, Qt::MouseButton button = Qt::LeftButton)
    {
        QMouseEvent e = makeMouse(QEvent::MouseButtonRelease, x, y, button);
        return control.mouseReleaseEvent(&e);
    }

    ViewToolResult move(int x, int y, Qt::KeyboardModifiers mods = Qt::NoModifier)
    {
        QMouseEvent e = makeMouse(QEvent::MouseMove, x, y, Qt::NoButton, mods);
        return control.mouseMoveEvent(&e);
    }

    /// @brief 单击选中直线起点附近的夹点，激活它
    DmLine* activateGrip()
    {
        DmLine* line = addSelectedLine();
        press(12, 10);
        release(12, 10);
        EXPECT_EQ(editTool.getStatus(), EditTool::MovingRef);
        return line;
    }

    int previewCount() { return view.getPreviewContainer()->size(); }
};
}  // namespace

TEST_F(EditToolFixture, 按在夹点上的按下归EditTool)
{
    addSelectedLine();

    EXPECT_EQ(press(12, 10), ViewToolResult::Handled);
    EXPECT_EQ(editTool.getStatus(), EditTool::Pressed);
    // 选择层收不到这次按下
    EXPECT_EQ(selectTool.getStatus(), SelectTool::Neutral);
    EXPECT_FALSE(editTool.getCursor().has_value());
}

TEST_F(EditToolFixture, 单击夹点激活且不取消选中)
{
    DmLine* line = addSelectedLine();

    press(12, 10);
    EXPECT_EQ(release(12, 10), ViewToolResult::Handled);
    EXPECT_EQ(editTool.getStatus(), EditTool::MovingRef);
    EXPECT_EQ(selectTool.getStatus(), SelectTool::Neutral);
    // 拆分前这次单击会切换选中，取消直线的选中
    EXPECT_TRUE(selection.contains(line));
    // 相对零点移到参考点，与拆分前一致
    EXPECT_EQ(view.getRelativeZero(), DmVector(10.0, 10.0));
    ASSERT_TRUE(view.lastCursor().has_value());
    EXPECT_EQ(*view.lastCursor(), DM::SelectCursor);
}

TEST_F(EditToolFixture, 激活后移动画出预览且不改动文档)
{
    DmLine* line = activateGrip();

    EXPECT_EQ(move(60, 60), ViewToolResult::Handled);
    EXPECT_EQ(previewCount(), 1);
    EXPECT_EQ(line->getStartpoint(), DmVector(10.0, 10.0));
}

TEST_F(EditToolFixture, 单击落位移动夹点并保持选中)
{
    // 修改的撤销命令构造时取消了选中（EntityTableModifyCmd），落位后由 EditTool 恢复
    // （doc/SELECTION_SET_PLAN.md 第 1 步）
    DmLine* line = activateGrip();

    EXPECT_EQ(press(70, 60), ViewToolResult::Handled);
    EXPECT_EQ(editTool.getStatus(), EditTool::Neutral);
    EXPECT_EQ(line->getStartpoint(), DmVector(70.0, 60.0));
    EXPECT_EQ(line->getEndpoint(), DmVector(50.0, 10.0));
    EXPECT_TRUE(selection.contains(line));

    doc.getCmdManager()->undo();
    EXPECT_EQ(line->getStartpoint(), DmVector(10.0, 10.0));
}

TEST_F(EditToolFixture, 按住夹点拖过阈值即激活)
{
    addSelectedLine();
    press(12, 10);

    EXPECT_EQ(move(40, 40), ViewToolResult::Handled);
    EXPECT_EQ(editTool.getStatus(), EditTool::MovingRef);
    EXPECT_EQ(previewCount(), 1);

    // 松开后参考点继续跟随鼠标，单击才落位
    EXPECT_EQ(release(40, 40), ViewToolResult::Handled);
    EXPECT_EQ(editTool.getStatus(), EditTool::MovingRef);
    EXPECT_EQ(selectTool.getStatus(), SelectTool::Neutral);
}

TEST_F(EditToolFixture, 按住夹点未超过阈值时不激活)
{
    addSelectedLine();
    press(12, 10);

    EXPECT_EQ(move(15, 12), ViewToolResult::Handled);
    EXPECT_EQ(editTool.getStatus(), EditTool::Pressed);
    EXPECT_EQ(previewCount(), 0);
}

TEST_F(EditToolFixture, 单击选中实体的线身仍切换选中)
{
    DmLine* line = addSelectedLine();

    EXPECT_EQ(press(30, 10), ViewToolResult::Handled);
    EXPECT_EQ(editTool.getStatus(), EditTool::Neutral);
    EXPECT_EQ(selectTool.getStatus(), SelectTool::Dragging);
    release(30, 10);
    EXPECT_FALSE(selection.contains(line));
}

TEST_F(EditToolFixture, 在选中实体的线身上拖动开始框选)
{
    // 空闲态不再拖动整个实体（拆分前进入 Moving），移动实体用移动命令
    DmLine* line = addSelectedLine();

    press(30, 10);
    move(30, 60);
    EXPECT_EQ(editTool.getStatus(), EditTool::Neutral);
    EXPECT_EQ(selectTool.getStatus(), SelectTool::SetCorner2);
    EXPECT_TRUE(selection.contains(line));
}

TEST_F(EditToolFixture, 未选中实体的端点归选择层)
{
    addLine(DmVector(10.0, 10.0), DmVector(50.0, 10.0), false);

    press(12, 10);
    EXPECT_EQ(editTool.getStatus(), EditTool::Neutral);
    EXPECT_EQ(selectTool.getStatus(), SelectTool::Dragging);
}

TEST_F(EditToolFixture, 框选点第二个角点时按在夹点上仍是框选)
{
    addSelectedLine();
    // 空白处单击：选择层转入框选，等第二个角点
    press(100, 100);
    release(100, 100);
    ASSERT_EQ(selectTool.getStatus(), SelectTool::SetCorner2);

    EXPECT_EQ(press(12, 10), ViewToolResult::Handled);
    EXPECT_EQ(editTool.getStatus(), EditTool::Neutral);
    EXPECT_EQ(selectTool.getStatus(), SelectTool::SetCorner2);
}

TEST_F(EditToolFixture, Ctrl左键按在夹点上归导航层平移)
{
    addSelectedLine();

    press(12, 10, Qt::LeftButton, Qt::ControlModifier);
    EXPECT_TRUE(panTool.isPanning());
    EXPECT_EQ(editTool.getStatus(), EditTool::Neutral);
    release(12, 10);
}

TEST_F(EditToolFixture, 按住Shift移动时只做角度吸附不另画引导线)
{
    // 原先另画一条借选中色的引导线，选择集移出 Model 时去掉（doc/SELECTION_SET_PLAN.md 第 5 节）
    activateGrip();

    move(60, 60);
    ASSERT_EQ(previewCount(), 1);
    move(60, 60, Qt::ShiftModifier);
    EXPECT_EQ(previewCount(), 1);
}

TEST_F(EditToolFixture, 右键取消夹点并清除预览)
{
    DmLine* line = activateGrip();
    move(60, 60);
    ASSERT_EQ(previewCount(), 1);

    EXPECT_EQ(press(60, 60, Qt::RightButton), ViewToolResult::Handled);
    EXPECT_EQ(editTool.getStatus(), EditTool::Neutral);
    EXPECT_EQ(previewCount(), 0);
    EXPECT_FALSE(editTool.getCursor().has_value());
    EXPECT_TRUE(selection.contains(line));
}

TEST_F(EditToolFixture, Esc取消夹点后选择层照常清空选择)
{
    DmLine* line = activateGrip();

    QKeyEvent esc(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    EXPECT_EQ(control.keyPressEvent(&esc), ViewToolResult::Handled);
    EXPECT_EQ(editTool.getStatus(), EditTool::Neutral);
    EXPECT_FALSE(selection.contains(line));
}

TEST_F(EditToolFixture, 夹点激活时中键平移不取消夹点)
{
    activateGrip();

    press(60, 60, Qt::MiddleButton);
    ASSERT_TRUE(panTool.isPanning());
    move(80, 60);
    EXPECT_GT(view.zoomPanCount, 0);
    release(80, 60, Qt::MiddleButton);
    EXPECT_FALSE(panTool.isPanning());
    EXPECT_EQ(editTool.getStatus(), EditTool::MovingRef);
}

TEST_F(EditToolFixture, 双击夹点不落到选择层)
{
    addSelectedLine();

    // Qt 的双击序列：按下、释放、双击、释放
    press(12, 10);
    release(12, 10);
    QMouseEvent dbl = makeMouse(QEvent::MouseButtonDblClick, 12, 10, Qt::LeftButton);
    EXPECT_EQ(control.mouseDoubleClickEvent(&dbl), ViewToolResult::Handled);
    release(12, 10);
    EXPECT_EQ(editTool.getStatus(), EditTool::MovingRef);
}

TEST_F(EditToolFixture, 离开画布清除预览但夹点保留)
{
    activateGrip();
    move(60, 60);
    ASSERT_EQ(previewCount(), 1);

    // 直接通知 EditTool：经 ViewToolControl 时选择层挂起也会清空同一个容器
    editTool.leaveEvent();
    EXPECT_EQ(previewCount(), 0);
    EXPECT_EQ(editTool.getStatus(), EditTool::MovingRef);

    move(70, 60);
    EXPECT_EQ(previewCount(), 1);
}

TEST_F(EditToolFixture, 没有夹点时移出业务栈或取消都不清除别人的预览)
{
    // 启动命令时总线把 EditTool 移出业务栈，结束全部命令时视图取消它，那时预览容器里可能是别的内容
    preview.addEntity(new DmLine(DmVector(0.0, 0.0), DmVector(1.0, 1.0)));
    control.deactivate(&editTool);
    editTool.cancel();
    EXPECT_EQ(previewCount(), 1);
}

TEST_F(EditToolFixture, 移出业务栈时取消激活的夹点)
{
    activateGrip();
    move(60, 60);
    ASSERT_EQ(previewCount(), 1);

    control.deactivate(&editTool);
    EXPECT_EQ(editTool.getStatus(), EditTool::Neutral);
    EXPECT_EQ(previewCount(), 0);
}
