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


/// @file ActionLayersColor.h
/// @brief 图层颜色 Action 类，处理用户修改图层颜色的交互

#ifndef ACTIONLAYERSCOLOR_H
#define ACTIONLAYERSCOLOR_H

#include "ActionInterface.h"

class DmLayer;

/// @brief 图层颜色 Action，弹出颜色对话框并修改图层颜色
class ActionLayersColor : public ActionInterface
{
    Q_OBJECT
public:
    /// @brief 构造函数
    /// @param sender 发送者对象指针
    /// @param doc 文档指针
    /// @param docView 文档视图指针
    ActionLayersColor(QObject* sender, DmDocument* doc, GuiDocumentView* docView);

    /// @brief 初始化并立即触发
    /// @param status 初始状态
    void init(int status = 0) override;

    /// @brief 触发图层颜色修改操作
    void trigger() override;

    /// @brief 根据图层名称设置图层
    /// @param layerName 图层名称
    void setLayer(const QString& layerName);

protected:
    DmLayer* layer;     ///< 目标图层指针
    QObject* theButton; ///< 触发按钮指针
};

#endif
