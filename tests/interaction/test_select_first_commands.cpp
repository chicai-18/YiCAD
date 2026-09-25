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

#include "BlockEditTool.h"
#include "BlockExtension.h"
#include "ExtensionManager.h"
#include "support/CommandExtensions.h"
#include "support/FakeExtensionHost.h"
#include "CircleData.h"
#include "CommandRegistry.h"
#include "DmCircle.h"
#include "DmDocument.h"
#include "DmLine.h"
#include "EntityTable.h"
#include "ExclusiveCommandBus.h"
#include "GuiDialogFactory.h"
#include "GuiDialogFactoryAdapter.h"
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
    return QMouseEvent(type, QPointF(x, y), QPointF(x, y), button, button, mods);
}

/// @brief 记录提示、命令行消息与选择计数的对话框工厂
class UiRecorder : public GuiDialogFactoryAdapter
{
public:
    std::vector<std::pair<QString, QString>> hints;
    std::vector<QString> messages;
    int selectionUpdates = 0;
    DialogAnswer answer = DialogAnswer::Cancel; ///< 是/否/取消对话框的回答
    int questions = 0;                          ///< 是/否/取消对话框弹出的次数
    std::vector<bool> blockEditOptions;         ///< 块编辑选项条的打开/关闭记录

    void updateMouseWidget(const QString& left, const QString& right) override { hints.emplace_back(left, right); }
    void commandMessage(const QString& message) override { messages.push_back(message); }
    void updateSelectionWidget(int) override { ++selectionUpdates; }
    DialogAnswer requestYesNoCancelDialog(const QString&, const QString&) override
    {
        ++questions;
        return answer;
    }
    void requestEditModeOptions(const std::function<QWidget*(QWidget*)>&, bool on) override
    {
        blockEditOptions.push_back(on);
    }
};

/// @brief 先选后建的 14 个命令
const char* const kSelectFirstCommands[] = {
    "ext.modify.move",    "ext.modify.copy",    "ext.modify.rotate", "ext.modify.scale", "ext.modify.mirror",
    "ext.modify.explode", "ext.modify.reverse", "ext.modify.delete", "ext.edit.copy",    "ext.edit.cut",
    "ext.modify.copy_to_layer", "ext.block.create", "ext.block.edit", "ext.measure.total_length"};

/// @brief 有放置工具、提示写在按键提示栏的命令，及其第一步提示的开头
struct FirstStep
{
    const char* id;
    const char* hint;
};
const FirstStep kPlaceToolCommands[] = {
    {"ext.modify.move", "Specify reference point"},
    {"ext.modify.copy", "Specify reference point or input copy number"},
    {"ext.modify.rotate", "Specify rotation center"},
    {"ext.modify.scale", "Specify reference point"},
    {"ext.modify.mirror", "Specify first point of mirror line"},
    {"ext.edit.copy", "Specify reference point"},
    {"ext.edit.cut", "Specify reference point"},
    {"ext.block.create", "Specify reference point"},
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
    ExclusiveCommandBus bus{&doc, &view, &control, &selectTool};
    /// @brief 创建块、编辑块在块扩展里（第三步⑥），其余先选后建命令在修改、编辑、查询扩展里
    ///        （第四步），用例期间启动它们
    yicad_test::FakeExtensionHost extensionHost;

    SelectFirstFixture()
    {
        yicad_test::registerCommandExtensions();
        ExtensionManager::instance().Register(std::make_unique<BlockExtension>());
        ExtensionManager::instance().BootAll(extensionHost);
        GuiDialogFactory::instance()->setFactoryObject(&ui);
        control.setNavigationTool(&panTool);
        control.setSelectionTool(&selectTool);
        selectTool.setOverlayQuery([this]()
                                   {
                                       if (bus.hasActiveCommand())
                                       {
                                           return SelectTool::Overlay::Command;
                                       }
                                       return bus.editMode() ? SelectTool::Overlay::EditMode
                                                             : SelectTool::Overlay::None;
                                   });
    }
    ~SelectFirstFixture() override
    {
        // 注销扩展的命令；已构造的命令与编辑模式照旧由总线析构时结束
        ExtensionManager::instance().Shutdown();
        GuiDialogFactory::instance()->setFactoryObject(nullptr);
    }

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

    /// @brief 进入块编辑模式（文档本身不进入块编辑：不跑事务，退出时也不改动文档）
    BlockEditTool* enterBlockEdit()
    {
        auto mode = std::make_unique<BlockEditTool>(bus);
        BlockEditTool* raw = mode.get();
        bus.enterEditMode(std::move(mode));
        return raw;
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
    ASSERT_TRUE(start("ext.measure.total_length"));

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
    ASSERT_TRUE(start("ext.measure.total_length"));

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
    ASSERT_TRUE(start("ext.measure.total_length"));

    EXPECT_EQ(pressKey(Qt::Key_Enter), ViewToolResult::Handled);
    EXPECT_TRUE(bus.hasActiveCommand());
    EXPECT_TRUE(selectTool.inSelectionPhase());
    EXPECT_TRUE(ui.messages.empty());
}

TEST_F(SelectFirstFixture, P5右键结束整个命令)
{
    ASSERT_TRUE(start("ext.measure.total_length"));

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
    ASSERT_TRUE(start("ext.measure.total_length"));

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
    ASSERT_TRUE(start("ext.measure.total_length"));

    QKeyEvent* space = nullptr;
    EXPECT_EQ(pressKey(Qt::Key_Space, &space), ViewToolResult::Handled);
    EXPECT_TRUE(space->isAccepted());
    EXPECT_TRUE(bus.hasActiveCommand());
}

TEST_F(SelectFirstFixture, P7已有选择集时跳过选择阶段)
{
    DmLine* line = addLine(DmVector(0.0, 0.0), DmVector(30.0, 40.0));
    line->setSelected(true);

    EXPECT_TRUE(start("ext.measure.total_length"));
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

    ASSERT_TRUE(start("ext.modify.delete"));
    EXPECT_TRUE(bus.hasActiveCommand());
    EXPECT_TRUE(selectTool.inSelectionPhase());
    EXPECT_TRUE(line->isSelected());
}

TEST_F(SelectFirstFixture, 选择阶段双击无反应)
{
    DmLine* line = addLine(DmVector(10.0, 10.0), DmVector(50.0, 10.0));
    ASSERT_TRUE(start("ext.measure.total_length"));

    QMouseEvent dbl = makeMouse(QEvent::MouseButtonDblClick, 30, 10, Qt::LeftButton);
    EXPECT_EQ(dispatch([&] { return control.mouseDoubleClickEvent(&dbl); }), ViewToolResult::Handled);
    EXPECT_FALSE(line->isSelected());
    EXPECT_EQ(selectTool.getStatus(), SelectTool::Neutral);
}

TEST_F(SelectFirstFixture, 选择阶段拖动不拖夹点也不拖实体)
{
    DmLine* line = addLine(DmVector(10.0, 10.0), DmVector(50.0, 10.0));
    line->setSelected(true);
    ASSERT_TRUE(start("ext.modify.delete"));

    // 在端点上按下并拖动超过阈值：空闲态会进入 MovingRef，选择阶段直接框选
    QMouseEvent press = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::LeftButton);
    QMouseEvent move = makeMouse(QEvent::MouseMove, 40, 40, Qt::NoButton);
    dispatch([&] { return control.mousePressEvent(&press); });
    dispatch([&] { return control.mouseMoveEvent(&move); });
    EXPECT_EQ(selectTool.getStatus(), SelectTool::SetCorner2);
}

