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


/// @file ActionModifyReverse.h
/// @brief 反向修改操作——将选中实体的方向反转

#ifndef ACTIONMODIFYREVERSE_H
#define ACTIONMODIFYREVERSE_H

#include "ActionInterface.h"

/// @brief 反向修改操作类，将选中实体（线、弧、椭圆、样条曲线）的方向反转
class ActionModifyReverse : public ActionInterface
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] doc 文档指针
    /// @param [in] docView 文档视图指针
    ActionModifyReverse(DmDocument* doc, GuiDocumentView* docView);

    /// @brief 初始化并直接执行反向操作
    /// @param [in] status 初始状态值
    void init(int status = 0) override;

    /// @brief 执行反向操作，反转所有选中实体的方向
    void trigger() override;

    /// @brief 更新鼠标按钮提示信息
    void updateMouseButtonHints() override;

    /// @brief 禁用拷贝构造
    ActionModifyReverse(const ActionModifyReverse&) = delete;

    /// @brief 禁用拷贝赋值
    ActionModifyReverse& operator=(const ActionModifyReverse&) = delete;
};

#endif //ACTIONMODIFYREVERSE_H
