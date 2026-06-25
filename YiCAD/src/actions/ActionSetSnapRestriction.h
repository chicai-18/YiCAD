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


/// @file ActionSetSnapRestriction.h
/// @brief 设置附加捕捉约束模式的Action

#ifndef ACTIONSETSNAPRESTRICTION_H
#define ACTIONSETSNAPRESTRICTION_H

#include "ActionInterface.h"

/// @brief 切换当前附加捕捉约束模式的Action
class ActionSetSnapRestriction : public ActionInterface
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] doc 文档指针
    /// @param [in] docView 文档视图指针
    /// @param [in] snapRes 要设置的捕捉约束模式
    ActionSetSnapRestriction(DmDocument* doc, GuiDocumentView* docView,
                             DM::SnapRestriction snapRes);

    /// @brief 初始化操作状态
    /// @param [in] status 初始状态值，默认为0
    void init(int status = 0) override;

    /// @brief 执行捕捉约束切换
    void trigger() override;

protected:
    DM::SnapRestriction snapRes; ///< 要设置的捕捉约束模式
};

#endif // ACTIONSETSNAPRESTRICTION_H
