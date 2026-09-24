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

/// @file DimensionCommand.cpp
/// @brief DimensionCommand 与 DimensionTool 的实现

#include "DimensionCommand.h"

#include "BaseExclusiveCommand.h"
#include "DmDimension.h"
#include "DmDimensionStyleTable.h"
#include "DmDocument.h"
#include "GuiDialogFactory.h"
#include "IDocumentView.h"

void DimensionCommand::showOptions()
{
    GUIDIALOGFACTORY->requestCommandOptions(this, true, true);
}

void DimensionCommand::hideOptions()
{
    GUIDIALOGFACTORY->requestCommandOptions(this, false);
}

DimensionTool::DimensionTool(DimensionCommand& command, DmDocument* doc, IDocumentView* view)
    : BasePlaceTool(command, doc, view)
{
    resetDimension();
}

DimensionTool::~DimensionTool() = default;

void DimensionTool::resetDimension()
{
    data = std::make_unique<DmDimensionData>(DmVector(false), DmVector(false), EMTextVertMode::kTextVertMid,
                                             EMTextHorzMode::kTextCenter, 1.0, "", 0.0,
                                             document()->getDimStyleTable()->getActive());
    diameter = false;
}

void DimensionTool::init(int s)
{
    if (s < 0)
    {
        command().finish();
        return;
    }
    restart(s);
}

void DimensionTool::setText(const QString& t)
{
    data->text = t;
}
