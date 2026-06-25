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


/// @file ActionDefault.h
/// @brief 默认动作类头文件，处理实体选择和基本交互

#ifndef ACTION_DEFAULT_H
#define ACTION_DEFAULT_H

#include "PreviewActionInterface.h"

/// @brief 处理默认用户交互事件（选择、拖拽、平移等）
class ActionDefault : public PreviewActionInterface
{
    Q_OBJECT
public:
    /// @brief 动作状态枚举
    enum Status
    {
        Neutral,    /**< 初始状态 */
        Dragging,   /**< 拖拽中（实体或选择窗口） */
        SetCorner2, /**< 设置选择窗口的第二个角点 */
        Moving,     /**< 移动实体 */
        MovingRef,  /**< 移动选中实体的参考点 */
        Panning     /**< 视图平移（Ctrl+鼠标拖拽） */
    };

public:
    /// @brief 构造函数
    /// @param[in] doc 文档指针
    /// @param[in] docView 文档视图指针
    ActionDefault(DmDocument* doc, GuiDocumentView* docView);

    /// @brief 析构函数
    ~ActionDefault() override;

    /// @brief 完成动作（空实现）
    void finish(bool /*updateTB*/ = true) override
    {
    }

    /// @brief 初始化动作
    /// @param[in] status 初始状态
    void init(int status = 0) override;

    /// @brief 键盘按下事件处理
    /// @param[in] e 键盘事件指针
    void keyPressEvent(QKeyEvent* e) override;

    /// @brief 键盘释放事件处理
    /// @param[in] e 键盘事件指针
    void keyReleaseEvent(QKeyEvent* e) override;

    /// @brief 鼠标移动事件处理
    /// @param[in] e 鼠标事件指针
    void mouseMoveEvent(QMouseEvent* e) override;

    /// @brief 鼠标按下事件处理
    /// @param[in] e 鼠标事件指针
    void mousePressEvent(QMouseEvent* e) override;

    /// @brief 鼠标释放事件处理
    /// @param[in] e 鼠标事件指针
    void mouseReleaseEvent(QMouseEvent* e) override;

    /// @brief 鼠标双击事件处理
    /// @param[in] e 鼠标事件指针
    void mouseDoubleClickEvent(QMouseEvent* e) override;

    /// @brief 获取可用命令列表
    /// @return 可用命令字符串列表
    QStringList getAvailableCommands() override;

    /// @brief 更新鼠标按钮提示
    void updateMouseButtonHints() override;

    /// @brief 更新鼠标光标
    void updateMouseCursor() override;

protected:
    struct Points;                          /**< 内部点数据结构 */
    std::unique_ptr<Points> pPoints;        /**< 点数据 */
    DM::SnapRestriction restrBak;           /**< 捕获限制备份 */
};

#endif
