/// @file test_select_first_commands.cpp
/// @brief 先选后建命令（SelectFirstCommand）选择阶段的单元测试
///
/// 对照 doc/INTERACTION_CHECKLIST.md 第 3 节 P1–P8，逐项锁定迁移后与原
/// ActionSelect + ActionSelectMultiple 一致的行为（doc/COMMAND_TOOL_MIGRATION_PLAN.md
/// 第二步第 6 项）。命令经 CommandRegistry 按 ID 构造，由与 UIView 相同装配的
/// 命令总线运行；视图是 FakeDocumentView，文档是空文档，实体用
/// EntityTable::add_direct 放进表（默认构造的 DmDocument 走 add() 的 Cmd 路径
/// 会崩溃，同理这里不执行真正的删除）。

#include <gtest/gtest.h>

#include <memory>
#include <utility>
#include <vector>

#include <QKeyEvent>
#include <QMouseEvent>

#include "ActionInterface.h"
#include "CircleData.h"
#include "CommandRegistry.h"
#include "DmCircle.h"
#include "DmDocument.h"
#include "DmLine.h"
#include "EntityTable.h"
#include "ExclusiveCommandBus.h"
#include "GuiDialogFactory.h"
#include "GuiDialogFactoryAdapter.h"
#include "GuiEventHandler.h"
#include "GuiCommandEvent.h"
#include "IExclusiveCommand.h"
#include "ModifyCopyCommand.h"
#include "ModifyMirrorCommand.h"
#include "PanZoomTool.h"
#include "Preview.h"
#include "SelectTool.h"
#include "Snapper.h"
#include "ViewToolControl.h"
#include "support/FakeDocumentView.h"

namespace
{
QMouseEvent makeMouse(QEvent::Type type, int x, int y, Qt::MouseButton button,
                      Qt::KeyboardModifiers mods = Qt::NoModifier)
{
    return QMouseEvent(type, QPointF(x, y), button, button, mods);
}

/// @brief 记录提示、命令行消息与选择计数的对话框工厂
class UiRecorder : public GuiDialogFactoryAdapter
{
public:
    std::vector<std::pair<QString, QString>> hints;
    std::vector<QString> messages;
    int selectionUpdates = 0;

    void updateMouseWidget(const QString& left, const QString& right) override { hints.emplace_back(left, right); }
    void commandMessage(const QString& message) override { messages.push_back(message); }
    void updateSelectionWidget(int) override { ++selectionUpdates; }
};

/// @brief 与 UIView 相同：旧 Action 栈之下是命令与选择层
struct LegacyStackBase : ILegacyStackBase
{
    ExclusiveCommandBus& bus;
    SelectTool& selectTool;
    LegacyStackBase(ExclusiveCommandBus& b, SelectTool& s)
        : bus(b)
        , selectTool(s)
    {
    }
    void suspendForLegacy() override
    {
        bus.suspend();
        selectTool.suspend();
    }
    void resumeAfterLegacy() override
    {
        selectTool.resume();
        bus.resume();
    }
    void resetAfterKill() override { selectTool.init(); }
};

/// @brief 先选后建的 13 个命令（编辑块见第二步的提交③）
const char* const kSelectFirstCommands[] = {
    "modify.move",    "modify.copy",    "modify.rotate", "modify.scale", "modify.mirror",
    "modify.explode", "modify.reverse", "modify.delete", "edit.copy",    "edit.cut",
    "modify.copy_to_layer", "blocks.create", "info.total_length"};

/// @brief 有放置工具、提示写在按键提示栏的命令，及其第一步提示的开头
struct FirstStep
{
    const char* id;
    const char* hint;
};
const FirstStep kPlaceToolCommands[] = {
    {"modify.move", "Specify reference point"},
    {"modify.copy", "Specify reference point or input copy number"},
    {"modify.rotate", "Specify rotation center"},
    {"modify.scale", "Specify reference point"},
    {"modify.mirror", "Specify first point of mirror line"},
    {"edit.copy", "Specify reference point"},
    {"edit.cut", "Specify reference point"},
    {"blocks.create", "Specify reference point"},
};

/// @brief 与 UIView 相同的装配；UiRecorder 在用例期间装进 GUIDIALOGFACTORY
struct SelectFirstFixture : ::testing::Test
{
    UiRecorder ui;
    DmDocument doc;
    FakeDocumentView view;
    Preview preview{&doc, &view};
    Snapper snapper{&doc, &view};
    PanZoomTool panTool{&view};
    SelectTool selectTool{&doc, &view, &snapper, &preview, &panTool};
    ViewToolControl control{&view};
    /// @brief 用例装上旧版 Action 栈时设置；声明在总线之前，总线析构时还会经选择层查询它
    GuiEventHandler* legacyHandler = nullptr;
    ExclusiveCommandBus bus{&doc, &view, &control, &selectTool};

