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


/// @file ActionOptionsDrawing.cpp
/// @brief 绘图选项交互命令实现

#include "ActionOptionsDrawing.h"

#include <QAction>

#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"

ActionOptionsDrawing::ActionOptionsDrawing(DmDocument* doc, GuiDocumentView* docView) :
    ActionInterface("Drawing Options", doc, docView)
{
    actionType = DM::ActionOptionsDrawing;
}

void ActionOptionsDrawing::init(int status)
{
    ActionInterface::init(status);

    trigger();
}

/// @brief 触发绘图选项对话框显示
///
/// 打开绘图选项对话框，并重置坐标显示控件。
void ActionOptionsDrawing::trigger()
{
    if (pDocument)
    {
        GUIDIALOGFACTORY->requestOptionsDrawingDialog(*pDocument);
        GUIDIALOGFACTORY->updateCoordinateWidget(DmVector(0.0, 0.0), DmVector(0.0, 0.0), true);
        docView->redraw();
    }

    finish(false);
}
