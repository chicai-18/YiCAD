/// @file CommandTestFixture.h
/// @brief 交互命令单测的公共夹具：与 UIView 相同的装配加一个记录界面调用的对话框工厂
///
/// 业务工具化第三步（doc/COMMAND_TOOL_MIGRATION_PLAN.md）迁移的命令都经它测试：
/// 命令经 CommandRegistry 按 ID 构造，由命令总线运行；视图是 FakeDocumentView，
/// 文档是空文档，实体用 EntityTable::add_direct 放进表。默认构造的 DmDocument
/// 走事务会崩溃（test_geometry_spatial_query 的说明），因此用例不执行提交。

#ifndef YICAD_TEST_COMMAND_TEST_FIXTURE_H
#define YICAD_TEST_COMMAND_TEST_FIXTURE_H

#include <gtest/gtest.h>

#include <memory>
#include <utility>
#include <vector>

#include <QKeyEvent>
#include <QMouseEvent>

#include "CommandRegistry.h"
#include "DmDocument.h"
#include "DmEntityContainer.h"
#include "EntityTable.h"
#include "ExclusiveCommandBus.h"
#include "GuiCommandEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDialogFactoryAdapter.h"
#include "IExclusiveCommand.h"
#include "PanZoomTool.h"
#include "Preview.h"
#include "SelectTool.h"
#include "Snapper.h"
#include "ViewToolControl.h"
#include "support/FakeDocumentView.h"

namespace yicad_test
{
inline QMouseEvent makeMouse(QEvent::Type type, int x, int y, Qt::MouseButton button,
                             Qt::KeyboardModifiers mods = Qt::NoModifier)
{
    return QMouseEvent(type, QPointF(x, y), button, button, mods);
}

/// @brief 选项条请求的记录
struct OptionsRequest
{
    QString commandId; ///< 请求的命令 ID
    bool on = false;   ///< 打开还是关闭
    bool update = false;
};

/// @brief 记录提示、命令行消息、选项条与对话框的对话框工厂
class UiRecorder : public GuiDialogFactoryAdapter
{
public:
    std::vector<std::pair<QString, QString>> hints;
    std::vector<QString> messages;
    std::vector<OptionsRequest> options;
    int selectionUpdates = 0;

    void updateMouseWidget(const QString& left, const QString& right) override { hints.emplace_back(left, right); }
    void commandMessage(const QString& message) override { messages.push_back(message); }
    void updateSelectionWidget(int) override { ++selectionUpdates; }
    void requestCommandOptions(IExclusiveCommand* command, bool on, bool update) override
    {
        options.push_back({command ? command->commandId() : QString(), on, update});
    }

    /// @brief 最近一次按键提示的左键部分
    QString lastHint() const { return hints.empty() ? QString() : hints.back().first; }
    /// @brief 最近一次按键提示的右键部分
    QString lastRightHint() const { return hints.empty() ? QString() : hints.back().second; }
};

/// @brief 与 UIView 相同的装配；UiRecorder 在用例期间装进 GUIDIALOGFACTORY
struct CommandFixture : ::testing::Test
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

    CommandFixture()
    {
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
    ~CommandFixture() override { GuiDialogFactory::instance()->setFactoryObject(nullptr); }

    /// @brief 按命令 ID 构造并启动命令
    bool start(const char* id)
    {
        std::unique_ptr<IExclusiveCommand> command =
            CommandRegistry::instance().createCommand(QString::fromLatin1(id), CommandContext{&doc, &view});
        EXPECT_NE(command, nullptr) << id;
        return command && bus.start(std::move(command));
    }

    /// @brief 结束活动命令（与"结束全部命令"相同的路径）
    void endCommand()
    {
        ASSERT_TRUE(bus.approveEnd(CommandEndReason::Cancelled));
        bus.end();
    }

    /// @brief 经 ViewToolControl 分发，与 UIView 一样包在分发范围里
    template <typename Dispatch>
    ViewToolResult dispatch(Dispatch&& d)
    {
        ExclusiveCommandBus::DispatchScope scope(&bus);
        return d();
    }

    /// @brief 在 (x,y) 单击（按下并释放）
    void click(int x, int y, Qt::MouseButton button = Qt::LeftButton)
    {
        QMouseEvent press = makeMouse(QEvent::MouseButtonPress, x, y, button);
        QMouseEvent release = makeMouse(QEvent::MouseButtonRelease, x, y, button);
        dispatch([&] { return control.mousePressEvent(&press); });
        dispatch([&] { return control.mouseReleaseEvent(&release); });
    }

    /// @brief 右键：与 UIView 一样只发释放（右键释放不经选择层）
    void rightClick()
    {
        QMouseEvent release = makeMouse(QEvent::MouseButtonRelease, 0, 0, Qt::RightButton);
        dispatch([&] { return control.mouseReleaseEvent(&release); });
    }

    /// @brief 鼠标移到 (x,y)
    void move(int x, int y, Qt::KeyboardModifiers mods = Qt::NoModifier)
    {
        QMouseEvent e(QEvent::MouseMove, QPointF(x, y), Qt::NoButton, Qt::NoButton, mods);
        dispatch([&] { return control.mouseMoveEvent(&e); });
    }

    /// @brief 命令行坐标（已换算成世界坐标）
    void typeCoordinate(double x, double y)
    {
        dispatch([&] { return control.coordinateEvent(DmVector(x, y)); });
    }

    /// @brief 命令行文本，与 UIView 一样只沿业务栈分发
    /// @return 文本是否被接受（没有被接受时随后被当作新命令）
    bool typeText(const QString& text)
    {
        GuiCommandEvent e(text);
        dispatch([&] { return control.commandEvent(&e); });
        return e.isAccepted();
    }

    /// @brief 按键；返回按键事件是否被接受
    bool pressKey(int key)
    {
        QKeyEvent e(QEvent::KeyPress, key, Qt::NoModifier);
        dispatch([&] { return control.keyPressEvent(&e); });
        return e.isAccepted();
    }

    /// @brief 预览容器里的实体数
    int previewCount() { return view.getPreviewContainer()->size(); }

    /// @brief 光标仲裁的结果（ViewToolControl 最后一次设置的光标）
    std::optional<DM::CursorType> cursor() const { return view.lastCursor(); }

    /// @brief 该命令最后一次选项条请求
    const OptionsRequest* lastOptions(const char* id) const
    {
        for (auto it = ui.options.rbegin(); it != ui.options.rend(); ++it)
        {
            if (it->commandId == QLatin1String(id))
            {
                return &*it;
            }
        }
        return nullptr;
    }
};
}  // namespace yicad_test

#endif  // YICAD_TEST_COMMAND_TEST_FIXTURE_H