    SelectFirstFixture()
    {
        GuiDialogFactory::instance()->setFactoryObject(&ui);
        control.setNavigationTool(&panTool);
        control.setSelectionTool(&selectTool);
        selectTool.setOverlayQuery([this]()
                                   {
                                       if (legacyHandler && legacyHandler->hasAction())
                                       {
                                           return SelectTool::Overlay::LegacyAction;
                                       }
                                       return bus.hasActiveCommand() ? SelectTool::Overlay::Command
                                                                     : SelectTool::Overlay::None;
                                   });
    }
    ~SelectFirstFixture() override { GuiDialogFactory::instance()->setFactoryObject(nullptr); }

    /// @brief 按命令 ID 构造并启动命令
    bool start(const char* id)
    {
        std::unique_ptr<IExclusiveCommand> command =
            CommandRegistry::instance().createCommand(QString::fromLatin1(id), CommandContext{&doc, &view});
        EXPECT_NE(command, nullptr) << id;
        return command && bus.start(std::move(command));
    }

    DmLine* addLine(const DmVector& a, const DmVector& b)
    {
        auto* line = new DmLine(a, b);
        line->calculateBorders();
        EXPECT_TRUE(doc.getEntityTable()->add_direct(line));
        return line;
    }

    DmCircle* addCircle(const DmVector& center, double radius)
    {
        auto* circle = new DmCircle(nullptr, CircleData(center, radius));
        circle->calculateBorders();
        EXPECT_TRUE(doc.getEntityTable()->add_direct(circle));
        return circle;
    }

    /// @brief 经 ViewToolControl 分发，与 UIView 一样包在分发范围里
    template <typename Dispatch>
    ViewToolResult dispatch(Dispatch&& d)
    {
        ExclusiveCommandBus::DispatchScope scope(&bus);
        return d();
    }

    /// @brief 从 (x1,y1) 到 (x2,y2) 框选：单击起点、移动、单击终点
    void boxSelect(int x1, int y1, int x2, int y2, Qt::KeyboardModifiers mods = Qt::NoModifier)
    {
        QMouseEvent press1 = makeMouse(QEvent::MouseButtonPress, x1, y1, Qt::LeftButton);
        QMouseEvent release1 = makeMouse(QEvent::MouseButtonRelease, x1, y1, Qt::LeftButton);
        QMouseEvent move = makeMouse(QEvent::MouseMove, x2, y2, Qt::NoButton);
        QMouseEvent press2 = makeMouse(QEvent::MouseButtonPress, x2, y2, Qt::LeftButton, mods);
        QMouseEvent release2 = makeMouse(QEvent::MouseButtonRelease, x2, y2, Qt::LeftButton, mods);
        dispatch([&] { return control.mousePressEvent(&press1); });
        dispatch([&] { return control.mouseReleaseEvent(&release1); });
        dispatch([&] { return control.mouseMoveEvent(&move); });
        dispatch([&] { return control.mousePressEvent(&press2); });
        dispatch([&] { return control.mouseReleaseEvent(&release2); });
    }

