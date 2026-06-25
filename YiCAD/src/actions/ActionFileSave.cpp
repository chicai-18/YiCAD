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


/// @file ActionFileSave.cpp
/// @brief 文件保存 Action 类的实现

#include "ActionFileSave.h"
#include "ApplicationWindow.h"
#include "UITabDrawWidget.h"
#include <QAction>

#include "Debug.h"
#include "GuiDocumentView.h"

/// @brief 默认文件保存格式类型
constexpr const char* YCD_FORMAT_TYPE = "ycd";

/// @brief 构造函数
/// @param doc 文档指针
/// @param docView 文档视图指针
ActionFileSave::ActionFileSave(DmDocument* doc, GuiDocumentView* docView)
    : ActionInterface("File Save", doc, docView)
{
}

/// @brief 触发文件保存操作
void ActionFileSave::trigger()
{
    DMSYSTEM->setCurrentFormatType(YCD_FORMAT_TYPE);
    ApplicationWindow::getAppWindow()->getTabDrawWidget()->slotFileSave();
    finish(false);
}

/// @brief 判断是否为独占模式
/// @return 始终返回 true
bool ActionFileSave::isExclusive()
{
    return true;
}

/// @brief 初始化并立即触发保存
/// @param status 初始状态
void ActionFileSave::init(int status)
{
    ActionInterface::init(status);
    trigger();
}
