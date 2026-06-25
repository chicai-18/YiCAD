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


/// @file ActionModifyRotate.h
/// @brief 旋转修改操作——处理用户鼠标事件以实现实体旋转功能

#ifndef ACTIONMODIFYROTATE_H
#define ACTIONMODIFYROTATE_H

#include "PreviewActionInterface.h"

class RotateData;

/// @brief 旋转修改操作类，处理用户鼠标事件实现实体旋转功能
class ActionModifyRotate : public PreviewActionInterface
{
    Q_OBJECT

public:
    /// @brief 旋转操作状态枚举
    enum Status
    {
        setCenterPoint, ///< 设置旋转中心
        setAngle        ///< 设置角度
    };

public:
    /// @brief 构造函数
    /// @param [in] doc 文档指针
    /// @param [in] docView 文档视图指针
    ActionModifyRotate(DmDocument* doc, GuiDocumentView* docView);

    /// @brief 析构函数
    ~ActionModifyRotate() override;

    /// @brief 初始化操作状态
    /// @param [in] status 初始状态值
    void init(int status = 0) override;

    /// @brief 执行旋转操作
    void trigger() override;

    /// @brief 处理鼠标移动事件，实时预览旋转效果
    /// @param [in] e 鼠标事件指针
    void mouseMoveEvent(QMouseEvent* e) override;

    /// @brief 处理鼠标释放事件，确认旋转或回退操作
    /// @param [in] e 鼠标事件指针
    void mouseReleaseEvent(QMouseEvent* e) override;

    /// @brief 处理坐标事件，根据状态设置旋转中心/角度并执行旋转
    /// @param [in] e 坐标事件指针
    void coordinateEvent(GuiCoordinateEvent* e) override;

    /// @brief 处理命令事件，支持命令行输入旋转角度
    /// @param [in] e 命令事件指针
    void commandEvent(GuiCommandEvent* e) override;

    /// @brief 更新鼠标按钮提示信息
    void updateMouseButtonHints() override;

    /// @brief 更新鼠标光标为十字光标
    void updateMouseCursor() override;

    /// @brief 禁用拷贝构造
    ActionModifyRotate(const ActionModifyRotate&) = delete;

    /// @brief 禁用拷贝赋值
    ActionModifyRotate& operator=(const ActionModifyRotate&) = delete;

private:
    std::unique_ptr<RotateData> data; ///< 旋转操作数据
};

#endif
