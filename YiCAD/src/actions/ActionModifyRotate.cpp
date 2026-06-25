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


/// @file ActionModifyRotate.cpp
/// @brief 旋转修改操作——处理用户鼠标事件以实现实体旋转功能

#include "ActionModifyRotate.h"

#include <QAction>
#include <QMouseEvent>

#include "Debug.h"
#include "GuiCoordinateEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Math2d.h"
#include "Modification.h"
#include "Preview.h"
#include "Transaction.h"
#include "DmBlockReference.h"
#include "GuiCommandEvent.h"

/// @brief 未定义角度时的默认值
constexpr double ROTATE_ANGLE_UNDEFINED = 0.0;

ActionModifyRotate::ActionModifyRotate(DmDocument* doc, GuiDocumentView* docView) :
    PreviewActionInterface("Rotate Entities", doc, docView), data(new RotateData())
{
    actionType = DM::ActionModifyRotate;
}

ActionModifyRotate::~ActionModifyRotate() = default;

/// @brief 初始化旋转操作状态
/// @param [in] status 初始状态值
void ActionModifyRotate::init(int status)
{
    ActionInterface::init(status);
}

/// @brief 执行旋转操作，对所有选中实体进行旋转变换
void ActionModifyRotate::trigger()
{
    Transaction t(tr("Rotate").toStdString(), pDocument);
    t.start();
    auto entTable = pDocument->getEntityTable();
    for (auto e : *entTable)
    {
        if (e->isSelected())
        {
            e->setSelected(false);
            entTable->startModify(e);
            e->rotateAngle(data->center, data->angle);
            if (e->getEntityType() == DM::EntityBlockReference)
            {
                ((DmBlockReference*)e)->update();
            }
        }
    }
    t.commit();
    GUIDIALOGFACTORY->updateSelectionWidget(pDocument->getEntityTable()->countSelect());
}

/// @brief 处理鼠标移动事件，实时预览旋转效果
/// @param [in] e 鼠标事件指针
void ActionModifyRotate::mouseMoveEvent(QMouseEvent* e)
{
    DmVector mouse = snapPoint(e);
    switch (getStatus())
    {
        case setCenterPoint:
            break;

        case setAngle:
            if (!mouse.valid)
            {
                return;
            }
            deletePreview();
            preview->addSelectionFromDocument();
            preview->getEntityContainer()->rotateAngle(data->center, Math2d::correctAngle((mouse - data->center).angle()));
            drawPreview();
            break;

        default:
            break;
    }
}

/// @brief 处理鼠标释放事件，确认旋转或回退操作
/// @param [in] e 鼠标事件指针
void ActionModifyRotate::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        GuiCoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    }
    else if (e->button() == Qt::RightButton)
    {
        deletePreview();
        init(getStatus() - 1);
    }
}

/// @brief 处理坐标事件，根据状态设置旋转中心/角度并执行旋转
/// @param [in] e 坐标事件指针
void ActionModifyRotate::coordinateEvent(GuiCoordinateEvent* e)
{
    if (e == nullptr)
    {
        return;
    }

    DmVector pos = e->getCoordinate();
    if (!pos.valid)
    {
        return;
    }
    switch (getStatus())
    {
        case setCenterPoint:
            data->center = pos;
            docView->moveRelativeZero(data->center);
            setStatus(setAngle);
            break;
        case setAngle:
            pos -= data->center;
            if (pos.squared() < DM_TOLERANCE2)
            {
                // 角度未定义（点与中心重合）
                data->angle = ROTATE_ANGLE_UNDEFINED;
            }
            else
            {
                data->angle = Math2d::correctAngle(pos.angle());
            }
            trigger();
            finish(false);
            break;

        default:
            break;
    }
}

/// @brief 处理命令事件，支持命令行输入旋转角度
/// @param [in] e 命令事件指针
void ActionModifyRotate::commandEvent(GuiCommandEvent* e)
{
    QString c = e->getCommand().toLower();
    if (getStatus() == setAngle)
    {
        bool ok = false;
        double r = Math2d::eval(c, &ok);
        if (ok)
        {
            data->angle = Math2d::deg2rad(r);
            trigger();
            finish(false);
        }
        else
        {
            GUIDIALOGFACTORY->updateMouseWidget(tr("Input invalid"), tr("Back"));
        }
        e->accept();
    }
}

/// @brief 更新鼠标按钮提示信息
void ActionModifyRotate::updateMouseButtonHints()
{
    switch (getStatus())
    {
        case setCenterPoint:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Specify rotation center"), tr("Back"));
            break;

        case setAngle:
            GUIDIALOGFACTORY->updateMouseWidget(tr("Input angle"), tr("Back"));
            break;
        default:
            GUIDIALOGFACTORY->updateMouseWidget();
            break;
    }
}

/// @brief 更新鼠标光标为十字光标
void ActionModifyRotate::updateMouseCursor()
{
    docView->setMouseCursor(DM::CadCursor);
}
