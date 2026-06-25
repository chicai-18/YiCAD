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


/// @file ActionBlocksSaveAs.cpp
/// @brief 块另存为动作类实现文件

#include "ActionBlocksSaveAs.h"

#include <QAction>

#include "ApplicationWindow.h"
#include "Debug.h"
#include "DmDocument.h"
#include "UIActionHandler.h"
#include "UIBlockSaveAs.h"

/// @brief 构造函数
/// @param[in] doc 文档指针
/// @param[in] docView 文档视图指针
ActionBlocksSaveAs::ActionBlocksSaveAs(
    DmDocument* doc, GuiDocumentView* docView)
    : ActionInterface("Block Save As", doc, docView)
    , m_pBlockSaveAs(nullptr)
{
}

/// @brief 析构函数
ActionBlocksSaveAs::~ActionBlocksSaveAs() = default;

/// @brief 触发动作执行，显示块另存为对话框
void ActionBlocksSaveAs::trigger()
{
    ApplicationWindow* appWin = ApplicationWindow::getAppWindow();
    UIActionHandler* handler = appWin->getActionHandler();
    if (!m_pBlockSaveAs)
    {
        m_pBlockSaveAs =
            new UIBlockSaveAs(handler, appWin, "Block SaveAs");
    }
    m_pBlockSaveAs->setBlockList(pDocument->getBlockTable());
    m_pBlockSaveAs->show();
    finish(false);
}

/// @brief 判断是否为独占动作
/// @return true 表示独占
bool ActionBlocksSaveAs::isExclusive()
{
    return true;
}

/// @brief 初始化动作
/// @param[in] status 初始状态
void ActionBlocksSaveAs::init(int status)
{
    ActionInterface::init(status);
    trigger();
}

// 文件结束
