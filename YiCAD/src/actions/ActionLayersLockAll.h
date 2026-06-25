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


/// @file ActionLayersLockAll.h
/// @brief 图层全部锁定/解锁 Action 类，处理锁定或解锁所有图层的用户交互

#ifndef ACTIONLAYERSLOCKALL_H
#define ACTIONLAYERSLOCKALL_H

#include "ActionInterface.h"

/// @brief 图层全部锁定/解锁 Action，处理锁定或解锁所有图层的操作
class ActionLayersLockAll : public ActionInterface
{
    Q_OBJECT
public:
    /// @brief 构造函数
    /// @param lock true 表示锁定所有图层，false 表示解锁所有图层
    /// @param doc 文档指针
    /// @param docView 文档视图指针
    ActionLayersLockAll(const bool lock, DmDocument* doc, GuiDocumentView* docView);

    /// @brief 初始化并立即触发
    /// @param status 初始状态
    void init(int status = 0) override;

    /// @brief 触发全部锁定/解锁图层操作
    void trigger() override;

private:
    bool lock; ///< true 锁定所有图层，false 解锁所有图层
};

#endif //!ACTIONLAYERSLOCKALL_H
