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


/// @file ActionInfoSelected.cpp
/// @brief 选中实体信息 Action 类的实现

#include "ActionInfoSelected.h"
#include "GuiDialogFactory.h"
#include "DmSpline.h"
#include "DmEntityHelper.h"

/// @brief 构造函数
/// @param doc 文档指针
/// @param docView 文档视图指针
ActionInfoSelected::ActionInfoSelected(DmDocument* doc, GuiDocumentView* docView)
        : ActionInterface("Info Selected Entities", doc, docView)
{
    actionType = DM::ActionInfoSelected;
}

/// @brief 初始化并立即触发
/// @param status 初始状态
void ActionInfoSelected::init(int status)
{
    ActionInterface::init(status);
    trigger();
}

/// @brief 触发信息显示
void ActionInfoSelected::trigger()
{
    QString str;
    auto table = pDocument->getEntityTable();
    for (auto e : *table)
    {
        if (e->isSelected())
        {
            getInfo(e, str);
            str.append("\n");
        }
    }
    GUIDIALOGFACTORY->commandMessage(str);
    finish();
}

/// @brief 获取实体信息
/// @param e 实体指针
/// @param info 输出信息字符串
void ActionInfoSelected::getInfo(const DmEntity* e, QString& info)
{
    // 基本信息
    info.append(tr("Type: "));
    info.append(QString::fromStdString(DmEntityHelper::getEntityNameByType(e->getEntityType())));
    info.append("\n");
    info.append(tr("ID: "));
    info.append(QString::fromStdString(e->getId().asString()));
    info.append("\n");
    info.append(tr("Layer: "));
    DmLayer* layer = e->getLayer(false);
    if (layer)
    {
        info.append(layer->getName());
    }
    else
    {
        info.append("NULL");
    }
    info.append("\n");

    // 针对不同的实体类型，获得不同的信息
    switch (e->getEntityType())
    {
        case DM::EntitySpline:
            getInfoForSpline(static_cast<const DmSpline*>(e), info);
            break;
        case DM::EntityLine:
            break;
        // ...
        default:
            break;
    }
}

/// @brief 格式化向量信息
/// @param vec 向量
/// @param info 输出信息字符串
void ActionInfoSelected::infoVector(const DmVector& vec, QString& info)
{
    info.append("(");
    info.append(QString::number(vec.x, 'f'));
    info.append(", ");
    info.append(QString::number(vec.y, 'f'));
    info.append(")");
}

/// @brief 获取样条曲线专用信息
/// @param spline 样条曲线指针
/// @param info 输出信息字符串
void ActionInfoSelected::getInfoForSpline(const DmSpline* spline, QString& info)
{
    const DmVector startPt = spline->getStartpoint();
    const DmVector endPt = spline->getEndpoint();
    auto knots = spline->getKnots();
    int knotCount = static_cast<int>(knots.size());
    auto ctrlPts = spline->getControlPoints();
    int ctrlPtCount = static_cast<int>(ctrlPts.size());

    info.append(tr("start point: "));
    infoVector(startPt, info);
    info.append("\n");
    info.append(tr("end point: "));
    infoVector(endPt, info);
    info.append("\n");

    info.append(tr("knots count: "));
    info.append(QString::number(knotCount));
    info.append("\n");
    info.append(tr("knots: "));
    for (int i = 0; i < knotCount; i++)
    {
        info.append(QString::number(knots.at(i)));
        if (i != knotCount - 1)
        {
            info.append(", ");
        }
    }
    info.append("\n");

    info.append(tr("control point count: "));
    info.append(QString::number(ctrlPtCount));
    info.append("\n");
    info.append(tr("control points: "));
    for (int i = 0; i < ctrlPtCount; i++)
    {
        infoVector(ctrlPts.at(i), info);
        if (i != ctrlPtCount - 1)
        {
            info.append(", ");
        }
    }
}
