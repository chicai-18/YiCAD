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


/// @file ActionTextStyle.cpp
/// @brief 文字样式设置Action实现

#include "ActionTextStyle.h"
#include "ApplicationWindow.h"
#include "GuiDialogFactory.h"
#include "DmDocument.h"

/// @brief 构造函数，初始化文字样式Action
/// @param [in] doc 文档指针
/// @param [in] docView 文档视图指针
ActionTextStyle::ActionTextStyle(DmDocument* doc, GuiDocumentView* docView)
    : ActionInterface("Text Style", doc, docView)
{
}

/// @brief 初始化操作状态，立即触发文字样式对话框
/// @param [in] status 初始状态值
void ActionTextStyle::init(int status)
{
    ActionInterface::init(status);
    trigger();
}

/// @brief 打开文字样式对话框
void ActionTextStyle::trigger()
{
    DmDocument* pDocument = ApplicationWindow::getAppWindow()->getDocument();
    GUIDIALOGFACTORY->requestTextStyleDialog(
        pDocument->getTextStyleTable(), pDocument);
    finish(false);
}