    /// @brief 在 (x,y) 单击（按下并释放）
    void click(int x, int y, Qt::MouseButton button = Qt::LeftButton)
    {
        QMouseEvent press = makeMouse(QEvent::MouseButtonPress, x, y, button);
        QMouseEvent release = makeMouse(QEvent::MouseButtonRelease, x, y, button);
        dispatch([&] { return control.mousePressEvent(&press); });
        dispatch([&] { return control.mouseReleaseEvent(&release); });
    }

    /// @brief 结束活动命令（与"结束全部命令"相同的路径）
    void endCommand()
    {
        ASSERT_TRUE(bus.approveEnd(CommandEndReason::Cancelled));
        bus.end();
    }

    /// @brief 命令行文本，与 UIView 一样只沿业务栈分发
    ViewToolResult typeText(GuiCommandEvent& e)
    {
        return dispatch([&] { return control.commandEvent(&e); });
    }

    ViewToolResult pressKey(int key, QKeyEvent** out = nullptr)
    {
        m_lastKey = std::make_unique<QKeyEvent>(QEvent::KeyPress, key, Qt::NoModifier);
        if (out)
        {
            *out = m_lastKey.get();
        }
        return dispatch([&] { return control.keyPressEvent(m_lastKey.get()); });
    }

private:
    std::unique_ptr<QKeyEvent> m_lastKey;
};
}  // namespace

TEST_F(SelectFirstFixture, P1没有选择集时进入选择阶段并给出原ActionSelectMultiple的提示)
{
    ASSERT_TRUE(start("info.total_length"));

    EXPECT_TRUE(bus.hasActiveCommand());
    EXPECT_TRUE(selectTool.inSelectionPhase());
    ASSERT_FALSE(ui.hints.empty());
    // 原 ActionSelect 的"Select to …"提示被 ActionSelectMultiple 覆盖，从未显示过
    EXPECT_EQ(ui.hints.back().first, QStringLiteral("Click and drag for the selection window"));
    EXPECT_EQ(ui.hints.back().second, QStringLiteral("Cancel"));
    ASSERT_TRUE(view.lastCursor().has_value());
    EXPECT_EQ(*view.lastCursor(), DM::SelectCursor);
}

TEST_F(SelectFirstFixture, P2P3框选后回车开始真正的命令)
{
    DmLine* line = addLine(DmVector(10.0, 10.0), DmVector(50.0, 10.0));
    ASSERT_TRUE(start("info.total_length"));

    boxSelect(0, 0, 100, 100);
    EXPECT_TRUE(line->isSelected());
    // 选择阶段只刷新选择计数，不发 selectedChanged（否则会启动多行文字属性编辑）
    EXPECT_GT(ui.selectionUpdates, 0);
    EXPECT_EQ(view.selectedChangedCount, 0);
    EXPECT_TRUE(bus.hasActiveCommand());

    EXPECT_EQ(pressKey(Qt::Key_Enter), ViewToolResult::Handled);
    ASSERT_FALSE(ui.messages.empty());
    EXPECT_TRUE(ui.messages.back().startsWith(QStringLiteral("Total Length of selected entities")));
    EXPECT_FALSE(bus.hasActiveCommand());
    EXPECT_FALSE(selectTool.inSelectionPhase());
}

TEST_F(SelectFirstFixture, P4没有选择集时回车无反应)
{
    ASSERT_TRUE(start("info.total_length"));

    EXPECT_EQ(pressKey(Qt::Key_Enter), ViewToolResult::Handled);
    EXPECT_TRUE(bus.hasActiveCommand());
    EXPECT_TRUE(selectTool.inSelectionPhase());
    EXPECT_TRUE(ui.messages.empty());
}

