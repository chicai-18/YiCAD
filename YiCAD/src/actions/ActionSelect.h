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


/// @file ActionSelect.h
/// @brief 实体选择操作，作为选择动作的入口，将实际选择委托给子Action（单选或框选）

#ifndef ACTIONSELECT_H
#define ACTIONSELECT_H

#include <set>

#include "ActionInterface.h"
#include "UIActionHandler.h"

/// @brief 实体选择操作，作为选择动作的入口，将实际选择委托给子Action（单选或框选）
class ActionSelect : public ActionInterface
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] a_handler UI动作处理器指针
    /// @param [in] doc 文档指针
    /// @param [in] docView 文档视图指针
    /// @param [in] nextAction 选择完成后要执行的后续动作类型
    /// @param [in] entityTypeList 允许选择的实体类型列表，为空则允许所有类型
    ActionSelect(
        UIActionHandler* a_handler, DmDocument* doc, GuiDocumentView* docView,
        DM::ActionType nextAction,
        std::list<DM::EntityType> const& entityTypeList = std::list<DM::EntityType>());

    /// @brief 初始化操作状态
    /// @param [in] status 初始状态值
    void init(int status) override;

    /// @brief 恢复操作
    void resume() override;

    /// @brief 处理鼠标释放事件
    /// @param [in] e 鼠标事件指针
    void mouseReleaseEvent(QMouseEvent* e) override;

    /// @brief 更新鼠标按钮提示文本
    void updateMouseButtonHints() override;

    /// @brief 更新鼠标光标样式
    void updateMouseCursor() override;

    /// @brief 获取已选中的实体数量
    /// @return 已选中的实体数量；若为0则显示提示消息
    int countSelected();

    /// @brief 处理键盘按键事件
    /// @param [in] e 键盘事件指针
    void keyPressEvent(QKeyEvent* e) override;

private:
    std::list<DM::EntityType> const entityTypeList;     ///< 允许选择的实体类型列表
    DM::ActionType nextAction;                          ///< 选择完成后要执行的动作类型
    UIActionHandler* action_handler;                    ///< UI动作处理器指针
};

#endif
