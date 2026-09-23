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


/// @file ActionDefault.cpp
/// @brief 默认动作类实现文件：转发给内部的 SelectTool

#include "ActionDefault.h"

#include "IDocumentView.h"
#include "Preview.h"
#include "SelectTool.h"

/// @brief 构造函数
/// @param[in] doc 文档指针
/// @param[in] docView 文档视图指针
/// @param[in] panTool 转交给内部的 SelectTool
ActionDefault::ActionDefault(DmDocument* doc, IDocumentView* docView, PanZoomTool* panTool)
    : ActionInterface("Default", doc, docView)
    , m_preview(std::make_unique<Preview>(doc))
    , m_selectTool(std::make_unique<SelectTool>(doc, docView, snapService(), m_preview.get(), panTool))
{
    actionType = DM::ActionDefault;
}

/// @brief 析构函数
ActionDefault::~ActionDefault() = default;

/// @brief 初始化动作
/// @param[in] status 初始状态
void ActionDefault::init(int status)
{
    ActionInterface::init(status);
    m_selectTool->init();
}

/// @brief 挂起此动作
void ActionDefault::suspend()
{
    ActionInterface::suspend();
    m_selectTool->onDeactivate();
}

/// @brief 从挂起状态恢复
void ActionDefault::resume()
{
    ActionInterface::resume();
    m_selectTool->onActivate();
}

/// @brief 键盘按下事件处理
/// @param[in] e 键盘事件指针
void ActionDefault::keyPressEvent(QKeyEvent* e)
{
    m_selectTool->keyPressEvent(e);
}

/// @brief 键盘释放事件处理
/// @param[in] e 键盘事件指针
void ActionDefault::keyReleaseEvent(QKeyEvent* e)
{
    m_selectTool->keyReleaseEvent(e);
}

/// @brief 鼠标移动事件处理
/// @param[in] e 鼠标事件指针
void ActionDefault::mouseMoveEvent(QMouseEvent* e)
{
    m_selectTool->mouseMoveEvent(e);
}

/// @brief 鼠标按下事件处理
/// @param[in] e 鼠标事件指针
void ActionDefault::mousePressEvent(QMouseEvent* e)
{
    m_selectTool->mousePressEvent(e);
}

/// @brief 鼠标释放事件处理
/// @param[in] e 鼠标事件指针
void ActionDefault::mouseReleaseEvent(QMouseEvent* e)
{
    m_selectTool->mouseReleaseEvent(e);
}

/// @brief 鼠标双击事件处理
/// @param[in] e 鼠标事件指针
void ActionDefault::mouseDoubleClickEvent(QMouseEvent* e)
{
    m_selectTool->mouseDoubleClickEvent(e);
}

/// @brief 获取可用命令列表
/// @return 可用命令字符串列表
QStringList ActionDefault::getAvailableCommands()
{
    return QStringList{};
}

/// @brief 更新鼠标按钮提示
void ActionDefault::updateMouseButtonHints()
{
    m_selectTool->updateButtonHints();
}

// EOF
