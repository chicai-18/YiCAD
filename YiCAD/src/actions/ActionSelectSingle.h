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


/// @file ActionSelectSingle.h
/// @brief 单选实体Action，处理用户事件以选择单个实体

#ifndef ACTIONSELECTSINGLE_H
#define ACTIONSELECTSINGLE_H

#include "ActionInterface.h"

/// @brief 单选实体Action类，处理用户事件以选择单个实体
class ActionSelectSingle : public ActionInterface
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] doc 文档指针
    /// @param [in] docView 文档视图指针
    /// @param [in] actionSelect 调用此选择动作的父Action指针
    /// @param [in] entityTypeList 允许选择的实体类型列表，为空则允许所有类型
    ActionSelectSingle(DmDocument* doc, GuiDocumentView* docView,
                       ActionInterface* actionSelect = nullptr,
                       std::list<DM::EntityType> const& entityTypeList =
                           std::list<DM::EntityType>{});

    /// @brief 执行选择操作
    void trigger() override;

    /// @brief 初始化选择操作
    void init(int status = 0) override;

    /// @brief 处理键盘按键事件
    /// @param [in] e 键盘事件指针
    void keyPressEvent(QKeyEvent* e) override;

    /// @brief 处理鼠标释放事件
    /// @param [in] e 鼠标事件指针
    void mouseReleaseEvent(QMouseEvent* e) override;

    /// @brief 更新鼠标光标样式
    void updateMouseCursor() override;

private:
    std::list<DM::EntityType> const entityTypeList; ///< 允许选择的实体类型列表

    DmEntity* en = nullptr;                ///< 当前捕获的实体指针
    ActionInterface* actionSelect = nullptr; ///< 调用此Action的父Action指针
};

#endif // ACTIONSELECTSINGLE_H
