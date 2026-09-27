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

/// @file CommandPreview.h
/// @brief 命令的预览：原 PreviewActionInterface 里预览的那一半
///
/// 命令持有预览（doc/COMMAND_TOOL_MIGRATION_PLAN.md 第 5 节"命令与工具"），
/// 放置工具在鼠标离开、回到画布时经它清除、重绘。清除与重绘的语义与原
/// PreviewActionInterface::deletePreview()/drawPreview() 一致，包括"刚构造时
/// 视为有预览"：第一次清除会清空视图共用的预览容器。

#ifndef COMMANDPREVIEW_H
#define COMMANDPREVIEW_H

#include "Preview.h"

class IDocumentView;
class SelectionSet;

/// @brief 命令的预览
class CommandPreview
{
public:
    /// @param selection 文档的选择集，预览选择集时从它取
    /// @param view 视图，预览画在它的预览容器里
    CommandPreview(SelectionSet* selection, IDocumentView* view);
    ~CommandPreview();

    CommandPreview(const CommandPreview&) = delete;
    CommandPreview& operator=(const CommandPreview&) = delete;

    /// @brief 预览容器，往里放实体后调用 draw()
    Preview& entities() { return m_preview; }

    /// @brief 清除预览与选择框（原 PreviewActionInterface::deletePreview）
    void clear();
    /// @brief 重绘并记下"有预览"（原 PreviewActionInterface::drawPreview）
    void draw();

private:
    IDocumentView* m_view = nullptr;
    Preview m_preview;
    bool m_hasPreview = true;
};

#endif // COMMANDPREVIEW_H
