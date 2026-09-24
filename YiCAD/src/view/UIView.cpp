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
#include "ActionModifyDelete.h"
#include "ActionZoomIn.h"
#include "DmDocument.h"
#include "DmSettings.h"
#include "EntityTable.h"
#include "GuiEventHandler.h"
#include "LegacyActionTool.h"
#include "PanZoomTool.h"
#include "Preview.h"
#include "SelectTool.h"
#include "ViewToolControl.h"

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
        getEventHandler()->setSelectTool(m_pSelectTool.get());
        m_pViewToolControl->setSelectionTool(m_pSelectTool.get());
    }
}

UIView::~UIView()
{
    // GuiEventHandler 归基类，比本类的成员活得久，先解除它对选择层的引用。
    getEventHandler()->setSelectTool(nullptr);
}

bool UIView::processKeyEvent(QKeyEvent* e)
{
    return m_pViewToolControl->keyPressEvent(e) != ViewToolResult::NotHandled;
}

void UIView::setDefaultSnapMode(SnapMode sm)
{
    GuiDocumentView::setDefaultSnapMode(sm);
    if (m_pSelectSnapper)
    {
        m_pSelectSnapper->setSnapMode(sm);
    }
}

void UIView::setSnapRestriction(DM::SnapRestriction sr)
{
    GuiDocumentView::setSnapRestriction(sr);
    if (m_pSelectSnapper)
    {
        m_pSelectSnapper->setSnapRestriction(sr);
    }
}

SnapResultType UIView::currentSnapResult()
{
    if (ActionInterface* action = getCurrentAction())
    {
        return action->getSnapResult();
    }
    return m_pSelectSnapper ? m_pSelectSnapper->getSnapResult() : SnapResultType::SnapNone;
}

DmVector UIView::currentSnapSpot()
{
    if (ActionInterface* action = getCurrentAction())
    {
        return action->getSnapSpot();
    }
    return m_pSelectSnapper ? m_pSelectSnapper->getSnapSpot() : DmVector(false);
}

void UIView::mousePressEvent(QMouseEvent* e)
{
    // 统一交给 ViewToolControl 分发：业务层（LegacyActionTool）在有业务
    // Action 时优先，空闲态整体让路给选择层（SelectTool）；中键与 Neutral
    // 状态下的 Ctrl/Meta+左键再由选择层让给导航层（PanZoomTool）。
    e->accept();
    m_pViewToolControl->mousePressEvent(e);
}

void UIView::mouseDoubleClickEvent(QMouseEvent* e)
{
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

    switch (e->button())
    {
    case Qt::RightButton:

        if (getEventHandler()->hasAction())
        {
            back();
        }
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
                    m_pSelectTool->pickAt(e->pos().x(), e->pos().y());

                    if (pDocument->getEntityTable()->hasSelect())
                    {
                        setCurrentAction(new ActionModifyDelete(pDocument, this));
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
    m_pViewToolControl->leaveEvent();
    GuiDocumentView::leaveEvent(e);
}

void UIView::enterEvent(QEvent* e)
{
    m_pViewToolControl->enterEvent();
    GuiDocumentView::enterEvent(e);
}

void UIView::focusInEvent(QFocusEvent* e)
{
    m_pViewToolControl->enterEvent();
    GuiDocumentView::focusInEvent(e);
}

void UIView::wheelEvent(QWheelEvent* e)
{
    const double ZOOM_FACTOR_MOUSE = 1.137;     // 鼠标滚轮缩放因子
    const double TRACKPAD_ZOOM_SCALE = 100.;     // 触控板缩放的每像素百分比
    const int TRACKPAD_ANGLE_DIVISOR = 4;        // 触控板角度增量除数

    DmVector mouse = toGraph(e->x(), e->y());

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

    m_pViewToolControl->keyPressEvent(e);
}

void UIView::keyReleaseEvent(QKeyEvent* e)
{
    m_pViewToolControl->keyReleaseEvent(e);
}
