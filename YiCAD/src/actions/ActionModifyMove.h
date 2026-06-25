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


/// @file ActionModifyMove.h
/// @brief 移动修改操作——处理用户鼠标事件以实现实体移动功能

#ifndef ACTIONMODIFYMOVE_H
#define ACTIONMODIFYMOVE_H

#include "PreviewActionInterface.h"

/// @brief 移动修改操作类，处理用户鼠标事件实现实体移动功能
class ActionModifyMove : public PreviewActionInterface
{
    Q_OBJECT

public:
    /// @brief 移动操作状态枚举
    enum Status
    {
        SetReferencePoint, ///< 设置参考点
        SetTargetPoint,    ///< 设置目标点
    };

public:
    /// @brief 构造函数
    /// @param [in] doc 文档指针
    /// @param [in] docView 文档视图指针
    ActionModifyMove(DmDocument* doc, GuiDocumentView* docView);

    /// @brief 析构函数
    ~ActionModifyMove() override;

    /// @brief 执行移动操作
    void trigger() override;

    /// @brief 处理鼠标移动事件，更新目标点位置
    /// @param [in] e 鼠标事件指针
    void mouseMoveEvent(QMouseEvent* e) override;

    /// @brief 处理鼠标释放事件，确认移动或回退操作
    /// @param [in] e 鼠标事件指针
    void mouseReleaseEvent(QMouseEvent* e) override;

    /// @brief 处理坐标事件，根据状态设置参考点/目标点并执行移动
    /// @param [in] e 坐标事件指针
    void coordinateEvent(GuiCoordinateEvent* e) override;

    /// @brief 更新鼠标按钮提示信息
    void updateMouseButtonHints() override;

    /// @brief 更新鼠标光标为十字光标
    void updateMouseCursor() override;

    /// @brief 禁用拷贝构造
    ActionModifyMove(const ActionModifyMove&) = delete;

    /// @brief 禁用拷贝赋值
    ActionModifyMove& operator=(const ActionModifyMove&) = delete;

private:
    struct Points;
    std::unique_ptr<Points> pPoints; ///< 移动操作数据点
};

#endif
