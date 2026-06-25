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


/// @file ActionBlocksSaveAs.h
/// @brief 块另存为动作类头文件

#ifndef ACTIONBLOCKSSAVEAS_H
#define ACTIONBLOCKSSAVEAS_H

#include "ActionInterface.h"

class UIBlockSaveAs;

/// @brief 处理将当前激活块另存为文件的用户事件
class ActionBlocksSaveAs : public ActionInterface
{
    Q_OBJECT
public:
    /// @brief 构造函数
    /// @param[in] doc 文档指针
    /// @param[in] docView 文档视图指针
    ActionBlocksSaveAs(DmDocument* doc, GuiDocumentView* docView);

    /// @brief 析构函数
    ~ActionBlocksSaveAs() override;

    /// @brief 判断是否为独占动作
    /// @return true 表示独占
    bool isExclusive() override;

    /// @brief 初始化动作
    /// @param[in] status 初始状态
    void init(int status = 0) override;

    /// @brief 触发动作执行
    void trigger() override;

private:
    UIBlockSaveAs* m_pBlockSaveAs; /**< 块另存为对话框指针 */
};

#endif
