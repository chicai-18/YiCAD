/**
 * Copyright (c) 2011-2018 by Andrew Mustun. All rights reserved.
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is part of the YiCAD project.
 *
 * YiCAD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YiCAD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */


/// @file ActionDefault.cpp
/// @brief 默认动作类实现文件

#include "ActionDefault.h"

#include <cmath>

#include <QMouseEvent>

#include "ActionDrawMText.h"
#include "DmHatch.h"
#include "DmLine.h"
#include "DmMText.h"
#include "GuiDialogFactory.h"
#include "Modification.h"
#include "Preview.h"
#include "Selection.h"
#include "Transaction.h"

/// @brief 拖拽判定的最小GUI距离（像素）
static constexpr double DRAG_THRESHOLD_GUI = 10.0;

/// @brief 参考点吸附的GUI距离（像素）
static constexpr double REF_SNAP_GUI_DIST = 8.0;

/// @brief 角度吸附步进（度）
static constexpr double ANGLE_SNAP_STEP = 15.0;

/// @brief 平移触发的最小平方距离
static constexpr double PAN_SQUARED_THRESHOLD = 64.0;

/// @brief 内部点数据结构
struct ActionDefault::Points
{
    DmVector v1; /**< 起始点 */
    DmVector v2; /**< 结束点 */
};

/// @brief 构造函数
/// @param[in] doc 文档指针
/// @param[in] docView 文档视图指针
ActionDefault::ActionDefault(
    DmDocument* doc, GuiDocumentView* docView)
    : PreviewActionInterface("Default", doc, docView)
    , pPoints(new Points{})
    , restrBak(DM::RestrictNothing)
{
    actionType = DM::ActionDefault;
}

/// @brief 析构函数
ActionDefault::~ActionDefault() = default;

/// @brief 初始化动作
/// @param[in] status 初始状态
void ActionDefault::init(int status)
{
    if (status == Neutral)
    {
        deletePreview();
        deleteSnapper();
    }
    PreviewActionInterface::init(status);
    pPoints->v1 = pPoints->v2 = {};
}

/// @brief 键盘按下事件处理
/// @param[in] e 键盘事件指针
void ActionDefault::keyPressEvent(QKeyEvent * e)
{
    switch (e->key())
    {
    case Qt::Key_Shift:
        restrBak = snapMode.restriction;
        setSnapRestriction(DM::RestrictOrthogonal);
        e->accept();
        break;  // avoid clearing command line at shift key
    case Qt::Key_Escape:
    {
        deletePreview();
        deleteSnapper();
        setStatus(Neutral);
        Selection s(pDocument, docView);
        s.selectAll(false);
        e->accept();
        break;
    }
    default:
        e->ignore();
    }
}

/// @brief 键盘释放事件处理
/// @param[in] e 键盘事件指针
void ActionDefault::keyReleaseEvent(QKeyEvent * e)
{
    if (e->key() == Qt::Key_Shift)
    {
        setSnapRestriction(restrBak);
        e->accept();
    }
}