TEST_F(SelectFirstFixture, P5右键结束整个命令)
{
    ASSERT_TRUE(start("info.total_length"));

    // 进行中的框选也一并取消
    QMouseEvent press = makeMouse(QEvent::MouseButtonPress, 0, 0, Qt::LeftButton);
    QMouseEvent release = makeMouse(QEvent::MouseButtonRelease, 0, 0, Qt::LeftButton);
    dispatch([&] { return control.mousePressEvent(&press); });
    dispatch([&] { return control.mouseReleaseEvent(&release); });
    ASSERT_EQ(selectTool.getStatus(), SelectTool::SetCorner2);

    QMouseEvent right = makeMouse(QEvent::MouseButtonRelease, 0, 0, Qt::RightButton);
    EXPECT_EQ(dispatch([&] { return control.mouseReleaseEvent(&right); }), ViewToolResult::Handled);
    EXPECT_FALSE(bus.hasActiveCommand());
    EXPECT_FALSE(selectTool.inSelectionPhase());
    EXPECT_EQ(selectTool.getStatus(), SelectTool::Neutral);
}

TEST_F(SelectFirstFixture, P6Esc不接受由主窗口结束全部命令)
{
    ASSERT_TRUE(start("info.total_length"));

    QKeyEvent* esc = nullptr;
    EXPECT_EQ(pressKey(Qt::Key_Escape, &esc), ViewToolResult::Cancel);
    EXPECT_FALSE(esc->isAccepted());
    // 结束全部命令由主窗口在事件未被接受时发起，选择阶段自己不结束
    EXPECT_TRUE(bus.hasActiveCommand());
}

TEST_F(SelectFirstFixture, P6空格被选择阶段接受命令继续)
{
    // 与清单原先写的不同：原 ActionSelectMultiple 不忽略空格，事件保持接受，
    // 主窗口因此不结束命令（迁移计划 9.2 节）。
    ASSERT_TRUE(start("info.total_length"));

    QKeyEvent* space = nullptr;
    EXPECT_EQ(pressKey(Qt::Key_Space, &space), ViewToolResult::Handled);
    EXPECT_TRUE(space->isAccepted());
    EXPECT_TRUE(bus.hasActiveCommand());
}

TEST_F(SelectFirstFixture, P7已有选择集时跳过选择阶段)
{
    DmLine* line = addLine(DmVector(0.0, 0.0), DmVector(30.0, 40.0));
    line->setSelected(true);

    EXPECT_TRUE(start("info.total_length"));
    EXPECT_FALSE(selectTool.inSelectionPhase());
    ASSERT_EQ(ui.messages.size(), 1u);
    EXPECT_TRUE(ui.messages.front().startsWith(QStringLiteral("Total Length of selected entities")));
    // 激活期间就完成：总线在激活返回后结束它
    EXPECT_FALSE(bus.hasActiveCommand());
}

TEST_F(SelectFirstFixture, P8已有选择集时删除仍先进入选择阶段)
{
    DmLine* line = addLine(DmVector(0.0, 0.0), DmVector(30.0, 40.0));
    line->setSelected(true);

    ASSERT_TRUE(start("modify.delete"));
    EXPECT_TRUE(bus.hasActiveCommand());
    EXPECT_TRUE(selectTool.inSelectionPhase());
    EXPECT_TRUE(line->isSelected());
}

TEST_F(SelectFirstFixture, 选择阶段双击无反应)
{
    DmLine* line = addLine(DmVector(10.0, 10.0), DmVector(50.0, 10.0));
    ASSERT_TRUE(start("info.total_length"));

    QMouseEvent dbl = makeMouse(QEvent::MouseButtonDblClick, 30, 10, Qt::LeftButton);
    EXPECT_EQ(dispatch([&] { return control.mouseDoubleClickEvent(&dbl); }), ViewToolResult::Handled);
    EXPECT_FALSE(line->isSelected());
    EXPECT_EQ(selectTool.getStatus(), SelectTool::Neutral);
}

