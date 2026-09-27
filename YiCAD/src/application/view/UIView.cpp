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

#include "AppDocument.h"
#include "DmDocument.h"
#include "DmSettings.h"
#include "EditTool.h"
#include "ExclusiveCommandBus.h"
#include "GuiCommandEvent.h"
#include "GuiCoordinateInput.h"
#include "GuiDialogFactory.h"
#include "PanZoomTool.h"
#include "Preview.h"
#include "SelectTool.h"
#include "SelectionSet.h"
#include "Snapper.h"
#include "ViewToolControl.h"

using DispatchScope = ExclusiveCommandBus::DispatchScope;

UIView::UIView(QWidget* parent, Qt::WindowFlags fl, AppDocument* doc)
    : GuiDocumentView(parent, fl, doc ? &doc->document() : nullptr)
{
    m_pPanZoomTool = std::make_unique<PanZoomTool>(this);
    m_pViewToolControl = std::make_unique<ViewToolControl>(this);
    m_pViewToolControl->setNavigationTool(m_pPanZoomTool.get());

    if (doc)
    {
        DmDocument* document = &doc->document();
        m_pSelection = &doc->selection();
        m_pSelectSnapper = std::make_unique<Snapper>(document, this);
        m_pSelectPreview = std::make_unique<Preview>(m_pSelection, this);
        // 文档画笔按图纸的选择集判断实体是否选中；选择改变不经文档通知，这里重建缓存并重绘
        setDocumentSelectionSource(m_pSelection);
        connect(m_pSelection, &SelectionSet::changed, this, [this]()
        {
            specifyDocumentModified();
            redraw();
        });
        m_pSelectTool = std::make_unique<SelectTool>(document, m_pSelection, this, m_pSelectSnapper.get(),
                                                     m_pSelectPreview.get(), m_pPanZoomTool.get());
        m_pViewToolControl->setSelectionTool(m_pSelectTool.get());
        // 夹点编辑工具与选择层共用捕捉器与预览容器：两者轮流使用，同一时刻只有一个在编辑夹点或选择。
        // 没有活动命令时它在业务栈上（onCommandStarting()/onCommandFinished()），选择阶段因此自然不激活夹点
        m_pEditTool = std::make_unique<EditTool>(document, m_pSelection, this, m_pSelectSnapper.get(),
                                                 m_pSelectPreview.get(), m_pPanZoomTool.get());
        // 框选时点第二个角点不激活夹点，那次按下仍是框选的角点
        m_pEditTool->setEnabledQuery([this]()
        {
            return m_pSelectTool->getStatus() == SelectTool::Neutral;
        });
        // 与 DS 的 UIView 构造时 Activate(m_editTool) 相同
        m_pViewToolControl->activate(m_pEditTool.get());

        m_pCommandBus = std::make_unique<ExclusiveCommandBus>(*this);
        connect(m_pCommandBus.get(), &ExclusiveCommandBus::commandStarting, this, &UIView::onCommandStarting);
        connect(m_pCommandBus.get(), &ExclusiveCommandBus::commandFinished, this, &UIView::onCommandFinished);
        // 选择层之上有命令时提示与光标归命令（选择阶段除外），有编辑模式时提示归模式
        m_pSelectTool->setOverlayQuery([this]()
        {
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
        // 双击实体启动它登记的编辑命令（如多行文字的就地编辑）
        m_pSelectTool->setCommandStarter([this](const QString& commandId, DmEntity* entity, const DmVector& point)
        {
            std::unique_ptr<IExclusiveCommand> command = CommandRegistry::instance().createCommand(
                commandId, CommandContext{getDocument(), this, m_pSelection, nullptr, entity, point});
            return command && startCommand(std::move(command));
        });
    }
}

UIView::~UIView()
{
    // 先结束活动命令：它的工具、选择层与 ViewToolControl 都还在。
    m_pCommandBus.reset();
}

void UIView::beginSelectionPhase(const EntityTypeList& entityTypes)
{
    m_pSelectTool->beginSelectionPhase(SelectTool::SelectionPhase{entityTypes});
}

void UIView::endSelectionPhase()
{
    m_pSelectTool->endSelectionPhase();
}

// 下面两个函数不访问 m_pCommandBus：析构函数里 reset() 先把它置空、再析构总线，
// 总线析构时结束活动命令，仍会发 commandFinished()
void UIView::onCommandStarting()
{
    // 夹点编辑工具移出业务栈，激活的夹点随之取消（EditTool::onDeactivate）。拆分前只清除它的
    // 预览，命令结束后夹点接着跟随鼠标，下一次单击会按命令改过的选择集落位（迁移计划 9.6 节）
    m_pViewToolControl->deactivate(m_pEditTool.get());
    // 挂起选择层（清除它的预览与捕捉标记），与原先 Action 从空闲态启动时一致
    m_pSelectTool->suspend();
}

void UIView::onCommandFinished()
{
    // 命令没有退出选择阶段就结束时，由视图清除约束（SelectFirstCommand 自己会退出，这里是兜底）
    if (m_pSelectTool->inSelectionPhase())
    {
        m_pSelectTool->endSelectionPhase();
    }
    // 恢复选择层（刷新提示，重绘预览与捕捉标记），与原先 Action 栈清空时一致
    m_pSelectTool->resume();
    // 夹点编辑工具放回业务栈顶，在编辑模式的工具之上（模式的工具常驻栈底）
    m_pViewToolControl->activate(m_pEditTool.get());
}

bool UIView::startCommand(std::unique_ptr<IExclusiveCommand> command)
{
    return m_pCommandBus && m_pCommandBus->start(std::move(command));
}

bool UIView::prepareInstantCommand(InstantInterrupt interrupt)
{
    switch (interrupt)
    {
    case InstantInterrupt::KeepAll:
        return true;

    case InstantInterrupt::EndAll:
        // 原排他 Action 的做法：先请命令与编辑模式让位，被否决时不执行
        if (m_pCommandBus && !m_pCommandBus->endAll(CommandEndReason::Replaced))
        {
            return false;
        }
        resetIdleTools();
        return true;

    case InstantInterrupt::EndUninterruptible:
        // 不可打断的命令（多行文字编辑与属性面板）先结束，否则它会继续编辑被删除或撤销的
        // 文字；结束前照常询问（多行文字编辑的保存提示没有"取消"，不会否决）。回调期间不结束，
        // 即时命令照常执行
        if (m_pCommandBus && !m_pCommandBus->isInCallback() && m_pCommandBus->activeCommand() &&
            m_pCommandBus->activeCommand()->isUninterruptible() &&
            !m_pCommandBus->endCommand(CommandEndReason::Replaced))
        {
            return false;
        }
        break;
    }
    return true;
}

void UIView::resetIdleTools()
{
    if (m_pEditTool)
    {
        m_pEditTool->cancel();
    }
    if (m_pSelectTool)
    {
        m_pSelectTool->init();
    }
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
    QMouseEvent e(QEvent::MouseButtonRelease, QPointF(0, 0), QPointF(0, 0), Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    DispatchScope scope(m_pCommandBus.get());
    routeBack(&e);
}

void UIView::routeBack(QMouseEvent* e)
{
    if (hasBusinessOnBus())
    {
        // 右键释放不走 ViewToolControl（主计划 5.7 节）；命令的工具、编辑模式在这里收到它
        DispatchScope scope(m_pCommandBus.get());
        m_pViewToolControl->mouseReleaseEvent(e);
    }
}

void UIView::commandEvent(GuiCommandEvent* e)
{
    // 空闲态不接受，由 UIActionHandler 当作新命令解析
    if (!hasBusinessOnBus())
    {
        return;
    }
    if (!isCoordinateInputEnabled() || e->isAccepted())
    {
        return;
    }

    // 坐标一律接受（工具用不用都算已处理），其余文本交给工具，没有工具接受时由
    // UIActionHandler 当作新命令解析。
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
    // 5.1 节：先征求命令、再征求编辑模式同意，被否决时什么也不做（调用方也不清空选择）
    if (m_pCommandBus && !m_pCommandBus->endAll(CommandEndReason::Cancelled))
    {
        return false;
    }
    resetIdleTools();
    return true;
}

void UIView::killAllActionsOnClose()
{
    if (m_pCommandBus)
    {
        // 不能否决：命令与编辑模式只在回调里保存或放弃
        m_pCommandBus->endAll(CommandEndReason::ViewClosing);
    }
    resetIdleTools();
}

bool UIView::hasActiveCommand() const
{
    return hasBusinessOnBus();
}

bool UIView::hasBusinessOnBus() const
{
    return m_pCommandBus && (m_pCommandBus->hasActiveCommand() || m_pCommandBus->editMode());
}

void UIView::setDefaultSnapMode(SnapMode sm)
{
    GuiDocumentView::setDefaultSnapMode(sm);
    if (m_pSelectSnapper)
    {
        m_pSelectSnapper->setSnapMode(sm);
    }
    if (ISnapService* snapper = activeCommandSnapper())
    {
        snapper->setSnapMode(sm);
    }
}

void UIView::setSnapRestriction(DM::SnapRestriction sr)
{
    GuiDocumentView::setSnapRestriction(sr);
    if (m_pSelectSnapper)
    {
        m_pSelectSnapper->setSnapRestriction(sr);
    }
    if (ISnapService* snapper = activeCommandSnapper())
    {
        snapper->setSnapRestriction(sr);
    }
}

ISnapService* UIView::activeCommandSnapper() const
{
    IExclusiveCommand* command = m_pCommandBus ? m_pCommandBus->activeCommand() : nullptr;
    return command ? command->snapService() : nullptr;
}

ISnapService* UIView::commandSnapService() const
{
    if (m_pSelectTool && m_pSelectTool->inSelectionPhase())
    {
        return nullptr;
    }
    return activeCommandSnapper();
}

SnapResultType UIView::currentSnapResult()
{
    if (ISnapService* snapper = commandSnapService())
    {
        return snapper->getSnapResult();
    }
    return m_pSelectSnapper ? m_pSelectSnapper->getSnapResult() : SnapResultType::SnapNone;
}

DmVector UIView::currentSnapSpot()
{
    if (ISnapService* snapper = commandSnapService())
    {
        return snapper->getSnapSpot();
    }
    return m_pSelectSnapper ? m_pSelectSnapper->getSnapSpot() : DmVector(false);
}

void UIView::mousePressEvent(QMouseEvent* e)
{
    // 统一交给 ViewToolControl 分发：业务层（命令的工具；没有命令时是夹点编辑工具
    // EditTool，只处理按在夹点上的按下与激活的夹点）优先，不处理的事件落到选择层
    // （SelectTool）；中键与 Neutral 状态下的 Ctrl/Meta+左键再由选择层让给导航层（PanZoomTool）。
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
        // ViewToolControl 统一分发：业务层优先，不处理的落到选择层；平移中的释放
        // 业务层与选择层都主动让路，由导航层 PanZoomTool 处理并结束这次平移。
        m_pViewToolControl->mouseReleaseEvent(e);
        break;
    }
}

void UIView::mouseMoveEvent(QMouseEvent* e)
{
    GuiDocumentView::mouseMoveEvent(e);
    DispatchScope scope(m_pCommandBus.get());

    // ViewToolControl 统一分发：不在平移中时，业务层不处理的移动落到选择层；
    // 平移中两层都主动让路，交给导航层 PanZoomTool 处理。
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
    if (!testAttribute(Qt::WA_UnderMouse))
    {
        return;
    }

    // 橡皮擦是笔的另一端：设备类型仍是 Stylus，指针类型为 Eraser，所以先按指针类型判断。
    // Qt 5 下原代码拿 QTabletEvent::Eraser（指针类型枚举）去比 device()（设备类型枚举），
    // 实际命中的是数值相同的 Airbrush，真正的橡皮擦走了笔的分支；Qt 6 的枚举不能再混比。
    if (e->pointerType() == QPointingDevice::PointerType::Eraser)
    {
        if (e->type() == QEvent::TabletRelease && pDocument && m_pSelectTool)
        {
            // 橡皮擦：单点拾取后删除选择集。未命中时照旧删除已有的选择集。
            // 删除是修改扩展的即时命令，不打断当前命令（清单 E6）；没有修改扩展时
            // 只拾取、不删除。
            const QPoint pos = e->position().toPoint();
            m_pSelectTool->pickAt(pos.x(), pos.y());

            if (!m_pSelection->isEmpty())
            {
                prepareInstantCommand();
                CommandRegistry::instance().runInstant(QStringLiteral("ext.modify.delete_no_select"),
                                                       CommandContext{pDocument, this, m_pSelection});
            }
        }
        return;
    }

    const QInputDevice::DeviceType device = e->deviceType();
    if (device != QInputDevice::DeviceType::Stylus && device != QInputDevice::DeviceType::Puck)
    {
        return;
    }

    // 笔与鼠标式定位器按鼠标左键处理
    if (e->type() == QEvent::TabletPress)
    {
        QMouseEvent ev(QEvent::MouseButtonPress, e->position(), e->globalPosition(),
                       Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        mousePressEvent(&ev);
    }
    else if (e->type() == QEvent::TabletRelease)
    {
        QMouseEvent ev(QEvent::MouseButtonRelease, e->position(), e->globalPosition(),
                       Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        mouseReleaseEvent(&ev);
    }
    else if (e->type() == QEvent::TabletMove)
    {
        QMouseEvent ev(QEvent::MouseMove, e->position(), e->globalPosition(),
                       Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        mouseMoveEvent(&ev);
    }
}

void UIView::leaveEvent(QEvent* e)
{
    DispatchScope scope(m_pCommandBus.get());
    m_pViewToolControl->leaveEvent();
    GuiDocumentView::leaveEvent(e);
}

void UIView::enterEvent(QEnterEvent* e)
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

    const QPoint pos = e->position().toPoint();
    DmVector mouse = toGraph(pos.x(), pos.y());
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
                // 缩放直接作用于视图，不经命令（原先压入一个视图 Action，挂起、恢复当前命令）
                if (v < 0)
                {
                    zoomIn(1 - v, mouse);
                }
                else
                {
                    zoomOut(1 + v, mouse);
                }
            }
            redraw();
        }
        e->accept();
        return;
    }

    // 取主方向的角度增量：纵向为主取 y，横向为主取 x，与 Qt 5 的 QWheelEvent::delta() 一致
    const QPoint angle = e->angleDelta();
    const int delta = (qAbs(angle.x()) > qAbs(angle.y())) ? angle.x() : angle.y();
    if (delta == 0)
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

        if ((delta > 0 && !invZoom) || (delta < 0 && invZoom))
        {
            zoomOut(ZOOM_FACTOR_MOUSE, mouse);
        }
        else
        {
            zoomIn(ZOOM_FACTOR_MOUSE, mouse);
        }
    }

    // 缩放后按原位置补发一次移动：基类更新鼠标世界坐标，再交给工具栈
    // （框选框、拖动预览跟上新的缩放）。
    QMouseEvent event(QEvent::MouseMove, QPointF(pos), e->globalPosition(), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
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
