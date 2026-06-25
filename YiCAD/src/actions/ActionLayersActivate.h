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


/// @file ActionLayersActivate.h
/// @brief 图层激活 Action 类，处理设置当前激活图层的用户交互

#ifndef ACTIONLAYERSACTIVATE_H
#define ACTIONLAYERSACTIVATE_H

#include "ActionInterface.h"

class DmLayer;
struct ComboBoxData;

/// @brief 图层激活 Action，将指定图层设为当前活动图层
class ActionLayersActivate : public ActionInterface
{
    Q_OBJECT
public:
    /// @brief 构造函数
    /// @param sender 发送者对象指针
    /// @param doc 文档指针
    /// @param docView 文档视图指针
    ActionLayersActivate(QObject* sender, DmDocument* doc, GuiDocumentView* docView);

    /// @brief 初始化并立即触发
    /// @param status 初始状态
    void init(int status = 0) override;

    /// @brief 触发图层激活操作
    void trigger() override;

    /// @brief 获取当前图层
    /// @return 图层指针
    DmLayer* getLayer() const;

    /// @brief 设置图层
    /// @param layer 图层指针
    void setLayer(DmLayer* layer);

    /// @brief 根据图层名称设置图层
    /// @param layerName 图层名称
    void setLayer(const QString& layerName);

    /// @brief 设置下拉框数据
    /// @param data 下拉框数据指针
    void setComboBoxData(ComboBoxData* data);

protected:
    DmLayer* layer;            ///< 目标图层指针
    ComboBoxData* cbxData;     ///< 下拉框数据指针
    QObject* theButton;        ///< 触发按钮指针
};

#endif
