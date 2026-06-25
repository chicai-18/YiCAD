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


/// @file ActionModifyCopy.cpp
/// @brief 复制实体 Action 类的实现

#include "ActionModifyCopy.h"

#include <QAction>
#include <QMouseEvent>
#include <cmath>

#include "Debug.h"
#include "DmLine.h"
#include "DmBlockReference.h"
#include "GuiCoordinateEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "GuiCommandEvent.h"
#include "Modification.h"
#include "Preview.h"
#include "Transaction.h"
#include "Math2d.h"
#include "DmSettings.h"

/// @brief 捕捉角度阈值（度）
static constexpr double SNAP_ANGLE_THRESHOLD = 15.0;
/// @brief 默认复制数量
static constexpr int DEFAULT_COPY_COUNT = 1;
/// @brief 最小复制数量
static constexpr int MIN_COPY_COUNT = 0;

struct ActionModifyCopy::Points
{
    CopyData data;              ///< 复制数据（偏移、数量等）
    DmVector referencePoint;    ///< 参考点
    DmVector targetPoint;       ///< 目标点
};

/// @brief 构造函数
/// @param doc 文档指针
/// @param docView 文档视图指针
ActionModifyCopy::ActionModifyCopy(DmDocument* doc, GuiDocumentView* docView) :
    PreviewActionInterface("Copy Entities", doc, docView), pPoints(new Points{})
{
    actionType = DM::ActionModifyCopy;
    initData();
}

/// @brief 析构函数
ActionModifyCopy::~ActionModifyCopy()
{
    DMSETTINGS->beginGroup("/Modify");
    DMSETTINGS->writeEntry("/CopyCount", pPoints->data.number);
    DMSETTINGS->endGroup();
}

/// @brief 初始化复制数据（从设置中读取默认复制数量）
void ActionModifyCopy::initData()
{
    DMSETTINGS->beginGroup("/Modify");
    pPoints->data.number = DMSETTINGS->readNumEntry("/CopyCount", DEFAULT_COPY_COUNT);
    DMSETTINGS->endGroup();
}

/// @brief 执行复制操作并提交事务
void ActionModifyCopy::trigger()
{
    Transaction t(tr("Copy").toStdString(), pDocument);
    t.start();
    auto entTable = pDocument->getEntityTable();

    // 添加的实体
    std::vector<DmEntity*> addedEnts = getCloneEntities();

    // 添加实体到实体表
    for (auto e : addedEnts)
    {
        entTable->add(e);
    }
    t.commit();

    GUIDIALOGFACTORY->updateSelectionWidget(pDocument->getEntityTable()->countSelect());
    finish(false);
}

/// @brief 鼠标移动事件处理
/// @param e 鼠标事件
void ActionModifyCopy::mouseMoveEvent(QMouseEvent* e)
{
    if (getStatus() == SetReferencePoint || getStatus() == SetTargetPoint)
    {
        DmVector mouse = snapPoint(e);
        switch (getStatus())
        {
            case SetReferencePoint:
                pPoints->referencePoint = mouse;
                break;

            case SetTargetPoint:
                if (pPoints->referencePoint.valid)
                {
                    if (e->modifiers() & Qt::ShiftModifier)
                    {
                        mouse = snapToAngle(mouse, pPoints->referencePoint, SNAP_ANGLE_THRESHOLD);
                    }
                    pPoints->targetPoint = mouse;

                    deletePreview();
                    pPoints->data.offset = pPoints->targetPoint - pPoints->referencePoint;
                    auto addEnts = getCloneEntities();
                    for (auto ent : addEnts)
                    {
                        preview->addEntity(ent);
                    }
                    if (e->modifiers() & Qt::ShiftModifier)
                    {
                        DmLine* line = new DmLine(nullptr, pPoints->referencePoint, mouse);
                        preview->addEntity(line);
                        line->setSelected(true);
                        line->setLayerToActive();
                        line->setPenToActive();
                    }
                    drawPreview();
                }
                break;

            default:
                break;
        }
    }
}

/// @brief 鼠标释放事件处理
/// @param e 鼠标事件
void ActionModifyCopy::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        DmVector snapped = snapPoint(e);
        if ((e->modifiers() & Qt::ShiftModifier) && getStatus() == SetTargetPoint)
        {
            snapped = snapToAngle(snapped, pPoints->referencePoint, SNAP_ANGLE_THRESHOLD);
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

/// @brief 坐标事件处理
/// @param e 坐标事件
void ActionModifyCopy::coordinateEvent(GuiCoordinateEvent* e)
{
    DmVector pos = e->getCoordinate();
    switch (getStatus())
    {
        case SetReferencePoint:
            pPoints->referencePoint = pos;
            docView->moveRelativeZero(pPoints->referencePoint);
            setStatus(SetTargetPoint);
            break;

        case SetTargetPoint:
            pPoints->targetPoint = pos;
            docView->moveRelativeZero(pPoints->targetPoint);
            pPoints->data.offset = pPoints->targetPoint - pPoints->referencePoint;
            trigger();
            break;

        default:
            break;
    }
}

/// @brief 命令事件处理（用于输入复制数量）
/// @param e 命令事件
void ActionModifyCopy::commandEvent(GuiCommandEvent* e)
{
    QString c = e->getCommand().toLower();
    bool ok = false;
    double r = Math2d::eval(c, &ok);
    if (ok && (static_cast<int>(r) > MIN_COPY_COUNT))
    {
        pPoints->data.number = static_cast<int>(r);
    }
    else
    {
        GUIDIALOGFACTORY->updateMouseWidget(tr("Input invalid"), tr("Back"));
    }
    e->accept();
}

/// @brief 更新鼠标按钮提示文本
void ActionModifyCopy::updateMouseButtonHints()
{
    switch (getStatus())
    {
        case SetReferencePoint:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify reference point or input copy number, default copy number is %1").arg(pPoints->data.number), tr("Cancel"));
            break;
        case SetTargetPoint:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify target point or input copy number, default copy number is %1").arg(pPoints->data.number), tr("Back"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

/// @brief 更新鼠标光标样式
void ActionModifyCopy::updateMouseCursor()
{
    if (docView != nullptr)
    {
        docView->setMouseCursor(DM::CadCursor);
    }
}

/// @brief 根据偏移及复制个数，克隆选中实体
/// @return 克隆出的实体列表
std::vector<DmEntity*> ActionModifyCopy::getCloneEntities() const
{
    auto entTable = pDocument->getEntityTable();

    // 拷贝数量
    int count = pPoints->data.number;

    // 偏移
    DmVector offset = pPoints->data.offset;

    // 添加的实体
    std::vector<DmEntity*> addedEnts;

    for (int num = 1; num <= count; num++)
    {
        for (auto e : *entTable)
        {
            if (e->isSelected())
            {
                auto cloneEnt = e->clone();
                cloneEnt->move(offset * num);
                if (cloneEnt->getEntityType() == DM::EntityBlockReference)
                {
                    static_cast<DmBlockReference*>(cloneEnt)->update();
                }
                addedEnts.emplace_back(cloneEnt);
            }
        }
    }
    return addedEnts;
}
