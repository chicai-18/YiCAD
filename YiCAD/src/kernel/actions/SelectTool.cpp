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

/// @file SelectTool.cpp
/// @brief SelectTool 的实现，逐状态从 ActionDefault 移植而来

#include "SelectTool.h"

#include <list>

#include <QKeyEvent>
#include <QMouseEvent>

#include "ActionDrawMText.h"
#include "DmLine.h"
#include "DmMText.h"
#include "GuiDialogFactory.h"
#include "GuiEventHandler.h"
#include "IDocumentView.h"
#include "ISnapService.h"
#include "Modification.h"
#include "PanZoomTool.h"
#include "Preview.h"
#include "Selection.h"

namespace
{
/// @brief 拖拽判定的最小GUI距离（像素）
constexpr double kDragThresholdGui = 10.0;
/// @brief 参考点吸附的GUI距离（像素）
constexpr double kRefSnapGuiDist = 8.0;
/// @brief 角度吸附步进（度）
constexpr double kAngleSnapStep = 15.0;
}  // namespace

SelectTool::SelectTool(DmDocument* doc, IDocumentView* docView, ISnapService* snapService, Preview* preview,
                       PanZoomTool* panTool)
    : m_pDocument(doc)
    , m_docView(docView)
    , m_snapService(snapService)
    , m_preview(preview)
    , m_panTool(panTool)
{
}

void SelectTool::init()
{
    deletePreview();
    m_snapService->deleteSnapper();
    m_snapService->init();
    m_points = Points{};
    m_status = Neutral;
    updateButtonHints();
    if (auto cursor = cursorForStatus())
    {
        m_docView->setMouseCursor(*cursor);
    }
}

void SelectTool::setStatus(int status)
{
    if (m_status == status)
    {
        return;
    }
    m_status = status;
    updateButtonHints();
    if (auto cursor = cursorForStatus())
    {
        m_docView->setMouseCursor(*cursor);
    }
}

void SelectTool::deletePreview()
{
    if (m_hasPreview)
    {
        m_preview->clear();
        m_hasPreview = false;
    }
    if (!m_docView->isCleanUp())
    {
        m_docView->disableOverlayBox();
    }
}

void SelectTool::drawPreview()
{
    m_docView->redraw();
    m_hasPreview = true;
}

void SelectTool::onActivate()
{
    drawPreview();
}

void SelectTool::onDeactivate()
{
    deletePreview();
}

void SelectTool::updateButtonHints() const
{
    switch (m_status)
    {
    case Neutral:
        GUIDIALOGFACTORY->updateMouseWidget();
        break;
    case SetCorner2:
        // GUIDIALOGFACTORY->updateMouseWidget(tr("Choose second edge"), tr("Back"));
        break;
    default:
        GUIDIALOGFACTORY->updateMouseWidget();
        break;
    }
}

std::optional<DM::CursorType> SelectTool::cursorForStatus() const
{
    switch (m_status)
    {
    case Neutral:
        return DM::ArrowCursor;
    case Moving:
    case MovingRef:
        return DM::SelectCursor;
    default:
        return std::nullopt;
    }
}

std::optional<DM::CursorType> SelectTool::getCursor() const
{
    // 有其它业务 Action 正活动时，光标由它自己直接调用 setMouseCursor()
    // 决定（105 个 Action 尚未改造，见阶段2 5.7 节），选择层在仲裁通道里
    // 保持沉默，不能用自己的偏好覆盖它们。这次查询不影响 setStatus()/
    // init() 的直接调用——那两处用的是不受这条限制约束的 cursorForStatus()。
    if (m_docView)
    {
        if (GuiEventHandler* handler = m_docView->getEventHandler())
        {
            if (handler->hasAction())
            {
                return std::nullopt;
            }
        }
    }
    return cursorForStatus();
}

ViewToolResult SelectTool::keyPressEvent(QKeyEvent* e)
{
    switch (e->key())
    {
    case Qt::Key_Shift:
        m_restrictionBak = m_snapService->getSnapMode()->restriction;
        m_snapService->setSnapRestriction(DM::RestrictOrthogonal);
        e->accept();
        break;  // avoid clearing command line at shift key

    case Qt::Key_Escape:
    {
        deletePreview();
        m_snapService->deleteSnapper();
        setStatus(Neutral);
        Selection s(m_pDocument, m_docView);
        s.selectAll(false);
        e->accept();
        break;
    }
    default:
        e->ignore();
        return ViewToolResult::NotHandled;
    }
    return ViewToolResult::Handled;
}