TEST_F(SelectFirstFixture, 选择阶段Ctrl左键不让给平移)
{
    ASSERT_TRUE(start("ext.measure.total_length"));

    QMouseEvent press = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::LeftButton, Qt::ControlModifier);
    dispatch([&] { return control.mousePressEvent(&press); });
    EXPECT_FALSE(panTool.isPanning());
    EXPECT_EQ(selectTool.getStatus(), SelectTool::Dragging);
}

TEST_F(SelectFirstFixture, 选择阶段中键仍由导航层平移)
{
    ASSERT_TRUE(start("ext.measure.total_length"));

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

TEST_F(SelectFirstFixture, P1先选后建的14个命令没有选择集时都进入选择阶段)
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
    ASSERT_TRUE(start("ext.modify.copy_to_layer"));
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
    ASSERT_TRUE(start("ext.modify.move"));

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
    ASSERT_TRUE(start("ext.modify.move"));

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
    ASSERT_TRUE(start("ext.modify.move"));

    QKeyEvent* esc = nullptr;
    EXPECT_EQ(pressKey(Qt::Key_Escape, &esc), ViewToolResult::Handled);
    EXPECT_FALSE(esc->isAccepted());

    QMouseEvent press = makeMouse(QEvent::MouseButtonPress, 10, 10, Qt::MiddleButton);
    QMouseEvent drag(QEvent::MouseMove, QPointF(40, 30), QPointF(40, 30), Qt::NoButton, Qt::MiddleButton, Qt::NoModifier);
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
    ASSERT_TRUE(start("ext.modify.rotate"));

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
    ASSERT_TRUE(start("ext.modify.scale"));

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
    ASSERT_TRUE(start("ext.modify.copy"));
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
    ASSERT_TRUE(start("ext.modify.mirror"));
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

TEST_F(SelectFirstFixture, 编辑块时选择集里没有块参照则启动失败)
{
    DmLine* line = addLine(DmVector(10.0, 10.0), DmVector(50.0, 10.0));
    ASSERT_TRUE(start("ext.block.edit"));
    ASSERT_TRUE(selectTool.inSelectionPhase());

    line->setSelected(true);
    pressKey(Qt::Key_Enter);
    ASSERT_FALSE(ui.messages.empty());
    EXPECT_EQ(ui.messages.back(), QStringLiteral("No block reference selected. Command cancelled."));
    EXPECT_FALSE(bus.hasActiveCommand());
    EXPECT_EQ(bus.editMode(), nullptr);
}

TEST_F(SelectFirstFixture, B1块编辑模式显示提示与选项条且选择层不改提示)
{
    enterBlockEdit();
    ASSERT_FALSE(ui.hints.empty());
    EXPECT_EQ(ui.hints.back().first, QStringLiteral("Edit block entities"));
    EXPECT_EQ(ui.hints.back().second, QStringLiteral("Finish / Cancel"));
    ASSERT_FALSE(ui.blockEditOptions.empty());
    EXPECT_TRUE(ui.blockEditOptions.back());

    // B2、B3：块内点选、框选照常由选择层完成，提示保持块编辑的
    DmLine* line = addLine(DmVector(10.0, 10.0), DmVector(50.0, 10.0));
    const size_t hintsBefore = ui.hints.size();
    boxSelect(0, 0, 100, 100);
    EXPECT_TRUE(line->isSelected());
    EXPECT_EQ(ui.hints.size(), hintsBefore);
    // 选择层照常给出光标
    ASSERT_TRUE(selectTool.getCursor().has_value());
    EXPECT_EQ(*selectTool.getCursor(), DM::ArrowCursor);
}

TEST_F(SelectFirstFixture, B4块编辑中Esc清空选择仍在块编辑)
{
    enterBlockEdit();
    DmLine* line = addLine(DmVector(10.0, 10.0), DmVector(50.0, 10.0));
    line->setSelected(true);

    QKeyEvent* esc = nullptr;
    EXPECT_EQ(pressKey(Qt::Key_Escape, &esc), ViewToolResult::Handled);
    EXPECT_TRUE(esc->isAccepted());
    EXPECT_FALSE(line->isSelected());
    EXPECT_NE(bus.editMode(), nullptr);
}

TEST_F(SelectFirstFixture, B5块编辑中双击无反应)
{
    enterBlockEdit();
    QMouseEvent dbl = makeMouse(QEvent::MouseButtonDblClick, 30, 10, Qt::LeftButton);
    EXPECT_EQ(dispatch([&] { return control.mouseDoubleClickEvent(&dbl); }), ViewToolResult::Handled);
    EXPECT_EQ(view.selectedChangedCount, 0);
}

TEST_F(SelectFirstFixture, B6块编辑中启动的命令结束后回到块编辑)
{
    enterBlockEdit();
    DmLine* line = addLine(DmVector(10.0, 10.0), DmVector(50.0, 10.0));
    line->setSelected(true);

    ASSERT_TRUE(start("ext.modify.move"));
    // 命令叠在模式之上：选项条收起，提示归命令
    EXPECT_FALSE(ui.blockEditOptions.back());
    EXPECT_EQ(ui.hints.back().first, QStringLiteral("Specify reference point"));

    click(0, 0, Qt::RightButton);
    EXPECT_FALSE(bus.hasActiveCommand());
    EXPECT_NE(bus.editMode(), nullptr);
    EXPECT_TRUE(ui.blockEditOptions.back());
    EXPECT_EQ(ui.hints.back().first, QStringLiteral("Edit block entities"));
}

TEST_F(SelectFirstFixture, B7块编辑中右键询问是否保存取消则继续编辑)
{
    enterBlockEdit();
    ui.answer = DialogAnswer::Cancel;
    click(0, 0, Qt::RightButton);
    EXPECT_EQ(ui.questions, 1);
    EXPECT_NE(bus.editMode(), nullptr);

    ui.answer = DialogAnswer::Yes;
    click(0, 0, Qt::RightButton);
    EXPECT_EQ(ui.questions, 2);
    EXPECT_EQ(bus.editMode(), nullptr);
    EXPECT_FALSE(ui.blockEditOptions.back());
}

TEST_F(SelectFirstFixture, 结束全部命令时块编辑弹出同样的对话框取消即否决)
{
    enterBlockEdit();
    ui.answer = DialogAnswer::Cancel;
    EXPECT_FALSE(bus.approveEndAll(CommandEndReason::Cancelled));
    EXPECT_EQ(ui.questions, 1);
    EXPECT_NE(bus.editMode(), nullptr);

    ui.answer = DialogAnswer::No;
    ASSERT_TRUE(bus.approveEndAll(CommandEndReason::Cancelled));
    bus.endAll();
    EXPECT_EQ(ui.questions, 2);
    EXPECT_EQ(bus.editMode(), nullptr);
}

TEST_F(SelectFirstFixture, 视图关闭时块编辑不提问)
{
    enterBlockEdit();
    EXPECT_TRUE(bus.approveEndAll(CommandEndReason::ViewClosing));
    bus.endAll();
    EXPECT_EQ(ui.questions, 0);
    EXPECT_EQ(bus.editMode(), nullptr);
}

TEST_F(SelectFirstFixture, 块编辑的选项条完成按钮经事件循环退出)
{
    BlockEditTool* blockEdit = enterBlockEdit();
    // 选项条按钮在分发范围之外：模式要等事件循环再退出，按钮的槽函数返回前不能销毁它
    blockEdit->completeEditing(true);
    EXPECT_EQ(bus.editMode(), blockEdit);
    for (int i = 0; i < 5 && bus.editMode(); ++i)
    {
        QCoreApplication::processEvents();
    }
    EXPECT_EQ(bus.editMode(), nullptr);
}
