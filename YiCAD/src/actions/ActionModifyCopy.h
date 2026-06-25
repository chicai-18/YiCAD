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


/// @file ActionModifyCopy.h
/// @brief 复制实体 Action 类，处理用户事件以复制选中的实体

#ifndef ACTIONMODIFYCOPY_H
#define ACTIONMODIFYCOPY_H

#include "PreviewActionInterface.h"

class CopyData;

/// @brief 处理实体复制的用户交互 Action
class ActionModifyCopy : public PreviewActionInterface
{
    Q_OBJECT

public:
    /// @brief Action 状态枚举
    enum Status
    {
        SetReferencePoint,  ///< 设置参考点
        SetTargetPoint,     ///< 设置目标点
    };

    /// @brief 构造函数
    /// @param doc 文档指针
    /// @param docView 文档视图指针
    ActionModifyCopy(DmDocument* doc, GuiDocumentView* docView);

    /// @brief 析构函数
    ~ActionModifyCopy() override;

    /// @brief 初始化复制数据（从设置中读取默认复制数量）
    void initData();

    /// @brief 执行复制操作并提交事务
    void trigger() override;

    /// @brief 鼠标移动事件处理
    /// @param e 鼠标事件
    void mouseMoveEvent(QMouseEvent* e) override;

    /// @brief 鼠标释放事件处理
    /// @param e 鼠标事件
    void mouseReleaseEvent(QMouseEvent* e) override;

    /// @brief 坐标事件处理
    /// @param e 坐标事件
    void coordinateEvent(GuiCoordinateEvent* e) override;

    /// @brief 命令事件处理（用于输入复制数量）
    /// @param e 命令事件
    void commandEvent(GuiCommandEvent* e) override;

    /// @brief 更新鼠标按钮提示文本
    void updateMouseButtonHints() override;

    /// @brief 更新鼠标光标样式
    void updateMouseCursor() override;

private:
    /// @brief 根据偏移及复制个数，克隆选中实体
    /// @return 克隆出的实体列表
    std::vector<DmEntity*> getCloneEntities() const;

private:
    struct Points;
    std::unique_ptr<Points> pPoints;  ///< 复制操作数据（参考点、目标点、复制数量等）
};

#endif // ACTIONMODIFYCOPY_H
