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


/// @file ActionPolylineDel.h
/// @brief 多段线删除节点操作，支持从现有多段线中删除指定节点

#ifndef ACTIONPOLYLINEDEL_H
#define ACTIONPOLYLINEDEL_H

#include <memory>

#include "PreviewActionInterface.h"

class DmEntity;
class DmVector;

/// @brief 多段线删除节点操作，支持从现有多段线中删除指定节点
class ActionPolylineDel : public PreviewActionInterface
{
    Q_OBJECT

public:
    /// @brief 操作状态枚举
    enum Status
    {
        ChooseEntity,   ///< 选择要删除节点的多段线
        SetDelPoint     ///< 设置要删除的节点位置
    };

public:
    /// @brief 构造函数
    /// @param [in] doc 文档指针
    /// @param [in] docView 文档视图指针
    ActionPolylineDel(DmDocument* doc, GuiDocumentView* docView);

    /// @brief 析构函数
    ~ActionPolylineDel() override;

    /// @brief 初始化操作状态
    /// @param [in] status 初始状态值，默认为0（ChooseEntity）
    void init(int status = 0) override;

    /// @brief 执行删除节点操作
    void trigger() override;

    /// @brief 处理鼠标移动事件
    /// @param [in] e 鼠标事件指针
    void mouseMoveEvent(QMouseEvent* e) override;

    /// @brief 处理鼠标释放事件
    /// @param [in] e 鼠标事件指针
    void mouseReleaseEvent(QMouseEvent* e) override;

    /// @brief 更新鼠标按钮提示文本
    void updateMouseButtonHints() override;

    /// @brief 更新鼠标光标样式
    void updateMouseCursor() override;

    /// @brief 完成操作，清理状态
    /// @param [in] updateTB 是否更新工具栏
    void finish(bool updateTB) override;

private:
    DmEntity* delEntity = nullptr;              ///< 待删除节点的多段线实体
    std::unique_ptr<DmVector> delPoint;         ///< 待删除节点的坐标点
};

#endif
