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


/// @file ActionModifyEntity.h
/// @brief 修改实体属性的交互动作类声明

#ifndef ACTIONMODIFYENTITY_H
#define ACTIONMODIFYENTITY_H

#include "ActionInterface.h"

/// @brief 修改实体属性的交互动作
///
/// 处理用户点击实体进行属性修改的操作，支持多行文字等特殊实体类型的处理。
class ActionModifyEntity : public ActionInterface
{
    Q_OBJECT
public:
    /// @brief 构造函数
    /// @param [in] doc 文档指针
    /// @param [in] docView 文档视图指针
    ActionModifyEntity(DmDocument* doc, GuiDocumentView* docView);

    /// @brief 触发修改操作
    void trigger() override;

    /// @brief 鼠标释放事件处理
    /// @param [in] e 鼠标事件指针
    void mouseReleaseEvent(QMouseEvent* e) override;

    /// @brief 更新鼠标光标样式
    void updateMouseCursor() override;

    /// @brief 更新鼠标按键提示信息
    void updateMouseButtonHints() override;

private:
    DmEntity* m_currentEntity = nullptr; ///< 当前拾取的实体指针
};

#endif // ACTIONMODIFYENTITY_H