TEST_F(SelectFirstFixture, 选择阶段拖动不拖夹点也不拖实体)
{
    DmLine* line = addLine(DmVector(10.0, 10.0), DmVector(50.0, 10.0));
    line->setSelected(true);
    ASSERT_TRUE(start("modify.delete"));

    // 在端点上按下并拖动超过阈值：空闲态会进入 MovingRef，选择阶段直接框选
    QMouseEvent press = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::LeftButton);
    QMouseEvent move = makeMouse(QEvent::MouseMove, 40, 40, Qt::NoButton);
    dispatch([&] { return control.mousePressEvent(&press); });
    dispatch([&] { return control.mouseMoveEvent(&move); });
    EXPECT_EQ(selectTool.getStatus(), SelectTool::SetCorner2);
}

TEST_F(SelectFirstFixture, 选择阶段Ctrl左键不让给平移)
{
    ASSERT_TRUE(start("info.total_length"));

    QMouseEvent press = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::LeftButton, Qt::ControlModifier);
    dispatch([&] { return control.mousePressEvent(&press); });
    EXPECT_FALSE(panTool.isPanning());
    EXPECT_EQ(selectTool.getStatus(), SelectTool::Dragging);
}

TEST_F(SelectFirstFixture, 选择阶段中键仍由导航层平移)
{
    ASSERT_TRUE(start("info.total_length"));

    QMouseEvent press = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::MiddleButton);
    dispatch([&] { return control.mousePressEvent(&press); });
    EXPECT_TRUE(panTool.isPanning());
}

TEST_F(SelectFirstFixture, 选择阶段按实体类型过滤)
{
    DmLine* line = addLine(DmVector(10.0, 10.0), DmVector(50.0, 10.0));
    DmCircle* circle = addCircle(DmVector(30.0, 50.0), 10.0);
    selectTool.beginSelectionPhase(SelectTool::SelectionPhase{EntityTypeList{DM::EntityCircle}});

    boxSelect(0, 0, 100, 100);
    EXPECT_FALSE(line->isSelected());
    EXPECT_TRUE(circle->isSelected());

    selectTool.endSelectionPhase();
}

TEST_F(SelectFirstFixture, 旧Action叠在命令之上时命令被挂起结束后恢复)
{
    LegacyStackBase stackBase{bus, selectTool};

    GuiEventHandler handler(nullptr);
    handler.setStackBase(&stackBase);
    legacyHandler = &handler;
    // 断言提前返回时也要在 handler 析构前解除引用：夹具析构总线时还会查询它
    struct Detach
    {
        GuiEventHandler*& ref;
        ~Detach() { ref = nullptr; }
    } detach{legacyHandler};
    ASSERT_TRUE(start("info.total_length"));

    auto* legacy = new ActionInterface("test-legacy-action", &doc, &view);
    handler.setCurrentAction(legacy);
    EXPECT_TRUE(bus.isSuspended());
    // 光标与提示归旧 Action，选择阶段让出
    EXPECT_FALSE(selectTool.getCursor().has_value());
    const size_t hintsBefore = ui.hints.size();
    selectTool.enterEvent();
    EXPECT_EQ(ui.hints.size(), hintsBefore);
    // 挂起时选择阶段工具被停用：回车不再确认，事件落到选择层
    QKeyEvent* enter = nullptr;
    EXPECT_EQ(pressKey(Qt::Key_Enter, &enter), ViewToolResult::NotHandled);

    legacy->finish();
    handler.cleanUp();
    EXPECT_FALSE(bus.isSuspended());
    EXPECT_TRUE(bus.hasActiveCommand());
    ASSERT_TRUE(selectTool.getCursor().has_value());
    EXPECT_EQ(*selectTool.getCursor(), DM::SelectCursor);
    EXPECT_EQ(pressKey(Qt::Key_Enter), ViewToolResult::Handled);
}

