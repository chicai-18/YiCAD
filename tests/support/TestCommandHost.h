/// @file TestCommandHost.h
/// @brief 测试用的命令宿主：像 UIView 一样实现 ICommandHost，并随命令启停让出、收回空闲态的工具
///
/// UIView 是 QOpenGLWidget，单测不构造它。夹具照 UIView 装配导航层、选择层、夹点编辑工具与
/// 工具控制器，本类补上 UIView 作为宿主的那部分：宿主能力，以及 UIView::onCommandStarting()/
/// onCommandFinished() 的同样做法。改动 UIView 的这部分时要同步这里。

#ifndef YICAD_TEST_TEST_COMMAND_HOST_H
#define YICAD_TEST_TEST_COMMAND_HOST_H

#include <QObject>

#include "DmDocument.h"
#include "EditTool.h"
#include "ExclusiveCommandBus.h"
#include "ICommandHost.h"
#include "IDocumentView.h"
#include "SelectTool.h"
#include "ViewToolControl.h"

namespace yicad_test
{
/// @brief 测试用的命令宿主
class TestCommandHost : public ICommandHost
{
public:
    /// @param editTool 夹点编辑工具；可为空，此时不装
    TestCommandHost(DmDocument& doc, IDocumentView& view, ViewToolControl& control, SelectTool& selectTool,
                    EditTool* editTool = nullptr)
        : m_doc(doc)
        , m_view(view)
        , m_control(control)
        , m_selectTool(selectTool)
        , m_editTool(editTool)
    {
    }

    /// @brief 接上总线：夹点编辑工具放上业务栈，连接命令启停的通知（与 UIView 构造时相同）
    /// @note 总线要在本对象之后构造、之前析构：它析构时仍会发 commandFinished()
    void attach(ExclusiveCommandBus& bus)
    {
        m_bus = &bus;
        if (m_editTool)
        {
            m_control.activate(m_editTool);
        }
        QObject::connect(&bus, &ExclusiveCommandBus::commandStarting, [this]() { onCommandStarting(); });
        QObject::connect(&bus, &ExclusiveCommandBus::commandFinished, [this]() { onCommandFinished(); });
    }

    DmDocument* document() override { return &m_doc; }
    IDocumentView* view() override { return &m_view; }
    ViewToolControl* viewToolControl() override { return &m_control; }
    ExclusiveCommandBus* commandBus() override { return m_bus; }
    void beginSelectionPhase(const EntityTypeList& entityTypes) override
    {
        m_selectTool.beginSelectionPhase(SelectTool::SelectionPhase{entityTypes});
    }
    void endSelectionPhase() override { m_selectTool.endSelectionPhase(); }

private:
    /// @brief 同 UIView::onCommandStarting()
    void onCommandStarting()
    {
        if (m_editTool)
        {
            m_control.deactivate(m_editTool);
        }
        m_selectTool.suspend();
    }

    /// @brief 同 UIView::onCommandFinished()
    void onCommandFinished()
    {
        if (m_selectTool.inSelectionPhase())
        {
            m_selectTool.endSelectionPhase();
        }
        m_selectTool.resume();
        if (m_editTool)
        {
            m_control.activate(m_editTool);
        }
    }

    DmDocument& m_doc;
    IDocumentView& m_view;
    ViewToolControl& m_control;
    SelectTool& m_selectTool;
    EditTool* m_editTool = nullptr;
    ExclusiveCommandBus* m_bus = nullptr;
};
}  // namespace yicad_test

#endif  // YICAD_TEST_TEST_COMMAND_HOST_H
