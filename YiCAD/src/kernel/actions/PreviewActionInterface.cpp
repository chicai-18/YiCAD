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

/// @file PreviewActionInterface.cpp
/// @brief 预览操作接口的实现

#include "PreviewActionInterface.h"

#include "GuiDocumentView.h"
#include "DmDocument.h"
#include "Preview.h"
#include "Debug.h"

/// @brief 设置继承此接口的操作类所操作的实体容器
/// @param name 操作名称
/// @param doc 文档指针
/// @param docView 文档视图指针
PreviewActionInterface::PreviewActionInterface(const char* name, DmDocument* doc, GuiDocumentView* docView)
    : ActionInterface(name, doc, docView)
    , preview(new Preview(doc))
    , pDocument(doc)
{
    hasPreview = true;
}

PreviewActionInterface::~PreviewActionInterface()
{
    deletePreview();
}

void PreviewActionInterface::init(int status)
{
    deletePreview();
    ActionInterface::init(status);
}

void PreviewActionInterface::finish(bool updateTB)
{
    deletePreview();
    ActionInterface::finish(updateTB);
}

void PreviewActionInterface::suspend()
{
    ActionInterface::suspend();
    deletePreview();
}

void PreviewActionInterface::resume()
{
    ActionInterface::resume();
    drawPreview();
}

void PreviewActionInterface::trigger()
{
    ActionInterface::trigger();
    deletePreview();
}

bool PreviewActionInterface::isViewAction()
{
    return false;
}

void PreviewActionInterface::deletePreview()
{
    if (hasPreview)
    {
        // 避免删除空预览
        preview->clear();
        hasPreview = false;
    }

    if (!docView->isCleanUp())
    {
        //docView->getOverlayContainer(DM::ActionPreviewEntity)->clear();
        docView->disableOverlayBox();
    }
}

void PreviewActionInterface::drawPreview()
{
    docView->redraw();
    hasPreview = true;
}
