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


/// @file ActionModifyExplode.h
/// @brief 分解实体（块、多段线、多行文字等）的交互动作类声明

#ifndef ACTIONMODIFYEXPLODE_H
#define ACTIONMODIFYEXPLODE_H

#include "PreviewActionInterface.h"

class DmMText;

/// @brief 分解实体的交互动作
///
/// 处理用户分解块(Block)、多段线(Polyline)、多行文字(MText)等复合实体为
/// 单个独立实体的操作。
class ActionModifyExplode : public PreviewActionInterface
{
    Q_OBJECT
public:
    /// @brief 构造函数
    /// @param [in] doc 文档指针
    /// @param [in] docView 文档视图指针
    ActionModifyExplode(DmDocument* doc, GuiDocumentView* docView);

    /// @brief 初始化动作
    /// @param [in] status 初始状态，默认为0
    void init(int status = 0) override;

    /// @brief 触发分解操作
    void trigger() override;

    /// @brief 执行分解操作
    /// @param [in] remove 是否在分解后删除原实体，默认为true
    /// @return 分解成功返回true，否则返回false
    bool explode(const bool remove = true);

    /// @brief 将多行文字分解为单行文字
    /// @param [in] text 多行文字实体指针
    /// @param [out] addList 分解后产生的单行文字实体列表
    /// @return 分解成功返回true，否则返回false
    bool explodeMTextIntoLetters(DmMText* text, std::vector<DmEntity*>& addList);
};

#endif // ACTIONMODIFYEXPLODE_H