TEST_F(SelectFirstFixture, P1先选后建的13个命令没有选择集时都进入选择阶段)
{
    for (const char* id : kSelectFirstCommands)
    {
        SCOPED_TRACE(id);
        ui.hints.clear();
        ASSERT_TRUE(start(id));
        EXPECT_TRUE(selectTool.inSelectionPhase());
        ASSERT_FALSE(ui.hints.empty());
        EXPECT_EQ(ui.hints.back().first, QStringLiteral("Click and drag for the selection window"));
        endCommand();
        EXPECT_FALSE(selectTool.inSelectionPhase());
    }
}

TEST_F(SelectFirstFixture, P3回车确认后进入命令的第一步)
{
    DmLine* line = addLine(DmVector(10.0, 10.0), DmVector(50.0, 10.0));
    for (const FirstStep& step : kPlaceToolCommands)
    {
        SCOPED_TRACE(step.id);
        line->setSelected(false);
        ASSERT_TRUE(start(step.id));
        ASSERT_TRUE(selectTool.inSelectionPhase());

        line->setSelected(true);
        ui.hints.clear();
        EXPECT_EQ(pressKey(Qt::Key_Enter), ViewToolResult::Handled);
        EXPECT_TRUE(bus.hasActiveCommand());
        EXPECT_FALSE(selectTool.inSelectionPhase());
        ASSERT_FALSE(ui.hints.empty());
        EXPECT_TRUE(ui.hints.back().first.startsWith(QString::fromLatin1(step.hint)))
            << ui.hints.back().first.toStdString();
        // 放置工具经 getCursor() 给出十字光标，捕捉器随工具活动
        ASSERT_TRUE(view.lastCursor().has_value());
        EXPECT_EQ(*view.lastCursor(), DM::CadCursor);
        EXPECT_NE(bus.activeCommand()->snapService(), nullptr);
        endCommand();
    }
}

TEST_F(SelectFirstFixture, P7已有选择集时直接进入命令的第一步)
{
    DmLine* line = addLine(DmVector(10.0, 10.0), DmVector(50.0, 10.0));
    for (const FirstStep& step : kPlaceToolCommands)
    {
        SCOPED_TRACE(step.id);
        line->setSelected(true);
        ui.hints.clear();
        ASSERT_TRUE(start(step.id));
        EXPECT_TRUE(bus.hasActiveCommand());
        EXPECT_FALSE(selectTool.inSelectionPhase());
        ASSERT_FALSE(ui.hints.empty());
        EXPECT_TRUE(ui.hints.back().first.startsWith(QString::fromLatin1(step.hint)))
            << ui.hints.back().first.toStdString();
        endCommand();
    }
}

TEST_F(SelectFirstFixture, 复制到图层的提示写在命令行且右键在第一步结束命令)
{
    DmLine* line = addLine(DmVector(10.0, 10.0), DmVector(50.0, 10.0));
    line->setSelected(true);
    ASSERT_TRUE(start("modify.copy_to_layer"));
    ASSERT_FALSE(ui.messages.empty());
    EXPECT_EQ(ui.messages.back(), QStringLiteral("Select the object on the target layer"));

    // 空白处单击拾取不到实体，仍在第一步
    click(200, 200);
    EXPECT_TRUE(bus.hasActiveCommand());

    click(200, 200, Qt::RightButton);
    EXPECT_FALSE(bus.hasActiveCommand());
}

TEST_F(SelectFirstFixture, 移动工具右键退回上一步第一步时结束命令)
{
    DmLine* line = addLine(DmVector(10.0, 10.0), DmVector(50.0, 10.0));
    line->setSelected(true);
    ASSERT_TRUE(start("modify.move"));

    click(0, 0);
    ASSERT_FALSE(ui.hints.empty());
    EXPECT_EQ(ui.hints.back().first, QStringLiteral("Specify target point"));

    // 第二步移动鼠标：预览跟随
    QMouseEvent move = makeMouse(QEvent::MouseMove, 20, 20, Qt::NoButton);
    dispatch([&] { return control.mouseMoveEvent(&move); });
    EXPECT_FALSE(view.getPreviewContainer()->isEmpty());

    click(20, 20, Qt::RightButton);
    EXPECT_TRUE(bus.hasActiveCommand());
    EXPECT_EQ(ui.hints.back().first, QStringLiteral("Specify reference point"));
    EXPECT_TRUE(view.getPreviewContainer()->isEmpty());

    click(20, 20, Qt::RightButton);
    EXPECT_FALSE(bus.hasActiveCommand());
}

