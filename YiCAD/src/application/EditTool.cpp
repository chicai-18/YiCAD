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

/// @file EditTool.cpp
/// @brief EditTool 的实现，预览与落位从 SelectTool 原先的 MovingRef 状态搬来

#include "EditTool.h"

#include <QKeyEvent>
#include <QMouseEvent>

#include "DmDocument.h"
#include "DmLine.h"
#include "EntityTable.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "Modification.h"
#include "PanZoomTool.h"
#include "Preview.h"

namespace
{
/// @brief 按在参考点多近（GUI 像素）算按在夹点上
constexpr double kRefSnapGuiDist = 8.0;
/// @brief 按住夹点移动超过这个 GUI 距离（像素）即激活
constexpr double kDragThresholdGui = 10.0;
/// @brief 角度吸附步进（度）
constexpr double kAngleSnapStep = 15.0;
}  // namespace

EditTool::EditTool(DmDocument* doc, IDocumentView* docView, ISnapService* snapService, Preview* preview,
                   PanZoomTool* panTool)
    : m_pDocument(doc)
    , m_docView(docView)
    , m_snapService(snapService)
    , m_preview(preview)
    , m_panTool(panTool)
{
}

void EditTool::cancel()
{
    if (m_status == Neutral)
    {
        return;
    }
    clearPreview();
    m_status = Neutral;
    m_docView->redraw();
}

void EditTool::onDeactivate()
{
    // 视图析构时也会调用，这里不重绘
    clearPreview();
    m_status = Neutral;
}

bool EditTool::isPanning() const
{
    return m_panTool && m_panTool->isPanning();
}

void EditTool::clearPreview()
{
    if (m_hasPreview)
    {
        m_preview->clear();
        m_hasPreview = false;
    }
}

std::optional<DM::CursorType> EditTool::getCursor() const
{
    if (m_status == MovingRef)
    {
        return DM::SelectCursor;
    }
    return std::nullopt;
}

void EditTool::leaveEvent()
{
    // 与拆分前选择层挂起时一样，只清除预览，夹点本身保留
    clearPreview();
}

ViewToolResult EditTool::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape)
    {
        cancel();
    }
    return ViewToolResult::NotHandled;
}

ViewToolResult EditTool::mousePressEvent(QMouseEvent* e)
{
    if (m_status != Neutral)
    {
        switch (e->button())
        {
        case Qt::LeftButton:
            if (m_status == MovingRef)
            {
                commit(e);
            }
            return ViewToolResult::Handled;
        case Qt::RightButton:
            cancel();
            e->accept();
            return ViewToolResult::Handled;
        default:
            // 中键平移属于导航层，平移结束后夹点照旧
            return ViewToolResult::NotHandled;
        }
    }

    if (e->button() != Qt::LeftButton || (e->modifiers() & (Qt::ControlModifier | Qt::MetaModifier)) ||
        (m_enabledQuery && !m_enabledQuery()))
    {
        return ViewToolResult::NotHandled;
    }

    const DmVector press = m_docView->toGraph(e->pos().x(), e->pos().y());
    double dist;
    const DmVector ref = m_pDocument->getEntityTable()->getNearestSelectedRef(press, &dist);
    if (!ref.valid || m_docView->toGuiDX(dist) >= kRefSnapGuiDist)
    {
        // 不在夹点上：点选、框选归选择层
        return ViewToolResult::NotHandled;
    }
    m_status = Pressed;
    m_pressPos = press;
    m_base = ref;
    return ViewToolResult::Handled;
}

ViewToolResult EditTool::mouseMoveEvent(QMouseEvent* e)
{
    if (isPanning())
    {
        // 导航层正在平移中，让路（见 SelectTool::mouseMoveEvent）
        return ViewToolResult::NotHandled;
    }

    switch (m_status)
    {
    case Pressed:
    {
        const DmVector mouse = m_docView->toGraph(e->pos().x(), e->pos().y());
        if (m_docView->toGuiDX(m_pressPos.distanceTo(mouse)) > kDragThresholdGui)
        {
            activate(e);
        }
        else
        {
            GUIDIALOGFACTORY->updateCoordinateWidget(mouse, mouse - m_docView->getRelativeZero());
        }
        return ViewToolResult::Handled;
    }
    case MovingRef:
        updatePreview(e);
        return ViewToolResult::Handled;
    default:
        return ViewToolResult::NotHandled;
    }
}

ViewToolResult EditTool::mouseReleaseEvent(QMouseEvent* e)
{
    if (isPanning())
    {
        // 见 mouseMoveEvent 顶部的说明：导航层正在平移中，让路。
        return ViewToolResult::NotHandled;
    }

    switch (m_status)
    {
    case Pressed:
        if (e->button() == Qt::LeftButton)
        {
            activate(e);
        }
        return ViewToolResult::Handled;
    case MovingRef:
        if (e->button() == Qt::RightButton)
        {
            cancel();
            e->accept();
        }
        // 松开左键时参考点继续跟随鼠标，单击才落位
        return ViewToolResult::Handled;
    default:
        return ViewToolResult::NotHandled;
    }
}

ViewToolResult EditTool::mouseDoubleClickEvent(QMouseEvent*)
{
    // 双击夹点：第一次单击已激活夹点，双击什么也不做。不能落到选择层：它此时在 Neutral，
    // 会启动实体的编辑命令
    return m_status != Neutral ? ViewToolResult::Handled : ViewToolResult::NotHandled;
}

void EditTool::activate(QMouseEvent* e)
{
    m_status = MovingRef;
    m_docView->moveRelativeZero(m_base);
    updatePreview(e);
}

void EditTool::updatePreview(QMouseEvent* e)
{
    DmVector mouse = m_docView->toGraph(e->pos().x(), e->pos().y());
    GUIDIALOGFACTORY->updateCoordinateWidget(mouse, mouse - m_docView->getRelativeZero());

    DmVector target = m_snapService->snapPoint(e);
    GUIDIALOGFACTORY->updateCoordinateWidget(target, target - m_docView->getRelativeZero());

    const bool shift = e->modifiers().testFlag(Qt::ShiftModifier);
    if (shift)
    {
        // 预览按鼠标位置做角度吸附，落位时按捕捉点，与拆分前一致（见 commit()）
        mouse = m_snapService->snapToAngle(mouse, m_base, kAngleSnapStep);
        target = mouse;
    }

    clearPreview();
    m_preview->addSelectionFromDocument();
    m_preview->moveRef(m_base, target - m_base);

    if (shift)
    {
        // 参考点到吸附点的引导线
        DmLine* line = new DmLine(nullptr, m_base, mouse);
        m_preview->addEntity(line);
        line->setSelected(true);
    }

    m_hasPreview = true;
    m_docView->redraw();
}

void EditTool::commit(QMouseEvent* e)
{
    DmVector target = m_snapService->snapPoint(e);
    if (e->modifiers() & Qt::ShiftModifier)
    {
        target = m_snapService->snapToAngle(target, m_base, kAngleSnapStep);
    }
    clearPreview();

    Modification m(m_pDocument);
    MoveRefData data;
    data.ref = m_base;
    data.offset = target - m_base;
    m.moveRef(data);
    m_status = Neutral;
    GUIDIALOGFACTORY->updateSelectionWidget(m_pDocument->getEntityTable()->countSelect());
}
