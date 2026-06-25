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


/// @file ActionDrawLineRectangle.h
/// @brief 矩形绘制动作类，通过两个对角点处理用户事件绘制矩形

#ifndef ACTIONDRAWLINERECTANGLE_H
#define ACTIONDRAWLINERECTANGLE_H

#include "PreviewActionInterface.h"

/// @brief 矩形绘制动作类
///
/// 通过两个对角点处理用户事件绘制矩形。
class ActionDrawLineRectangle : public PreviewActionInterface
{
    Q_OBJECT

public:
    /// @brief 动作状态枚举
    enum Status
    {
        SetCorner1, ///< 设置第一个角点
        SetCorner2  ///< 设置第二个角点
    };

public:
    /// @brief 构造函数
    /// @param doc 文档对象指针
    /// @param docView 文档视图指针
    ActionDrawLineRectangle(DmDocument* doc, GuiDocumentView* docView);

    /// @brief 析构函数
    ~ActionDrawLineRectangle() override;

    /// @brief 触发动作执行，创建矩形并添加到文档
    void trigger() override;

    /// @brief 鼠标移动事件处理，实时预览矩形
    /// @param e 鼠标事件指针
    void mouseMoveEvent(QMouseEvent* e) override;

    /// @brief 鼠标释放事件处理，确定角点位置
    /// @param e 鼠标事件指针
    void mouseReleaseEvent(QMouseEvent* e) override;

    /// @brief 坐标输入事件处理，设置角点并触发绘制
    /// @param e 坐标事件指针
    void coordinateEvent(GuiCoordinateEvent* e) override;

    /// @brief 命令事件处理
    /// @param e 命令事件指针
    void commandEvent(GuiCommandEvent* e) override;

    /// @brief 更新鼠标按钮提示文本
    void updateMouseButtonHints() override;

    /// @brief 更新鼠标光标样式
    void updateMouseCursor() override;

private:
    /// @brief 创建矩形多段线
    /// @param container 实体容器指针，可为 nullptr
    /// @return 创建的矩形多段线指针，调用者负责管理生命周期
    DmPolyline* createRectangle(DmEntityContainer* container);

protected:
    struct Points;

    std::unique_ptr<Points> pPoints; ///< 矩形两个角点数据
};

#endif