TEST_F(SelectFirstFixture, 放置工具接收命令行坐标但不接受文本)
{
    DmLine* line = addLine(DmVector(10.0, 10.0), DmVector(50.0, 10.0));
    line->setSelected(true);
    ASSERT_TRUE(start("modify.move"));

    EXPECT_EQ(dispatch([&] { return control.coordinateEvent(DmVector(5.0, 6.0)); }), ViewToolResult::Handled);
    EXPECT_EQ(ui.hints.back().first, QStringLiteral("Specify target point"));
    EXPECT_DOUBLE_EQ(view.getRelativeZero().x, 5.0);
    EXPECT_DOUBLE_EQ(view.getRelativeZero().y, 6.0);

    // 移动没有命令行选项：文本不被接受，随后会被当作新命令解析
    GuiCommandEvent text("line");
    EXPECT_EQ(typeText(text), ViewToolResult::NotHandled);
    EXPECT_FALSE(text.isAccepted());
}

TEST_F(SelectFirstFixture, 放置工具不接受Esc且把中键平移让给导航层)
{
    DmLine* line = addLine(DmVector(10.0, 10.0), DmVector(50.0, 10.0));
    line->setSelected(true);
    ASSERT_TRUE(start("modify.move"));

    QKeyEvent* esc = nullptr;
    EXPECT_EQ(pressKey(Qt::Key_Escape, &esc), ViewToolResult::Handled);
    EXPECT_FALSE(esc->isAccepted());

    QMouseEvent press = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::MiddleButton);
    QMouseEvent drag(QEvent::MouseMove, QPointF(40, 30), Qt::NoButton, Qt::MiddleButton, Qt::NoModifier);
    QMouseEvent release = makeMouse(QEvent::MouseButtonRelease, 40, 30, Qt::MiddleButton);
    dispatch([&] { return control.mousePressEvent(&press); });
    EXPECT_TRUE(panTool.isPanning());
    dispatch([&] { return control.mouseMoveEvent(&drag); });
    EXPECT_GT(view.zoomPanCount, 0);
    dispatch([&] { return control.mouseReleaseEvent(&release); });
    EXPECT_FALSE(panTool.isPanning());
    EXPECT_TRUE(bus.hasActiveCommand());
}

TEST_F(SelectFirstFixture, 旋转工具设置中心时不接受文本设置角度时接受)
{
    DmLine* line = addLine(DmVector(10.0, 10.0), DmVector(50.0, 10.0));
    line->setSelected(true);
    ASSERT_TRUE(start("modify.rotate"));

    GuiCommandEvent early("30");
    EXPECT_EQ(typeText(early), ViewToolResult::NotHandled);
    EXPECT_FALSE(early.isAccepted());

    dispatch([&] { return control.coordinateEvent(DmVector(0.0, 0.0)); });
    EXPECT_EQ(ui.hints.back().first, QStringLiteral("Input angle"));

    GuiCommandEvent invalid("abc");
    EXPECT_EQ(typeText(invalid), ViewToolResult::Handled);
    EXPECT_TRUE(invalid.isAccepted());
    EXPECT_EQ(ui.hints.back().first, QStringLiteral("Input invalid"));
    EXPECT_TRUE(bus.hasActiveCommand());
}

