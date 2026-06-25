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

/// @file Preview.cpp
/// @brief 预览功能类的实现

#include "Preview.h"

#include "DmEntityContainer.h"
#include "DmLine.h"
#include "GuiDocumentView.h"
#include "Information.h"
#include "DmSettings.h"
#include "DmDocument.h"

Preview::Preview(DmDocument* pDocument)
    : m_pDocument(pDocument)
    , m_pPreviewContainer(nullptr)
{
    if (m_pDocument)
    {
        auto docView = m_pDocument->getDocumentView();
        m_pPreviewContainer = docView->getPreviewContainer();
    }
}

DM::EntityType Preview::getEntityType() const
{
    return DM::EntityPreview;
}

void Preview::addCloneOf(DmEntity* entity)
{
    if (!entity)
    {
        return;
    }

    DmEntity* clone = entity->clone();
    clone->setParent(nullptr);
    addEntity(clone);
}

void Preview::addAllFrom(DmEntityContainer& container)
{
    int c = 0;
    for (auto e : container)
    {
        DmEntity* clone = e->clone();
        clone->setSelected(false);
        clone->setParent(nullptr);

        c++;
        addEntity(clone);
    }
}

void Preview::addSelectionFromDocument()
{
    if (!m_pDocument)
    {
        return;
    }
    auto table = m_pDocument->getEntityTable();
    for (auto e : *table)
    {
        if (e->isSelected())
        {
            DmEntity* clone = e->clone();
            clone->setSelected(false);
            clone->setParent(nullptr);
            addEntity(clone);
        }
    }
}

void Preview::addEntity(DmEntity* pEntity)
{
    //pEntity->setDocument(m_pDocument);
    m_pPreviewContainer->addEntity(pEntity);
    specifyPreviewModified();
}

void Preview::appendEntity(DmEntity* pEntity)
{
    //pEntity->setDocument(m_pDocument);
    m_pPreviewContainer->appendEntity(pEntity);
    specifyPreviewModified();
}

void Preview::removeEntity(DmEntity* pEntity)
{
    m_pPreviewContainer->removeEntity(pEntity);
    specifyPreviewModified();
}

void Preview::clear()
{
    m_pPreviewContainer->clear();
    specifyPreviewModified();
}

void Preview::setModelOffset(const DmVector& offset)
{
    if (m_pDocument)
    {
        auto docView = m_pDocument->getDocumentView();
        docView->setPreviewModelOffset(offset);
    }
}

bool Preview::isEmpty()
{
    return m_pPreviewContainer->isEmpty();
}

void Preview::move(DmVector offset)
{
    m_pPreviewContainer->move(offset);
    specifyPreviewModified();
}

void Preview::moveRef(DmVector v1, DmVector v2)
{
    m_pPreviewContainer->moveRef(v1, v2);
    specifyPreviewModified();
}

void Preview::setVisible(bool isVisble)
{
    m_pPreviewContainer->setVisible(isVisble);
    specifyPreviewModified();
}

DmEntityContainer* Preview::getEntityContainer()
{
    return m_pPreviewContainer;
}

/// @brief 通知文档视图预览内容已修改，触发重绘
void Preview::specifyPreviewModified()
{
    if (m_pDocument)
    {
        auto docView = m_pDocument->getDocumentView();
        docView->specifyPreviewModified();
    }
}
