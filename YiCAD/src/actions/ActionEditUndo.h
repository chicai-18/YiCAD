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


/// @file ActionEditUndo.h
/// @brief 撤销/重做 Action 类，处理用户撤销和重做的事件

#ifndef ACTIONEDITUNDO_H
#define ACTIONEDITUNDO_H

#include "ActionInterface.h"

/// @brief 处理撤销和重做用户事件的 Action 类
class ActionEditUndo : public ActionInterface
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param undo true 为撤销，false 为重做
    /// @param doc 文档指针
    /// @param docView 文档视图指针
    ActionEditUndo(bool undo, DmDocument* doc, GuiDocumentView* docView);

    /// @brief 初始化并立即执行
    /// @param status 初始状态
    void init(int status = 0) override;

    /// @brief 触发撤销或重做操作
    void trigger() override;

protected:
    const bool undo; ///< Undo (true) 或 redo (false)
};

#endif // ACTIONEDITUNDO_H
