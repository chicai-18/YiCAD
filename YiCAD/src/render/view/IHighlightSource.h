/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/// @file IHighlightSource.h
/// @brief 绘制时取要高亮的实体的只读接口
///
/// 高亮归 Application 层的高亮集（HighlightSet，每个视图一个），Render 只经本接口读取，不认识它
/// （doc/HIGHLIGHT_SET_PLAN.md 3.2 节）。画布把它交给文档画笔，见
/// GuiDocumentView::setDocumentHighlightSource()；预览画笔不设来源，预览不涉及高亮。
///
/// 与 ISelectionSource 的逐实体查询不同，这里交出列表（D3）：高亮通常只有一到几个实体，
/// 画笔据此组高亮组，不必对每个顶层实体查一次。

#ifndef IHIGHLIGHTSOURCE_H
#define IHIGHLIGHTSOURCE_H

#include <vector>

class DmEntity;

/// @brief 绘制时取要高亮的实体，见文件说明
class IHighlightSource
{
public:
    virtual ~IHighlightSource() = default;

    /// @brief 要按高亮绘制的顶层实体：只含当前实体表里可见、未删除的
    virtual std::vector<DmEntity*> highlightedEntities() const = 0;
};

#endif  // IHIGHLIGHTSOURCE_H
