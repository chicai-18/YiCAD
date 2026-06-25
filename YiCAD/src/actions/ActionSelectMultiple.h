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


/// @file ActionSelectMultiple.h
/// @brief 支持单选与框选的选择Action，支持Shift组合键切换选择/取消选择模式

#ifndef ACTIONSELECTMULTIPLE_H
#define ACTIONSELECTMULTIPLE_H

#include <memory>

#include "PreviewActionInterface.h"

/// @brief 支持单选与框选的选择Action（主要参考ActionDefault）
class ActionSelectMultiple : public PreviewActionInterface
{
    Q_OBJECT

public:
    /// @brief 操作状态枚举
    enum Status
    {
        Neutral,    ///< 初始状态，等待用户操作
        Dragging,   ///< 正在拖拽（拖拽实体或选择窗口第一角）
        SetCorner2, ///< 设置选择窗口的第二角
    };

    /// @brief 选择窗口的两个角点
    struct Points
    {
        DmVector v1;    ///< 第一角点坐标
        DmVector v2;    ///< 第二角点坐标
    };

public:
    /// @brief 构造函数
    /// @param [in] doc 文档指针
    /// @param [in] docView 文档视图指针
    /// @param [in] actionSelect 调用此选择动作的父Action指针
    /// @param [in] entityTypeList 允许选择的实体类型列表，为空则允许所有类型
    ActionSelectMultiple(DmDocument* doc, GuiDocumentView* docView,
                         ActionInterface* actionSelect = nullptr,
                         std::list<DM::EntityType> const& entityTypeList =
                             std::list<DM::EntityType>{});

    /// @brief 初始化操作状态
    /// @param [in] status 初始状态值，默认为0
    void init(int status = 0) override;

    /// @brief 执行选择操作（窗口选择或结束操作）
    void trigger() override;

    /// @brief 处理键盘按键事件
    /// @param [in] e 键盘事件指针
    void keyPressEvent(QKeyEvent* e) override;

    /// @brief 处理鼠标移动事件
    /// @param [in] e 鼠标事件指针
    void mouseMoveEvent(QMouseEvent* e) override;

    /// @brief 处理鼠标按下事件
    /// @param [in] e 鼠标事件指针
    void mousePressEvent(QMouseEvent* e) override;

    /// @brief 处理鼠标释放事件
    /// @param [in] e 鼠标事件指针
    void mouseReleaseEvent(QMouseEvent* e) override;

    /// @brief 更新鼠标按钮提示文本
    void updateMouseButtonHints() override;

    /// @brief 更新鼠标光标样式
    void updateMouseCursor() override;

private:
    std::list<DM::EntityType> const entityTypeList;     ///< 允许选择的实体类型列表
    ActionInterface* actionSelect = nullptr;            ///< 调用此Action的父Action指针
    bool select = true;                                 ///< 选择模式（true为选择，false为取消选择）
    std::unique_ptr<Points> pPoints;                    ///< 选择窗口角点数据
};

#endif // ACTIONSELECTMULTIPLE_H
