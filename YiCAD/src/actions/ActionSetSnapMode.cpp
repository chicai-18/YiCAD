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


/// @file ActionSetSnapMode.cpp
/// @brief 设置捕捉模式的Action实现

#include "ActionSetSnapMode.h"

#include "GuiDocumentView.h"

/// @brief 构造函数，设置新的捕捉模式
/// @param [in] doc 文档指针
/// @param [in] docView 文档视图指针
/// @param [in] snapMode 新的捕捉模式
ActionSetSnapMode::ActionSetSnapMode(DmDocument* doc,
                                     GuiDocumentView* docView,
                                     DM::SnapMode snapMode) :
    ActionInterface("Set Snap Mode", doc, docView)
{
    this->snapMode = snapMode;
}

/// @brief 初始化操作状态，立即触发捕捉模式切换
/// @param [in] status 初始状态值
void ActionSetSnapMode::init(int status)
{
    ActionInterface::init(status);
    trigger();
}

/// @brief 执行捕捉模式切换，根据当前模式更新捕捉设置
void ActionSetSnapMode::trigger()
{
    SnapMode s = docView->getDefaultSnapMode();

    switch (snapMode)
    {
        case DM::SnapFree:
            s.clear();
            break;

        case DM::SnapCenter:
            s.snapCenter = !s.snapCenter;
            break;

        case DM::SnapEndpoint:
            s.snapEndpoint = !s.snapEndpoint;
            break;

        case DM::SnapGrid:
            s.snapGrid = !s.snapGrid;
            break;

        case DM::SnapOnEntity:
            s.snapOnEntity = !s.snapOnEntity;
            break;

        case DM::SnapMiddle:
            s.snapMiddle = !s.snapMiddle;
            break;

        case DM::SnapIntersection:
            s.snapIntersection = !s.snapIntersection;
            break;

        default:
            break;
    }

    docView->setDefaultSnapMode(s);

    finish(false);
}
