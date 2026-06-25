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


/// @file ActionDrawLineTangent1.h
/// @brief 点到圆的切线绘制交互动作头文件
#ifndef ACTIONDRAWLINETANGENT1_H
#define ACTIONDRAWLINETANGENT1_H

#include "PreviewActionInterface.h"

class DmLine;

/// @brief 点到圆的切线绘制动作类
///
/// 处理用户鼠标事件绘制点到圆的切线交互动作类。
/// 支持先选择起点再选择圆或弧，自动计算切点并绘制切线。
class ActionDrawLineTangent1 : public PreviewActionInterface
{
    Q_OBJECT

private:

    /// 动作状态枚举
    enum Status
    {
        SetPoint,   ///< 选择起点
        SetCircle   ///< 选择圆或弧
    };

public:

    /// @brief 构造函数
    /// @param doc CAD 文档指针
    /// @param docView 文档视图指针
    ActionDrawLineTangent1(DmDocument* doc, GuiDocumentView* docView);

    /// @brief 析构函数
    ~ActionDrawLineTangent1() override;

    /// @brief 触发动作，创建切线实体
    void trigger() override;

    /// @brief 鼠标移动事件处理
    /// @param e 鼠标事件
    void mouseMoveEvent(QMouseEvent* e) override;

    /// @brief 鼠标释放事件处理
    /// @param e 鼠标事件
    void mouseReleaseEvent(QMouseEvent* e) override;

    /// @brief 坐标输入事件处理
    /// @param e 坐标事件
    void coordinateEvent(GuiCoordinateEvent* e) override;

    /// @brief 更新鼠标按钮提示
    void updateMouseButtonHints() override;

    /// @brief 更新鼠标光标样式
    void updateMouseCursor() override;

    /// @brief 结束动作，清除高亮
    /// @param updateTB 是否更新工具栏
    void finish(bool updateTB) override;

private:

    /// @brief 创建切线
    /// @param container 实体容器指针
    /// @param coord 当前鼠标坐标
    /// @param point 起点坐标
    /// @param circle 目标圆或弧实体
    /// @return 创建的切线实体，失败返回 nullptr
    DmLine* createTangent1(DmEntityContainer* container,
                           const DmVector& coord,
                           const DmVector& point,
                           DmEntity* circle);

private:
    std::unique_ptr<DmLine> tangent; ///< 最近的切线预览
    std::unique_ptr<DmVector> point; ///< 选定的起点坐标
    DmEntity* circle = nullptr;      ///< 选定的圆或弧实体指针
};

#endif
