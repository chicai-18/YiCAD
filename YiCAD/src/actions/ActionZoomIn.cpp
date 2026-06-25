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


/// @file ActionZoomIn.cpp
/// @brief 缩放Action实现

#include "ActionZoomIn.h"

#include <QAction>

#include "GuiDocumentView.h"

/// @brief 构造函数，初始化缩放参数
/// @param [in] doc 文档指针
/// @param [in] docView 文档视图指针
/// @param [in] direction 缩放方向（In放大/Out缩小）
/// @param [in] axis 受影响的轴向
/// @param [in] pCenter 缩放中心点
/// @param [in] factor 缩放因子
ActionZoomIn::ActionZoomIn(DmDocument* doc, GuiDocumentView* docView,
                           DM::ZoomDirection direction,
                           DM::Axis axis, DmVector const* pCenter,
                           double factor) :
    ActionInterface("Zoom in", doc, docView),
    zoom_factor(factor), direction(direction), axis(axis),
    center(pCenter ? new DmVector{*pCenter} : new DmVector{})
{
}

ActionZoomIn::~ActionZoomIn() = default;

/// @brief 初始化操作状态，立即执行缩放
/// @param [in] status 初始状态值
void ActionZoomIn::init(int status)
{
    ActionInterface::init(status);
    trigger();
}

/// @brief 执行缩放操作，根据轴向和方向调用相应的缩放方法
void ActionZoomIn::trigger()
{
    switch (axis)
    {
        case DM::Both:
            if (direction == DM::In)
            {
                docView->zoomIn(zoom_factor, *center);
            }
            else
            {
                docView->zoomOut(zoom_factor, *center);
            }
            break;

        default:
            break;
    }
    finish(false);
}