TEST_F(SelectFirstFixture, 缩放工具设置基点时文本被接受但不起作用)
{
    DmLine* line = addLine(DmVector(10.0, 10.0), DmVector(50.0, 10.0));
    line->setSelected(true);
    ASSERT_TRUE(start("modify.scale"));

    GuiCommandEvent text("2");
    EXPECT_EQ(typeText(text), ViewToolResult::Handled);
    EXPECT_TRUE(text.isAccepted());
    EXPECT_TRUE(bus.hasActiveCommand());
    EXPECT_EQ(ui.hints.back().first, QStringLiteral("Specify reference point"));
}

TEST_F(SelectFirstFixture, 复制工具随时可输入复制数量)
{
    DmLine* line = addLine(DmVector(10.0, 10.0), DmVector(50.0, 10.0));
    line->setSelected(true);
    ASSERT_TRUE(start("modify.copy"));
    auto* command = dynamic_cast<ModifyCopyCommand*>(bus.activeCommand());
    ASSERT_NE(command, nullptr);

    GuiCommandEvent count("3");
    EXPECT_EQ(typeText(count), ViewToolResult::Handled);
    EXPECT_EQ(command->copyCount(), 3);

    GuiCommandEvent invalid("0");
    EXPECT_EQ(typeText(invalid), ViewToolResult::Handled);
    EXPECT_EQ(ui.hints.back().first, QStringLiteral("Input invalid"));
    EXPECT_EQ(command->copyCount(), 3);

    // 提示里的数量在状态变化时才刷新（原有行为）
    click(0, 0);
    EXPECT_TRUE(ui.hints.back().first.endsWith(QStringLiteral("is 3"))) << ui.hints.back().first.toStdString();
}

TEST_F(SelectFirstFixture, 镜像工具输入YN切换复制方式)
{
    DmLine* line = addLine(DmVector(10.0, 10.0), DmVector(50.0, 10.0));
    line->setSelected(true);
    ASSERT_TRUE(start("modify.mirror"));
    auto* command = dynamic_cast<ModifyMirrorCommand*>(bus.activeCommand());
    ASSERT_NE(command, nullptr);

    GuiCommandEvent no("N");
    EXPECT_EQ(typeText(no), ViewToolResult::Handled);
    EXPECT_FALSE(command->copies());

    GuiCommandEvent other("x");
    EXPECT_EQ(typeText(other), ViewToolResult::Handled);
    EXPECT_EQ(ui.hints.back().first, QStringLiteral("Input invalid"));

    click(0, 0);
    EXPECT_TRUE(ui.hints.back().first.endsWith(QStringLiteral("[delete origin]")))
        << ui.hints.back().first.toStdString();

    GuiCommandEvent yes("y");
    typeText(yes);
    EXPECT_TRUE(command->copies());
}

TEST_F(SelectFirstFixture, 旧Action叠在放置工具之上时停用工具结束后恢复)
{
    LegacyStackBase stackBase{bus, selectTool};
    GuiEventHandler handler(nullptr);
    handler.setStackBase(&stackBase);
    legacyHandler = &handler;
    struct Detach
    {
        GuiEventHandler*& ref;
        ~Detach() { ref = nullptr; }
    } detach{legacyHandler};

    DmLine* line = addLine(DmVector(10.0, 10.0), DmVector(50.0, 10.0));
    line->setSelected(true);
    ASSERT_TRUE(start("modify.move"));

    auto* legacy = new ActionInterface("test-legacy-action", &doc, &view);
    handler.setCurrentAction(legacy);
    EXPECT_TRUE(bus.isSuspended());
    // 工具被停用：命令行坐标没有业务工具接收
    EXPECT_EQ(dispatch([&] { return control.coordinateEvent(DmVector(1.0, 1.0)); }), ViewToolResult::NotHandled);

    legacy->finish();
    handler.cleanUp();
    EXPECT_FALSE(bus.isSuspended());
    EXPECT_EQ(ui.hints.back().first, QStringLiteral("Specify reference point"));
    EXPECT_EQ(dispatch([&] { return control.coordinateEvent(DmVector(1.0, 1.0)); }), ViewToolResult::Handled);
}