ViewToolResult SelectTool::keyReleaseEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Shift)
    {
        m_snapService->setSnapRestriction(m_restrictionBak);
        e->accept();
        return ViewToolResult::Handled;
    }
    return ViewToolResult::NotHandled;
}

ViewToolResult SelectTool::mouseMoveEvent(QMouseEvent* e)
{
    if (m_panTool && m_panTool->isPanning())
    {
        // 导航层正在平移中，让路——否则选择层会在移动事件上抢在导航层
        // 结束这次平移之前把事件处理掉。见头部说明与 LegacyActionTool
        // 里同样的判断。
        return ViewToolResult::NotHandled;
    }

    DmVector mouse = m_docView->toGraph(e->x(), e->y());
    DmVector relMouse = mouse - m_docView->getRelativeZero();

    GUIDIALOGFACTORY->updateCoordinateWidget(mouse, relMouse);

    switch (m_status)
    {
    case Neutral:
        m_snapService->deleteSnapper();
        break;

    case Dragging:
        m_points.v2 = mouse;

        if (m_docView->toGuiDX(m_points.v1.distanceTo(m_points.v2)) > kDragThresholdGui)
        {
            // look for reference points to drag:
            double dist;
            DmVector ref = m_pDocument->getEntityTable()->getNearestSelectedRef(m_points.v1, &dist);
            if (ref.valid && m_docView->toGuiDX(dist) < kRefSnapGuiDist)
            {
                setStatus(MovingRef);
                m_points.v1 = ref;
                m_docView->moveRelativeZero(m_points.v1);
            }
            else
            {
                // test for an entity to drag:
                DmEntity* en = m_snapService->catchEntity(m_points.v1);
                if (en && en->isSelected())
                {
                    setStatus(Moving);
                    DmVector vp = en->getNearestRef(m_points.v1);
                    if (vp.valid)
                        m_points.v1 = vp;
                }
                // no entity found. start area selection:
                else
                {
                    setStatus(SetCorner2);
                }
            }
        }
        break;

    case MovingRef:
        m_points.v2 = m_snapService->snapPoint(e);
        GUIDIALOGFACTORY->updateCoordinateWidget(m_points.v2, m_points.v2 - m_docView->getRelativeZero());

        if (e->modifiers() & Qt::ShiftModifier)
        {
            mouse = m_snapService->snapToAngle(mouse, m_points.v1, kAngleSnapStep);
            m_points.v2 = mouse;
        }

        deletePreview();
        m_preview->addSelectionFromDocument();
        m_preview->moveRef(m_points.v1, m_points.v2 - m_points.v1);

        if (e->modifiers() & Qt::ShiftModifier)
        {
            DmLine* line = new DmLine(nullptr, m_points.v1, mouse);
            m_preview->addEntity(line);
            line->setSelected(true);
        }

        drawPreview();
        break;

    case Moving:
        m_points.v2 = m_snapService->snapPoint(e);
        GUIDIALOGFACTORY->updateCoordinateWidget(m_points.v2, m_points.v2 - m_docView->getRelativeZero());

        if (e->modifiers() & Qt::ShiftModifier)
        {
            mouse = m_snapService->snapToAngle(mouse, m_points.v1, kAngleSnapStep);
            m_points.v2 = mouse;
        }

        deletePreview();
        m_preview->addSelectionFromDocument();
        m_preview->move(m_points.v2 - m_points.v1);

        if (e->modifiers() & Qt::ShiftModifier)
        {
            DmLine* line = new DmLine(nullptr, m_points.v1, mouse);
            m_preview->addEntity(line);
            line->setSelected(true);
        }

        drawPreview();
        break;

    case SetCorner2:
        if (m_points.v1.valid)
        {
            m_points.v2 = mouse;
            m_docView->setOverlayCorners(m_points.v1, m_points.v2);
            m_docView->redraw();
        }
        break;

    default:
        break;
    }
    return ViewToolResult::Handled;
}

