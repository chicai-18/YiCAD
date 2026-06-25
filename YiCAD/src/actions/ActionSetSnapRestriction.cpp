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


/// @file ActionSetSnapRestriction.cpp
/// @brief 设置附加捕捉约束模式的Action实现

#include "ActionSetSnapRestriction.h"

#include "GuiDocumentView.h"

/// @brief 构造函数，设置新的附加捕捉约束模式
/// @param [in] doc 文档指针
/// @param [in] docView 文档视图指针
/// @param [in] snapRes 新的捕捉约束模式
ActionSetSnapRestriction::ActionSetSnapRestriction(DmDocument* doc,
                                                   GuiDocumentView* docView,
                                                   DM::SnapRestriction snapRes) :
    ActionInterface("Set Additional Snap Mode", doc, docView)
{
    this->snapRes = snapRes;
}

/// @brief 初始化操作状态，立即触发捕捉约束切换
/// @param [in] status 初始状态值
void ActionSetSnapRestriction::init(int status)
{
    ActionInterface::init(status);
    trigger();
}

/// @brief 执行捕捉约束切换
void ActionSetSnapRestriction::trigger()
{
    docView->setSnapRestriction(snapRes);

    finish(false);
}
