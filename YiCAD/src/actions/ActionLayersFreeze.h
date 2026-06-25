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


/// @file ActionLayersFreeze.h
/// @brief 图层冻结/解冻动作类头文件

#ifndef ACTIONLAYERSFREEZE_H
#define ACTIONLAYERSFREEZE_H

#include "ActionInterface.h"

class DmLayer;

/// @brief 图层冻结/解冻动作类
class ActionLayersFreeze : public ActionInterface
{
    Q_OBJECT
public:
    /// @brief 构造函数
    /// @param sender 触发此动作的按钮对象
    /// @param doc 文档指针
    /// @param docView 文档视图指针
    ActionLayersFreeze(QObject* sender, DmDocument* doc, GuiDocumentView* docView);

    /// @brief 初始化动作
    /// @param status 状态参数，默认为0
    void init(int status = 0) override;

    /// @brief 执行冻结/解冻操作
    void trigger() override;

    /// @brief 设置要操作的图层
    /// @param layerName 图层名称
    void setLayer(const QString& layerName);

    /// @brief 设置目标状态（开启或关闭）
    /// @param on true表示设为开启，false表示关闭
    void setToBeOn(bool on);

protected:
    DmLayer* layer;      ///< 目标图层指针
    bool toBeOn;         ///< 目标状态：true为开启，false为冻结
    QObject* theButton;  ///< 触发此动作的按钮对象
};

#endif //!ACTIONLAYERSFREEZE_H
