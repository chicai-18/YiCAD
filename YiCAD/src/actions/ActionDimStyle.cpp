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


/// @file ActionDimStyle.cpp
/// @brief 标注样式管理操作类实现

#include "ActionDimStyle.h"
#include "ApplicationWindow.h"
#include "GuiDialogFactory.h"
#include "DmDocument.h"

ActionDimStyle::ActionDimStyle(DmDocument* doc, GuiDocumentView* docView) :
    ActionInterface("Dimension Style", doc, docView)
{
}

void ActionDimStyle::init(int status)
{
    ActionInterface::init(status);
    trigger();
}

void ActionDimStyle::trigger()
{
    DmDocument* pDocument =
                ApplicationWindow::getAppWindow()->getDocument();
    GUIDIALOGFACTORY->requestDimStyleMgrDialog(
                pDocument->getDimStyleTable(), pDocument);
    finish(false);
}

