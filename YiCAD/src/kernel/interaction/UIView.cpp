/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/// @file UIView.cpp
/// @brief UIView 的实现：交互层工具的装配与 Qt 输入事件的分发

#include "UIView.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QTabletEvent>
#include <QWheelEvent>

#include "ActionInterface.h"
#include "ActionZoomIn.h"
#include "CommandRegistry.h"
#include "DmDocument.h"
#include "DmSettings.h"
#include "EntityTable.h"
#include "ExclusiveCommandBus.h"
#include "GuiCommandEvent.h"
#include "GuiCoordinateInput.h"
#include "GuiDialogFactory.h"
#include "GuiEventHandler.h"
#include "IEditMode.h"
#include "LegacyActionTool.h"
#include "PanZoomTool.h"
#include "Preview.h"
#include "SelectTool.h"
#include "ViewToolControl.h"

using DispatchScope = ExclusiveCommandBus::DispatchScope;

UIView::UIView(QWidget* parent, Qt::WindowFlags fl, DmDocument* doc)
    : GuiDocumentView(parent, fl, doc)
{
    m_pPanZoomTool = std::make_unique<PanZoomTool>(this);
    m_pLegacyActionTool = std::make_unique<LegacyActionTool>(getEventHandler(), m_pPanZoomTool.get());
    m_pViewToolControl = std::make_unique<ViewToolControl>(this);
    m_pViewToolControl->setNavigationTool(m_pPanZoomTool.get());
    m_pViewToolControl->activate(m_pLegacyActionTool.get());

    if (doc)
    {
        // 基类构造时已把文档关联到本视图，Preview 的构造依赖这一点。
        m_pSelectSnapper = std::make_unique<Snapper>(doc, this);
        m_pSelectPreview = std::make_unique<Preview>(doc);
        m_pSelectTool = std::make_unique<SelectTool>(doc, this, m_pSelectSnapper.get(), m_pSelectPreview.get(),
                                                     m_pPanZoomTool.get());
        m_pViewToolControl->setSelectionTool(m_pSelectTool.get());

        m_pCommandBus = std::make_unique<ExclusiveCommandBus>(doc, this, m_pViewToolControl.get(),
                                                              m_pSelectTool.get());
        // 选择层之上有旧 Action 或命令时，提示与光标归它们（命令的选择阶段除外）
        m_pSelectTool->setOverlayQuery([this]()
        {
            if (getEventHandler()->hasAction())
            {
                return SelectTool::Overlay::LegacyAction;
            }
            // 析构时总线先于选择层释放，释放过程中也会查询
            if (m_pCommandBus && m_pCommandBus->hasActiveCommand())
            {
                return SelectTool::Overlay::Command;
            }
            if (m_pCommandBus && m_pCommandBus->editMode())
            {
                return SelectTool::Overlay::EditMode;
            }
            return SelectTool::Overlay::None;
        });
        getEventHandler()->setStackBase(this);
    }
}

UIView::~UIView()
{
    // 先结束活动命令：它的工具、选择层与 ViewToolControl 都还在。
    m_pCommandBus.reset();
    // GuiEventHandler 归基类，比本类的成员活得久，先解除它对本类的引用。
    getEventHandler()->setStackBase(nullptr);
}

bool UIView::startCommand(std::unique_ptr<IExclusiveCommand> command)
{
    if (!m_pCommandBus || !command || m_pCommandBus->isInCallback())
    {
        return false;
    }
    // 5.1 节：先请当前命令让位，被否决时新命令直接销毁、不激活。
    // 编辑模式不受影响，新命令叠在它上面。
    if (!m_pCommandBus->approveEnd(CommandEndReason::Replaced))
    {
        return false;
    }
    m_pCommandBus->end();

    // 过渡期：启动命令时结束全部旧 Action，不再恢复（迁移计划 9.2 节）
    GuiEventHandler* handler = getEventHandler();
    if (handler->getCurrentActionNum() > 0)
    {
        handler->killAllActions();
        handler->cleanUp();
    }
    return m_pCommandBus->start(std::move(command));
}

