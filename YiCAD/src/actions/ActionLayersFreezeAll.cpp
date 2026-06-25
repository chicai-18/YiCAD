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


/// @file ActionLayersFreezeAll.cpp
/// @brief 冻结/解冻所有图层交互动作的实现

#include "ActionLayersFreezeAll.h"

#include <QAction>

#include "ApplicationWindow.h"
#include "CustomComboboxItem.h"
#include "Debug.h"
#include "DmDocument.h"
#include "GuiDocumentView.h"
#include "Transaction.h"

ActionLayersFreezeAll::ActionLayersFreezeAll(const bool freeze, DmDocument* doc, GuiDocumentView* docView) :
    ActionInterface("Freeze all Layers", doc, docView)
    , freeze(freeze)
{
}

void ActionLayersFreezeAll::init(int status)
{
    ActionInterface::init(status);
    trigger();
}

void ActionLayersFreezeAll::trigger()
{
    if (pDocument)
    {
        Transaction t(tr("Freeze Layer").toStdString(), pDocument);
        t.start();
        auto table = pDocument->getLayerTable();
        for (auto it = table->begin(); it != table->end(); ++it)
        {
            table->startModify(*it);
            (*it)->freeze(freeze);
        }
        t.commit();
    }
    finish(false);
}

// EOF
