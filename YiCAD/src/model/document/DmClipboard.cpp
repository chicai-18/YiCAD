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

#include "DmClipboard.h"

#include "DmDocumentTransfer.h"
#include "DmEntity.h"
#include "EntityTable.h"

DmClipboard* DmClipboard::uniqueInstance = nullptr;

DmClipboard::DmClipboard()
    : m_pDocument(std::make_unique<DmDocument>())
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
    // 换新文档而不是逐表清空：新文档的表只有默认条目，上次复制进来的同名样式、块不会留下来被下次复制取到
    m_pDocument = std::make_unique<DmDocument>();
}

void DmClipboard::addEntity(DmEntity* e)
{
    if (e)
    {
        DmDocumentTransfer transfer(*m_pDocument, DmDocumentTransfer::Missing::AddDirect);
        e->transferTo(transfer);
        m_pDocument->getEntityTable()->add_direct(e);
        m_pDocument->getEntityTable()->updateContainer();
    }
}

unsigned DmClipboard::count()
{
    return m_pDocument->getEntityTable()->count();
}

DmDocument* DmClipboard::getDocument()
{
    return m_pDocument.get();
}
