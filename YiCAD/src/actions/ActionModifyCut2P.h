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


/// @file ActionModifyCut2P.h
/// @brief 两点裁剪 Action 类，处理用户交互以通过两点裁剪实体（直线、圆弧、圆、椭圆或多段线）

#ifndef ACTIONMODIFYCUT2P_H
#define ACTIONMODIFYCUT2P_H

#include "PreviewActionInterface.h"

/// @brief 两点裁剪 Action，通过两个裁剪点对实体进行裁剪操作
class ActionModifyCut2P : public PreviewActionInterface
{
    Q_OBJECT
public:
    /// @brief Action 状态枚举
    enum Status
    {
        ChooseCutEntity,  ///< 选择待裁剪实体
        SetCutCoord       ///< 设置裁剪坐标
    };

    /// @brief 构造函数
    /// @param doc 文档指针
    /// @param docView 文档视图指针
    ActionModifyCut2P(DmDocument* doc, GuiDocumentView* docView);

    /// @brief 析构函数
    ~ActionModifyCut2P() override;

    /// @brief 初始化 Action 状态
    /// @param status 初始状态
    void init(int status = 0) override;

    /// @brief 执行裁剪操作
    void trigger() override;

    /// @brief 鼠标移动事件处理
    /// @param e 鼠标事件指针
    void mouseMoveEvent(QMouseEvent* e) override;

    /// @brief 鼠标释放事件处理
    /// @param e 鼠标事件指针
    void mouseReleaseEvent(QMouseEvent* e) override;

    /// @brief 更新鼠标光标样式
    void updateMouseCursor() override;

private:
    DmEntity* cutEntity;                     ///< 待裁剪的实体指针
    std::unique_ptr<DmVector> firstCoord;    ///< 第一个裁剪点坐标
    std::unique_ptr<DmVector> secondCoord;   ///< 第二个裁剪点坐标
};

#endif // ACTIONMODIFYCUT2P_H
