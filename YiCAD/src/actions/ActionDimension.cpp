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


/// @file ActionDimension.cpp
/// @brief 标注操作基类实现

#include "ActionDimAligned.h"

#include "DmDimension.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "DmDocument.h"

ActionDimension::ActionDimension(const char* name, DmDocument* doc,
                                 GuiDocumentView* docView) :
    PreviewActionInterface(name, doc, docView)
{
    reset();
}

ActionDimension::~ActionDimension() = default;

void ActionDimension::reset()
{
    data.reset(new DmDimensionData(DmVector(false), DmVector(false),
                EMTextVertMode::kTextVertMid, EMTextHorzMode::kTextCenter,
                1.0, "", 0.0,
                docView->getDocument()->getDimStyleTable()->getActive()));
    diameter = false;
}

void ActionDimension::init(int status)
{
    PreviewActionInterface::init(status);
}

void ActionDimension::hideOptions()
{
    ActionInterface::hideOptions();

    GUIDIALOGFACTORY->requestOptions(this, false);
}

void ActionDimension::showOptions()
{
    ActionInterface::showOptions();

    GUIDIALOGFACTORY->requestOptions(this, true, true);
}

void ActionDimension::updateMouseCursor()
{
    docView->setMouseCursor(DM::SelectCursor);
}

bool ActionDimension::isDimensionAction(DM::ActionType type)
{
    switch (type)
    {
        case DM::ActionDimAligned:
        case DM::ActionDimLinear:
        case DM::ActionDimAngular:
        case DM::ActionDimDiametric:
        case DM::ActionDimRadial:
            return true;
        default:
            return false;
    }
}

void ActionDimension::setText(const QString& t)
{
    data->text = t;
}

const QString& ActionDimension::getLabel() const
{
    return label;
}

void ActionDimension::setLabel(const QString& t)
{
    label = t;
}

const QString& ActionDimension::getTol1() const
{
    return tol1;
}

void ActionDimension::setTol1(const QString& t)
{
    tol1 = t;
}

const QString& ActionDimension::getTol2() const
{
    return tol2;
}

void ActionDimension::setTol2(const QString& t)
{
    tol2 = t;
}

bool ActionDimension::getDiameter() const
{
    return diameter;
}

void ActionDimension::setDiameter(bool d)
{
    diameter = d;
}

