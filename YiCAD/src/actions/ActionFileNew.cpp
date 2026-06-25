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


/// @file ActionFileNew.cpp
/// @brief 新建文件 Action 类的实现

#include "ActionFileNew.h"
#include "ApplicationWindow.h"
#include "UITabDrawWidget.h"
#include <QAction>

#include "Debug.h"

/// @brief 构造函数
/// @param doc 文档指针
/// @param docView 文档视图指针
ActionFileNew::ActionFileNew(DmDocument* doc, GuiDocumentView* docView) :
    ActionInterface("File New", doc, docView)
    , m_strFileName("")
{
}

/// @brief 触发新建文件操作
void ActionFileNew::trigger()
{
    ApplicationWindow::getAppWindow()->getTabDrawWidget()->slotFileNew(m_strFileName);
    if (docView)
    {
        finish(false);
    }
    else
    {
        // docView 为空时不完成操作
    }
}

/// @brief 获取文件名
/// @return 文件全路径
QString ActionFileNew::getFileName() const
{
    return m_strFileName;
}

/// @brief 设置文件名
/// @param fileName 文件全路径
void ActionFileNew::setFileName(const QString& fileName)
{
    m_strFileName = fileName;
}

/// @brief 判断是否为独占模式
/// @return 始终返回 true
bool ActionFileNew::isExclusive()
{
    return true;
}

/// @brief 初始化并立即触发新建
/// @param status 初始状态
void ActionFileNew::init(int status)
{
    ActionInterface::init(status);
    trigger();
}
