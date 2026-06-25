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


/// @file ActionModifyDelete.h
/// @brief 删除选中实体的交互动作类声明

#ifndef ACTIONMODIFYDELETE_H
#define ACTIONMODIFYDELETE_H

#include "ActionInterface.h"

/// @brief 删除选中实体的交互动作
///
/// 处理用户删除选定实体的操作，选中实体后直接执行删除逻辑。
class ActionModifyDelete : public ActionInterface
{
    Q_OBJECT
public:
    /// @brief 动作状态枚举
    enum Status
    {
        Acknowledge ///< 确认或取消
    };

public:
    /// @brief 构造函数
    /// @param [in] doc 文档指针
    /// @param [in] docView 文档视图指针
    ActionModifyDelete(DmDocument* doc, GuiDocumentView* docView);

    /// @brief 初始化动作
    /// @param [in] status 初始状态，默认为0
    void init(int status = 0) override;

    /// @brief 触发删除操作，调用Modification::remove()执行
    void trigger() override;

    /// @brief 更新鼠标按键提示信息
    void updateMouseButtonHints() override;

    /// @brief 更新鼠标光标为删除样式
    void updateMouseCursor() override;
};

#endif // ACTIONMODIFYDELETE_H
