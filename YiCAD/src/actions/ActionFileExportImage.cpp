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


/// @file ActionFileExportImage.cpp
/// @brief 图像导出 Action 类的实现

#include "ActionFileExportImage.h"
#include "ApplicationWindow.h"
#include "UITabDrawWidget.h"

/// @brief 构造函数
/// @param doc 文档指针
/// @param docView 文档视图指针
ActionFileExportImage::ActionFileExportImage(DmDocument* doc, GuiDocumentView* docView) :
    ActionInterface("Image Export", doc, docView)
{
}

/// @brief 判断是否为独占模式
/// @return 始终返回 true
bool ActionFileExportImage::isExclusive()
{
    return true;
}

/// @brief 初始化并立即触发导出
/// @param status 初始状态
void ActionFileExportImage::init(int status)
{
    ActionInterface::init(status);
    trigger();
}

/// @brief 触发图像导出操作
void ActionFileExportImage::trigger()
{
    ApplicationWindow::getAppWindow()->getTabDrawWidget()->slotFileExportImage();
    finish(false);
}