/// @brief 鼠标移动事件处理
/// @param[in] e 鼠标事件指针
void ActionDefault::mouseMoveEvent(QMouseEvent * e)
{
    DmVector mouse = docView->toGraph(e->x(), e->y());
    DmVector relMouse = mouse - docView->getRelativeZero();

    GUIDIALOGFACTORY->updateCoordinateWidget(mouse, relMouse);

    switch (getStatus())
    {
    case Neutral:
        deleteSnapper();
        break;
    case Dragging:
        pPoints->v2 = mouse;

        if (docView->toGuiDX(pPoints->v1.distanceTo(pPoints->v2)) > DRAG_THRESHOLD_GUI)
        {
            // look for reference points to drag:
            double dist;
            DmVector ref = pDocument->getEntityTable()->getNearestSelectedRef(pPoints->v1, &dist);
            if (ref.valid && docView->toGuiDX(dist) < REF_SNAP_GUI_DIST)
            {
                setStatus(MovingRef);
                pPoints->v1 = ref;
                docView->moveRelativeZero(pPoints->v1);
            }
            else
            {
                // test for an entity to drag:
                DmEntity* en = catchEntity(pPoints->v1);
                if (en && en->isSelected())
                {
                    setStatus(Moving);
                    DmVector vp = en->getNearestRef(pPoints->v1);
                    if (vp.valid)
                        pPoints->v1 = vp;
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
        pPoints->v2 = snapPoint(e);
        GUIDIALOGFACTORY->updateCoordinateWidget(pPoints->v2, pPoints->v2 - docView->getRelativeZero());

        if (e->modifiers() & Qt::ShiftModifier)
        {
            mouse = snapToAngle(mouse, pPoints->v1, ANGLE_SNAP_STEP);
            pPoints->v2 = mouse;
        }

        deletePreview();
        preview->addSelectionFromDocument();
        preview->moveRef(pPoints->v1, pPoints->v2 - pPoints->v1);

        if (e->modifiers() & Qt::ShiftModifier)
        {
            DmLine* line = new DmLine(nullptr, pPoints->v1, mouse);
            preview->addEntity(line);
            line->setSelected(true);
        }

        drawPreview();
        break;

    case Moving:
        pPoints->v2 = snapPoint(e);
        GUIDIALOGFACTORY->updateCoordinateWidget(pPoints->v2, pPoints->v2 - docView->getRelativeZero());

        if (e->modifiers() & Qt::ShiftModifier)
        {
            mouse = snapToAngle(mouse, pPoints->v1, ANGLE_SNAP_STEP);
            pPoints->v2 = mouse;
        }

        deletePreview();
        preview->addSelectionFromDocument();
        preview->move(pPoints->v2 - pPoints->v1);

        if (e->modifiers() & Qt::ShiftModifier)
        {
            DmLine* line = new DmLine(nullptr, pPoints->v1, mouse);
            preview->addEntity(line);
            line->setSelected(true);
        }

        drawPreview();
        break;

    case SetCorner2:
        if (pPoints->v1.valid)
        {
            pPoints->v2 = mouse;
            docView->setOverlayCorners(pPoints->v1, pPoints->v2);
            docView->redraw();
        }
        break;
    case Panning: {
        DmVector const vTarget(e->x(), e->y());
        DmVector const v01 = vTarget - pPoints->v1;
        if (v01.squared() >= PAN_SQUARED_THRESHOLD)
        {
            docView->zoomPan((int)v01.x, (int)v01.y);
            pPoints->v1 = vTarget;
        }
    }
                break;

    default:
        break;
    }
}

/// @brief 鼠标按下事件处理
/// @param[in] e 鼠标事件指针
void ActionDefault::mousePressEvent(QMouseEvent * e)
{
    if (e->button() == Qt::LeftButton)
    {
        switch (getStatus())
        {
        case Neutral: {
            auto const m = e->modifiers();
            if (m & (Qt::ControlModifier | Qt::MetaModifier))
            {
                pPoints->v1 = DmVector(e->x(), e->y());
                setStatus(Panning);
            }
            else
            {
                pPoints->v1 = docView->toGraph(e->x(), e->y());
                setStatus(Dragging);
            }
        }
                    break;

        case Moving: {
            pPoints->v2 = snapPoint(e);
            if (e->modifiers() & Qt::ShiftModifier)
            {
                pPoints->v2 = snapToAngle(pPoints->v2, pPoints->v1, ANGLE_SNAP_STEP);
            }
            deletePreview();
            Modification m(docView);
            DmVector offset = pPoints->v2 - pPoints->v1;
            m.move(offset);
            setStatus(Neutral);
            GUIDIALOGFACTORY->updateSelectionWidget(pDocument->getEntityTable()->countSelect());
            deleteSnapper();
        }
                   break;

        case MovingRef: {
            pPoints->v2 = snapPoint(e);
            if (e->modifiers() & Qt::ShiftModifier)
            {
                pPoints->v2 = snapToAngle(pPoints->v2, pPoints->v1, ANGLE_SNAP_STEP);
            }
            deletePreview();
            Modification m(docView);
            MoveRefData data;
            data.ref = pPoints->v1;
            data.offset = pPoints->v2 - pPoints->v1;
            m.moveRef(data);
            setStatus(Neutral);
            GUIDIALOGFACTORY->updateSelectionWidget(pDocument->getEntityTable()->countSelect());
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
}

/// @brief 鼠标释放事件处理
/// @param[in] e 鼠标事件指针
void ActionDefault::mouseReleaseEvent(QMouseEvent * e)
{
    if (e->button() == Qt::LeftButton)
    {
        pPoints->v2 = docView->toGraph(e->x(), e->y());
        switch (getStatus())
        {
        case Dragging:
        {
            // select single entity:
            DmEntity* en = catchEntity(e);

            if (en)
            {
                deletePreview();

                Selection s(pDocument, docView);
                s.selectSingle(en);
                docView->emitSelectedChanged();
                docView->redraw();
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
            pPoints->v2 = docView->toGraph(e->x(), e->y());

            deletePreview();

            bool cross = (pPoints->v1.x > pPoints->v2.x);
            Selection s(pDocument, docView);
            bool select = (e->modifiers() & Qt::ShiftModifier) ? false : true;
            s.selectWindow(pPoints->v1, pPoints->v2, select, cross);
            docView->emitSelectedChanged();
            docView->redraw();
            setStatus(Neutral);
            e->accept();
        }
        break;

        case Panning:
            setStatus(Neutral);
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
}

/// @brief 鼠标双击事件处理，进入实体编辑模式
/// @param[in] e 鼠标事件指针
void ActionDefault::mouseDoubleClickEvent(QMouseEvent * e)
{
    if (getStatus() != Neutral)
    {
        return;
    }
    DmVector clickPos = docView->toGraph(e->x(), e->y());

    //获得选择的实体，如果超过1个，不进入编辑状态
    std::list<DmEntity*> ents;
    for (auto& ent : *pDocument->getEntityTable())
    {
        if (ent->isSelected())
        {
            ents.emplace_back(ent);
        }
    }
    auto selectCount = ents.size();
    if (selectCount > 1)
        return;
    DmEntity* en = catchEntity(e);
    if (en == nullptr)
    {
        return;
    }

    //修改实体
    if (selectCount == 0 || (selectCount == 1 && en == ents.front()))
    {
        //多行文字的编辑不是模态对话框，且与Action关联，需要特殊处理
        if (en->getEntityType() == DM::EntityMText)
        {
            ActionDrawMText* mtextAction = new ActionDrawMText(pDocument, docView, true);
            mtextAction->setModifyData(static_cast<DmMText*>(en), clickPos);
            docView->setCurrentAction(mtextAction);
            return;
        }

        // 不是多行文字类型处理
        // 抄的ActionModifyEntity::trigger()
        GUIDIALOGFACTORY->requestModifyEntityDialog(en);
    }
}

/// @brief 获取可用命令列表
/// @return 可用命令字符串列表
QStringList ActionDefault::getAvailableCommands()
{
    QStringList cmd;
    return cmd;
}

/// @brief 更新鼠标按钮提示
void ActionDefault::updateMouseButtonHints()
{
    switch (getStatus())
    {
    case Neutral:
        GUIDIALOGFACTORY->updateMouseWidget();
        break;
    case SetCorner2:
        //GUIDIALOGFACTORY->updateMouseWidget(tr("Choose second edge"), tr("Back"));
        break;
    default:
        GUIDIALOGFACTORY->updateMouseWidget();
        break;
    }
}

/// @brief 更新鼠标光标
void ActionDefault::updateMouseCursor()
{
    switch (getStatus())
    {
    case Neutral:
        docView->setMouseCursor(DM::ArrowCursor);
        break;
    case Moving:
    case MovingRef:
        docView->setMouseCursor(DM::SelectCursor);
        break;
    case Panning:
        docView->setMouseCursor(DM::ClosedHandCursor);
        break;
    default:
        break;
    }
}

// EOF