ViewToolResult SelectTool::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        switch (m_status)
        {
        case Neutral:
            if (e->modifiers() & (Qt::ControlModifier | Qt::MetaModifier))
            {
                // Ctrl/Meta+左键从 Neutral 状态发起是导航层的平移手势
                // （见 PanZoomTool），选择层让路。业务层已经在没有业务
                // Action 活动时对这个组合让过一次路（LegacyActionTool::
                // wantsPress），本类只有在那之后才可能真正收到这次按下。
                return ViewToolResult::NotHandled;
            }
            m_points.v1 = m_docView->toGraph(e->x(), e->y());
            setStatus(Dragging);
            break;

        case Moving:
        {
            m_points.v2 = m_snapService->snapPoint(e);
            if (e->modifiers() & Qt::ShiftModifier)
            {
                m_points.v2 = m_snapService->snapToAngle(m_points.v2, m_points.v1, kAngleSnapStep);
            }
            deletePreview();
            Modification m(m_docView);
            DmVector offset = m_points.v2 - m_points.v1;
            m.move(offset);
            setStatus(Neutral);
            GUIDIALOGFACTORY->updateSelectionWidget(m_pDocument->getEntityTable()->countSelect());
            m_snapService->deleteSnapper();
        }
        break;

        case MovingRef:
        {
            m_points.v2 = m_snapService->snapPoint(e);
            if (e->modifiers() & Qt::ShiftModifier)
            {
                m_points.v2 = m_snapService->snapToAngle(m_points.v2, m_points.v1, kAngleSnapStep);
            }
            deletePreview();
            Modification m(m_docView);
            MoveRefData data;
            data.ref = m_points.v1;
            data.offset = m_points.v2 - m_points.v1;
            m.moveRef(data);
            setStatus(Neutral);
            GUIDIALOGFACTORY->updateSelectionWidget(m_pDocument->getEntityTable()->countSelect());
        }
        break;

        default:
            break;
        }
    }
    else if (e->button() == Qt::RightButton)
    {
        // cleanup
        setStatus(Neutral);
        e->accept();
    }
    return ViewToolResult::Handled;
}

ViewToolResult SelectTool::mouseReleaseEvent(QMouseEvent* e)
{
    if (m_panTool && m_panTool->isPanning())
    {
        // 见 mouseMoveEvent 顶部的说明：导航层正在平移中，让路。
        return ViewToolResult::NotHandled;
    }

    if (e->button() == Qt::LeftButton)
    {
        m_points.v2 = m_docView->toGraph(e->x(), e->y());
        switch (m_status)
        {
        case Dragging:
        {
            // select single entity:
            DmEntity* en = m_snapService->catchEntity(e);

            if (en)
            {
                deletePreview();

                Selection s(m_pDocument, m_docView);
                s.selectSingle(en);
                m_docView->emitSelectedChanged();
                m_docView->redraw();
                e->accept();
                setStatus(Neutral);
            }
            else
            {
                setStatus(SetCorner2);
            }
        }
        break;

        case SetCorner2:
        {
            m_points.v2 = m_docView->toGraph(e->x(), e->y());

            deletePreview();

            bool cross = (m_points.v1.x > m_points.v2.x);
            Selection s(m_pDocument, m_docView);
            bool select = (e->modifiers() & Qt::ShiftModifier) ? false : true;
            s.selectWindow(m_points.v1, m_points.v2, select, cross);
            m_docView->emitSelectedChanged();
            m_docView->redraw();
            setStatus(Neutral);
            e->accept();
        }
        break;

        default:
            break;
        }
    }
    else if (e->button() == Qt::RightButton)
    {
        // cleanup
        setStatus(Neutral);
        e->accept();
    }
    return ViewToolResult::Handled;
}

ViewToolResult SelectTool::mouseDoubleClickEvent(QMouseEvent* e)
{
    if (m_status != Neutral)
    {
        return ViewToolResult::NotHandled;
    }
    DmVector clickPos = m_docView->toGraph(e->x(), e->y());

    // 获得选择的实体，如果超过1个，不进入编辑状态
    std::list<DmEntity*> ents;
    for (auto& ent : *m_pDocument->getEntityTable())
    {
        if (ent->isSelected())
        {
            ents.emplace_back(ent);
        }
    }
    auto selectCount = ents.size();
    if (selectCount > 1)
    {
        return ViewToolResult::Handled;
    }
    DmEntity* en = m_snapService->catchEntity(e);
    if (en == nullptr)
    {
        return ViewToolResult::Handled;
    }

    // 修改实体
    if (selectCount == 0 || (selectCount == 1 && en == ents.front()))
    {
        // 多行文字的编辑不是模态对话框，且与Action关联，需要特殊处理
        if (en->getEntityType() == DM::EntityMText)
        {
            ActionDrawMText* mtextAction = new ActionDrawMText(m_pDocument, m_docView, true);
            mtextAction->setModifyData(static_cast<DmMText*>(en), clickPos);
            m_docView->setCurrentAction(mtextAction);
            return ViewToolResult::Handled;
        }

        // 不是多行文字类型处理
        GUIDIALOGFACTORY->requestModifyEntityDialog(en);
    }
    return ViewToolResult::Handled;
}
