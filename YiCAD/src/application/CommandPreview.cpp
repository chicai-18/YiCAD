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

/// @file CommandPreview.cpp
/// @brief CommandPreview 的实现

#include "CommandPreview.h"

#include "IDocumentView.h"

CommandPreview::CommandPreview(DmDocument* doc, IDocumentView* view)
    : m_view(view)
    , m_preview(doc, view)
{
}

CommandPreview::~CommandPreview()
{
    clear();
}

void CommandPreview::clear()
{
    if (m_hasPreview)
    {
        m_preview.clear();
        m_hasPreview = false;
    }
    if (!m_view->isCleanUp())
    {
        m_view->disableOverlayBox();
    }
}

void CommandPreview::draw()
{
    m_view->redraw();
    m_hasPreview = true;
}
