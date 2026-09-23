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
/// @brief 默认动作类头文件：GuiEventHandler 的兜底 Action
///
/// 真正的点选/框选/交叉选/拖拽实体与夹点逻辑已抽到 SelectTool（阶段2
/// 第5.4节第3项），本类现在是一个薄适配器，理由见 SelectTool.h 顶部
/// 说明与 doc/ARCHITECTURE_EVOLUTION_PLAN.md 阶段2 5.7 节。

#ifndef ACTION_DEFAULT_H
#define ACTION_DEFAULT_H

#include <memory>

#include "ActionInterface.h"

class Preview;
class SelectTool;

/// @brief 处理默认用户交互事件（选择、拖拽等），转发给内部的 SelectTool
class ActionDefault : public ActionInterface
{
    Q_OBJECT
public:
    /// @brief 构造函数
    /// @param[in] doc 文档指针
    /// @param[in] docView 文档视图指针
    ActionDefault(DmDocument* doc, IDocumentView* docView);

    /// @brief 析构函数
    ~ActionDefault() override;

    /// @brief 完成动作（空实现）：默认Action不允许被结束
    void finish(bool /*updateTB*/ = true) override
    {
    }

    /// @brief 初始化动作
    /// @param[in] status 初始状态
    void init(int status = 0) override;

    /// @brief 挂起：委托给 SelectTool 释放预览
    void suspend() override;
    /// @brief 恢复：委托给 SelectTool 重绘预览
    void resume() override;

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

private:
    std::unique_ptr<Preview> m_preview;        ///< 与 SelectTool 共享的预览容器
    std::unique_ptr<SelectTool> m_selectTool;  ///< 真正的选择/拖拽状态机
};

#endif