void UIView::prepareInstantCommand()
{
    getEventHandler()->interruptForInstantCommand();
}

QString UIView::activeCommandId() const
{
    return m_pCommandBus ? m_pCommandBus->activeCommandId() : QString();
}

bool UIView::processKeyEvent(QKeyEvent* e)
{
    DispatchScope scope(m_pCommandBus.get());
    return m_pViewToolControl->keyPressEvent(e) != ViewToolResult::NotHandled;
}

void UIView::back()
{
    QMouseEvent e(QEvent::MouseButtonRelease, QPoint(0, 0), Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    routeBack(&e);
}

void UIView::routeBack(QMouseEvent* e)
{
    if (getEventHandler()->hasAction())
    {
        GuiDocumentView::back();
    }
    else if (hasBusinessOnBus())
    {
        // 右键释放不走 ViewToolControl（主计划 5.7 节）；命令的工具、编辑模式在这里收到它
        DispatchScope scope(m_pCommandBus.get());
        m_pViewToolControl->mouseReleaseEvent(e);
    }
}

void UIView::commandEvent(GuiCommandEvent* e)
{
    if (getEventHandler()->hasAction() || !hasBusinessOnBus())
    {
        GuiDocumentView::commandEvent(e);
        return;
    }
    if (!getEventHandler()->isCoordinateInputEnabled() || e->isAccepted())
    {
        return;
    }

    // 与旧 Action 同一套解析：坐标一律接受（工具用不用都算已处理），其余文本
    // 交给工具，没有工具接受时由 UIActionHandler 当作新命令解析。
    DispatchScope scope(m_pCommandBus.get());
    const GuiCoordinateInput input = GuiCoordinateInput::parse(e->getCommand(), getRelativeZero());
    switch (input.status)
    {
    case GuiCoordinateInput::Status::Ok:
        m_pViewToolControl->coordinateEvent(input.position);
        e->accept();
        break;
    case GuiCoordinateInput::Status::SyntaxError:
        GUIDIALOGFACTORY->commandMessage("Expression Syntax Error");
        e->accept();
        break;
    case GuiCoordinateInput::Status::NotCoordinate:
        m_pViewToolControl->commandEvent(e);
        break;
    }
}

bool UIView::killAllActions()
{
    if (m_pCommandBus)
    {
        // 5.1 节：先征求命令、再征求编辑模式同意，被否决时什么也不做（调用方也不清空选择）
        if (!m_pCommandBus->approveEndAll(CommandEndReason::Cancelled))
        {
            return false;
        }
        m_pCommandBus->endAll();
    }
    return GuiDocumentView::killAllActions();
}

void UIView::killAllActionsOnClose()
{
    if (m_pCommandBus)
    {
        // 不能否决：命令与编辑模式只在回调里保存或放弃
        m_pCommandBus->approveEndAll(CommandEndReason::ViewClosing);
        m_pCommandBus->endAll();
    }
    GuiDocumentView::killAllActionsOnClose();
}

bool UIView::hasActiveCommand()
{
    return GuiDocumentView::hasActiveCommand() || hasBusinessOnBus();
}

bool UIView::hasBusinessOnBus() const
{
    return m_pCommandBus && (m_pCommandBus->hasActiveCommand() || m_pCommandBus->editMode());
}

void UIView::setCurrentAction(ActionInterface* action)
{
    if (!action)
    {
        return;
    }
    if (m_pCommandBus)
    {
        if (m_pCommandBus->isInCallback())
        {
            // 5.1 节：回调期间的启动请求一律忽略
            delete action;
            return;
        }
        if (action->isExclusive())
        {
            // 排他的旧 Action 要结束全部，先请命令与编辑模式让位；被否决时不启动它
            if (!m_pCommandBus->approveEndAll(CommandEndReason::Replaced))
            {
                delete action;
                return;
            }
            m_pCommandBus->endAll();
        }
    }
    // 其余旧 Action 叠在命令之上：GuiEventHandler 从空栈启动它时经
    // suspendForLegacy() 挂起命令，栈清空时经 resumeAfterLegacy() 恢复
    GuiDocumentView::setCurrentAction(action);
}

void UIView::setDefaultSnapMode(SnapMode sm)
{
    GuiDocumentView::setDefaultSnapMode(sm);
    if (m_pSelectSnapper)
    {
        m_pSelectSnapper->setSnapMode(sm);
    }
    if (m_pCommandBus)
    {
        m_pCommandBus->setSnapMode(sm);
    }
}

void UIView::setSnapRestriction(DM::SnapRestriction sr)
{
    GuiDocumentView::setSnapRestriction(sr);
    if (m_pSelectSnapper)
    {
        m_pSelectSnapper->setSnapRestriction(sr);
    }
    if (m_pCommandBus)
    {
        m_pCommandBus->setSnapRestriction(sr);
    }
}

void UIView::suspendForLegacy()
{
    if (m_pCommandBus)
    {
        if (m_pCommandBus->hasActiveCommand())
        {
            m_pCommandBus->suspend();
        }
        else if (IEditMode* mode = m_pCommandBus->editMode())
        {
            // 命令活动时模式已被它挂起
            mode->suspendMode();
        }
    }
    if (m_pSelectTool)
    {
        m_pSelectTool->suspend();
    }
}

void UIView::resumeAfterLegacy()
{
    if (m_pSelectTool)
    {
        m_pSelectTool->resume();
    }
    if (m_pCommandBus)
    {
        if (m_pCommandBus->hasActiveCommand())
        {
            m_pCommandBus->resume();
        }
        else if (IEditMode* mode = m_pCommandBus->editMode())
        {
            mode->resumeMode();
        }
    }
}

void UIView::resetAfterKill()
{
    if (m_pSelectTool)
    {
        m_pSelectTool->init();
    }
}

ISnapService* UIView::commandSnapService() const
{
    if (!m_pCommandBus || !m_pCommandBus->hasActiveCommand() || m_pCommandBus->isSuspended())
    {
        return nullptr;
    }
    if (m_pSelectTool && m_pSelectTool->inSelectionPhase())
    {
        return nullptr;
    }
    return m_pCommandBus->activeCommand()->snapService();
}

SnapResultType UIView::currentSnapResult()
{
    if (ActionInterface* action = getCurrentAction())
    {
        return action->getSnapResult();
    }
    if (ISnapService* snapper = commandSnapService())
    {
        return snapper->getSnapResult();
    }
    return m_pSelectSnapper ? m_pSelectSnapper->getSnapResult() : SnapResultType::SnapNone;
}

DmVector UIView::currentSnapSpot()
{
    if (ActionInterface* action = getCurrentAction())
    {
        return action->getSnapSpot();
    }
    if (ISnapService* snapper = commandSnapService())
    {
        return snapper->getSnapSpot();
    }
    return m_pSelectSnapper ? m_pSelectSnapper->getSnapSpot() : DmVector(false);
}

void UIView::mousePressEvent(QMouseEvent* e)
{
    // 统一交给 ViewToolControl 分发：业务层（LegacyActionTool）在有业务
    // Action 时优先，空闲态整体让路给选择层（SelectTool）；中键与 Neutral
    // 状态下的 Ctrl/Meta+左键再由选择层让给导航层（PanZoomTool）。
    e->accept();
    DispatchScope scope(m_pCommandBus.get());
    m_pViewToolControl->mousePressEvent(e);
}

void UIView::mouseDoubleClickEvent(QMouseEvent* e)
{
    DispatchScope scope(m_pCommandBus.get());
    switch (e->button())
    {
    case Qt::MiddleButton:
        zoomAuto();
        break;
    case Qt::LeftButton:
        m_pViewToolControl->mouseDoubleClickEvent(e);
        break;
    default:
        break;
    }
    e->accept();
}

void UIView::mouseReleaseEvent(QMouseEvent* e)
{
    e->accept();
    DispatchScope scope(m_pCommandBus.get());

    switch (e->button())
    {
    case Qt::RightButton:
        routeBack(e);
        break;

    case Qt::XButton1:
        enter();
        emit xbutton1_released();
        break;

    default:
    {
        // ViewToolControl 统一分发：有业务 Action 时业务层（LegacyActionTool）
        // 优先，空闲态落到选择层；平移中的释放业务层与选择层都主动让路，
        // 由导航层 PanZoomTool 处理并结束这次平移。
        if (m_pViewToolControl->mouseReleaseEvent(e) == ViewToolResult::Handled)
        {
            // 无论是平移刚结束还是普通业务释放，都让当前 Action 重新声明
            // 一次光标：平移结束时避免 ClosedHandCursor 残留在画布上（见
            // ViewToolControl::refreshCursor"无偏好则不动"的策略）；
            // 普通释放时这只是一次无害的重复刷新。
            if (ActionInterface* action = getCurrentAction())
            {
                action->updateMouseCursor();
            }
        }
        break;
    }
    }
}

void UIView::mouseMoveEvent(QMouseEvent* e)
{
    GuiDocumentView::mouseMoveEvent(e);
    DispatchScope scope(m_pCommandBus.get());

    // ViewToolControl 统一分发：不在平移中时，有业务 Action 则由业务层
    // （LegacyActionTool）转给它，空闲态落到选择层；平移中两层都主动让路，
    // 交给导航层 PanZoomTool 处理。
    m_pViewToolControl->mouseMoveEvent(e);

    if (m_pPanZoomTool->isPanning())
    {
        // 平移期间没有捕捉结果，跳过捕捉提示。
        return;
    }
    updateSnapTooltip(e->pos());
}

void UIView::tabletEvent(QTabletEvent* e)
{
    if (testAttribute(Qt::WA_UnderMouse))
    {
        switch (e->device())
        {
        case QTabletEvent::Eraser:
            if (e->type() == QEvent::TabletRelease)
            {
                if (pDocument && m_pSelectTool)
                {
                    // 橡皮擦：单点拾取后删除选择集。未命中时照旧删除已有的选择集。
                    // 删除是即时命令，不打断当前命令（清单 E6）。
                    m_pSelectTool->pickAt(e->pos().x(), e->pos().y());

                    if (pDocument->getEntityTable()->hasSelect())
                    {
                        prepareInstantCommand();
                        CommandRegistry::instance().runInstant(QStringLiteral("modify.delete_no_select"),
                                                               CommandContext{pDocument, this});
                    }
                }
            }
            break;

        case QTabletEvent::Stylus:
        case QTabletEvent::Puck:
            if (e->type() == QEvent::TabletPress)
            {
                QMouseEvent ev(QEvent::MouseButtonPress, e->pos(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                mousePressEvent(&ev);
            }
            else if (e->type() == QEvent::TabletRelease)
            {
                QMouseEvent ev(QEvent::MouseButtonRelease, e->pos(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                mouseReleaseEvent(&ev);
            }
            else if (e->type() == QEvent::TabletMove)
            {
                QMouseEvent ev(QEvent::MouseMove, e->pos(), Qt::NoButton, 0, Qt::NoModifier);
                mouseMoveEvent(&ev);
            }
            break;

        default:
            break;
        }
    }
}

void UIView::leaveEvent(QEvent* e)
{
    DispatchScope scope(m_pCommandBus.get());
    m_pViewToolControl->leaveEvent();
    GuiDocumentView::leaveEvent(e);
}

void UIView::enterEvent(QEvent* e)
{
    DispatchScope scope(m_pCommandBus.get());
    m_pViewToolControl->enterEvent();
    GuiDocumentView::enterEvent(e);
}

void UIView::focusInEvent(QFocusEvent* e)
{
    DispatchScope scope(m_pCommandBus.get());
    m_pViewToolControl->enterEvent();
    GuiDocumentView::focusInEvent(e);
}

void UIView::wheelEvent(QWheelEvent* e)
{
    const double ZOOM_FACTOR_MOUSE = 1.137;     // 鼠标滚轮缩放因子
    const double TRACKPAD_ZOOM_SCALE = 100.;     // 触控板缩放的每像素百分比
    const int TRACKPAD_ANGLE_DIVISOR = 4;        // 触控板角度增量除数

    DmVector mouse = toGraph(e->x(), e->y());
    DispatchScope scope(m_pCommandBus.get());

    if (getStrDevice() == "Trackpad")
    {
        QPoint numPixels = e->pixelDelta();

        // 高分辨率滚轮触发平移而不是缩放
        isSmoothScrolling |= !numPixels.isNull();

        if (isSmoothScrolling)
        {
            if (e->phase() == Qt::ScrollEnd)
            {
                isSmoothScrolling = false;
            }
        }
        else // Trackpads that without high-resolution scrolling
        {
            numPixels = e->angleDelta() / TRACKPAD_ANGLE_DIVISOR;
        }

        if (!numPixels.isNull())
        {
            if (e->modifiers() == Qt::ControlModifier)
            {
                DMSETTINGS->beginGroup("/Defaults");
                bool invZoom = (DMSETTINGS->readNumEntry("/InvertZoomDirection", 0) == 1);
                DMSETTINGS->endGroup();

                // Hold ctrl to zoom. 1 % per pixel
                double v = (invZoom) ? (numPixels.y() / TRACKPAD_ZOOM_SCALE) : (-numPixels.y() / TRACKPAD_ZOOM_SCALE);
                DM::ZoomDirection direction;
                double factor;

                if (v < 0)
                {
                    direction = DM::In; factor = 1 - v;
                }
                else
                {
                    direction = DM::Out;  factor = 1 + v;
                }

                setCurrentAction(new ActionZoomIn(pDocument, this, direction, DM::Both, &mouse, factor));
            }
            redraw();
        }
        e->accept();
        return;
    }

    if (e->delta() == 0)
    {
        // A zero delta event occurs when smooth scrolling is ended. Ignore this
        e->accept();
        return;
    }

    // zoom in/out:
    if (e->modifiers() == 0)
    {
        DMSETTINGS->beginGroup("/Defaults");
        bool invZoom = (DMSETTINGS->readNumEntry("/InvertZoomDirection", 0) == 1);
        DMSETTINGS->endGroup();

        if ((e->delta() > 0 && !invZoom) || (e->delta() < 0 && invZoom))
        {
            setCurrentAction(new ActionZoomIn(pDocument, this, DM::Out, DM::Both, &mouse, ZOOM_FACTOR_MOUSE));
        }
        else
        {
            setCurrentAction(new ActionZoomIn(pDocument, this, DM::In, DM::Both, &mouse, ZOOM_FACTOR_MOUSE));
        }
    }

    // 缩放后按原位置补发一次移动：基类更新鼠标世界坐标，再交给工具栈
    // （框选框、拖动预览跟上新的缩放）。
    QMouseEvent event(QEvent::MouseMove, QPoint(e->x(), e->y()), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    GuiDocumentView::mouseMoveEvent(&event);
    m_pViewToolControl->mouseMoveEvent(&event);

    e->accept();
    emit viewChanged();
}

void UIView::keyPressEvent(QKeyEvent* e)
{
    // 有文档时键盘事件由主窗口转交（ApplicationWindow::keyPressEvent → processKeyEvent）
    if (pDocument)
    {
        return;
    }

    DispatchScope scope(m_pCommandBus.get());
    m_pViewToolControl->keyPressEvent(e);
}

void UIView::keyReleaseEvent(QKeyEvent* e)
{
    DispatchScope scope(m_pCommandBus.get());
    m_pViewToolControl->keyReleaseEvent(e);
}
