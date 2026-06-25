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


/// @file ActionLayersFreezeAll.h
/// @brief 冻结/解冻所有图层的交互动作类
#ifndef ACTIONLAYERSFREEZEALL_H
#define ACTIONLAYERSFREEZEALL_H

#include "ActionInterface.h"

/// @brief 冻结/解冻所有图层的动作类
///
/// 处理用户冻结或解冻文档中所有图层的交互动作。
/// 通过 @p freeze 参数控制是冻结还是解冻操作。
class ActionLayersFreezeAll : public ActionInterface
{
    Q_OBJECT

public:

    /// @brief 构造函数
    /// @param freeze 是否冻结所有图层（true 为冻结，false 为解冻）
    /// @param doc CAD 文档指针
    /// @param docView 文档视图指针
    ActionLayersFreezeAll(const bool freeze, DmDocument* doc, GuiDocumentView* docView);

    /// @brief 初始化动作并立即触发
    /// @param status 初始状态（默认为 0）
    void init(int status = 0) override;

    /// @brief 执行冻结/解冻所有图层的操作
    void trigger() override;

private:
    bool freeze; ///< 是否冻结（true 为冻结，false 为解冻）
};

#endif // ACTIONLAYERSFREEZEALL_H
