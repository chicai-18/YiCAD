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


/// @file ActionModifyMirror.cpp
/// @brief 镜像修改操作——处理用户鼠标事件以实现实体镜像功能

#include <QAction>
#include "ActionModifyMirror.h"

#include <QMouseEvent>

#include "Debug.h"
#include "DmLine.h"
#include "DmBlockReference.h"
#include "GuiCoordinateEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Modification.h"
#include "Preview.h"
#include "Transaction.h"
#include "GuiCommandEvent.h"
#include "DmSettings.h"

/// @brief 镜像操作步进角度（用于 Shift 约束）
constexpr double MIRROR_SNAP_ANGLE = 15.0;

struct ActionModifyMirror::Points
{
    MirrorData data;
    DmVector axisPoint1;
    DmVector axisPoint2;
};

ActionModifyMirror::ActionModifyMirror(DmDocument* doc, GuiDocumentView* docView) :
    PreviewActionInterface("Mirror Entities", doc, docView), pPoints(new Points{})
{
    actionType = DM::ActionModifyMirror;
    initData();
}

ActionModifyMirror::~ActionModifyMirror()
{
    DMSETTINGS->beginGroup("/Modify");
    DMSETTINGS->writeEntry("/MirrorCopy", (int)pPoints->data.copy);
    DMSETTINGS->endGroup();
}

/// @brief 从设置中恢复镜像操作数据（是否复制）
void ActionModifyMirror::initData()
{
    DMSETTINGS->beginGroup("/Modify");
    pPoints->data.copy = (bool)DMSETTINGS->readNumEntry("/MirrorCopy", 1);
    DMSETTINGS->endGroup();
}

/// @brief 初始化镜像操作状态
/// @param [in] status 初始状态值
void ActionModifyMirror::init(int status)
{
    ActionInterface::init(status);
}

/// @brief 执行镜像操作，对所有选中的实体进行镜像变换
void ActionModifyMirror::trigger()
{
    Transaction t(tr("mirror").toStdString(), pDocument);
    t.start();
    auto entTable = pDocument->getEntityTable();

    // 复制
    bool copy = pPoints->data.copy;

    std::vector<DmEntity*> addEnts;
    for (auto e : *entTable)
    {
        if (e->isSelected())
        {
            e->setSelected(false);
            DmEntity* theEnt = nullptr;

            if (copy)
            {
                // 复制
                theEnt = e->clone();
            }
            else
            {
                // 删除原始
                theEnt = e;
                entTable->startModify(theEnt);
            }

            theEnt->mirror(pPoints->data.axisPoint1, pPoints->data.axisPoint2);
            if (theEnt->getEntityType() == DM::EntityBlockReference)
            {
                ((DmBlockReference*)theEnt)->update();
            }
            if (copy)
            {
                addEnts.emplace_back(theEnt);
            }
        }
    }

    // 实体添加到表中
    for (auto ent : addEnts)
    {
        entTable->add(ent);
    }
    t.commit();

    GUIDIALOGFACTORY->updateSelectionWidget(pDocument->getEntityTable()->countSelect());
}

/// @brief 处理鼠标移动事件，更新镜像轴点位置
/// @param [in] e 鼠标事件指针
void ActionModifyMirror::mouseMoveEvent(QMouseEvent* e)
{
    if (getStatus() == SetAxisPoint1 || getStatus() == SetAxisPoint2)
    {
        DmVector mouse = snapPoint(e);
        switch (getStatus())
        {
            case SetAxisPoint1:
                pPoints->axisPoint1 = mouse;
                break;

            case SetAxisPoint2:
                if (pPoints->axisPoint1.valid)
                {
                    if (e->modifiers() & Qt::ShiftModifier)
                    {
                        mouse = snapToAngle(mouse, pPoints->axisPoint1, MIRROR_SNAP_ANGLE);
                    }

                    pPoints->axisPoint2 = mouse;

                    deletePreview();
                    preview->addSelectionFromDocument();
                    preview->getEntityContainer()->mirror(pPoints->axisPoint1, pPoints->axisPoint2);

                    preview->addEntity(new DmLine(nullptr, pPoints->axisPoint1, pPoints->axisPoint2));

                    drawPreview();
                }
                break;

            default:
                break;
        }
    }
}

/// @brief 处理鼠标释放事件，确认镜像轴点或回退操作
/// @param [in] e 鼠标事件指针
void ActionModifyMirror::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        DmVector snapped = snapPoint(e);
        if ((e->modifiers() & Qt::ShiftModifier) && getStatus() == SetAxisPoint2)
        {
            snapped = snapToAngle(snapped, pPoints->axisPoint1, MIRROR_SNAP_ANGLE);
        }

        GuiCoordinateEvent ce(snapped);
        coordinateEvent(&ce);
    }
    else if (e->button() == Qt::RightButton)
    {
        deletePreview();
        init(getStatus() - 1);
    }
}

/// @brief 处理命令事件，支持 Y/N 切换复制/删除原图模式
/// @param [in] e 命令事件指针
void ActionModifyMirror::commandEvent(GuiCommandEvent* e)
{
    QString c = e->getCommand().toLower();
    if (c == "y")
    {
        pPoints->data.copy = true;
    }
    else if (c == "n")
    {
        pPoints->data.copy = false;
    }
    else
    {
        GUIDIALOGFACTORY->updateMouseWidget(tr("Input invalid"), tr("Back"));
    }
    e->accept();
}

/// @brief 处理坐标事件，根据状态设置镜像轴点并执行镜像
/// @param [in] e 坐标事件指针
void ActionModifyMirror::coordinateEvent(GuiCoordinateEvent* e)
{
    DmVector mouse = e->getCoordinate();

    switch (getStatus())
    {
        case SetAxisPoint1:
            pPoints->axisPoint1 = mouse;
            setStatus(SetAxisPoint2);
            docView->moveRelativeZero(mouse);
            break;

        case SetAxisPoint2:
            pPoints->axisPoint2 = mouse;
            docView->moveRelativeZero(mouse);
            pPoints->data.axisPoint1 = pPoints->axisPoint1;
            pPoints->data.axisPoint2 = pPoints->axisPoint2;
            trigger();
            finish(false);
            break;

        default:
            break;
    }
}

/// @brief 更新鼠标按钮提示信息
void ActionModifyMirror::updateMouseButtonHints()
{
    QString copyStr = pPoints->data.copy ? tr("copy") : tr("delete origin");
    switch (getStatus())
    {
        case SetAxisPoint1:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify first point of mirror line, or type Y to copy, type N to delete origin, the default is [%1]").arg(copyStr), tr("Cancel"));
            break;
        case SetAxisPoint2:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify second point of mirror line, or type Y to copy, type N to delete origin, the default is [%1]").arg(copyStr), tr("Back"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

/// @brief 更新鼠标光标为十字光标
void ActionModifyMirror::updateMouseCursor()
{
    docView->setMouseCursor(DM::CadCursor);
}
