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

#include "CommandRegistry.h"
#include "DmLine.h"
#include "DmMText.h"
#include "GuiDialogFactory.h"
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

/// @brief 选择层之上的业务是否连选择阶段的输入、提示与光标也一并接管
bool ownsEverything(SelectTool::Overlay above)
{
    return above == SelectTool::Overlay::ViewTool;
}
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

void SelectTool::suspend()
{
    m_snapService->suspend();
    deletePreview();
}

void SelectTool::resume()
{
    updateButtonHints();
    m_snapService->resume();
    drawPreview();
}

void SelectTool::enterEvent()
{
    const Overlay above = overlay();
    if (inSelectionPhase() ? !ownsEverything(above) : above == Overlay::None || above == Overlay::EditMode)
    {
        resume();
    }
}

void SelectTool::leaveEvent()
{
    const Overlay above = overlay();
    if (inSelectionPhase() ? !ownsEverything(above) : above == Overlay::None || above == Overlay::EditMode)
    {
        suspend();
    }
}

void SelectTool::setOverlayQuery(OverlayQuery query)
{
    m_overlayQuery = std::move(query);
}

void SelectTool::beginSelectionPhase(const SelectionPhase& phase)
{
    m_phase = phase;
    init();
}

void SelectTool::endSelectionPhase()
{
    if (!m_phase)
    {
        return;
    }
    m_phase.reset();
    init();
}

DmEntity* SelectTool::pickAt(int guiX, int guiY)
{
    DmEntity* en = m_snapService->catchEntity(DmVector(m_docView->toGraphX(guiX), m_docView->toGraphY(guiY)));
    if (en)
    {
        Selection s(m_pDocument, m_docView);
        s.selectSingle(en);
        GUIDIALOGFACTORY->updateSelectionWidget(m_pDocument->getEntityTable()->countSelect());
    }
    return en;
}

SelectTool::Overlay SelectTool::overlay() const
{
    return m_overlayQuery ? m_overlayQuery() : Overlay::None;
}

bool SelectTool::phaseOwnsInput() const
{
    return inSelectionPhase() && !ownsEverything(overlay());
}

void SelectTool::notifySelectionChanged()
{
    if (inSelectionPhase())
    {
        GUIDIALOGFACTORY->updateSelectionWidget(m_pDocument->getEntityTable()->countSelect());
    }
    else
    {
        m_docView->emitSelectedChanged();
    }
    m_docView->redraw();
}

void SelectTool::updateButtonHints() const
{
    if (inSelectionPhase())
    {
        if (!phaseOwnsInput())
        {
            // 临时视图工具叠在命令之上：提示归它
            return;
        }
        // 选择阶段的提示取原 ActionSelectMultiple 的（原 ActionSelect 那套
        // "Select to …"提示被它覆盖，从未显示过，见迁移计划 9.2 节）
        switch (m_status)
        {
        case Neutral:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Click and drag for the selection window"), tr("Cancel"));
            break;
        case SetCorner2:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Choose second edge"), tr("Back"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
        }
        return;
    }
    if (overlay() != Overlay::None)
    {
        return;
    }
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
    if (inSelectionPhase())
    {
        // 原 ActionSelectMultiple 在各状态下都用选择光标
        return DM::SelectCursor;
    }
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
    // 有命令正活动时，光标由命令的工具经仲裁给出，选择层在仲裁通道里保持沉默，
    // 不能用自己的偏好覆盖它们。这次查询不影响 setStatus()/init() 的直接调用——
    // 那两处用的是不受这条限制约束的 cursorForStatus()。
    // 选择阶段由本类负责选择，光标也由本类给出，除非临时视图工具叠在命令之上。
    if (inSelectionPhase())
    {
        return phaseOwnsInput() ? cursorForStatus() : std::nullopt;
    }
    const Overlay above = overlay();
    if (above == Overlay::Command || ownsEverything(above))
    {
        return std::nullopt;
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
        // 结束这次平移之前把事件处理掉。见头部说明。
        return ViewToolResult::NotHandled;
    }

    DmVector mouse = m_docView->toGraph(e->pos().x(), e->pos().y());
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
            if (inSelectionPhase())
            {
                // 选择阶段不拖夹点、不拖实体，超过阈值即开始框选
                setStatus(SetCorner2);
                break;
            }
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
    if (e->button() == Qt::MiddleButton)
    {
        // 中键平移属于导航层（PanZoomTool），选择层让路。
        return ViewToolResult::NotHandled;
    }

    if (e->button() == Qt::LeftButton)
    {
        switch (m_status)
        {
        case Neutral:
            if ((e->modifiers() & (Qt::ControlModifier | Qt::MetaModifier)) && !inSelectionPhase())
            {
                // Ctrl/Meta+左键从 Neutral 状态发起是导航层的平移手势
                // （见 PanZoomTool），选择层让路。选择阶段不让：原
                // ActionSelectMultiple 把它当作普通的框选起点。
                return ViewToolResult::NotHandled;
            }
            m_points.v1 = m_docView->toGraph(e->pos().x(), e->pos().y());
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
        m_points.v2 = m_docView->toGraph(e->pos().x(), e->pos().y());
        switch (m_status)
        {
        case Dragging:
        {
            // select single entity:
            DmEntity* en = (m_phase && !m_phase->entityTypes.empty())
                               ? m_snapService->catchEntity(e, m_phase->entityTypes)
                               : m_snapService->catchEntity(e);

            if (en)
            {
                deletePreview();

                Selection s(m_pDocument, m_docView);
                s.selectSingle(en);
                notifySelectionChanged();
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
            m_points.v2 = m_docView->toGraph(e->pos().x(), e->pos().y());

            deletePreview();

            bool cross = (m_points.v1.x > m_points.v2.x);
            Selection s(m_pDocument, m_docView);
            bool select = (e->modifiers() & Qt::ShiftModifier) ? false : true;
            s.selectWindow(m_points.v1, m_points.v2, select, cross,
                           m_phase ? m_phase->entityTypes : EntityTypeList{});
            notifySelectionChanged();
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
    DmVector clickPos = m_docView->toGraph(e->pos().x(), e->pos().y());

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

    // 修改实体：优先用双击编辑命令（多行文字的就地编辑），没有时用属性编辑命令（属性对话框），
    // 两者都由实体所在的扩展登记
    if (selectCount == 0 || (selectCount == 1 && en == ents.front()))
    {
        const CommandRegistry& registry = CommandRegistry::instance();
        QString editor = registry.entityEditor(en->getEntityType());
        if (editor.isEmpty())
        {
            editor = registry.propertyEditor(en->getEntityType());
        }
        if (registry.kind(editor) == CommandKind::Instant)
        {
            registry.runInstant(editor, CommandContext{m_pDocument, m_docView, nullptr, en, clickPos});
        }
        else if (!editor.isEmpty() && m_commandStarter)
        {
            m_commandStarter(editor, en, clickPos);
        }
    }
    return ViewToolResult::Handled;
}
