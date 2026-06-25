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


/// @file DmClipboard.cpp
/// @brief 剪贴板实现

#include <iostream>
#include "DmClipboard.h"
#include "DmBlock.h"
#include "DmLayer.h"
#include "DmEntity.h"

DmClipboard* DmClipboard::uniqueInstance = nullptr;

DmClipboard::DmClipboard()
{
}

DmClipboard* DmClipboard::instance()
{
    if (uniqueInstance == nullptr)
    {
        uniqueInstance = new DmClipboard();
    }
    return uniqueInstance;
}

void DmClipboard::clear()
{
    pDocument.getEntityTable()->clear_direct();

    auto layerTable = pDocument.getLayerTable();
    if (layerTable)
    {
        std::vector<DmLayer*> layers;
        for (auto it = layerTable->begin(); it != layerTable->end(); ++it)
        {
            layers.push_back(*it);
        }
        for (auto l : layers)
        {
            layerTable->remove_direct(l);
        }
    }
}

void DmClipboard::addBlock(DmBlock* b)
{
    if (b)
    {
        pDocument.getBlockTable()->add_direct(b);
    }
}

bool DmClipboard::hasBlock(const QString& name)
{
    return (pDocument.getBlockTable()->find(name) != nullptr);
}

int DmClipboard::countBlocks()
{
    return pDocument.getBlockTable()->count();
}

void DmClipboard::addLayer(DmLayer* l)
{
    if (l)
    {
        pDocument.getLayerTable()->add_direct(l);
    }
}

bool DmClipboard::hasLayer(const QString& name)
{
    return (pDocument.getLayerTable()->find(name) != nullptr);
}

void DmClipboard::addEntity(DmEntity* e)
{
    if (e)
    {
        pDocument.getEntityTable()->add_direct(e);
        pDocument.getEntityTable()->updateContainer();
    }
}

unsigned DmClipboard::count()
{
    return pDocument.getEntityTable()->count();
}

DmDocument* DmClipboard::getDocument()
{
    return &pDocument;
}
