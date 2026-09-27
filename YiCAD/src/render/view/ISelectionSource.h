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

/// @file ISelectionSource.h
/// @brief 绘制时判断实体是否选中的只读接口
///
/// 选中状态归 Application 层的选择集（SelectionSet），Render 只经本接口读取，不认识它
/// （doc/SELECTION_SET_PLAN.md 3.2 节）。画布把它交给文档画笔，见
/// GuiDocumentView::setDocumentSelectionSource()；预览画笔不设来源，预览里的临时实体没有选中状态。

#ifndef ISELECTIONSOURCE_H
#define ISELECTIONSOURCE_H

class DmEntity;

/// @brief 绘制时判断实体是否选中，见文件说明
class ISelectionSource
{
public:
    virtual ~ISelectionSource() = default;

    /// @brief 实体是否按选中绘制
    /// @param entity 画笔要绘制的顶层实体
    virtual bool isSelected(const DmEntity& entity) const = 0;
};

#endif  // ISELECTIONSOURCE_H
