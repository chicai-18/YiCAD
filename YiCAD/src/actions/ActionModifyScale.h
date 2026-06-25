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


/// @file ActionModifyScale.h
/// @brief 缩放实体交互命令头文件

#ifndef ACTIONMODIFYSCALE_H
#define ACTIONMODIFYSCALE_H

#include "PreviewActionInterface.h"

class ScaleData;

/// @brief 缩放实体交互命令
///
/// 处理用户交互事件，实现选中实体的比例缩放功能。
/// 支持设置参考点和输入缩放比例两种操作方式。
class ActionModifyScale : public PreviewActionInterface
{
    Q_OBJECT

public:
    /// @brief 命令状态枚举
    enum Status
    {
        SetReferencePoint, ///< 设置参考点
        SetScale           ///< 设置缩放比例
    };

public:
    ActionModifyScale(DmDocument* doc, GuiDocumentView* docView);
    ~ActionModifyScale() override;

    /// @brief 初始化实体包围框数据，计算选中实体的宽高较大值
    void initBox();

    /// @brief 初始化命令状态
    /// @param [in] status 初始状态，默认为0（SetReferencePoint）
    void init(int status = 0) override;

    /// @brief 执行缩放操作
    void trigger() override;

    /// @brief 处理鼠标移动事件
    /// @param [in] e 鼠标事件对象
    void mouseMoveEvent(QMouseEvent* e) override;

    /// @brief 处理鼠标释放事件
    /// @param [in] e 鼠标事件对象
    void mouseReleaseEvent(QMouseEvent* e) override;

    /// @brief 处理坐标输入事件
    /// @param [in] e 坐标事件对象
    void coordinateEvent(GuiCoordinateEvent* e) override;

    /// @brief 处理命令行输入事件
    /// @param [in] e 命令事件对象
    void commandEvent(GuiCommandEvent* e) override;

    /// @brief 更新鼠标按钮提示
    void updateMouseButtonHints() override;

    /// @brief 更新鼠标光标样式
    void updateMouseCursor() override;

private:
    struct Points;
    std::unique_ptr<Points> pPoints; ///< 缩放操作中使用的点数据
    double m_boxRange;               ///< 选中实体的宽高较大值
};

#endif
