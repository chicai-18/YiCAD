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


/// @file DmEntityContainer.cpp
/// @brief 实体容器类实现

#include <iostream>
#include <cmath>
#include <set>
#include <QObject>

#include "GuiDialogFactory.h"
#include "UIDialogFactory.h"
#include "DmEntityContainer.h"

#include "Debug.h"
#include "DmDimension.h"
#include "DmLayer.h"
#include "DmArc.h"
#include "DmEllipse.h"
#include "DmLine.h"
#include "DmBlockReference.h"
#include "DmSpline.h"
#include "DmSolid.h"
#include "DmTriangle.h"
#include "Information.h"
#include "GuiDocumentView.h"
#include "DmConstructionLine.h"
#include "DmSettings.h"
#include "DmEntity.h"
#include "DmDocument.h"

TYPESYSTEM_SOURCE(DmEntityContainer, DmEntity, 0);

/// @param owner True if we own and also delete the entities.
DmEntityContainer::DmEntityContainer(DmEntity* parent, bool owner)
    : DmEntity(parent)
{
    m_bAutoDelete = owner;
    m_subContainer = nullptr;
    m_iEntIdx = -1;
}

DmEntityContainer::~DmEntityContainer()
{
    if (m_bAutoDelete)
    {
        while (!entities.isEmpty())
        {
            delete entities.takeFirst();
        }
    }
    else
    {
        entities.clear();
    }
}

DmEntity* DmEntityContainer::clone() const
{
    DmEntityContainer* ec = new DmEntityContainer(*this);
    ec->setOwner(m_bAutoDelete);
    ec->detach();

    return ec;
}

DM::EntityType DmEntityContainer::getEntityType() const
{
    return DM::EntityContainer;
}

// Detaches shallow copies and creates deep copies of all subentities.
// This is called after cloning entity containers.
void DmEntityContainer::detach()
{
    QList<DmEntity*> tmp;
    bool autoDel = isOwner();
    setOwner(false);

    // make deep copies of all entities:
    for (auto e : entities)
    {
        if (!e->getFlag(DM::FlagTemp))
        {
            tmp.append(e->clone());
        }
    }

    // clear shared pointers:
    entities.clear();
    setOwner(autoDel);

    // point to new deep copies:
    for (auto e : tmp)
    {
        entities.append(e);
        e->setParent(this);
    }
}

bool DmEntityContainer::isContainer() const
{
    return true;
}

void DmEntityContainer::setVisible(bool v)
{
    DmEntity::setVisible(v);
    for (auto e : entities)
    {
        e->setVisible(v);
    }
}

double DmEntityContainer::getLength() const
{
    double ret = 0.0;

    for (auto e : entities)
    {
        if (e->isVisible())
        {
            double l = e->getLength();
            if (l < 0.0)
            {
                ret = -1.0;
                break;
            }
            else
            {
                ret += l;
            }
        }
    }

    return ret;
}

bool DmEntityContainer::setSelected(bool select)
{
    // This entity's select:
    if (DmEntity::setSelected(select))
    {
        // All sub-entity's select:
        for (auto e : entities)
        {
            if (e->isVisible())
            {
                e->setSelected(select);
            }
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool DmEntityContainer::toggleSelected()
{
    // Toggle this entity's select:
    if (DmEntity::toggleSelected())
    {
        return true;
    }
    else
    {
        return false;
    }
}

void DmEntityContainer::setHighlighted(bool highlight)
{
    DmEntity::setHighlighted(highlight);
    for (auto e : entities)
    {
        if (e)
        {
            if (e->isVisible())
            {
                e->setHighlighted(highlight);
            }
        }
    }
}

std::list<DmEntity*> DmEntityContainer::selectEntitiesInWindow(DmVector v1, DmVector v2, bool cross)
{
    std::list<DmEntity*> entitiesInWindow = std::list<DmEntity*>();
    bool included = false;
    for (auto e : entities)
    {
        included = false;
        if (e->isVisible())
        {
            if (e->isInWindow(v1, v2))
            {
                included = true;
            }
            else if (!cross)
            {
                DmEntityContainer l;
                l.addRectangle(v1, v2);
                DmVectorSolutions sol;

                if (e->isContainer())
                {
                    DmEntityContainer* ec = (DmEntityContainer*)e;
                    for (DmEntity* se = ec->firstEntity(DM::ResolveAll); se && included == false; se = ec->nextEntity(DM::ResolveAll))
                    {
                        if (se->getEntityType() == DM::EntityTriangle)
                        {
                            included = static_cast<DmTriangle*>(se)->isInCrossWindow(v1, v2);
                        }
                        else if (se->getEntityType() == DM::EntitySolid)
                        {
                            included = static_cast<DmSolid*>(se)->isInCrossWindow(v1, v2);
                        }
                        else
                        {
                            for (auto line : l)
                            {
                                sol = Information::getIntersection(se, line, true);
                                if (sol.hasValid())
                                {
                                    included = true;
                                    break;
                                }
                            }
                        }
                    }
                }
                else if (e->getEntityType() == DM::EntityTriangle)
                {
                    included = static_cast<DmTriangle*>(e)->isInCrossWindow(v1, v2);
                }
                else if (e->getEntityType() == DM::EntitySolid)
                {
                    included = static_cast<DmSolid*>(e)->isInCrossWindow(v1, v2);
                }
                else
                {
                    for (auto line : l)
                    {
                        sol = Information::getIntersection(e, line, true);
                        if (sol.hasValid())
                        {
                            included = true;
                            break;
                        }
                    }
                }
            }
        }

        if (included)
        {
            entitiesInWindow.emplace_back(e);
        }
    }

    return entitiesInWindow;
}

void DmEntityContainer::addEntity(DmEntity* entity)
{
    if (!entity)
    {
        return;
    }

    if (entity->getEntityType() == DM::EntityImage || entity->getEntityType() == DM::EntityHatch)
    {
        entities.prepend(entity);
    }
    else
    {
        entities.append(entity);
    }
    adjustBorders(entity);
}

void DmEntityContainer::appendEntity(DmEntity* entity)
{
    if (!entity)
    {
        return;
    }

    entities.append(entity);
    adjustBorders(entity);
}

void DmEntityContainer::prependEntity(DmEntity* entity)
{
    if (!entity)
    {
        return;
    }

    entities.prepend(entity);
    adjustBorders(entity);
}

void DmEntityContainer::insertEntity(int index, DmEntity* entity)
{
    if (!entity)
    {
        return;
    }

    entities.insert(index, entity);
    adjustBorders(entity);
}

// Removes an entity from this container and updates the borders of this entity-container.
bool DmEntityContainer::removeEntity(DmEntity* entity)
{
    bool ret = false;
    ret = entities.removeOne(entity);

    if (m_bAutoDelete && ret)
    {
        delete entity;
    }
    calculateBorders();

    return ret;
}

void DmEntityContainer::removeAt(int index)
{
    auto entId = entityAt(index)->getId();
    entities.removeAt(index);
}

// Erases all entities in this container and resets the borders..
void DmEntityContainer::clear()
{
    if (m_bAutoDelete)
    {
        while (!entities.isEmpty())
        {
            delete entities.takeFirst();
        }
    }
    else
    {
        entities.clear();
    }
    resetBorders();
}

bool DmEntityContainer::isEmpty() const
{
    return entities.size() == 0;
}

int DmEntityContainer::size() const
{
    return entities.size();
}

void DmEntityContainer::adjustBorders(DmEntity* entity)
{
    if (entity)
    {
        int containerSize = 0;
        if (entity->isContainer())
        {
            auto container = static_cast<DmEntityContainer*>(entity);
            containerSize = container->size();
        }
        if (!entity->isContainer() || containerSize > 0)
        {
            minV = DmVector::minimum(entity->getMin(), minV);
            maxV = DmVector::maximum(entity->getMax(), maxV);
        }
    }
}

void DmEntityContainer::adjustBorders()
{
    resetBorders();
    for (DmEntity* e : entities)
    {
        DmLayer* layer = e->getLayer();
        if (e->isVisible() && !(layer && layer->isFrozen()))
        {
            adjustBorders(e);
        }
    }
}

// Recalculates the borders of this entity container.
void DmEntityContainer::calculateBorders()
{
    resetBorders();
    for (DmEntity* e : entities)
    {
        DmLayer* layer = e->getLayer();
        if (e->isVisible() && !(layer && layer->isFrozen()))
        {
            e->calculateBorders();
            adjustBorders(e);
        }
    }

    // needed for correcting corrupt data (PLANS.dxf)
    if (minV.x > maxV.x || minV.x > DM_MAXDOUBLE || maxV.x > DM_MAXDOUBLE || minV.x < DM_MINDOUBLE || maxV.x < DM_MINDOUBLE)
    {
        minV.x = 0.0;
        maxV.x = 0.0;
    }
    if (minV.y > maxV.y || minV.y > DM_MAXDOUBLE || maxV.y > DM_MAXDOUBLE || minV.y < DM_MINDOUBLE || maxV.y < DM_MINDOUBLE)
    {
        minV.y = 0.0;
        maxV.y = 0.0;
    }
}

// Recalculates the borders of this entity container including invisible entities.
void DmEntityContainer::forcedCalculateBorders()
{
    resetBorders();
    for (DmEntity* e : entities)
    {
        if (e->isContainer())
        {
            ((DmEntityContainer*)e)->forcedCalculateBorders();
        }
        else
        {
            e->calculateBorders();
        }
        adjustBorders(e);
    }

    // needed for correcting corrupt data (PLANS.dxf)
    if (minV.x > maxV.x || minV.x > DM_MAXDOUBLE || maxV.x > DM_MAXDOUBLE || minV.x < DM_MINDOUBLE || maxV.x < DM_MINDOUBLE)
    {
        minV.x = 0.0;
        maxV.x = 0.0;
    }
    if (minV.y > maxV.y || minV.y > DM_MAXDOUBLE || maxV.y > DM_MAXDOUBLE || minV.y < DM_MINDOUBLE || maxV.y < DM_MINDOUBLE)
    {
        minV.y = 0.0;
        maxV.y = 0.0;
    }
}

// Updates the sub entities of this container.
void DmEntityContainer::update()
{
    for (DmEntity* e : entities)
    {
        e->update();
    }
    adjustBorders();
}

void DmEntityContainer::addRectangle(DmVector const& v0, DmVector const& v1)
{
    addEntity(new DmLine(this, v0, { v1.x, v0.y }));
    addEntity(new DmLine(this, { v1.x, v0.y }, v1));
    addEntity(new DmLine(this, v1, { v0.x, v1.y }));
    addEntity(new DmLine(this, { v0.x, v1.y }, v0));
}

/// Returns the first entity or nullptr if this document is empty.
/// @param level
DmEntity* DmEntityContainer::firstEntity(DM::ResolveLevel level) const
{
    DmEntity* e = nullptr;
    m_iEntIdx = -1;
    switch (level)
    {
    case DM::ResolveNone:
        if (!entities.isEmpty())
        {
            m_iEntIdx = 0;
            return entities.first();
        }
        break;

    case DM::ResolveAllButInserts:
    {
        m_subContainer = nullptr;
        if (!entities.isEmpty())
        {
            m_iEntIdx = 0;
            e = entities.first();
        }
        if (e && e->isContainer() && e->getEntityType() != DM::EntityBlockReference)
        {
            m_subContainer = (DmEntityContainer*)e;
            e = ((DmEntityContainer*)e)->firstEntity(level);
            // empty container:
            if (!e)
            {
                m_subContainer = nullptr;
                e = nextEntity(level);
            }
        }
        return e;
    }
    break;

    case DM::ResolveAllButTextImage:
    case DM::ResolveAllButTexts:
    {
        m_subContainer = nullptr;
        if (!entities.isEmpty())
        {
            m_iEntIdx = 0;
            e = entities.first();
        }
        if (e && e->isContainer() && e->getEntityType() != DM::EntityText && e->getEntityType() != DM::EntityMText)
        {
            m_subContainer = (DmEntityContainer*)e;
            e = ((DmEntityContainer*)e)->firstEntity(level);
            // empty container:
            if (!e)
            {
                m_subContainer = nullptr;
                e = nextEntity(level);
            }
        }
        return e;
    }
    break;

    case DM::ResolveAll:
    {
        m_subContainer = nullptr;
        if (!entities.isEmpty())
        {
            m_iEntIdx = 0;
            e = entities.first();
        }
        if (e && e->isContainer())
        {
            m_subContainer = (DmEntityContainer*)e;
            e = ((DmEntityContainer*)e)->firstEntity(level);
            // empty container:
            if (!e)
            {
                m_subContainer = nullptr;
                e = nextEntity(level);
            }
        }
        return e;
    }
    break;
    default:
        break;
    }

    return nullptr;
}

// Returns the last entity or \p nullptr if this document is empty.
DmEntity* DmEntityContainer::lastEntity(DM::ResolveLevel level) const
{
    DmEntity* e = nullptr;
    if (!entities.size())
    {
        return nullptr;
    }
    m_iEntIdx = entities.size() - 1;
    switch (level)
    {
    case DM::ResolveNone:
        if (!entities.isEmpty())
        {
            return entities.last();
        }
        break;
    case DM::ResolveAllButInserts:
    {
        if (!entities.isEmpty())
        {
            e = entities.last();
        }
        m_subContainer = nullptr;
        if (e && e->isContainer() && e->getEntityType() != DM::EntityBlockReference)
        {
            m_subContainer = (DmEntityContainer*)e;
            e = ((DmEntityContainer*)e)->lastEntity(level);
        }
        return e;
    }
    break;
    case DM::ResolveAllButTextImage:
    case DM::ResolveAllButTexts:
    {
        if (!entities.isEmpty())
        {
            e = entities.last();
        }
        m_subContainer = nullptr;
        if (e && e->isContainer() && e->getEntityType() != DM::EntityText && e->getEntityType() != DM::EntityMText)
        {
            m_subContainer = (DmEntityContainer*)e;
            e = ((DmEntityContainer*)e)->lastEntity(level);
        }
        return e;
    }
    break;
    case DM::ResolveAll:
    {
        if (!entities.isEmpty())
        {
            e = entities.last();
        }
        m_subContainer = nullptr;
        if (e && e->isContainer())
        {
            m_subContainer = (DmEntityContainer*)e;
            e = ((DmEntityContainer*)e)->lastEntity(level);
        }
        return e;
    }
    break;
    default:
        break;
    }

    return nullptr;
}

// Returns the next entity or container or \p nullptr if the last entity
// returned by \p next() was the last entity in the container.
DmEntity* DmEntityContainer::nextEntity(DM::ResolveLevel level) const
{
    // set m_iEntIdx pointing in next entity and check if is out of range
    ++m_iEntIdx;
    switch (level)
    {
    case DM::ResolveNone:
        if (m_iEntIdx < entities.size())
        {
            return entities.at(m_iEntIdx);
        }
        break;

    case DM::ResolveAllButInserts:
    {
        DmEntity* e = nullptr;
        if (m_subContainer)
        {
            e = m_subContainer->nextEntity(level);
            if (e)
            {
                --m_iEntIdx; //return a sub-entity, index not advanced
                return e;
            }
            else
            {
                if (m_iEntIdx < entities.size())
                {
                    e = entities.at(m_iEntIdx);
                }
            }
        }
        else
        {
            if (m_iEntIdx < entities.size())
            {
                e = entities.at(m_iEntIdx);
            }
        }
        if (e && e->isContainer() && e->getEntityType() != DM::EntityBlockReference)
        {
            m_subContainer = (DmEntityContainer*)e;
            e = ((DmEntityContainer*)e)->firstEntity(level);
            // empty container:
            if (!e)
            {
                m_subContainer = nullptr;
                e = nextEntity(level);
            }
        }
        return e;
    }
    break;
    case DM::ResolveAllButTextImage:
    case DM::ResolveAllButTexts:
    {
        DmEntity* e = nullptr;
        if (m_subContainer)
        {
            e = m_subContainer->nextEntity(level);
            if (e)
            {
                --m_iEntIdx; //return a sub-entity, index not advanced
                return e;
            }
            else
            {
                if (m_iEntIdx < entities.size())
                {
                    e = entities.at(m_iEntIdx);
                }
            }
        }
        else
        {
            if (m_iEntIdx < entities.size())
            {
                e = entities.at(m_iEntIdx);
            }
        }
        if (e && e->isContainer() && e->getEntityType() != DM::EntityText && e->getEntityType() != DM::EntityMText)
        {
            m_subContainer = (DmEntityContainer*)e;
            e = ((DmEntityContainer*)e)->firstEntity(level);
            // empty container:
            if (!e)
            {
                m_subContainer = nullptr;
                e = nextEntity(level);
            }
        }
        return e;
    }
    break;

    case DM::ResolveAll:
    {
        DmEntity* e = nullptr;
        if (m_subContainer)
        {
            e = m_subContainer->nextEntity(level);
            if (e)
            {
                --m_iEntIdx; //return a sub-entity, index not advanced
                return e;
            }
            else
            {
                if (m_iEntIdx < entities.size())
                {
                    e = entities.at(m_iEntIdx);
                }
            }
        }
        else
        {
            if (m_iEntIdx < entities.size())
            {
                e = entities.at(m_iEntIdx);
            }
        }
        if (e && e->isContainer())
        {
            m_subContainer = (DmEntityContainer*)e;
            e = ((DmEntityContainer*)e)->firstEntity(level);
            // empty container:
            if (!e)
            {
                m_subContainer = nullptr;
                e = nextEntity(level);
            }
        }
        return e;
    }
    break;
    default:
        break;
    }
    return nullptr;
}

// Returns the prev entity or container or \p nullptr if the last entity
// returned by \p prev() was the first entity in the container.
DmEntity* DmEntityContainer::prevEntity(DM::ResolveLevel level) const
{
    // set m_iEntIdx pointing in prev entity and check if is out of range
    --m_iEntIdx;
    switch (level)
    {
    case DM::ResolveNone:
        if (m_iEntIdx >= 0)
        {
            return entities.at(m_iEntIdx);
        }
        break;
    case DM::ResolveAllButInserts:
    {
        DmEntity* e = nullptr;
        if (m_subContainer)
        {
            e = m_subContainer->prevEntity(level);
            if (e)
            {
                return e;
            }
            else
            {
                if (m_iEntIdx >= 0)
                {
                    e = entities.at(m_iEntIdx);
                }
            }
        }
        else
        {
            if (m_iEntIdx >= 0)
            {
                e = entities.at(m_iEntIdx);
            }
        }
        if (e && e->isContainer() && e->getEntityType() != DM::EntityBlockReference)
        {
            m_subContainer = (DmEntityContainer*)e;
            e = ((DmEntityContainer*)e)->lastEntity(level);
            // empty container:
            if (!e)
            {
                m_subContainer = nullptr;
                e = prevEntity(level);
            }
        }
        return e;
    }
    break;
    case DM::ResolveAllButTextImage:
    case DM::ResolveAllButTexts:
    {
        DmEntity* e = nullptr;
        if (m_subContainer)
        {
            e = m_subContainer->prevEntity(level);
            if (e)
            {
                return e;
            }
            else
            {
                if (m_iEntIdx >= 0)
                {
                    e = entities.at(m_iEntIdx);
                }
            }
        }
        else
        {
            if (m_iEntIdx >= 0)
            {
                e = entities.at(m_iEntIdx);
            }
        }
        if (e && e->isContainer() && e->getEntityType() != DM::EntityText && e->getEntityType() != DM::EntityMText)
        {
            m_subContainer = (DmEntityContainer*)e;
            e = ((DmEntityContainer*)e)->lastEntity(level);
            // empty container:
            if (!e)
            {
                m_subContainer = nullptr;
                e = prevEntity(level);
            }
        }
        return e;
    }
    break;
    case DM::ResolveAll:
    {
        DmEntity* e = nullptr;
        if (m_subContainer)
        {
            e = m_subContainer->prevEntity(level);
            if (e)
            {
                ++m_iEntIdx; //return a sub-entity, index not advanced
                return e;
            }
            else
            {
                if (m_iEntIdx >= 0)
                {
                    e = entities.at(m_iEntIdx);
                }
            }
        }
        else
        {
            if (m_iEntIdx >= 0)
            {
                e = entities.at(m_iEntIdx);
            }
        }
        if (e && e->isContainer())
        {
            m_subContainer = (DmEntityContainer*)e;
            e = ((DmEntityContainer*)e)->lastEntity(level);
            // empty container:
            if (!e)
            {
                m_subContainer = nullptr;
                e = prevEntity(level);
            }
        }
        return e;
    }
    break;
    default:
        break;
    }
    return nullptr;
}

/// @return Entity at the given index or nullptr if the index is out of range.
DmEntity* DmEntityContainer::entityAt(int index)
{
    if (entities.size() > index && index >= 0)
    {
        return entities.at(index);
    }
    else
    {
        return nullptr;
    }
}

void DmEntityContainer::setEntityAt(int index, DmEntity* en)
{
    if (m_bAutoDelete && entities.at(index))
    {
        delete entities.at(index);
    }
    entities[index] = en;
}

// Finds the given entity and makes it the current entity if found.
int DmEntityContainer::findEntity(DmEntity const* const entity)
{
    m_iEntIdx = entities.indexOf(const_cast<DmEntity*>(entity));
    return m_iEntIdx;
}

DmVector DmEntityContainer::getNearestEndpoint(const DmVector& coord, double* dist)const
{
    double minDist = DM_MAXDOUBLE;  // minimum measured distance
    double curDist = 0.0;           // currently measured distance
    DmVector closestPoint(false);   // closest found endpoint
    DmVector point;                 // endpoint found

    for (DmEntity* en : entities)
    {
        if (en->isVisible())
        {
            point = en->getNearestEndpoint(coord, &curDist);
            if (point.valid && curDist < minDist)
            {
                closestPoint = point;
                minDist = curDist;
                if (dist)
                {
                    *dist = minDist;
                }
            }
        }
    }

    return closestPoint;
}

DmVector DmEntityContainer::getNearestPointOnEntity(const DmVector& coord, bool onEntity, double* dist, DmEntity** entity)const
{
    DmVector point(false);

    DmEntity* en = getNearestEntity(coord, dist, DM::ResolveNone);

    if (en && en->isVisible())
    {
        point = en->getNearestPointOnEntity(coord, onEntity, dist, entity);
        if (!point.valid)
        {
            // DmText/DmMText等实体的getNearestPointOnEntity返回无效点，
            // 回退到getDistanceToPoint以正确计算距离
            double curDist = en->getDistanceToPoint(coord);
            if (dist)
            {
                *dist = curDist;
            }
            point = coord;
        }
    }

    return point;
}

DmVector DmEntityContainer::getNearestCenter(const DmVector& coord, double* dist) const
{
    double minDist = DM_MAXDOUBLE;  // minimum measured distance
    double curDist = DM_MAXDOUBLE;  // currently measured distance
    DmVector closestPoint(false);  // closest found endpoint
    DmVector point;                // endpoint found

    for (auto en : entities)
    {
        if (en->isVisible())
        {
            point = en->getNearestCenter(coord, &curDist);
            if (point.valid && curDist < minDist)
            {
                closestPoint = point;
                minDist = curDist;
            }
        }
    }
    if (dist)
    {
        *dist = minDist;
    }

    return closestPoint;
}

/// @return the nearest of equidistant middle points of the line.
DmVector DmEntityContainer::getNearestMiddle(const DmVector& coord, double* dist, int middlePoints) const
{
    double minDist = DM_MAXDOUBLE;  // minimum measured distance
    double curDist = DM_MAXDOUBLE;  // currently measured distance
    DmVector closestPoint(false);  // closest found endpoint
    DmVector point;                // endpoint found

    for (auto en : entities)
    {
        if (en->isVisible())
        {
            point = en->getNearestMiddle(coord, &curDist, middlePoints);
            if (point.valid && curDist < minDist)
            {
                closestPoint = point;
                minDist = curDist;
            }
        }
    }
    if (dist)
    {
        *dist = minDist;
    }

    return closestPoint;
}

DmVector DmEntityContainer::getNearestRef(const DmVector& coord, double* dist) const
{
    double minDist = DM_MAXDOUBLE;  // minimum measured distance
    double curDist = 0.0;           // currently measured distance
    DmVector closestPoint(false);   // closest found endpoint
    DmVector point;                 // endpoint found

    for (auto en : entities)
    {
        if (en->isVisible())
        {
            point = en->getNearestRef(coord, &curDist);
            if (point.valid && curDist < minDist)
            {
                closestPoint = point;
                minDist = curDist;
                if (dist)
                {
                    *dist = minDist;
                }
            }
        }
    }
    return closestPoint;
}

double DmEntityContainer::getDistanceToPoint(const DmVector& coord, DmEntity** entity, DM::ResolveLevel level) const
{
    double minDist = DM_MAXDOUBLE;      // minimum measured distance
    double curDist = 0.0;               // currently measured distance
    DmEntity* closestEntity = nullptr;    // closest entity found
    DmEntity* subEntity = nullptr;

    for (auto e : entities)
    {
        if (e->isVisible())
        {
            if (level == DM::ResolveAllButTextImage && e->getEntityType() == DM::EntityImage)
            {
                continue;
            }
            curDist = e->getDistanceToPoint(coord, &subEntity, level);

            // By using '<=', we will prefer the *last* item in the container if there are multiple
            // entities that are *exactly* the same distance away, which should tend to be the one
            // drawn most recently, and the one most likely to be visible (as it is also the order
            // that the software draws the entities). This makes a difference when one entity is
            // drawn directly over top of another, and it's reasonable to assume that humans will
            // tend to want to reference entities that they see or have recently drawn as opposed
            // to deeper more forgotten and invisible ones...
            if (curDist <= minDist)
            {
                switch (level)
                {
                case DM::ResolveAll:
                case DM::ResolveAllButTextImage:
                    closestEntity = subEntity;
                    break;
                default:
                    closestEntity = e;
                    break;
                }
                minDist = curDist;
            }
        }
    }

    if (entity)
    {
        *entity = closestEntity;
    }
    return minDist;
}

DmEntity* DmEntityContainer::getNearestEntity(const DmVector& coord, double* dist, DM::ResolveLevel level) const
{
    DmEntity* e = nullptr;
    double d = getDistanceToPoint(coord, &e, level);
    if (dist)
    {
        *dist = d;
    }
    return e;
}

bool DmEntityContainer::hasEndpointsWithinWindow(const DmVector& v1, const DmVector& v2)
{
    for (auto e : entities)
    {
        if (e->hasEndpointsWithinWindow(v1, v2))
        {
            return true;
        }
    }
    return false;
}

void DmEntityContainer::move(const DmVector& offset)
{
    for (auto e : entities)
    {
        e->move(offset);
    }
    moveBorders(offset);
}

void DmEntityContainer::rotate(const DmVector& center, const DmVector& angleVector)
{
    for (auto e : entities)
    {
        e->rotate(center, angleVector);
    }
    adjustBorders();
}

void DmEntityContainer::scale(const DmVector& center, const DmVector& factor)
{
    if (fabs(factor.x) > DM_TOLERANCE && fabs(factor.y) > DM_TOLERANCE)
    {
        for (auto e : entities)
        {
            e->scale(center, factor);
        }
        adjustBorders();
    }
}

void DmEntityContainer::mirror(const DmVector& axisPoint1, const DmVector& axisPoint2)
{
    if (axisPoint1.distanceTo(axisPoint2) > DM_TOLERANCE)
    {
        for (auto e : entities)
        {
            e->mirror(axisPoint1, axisPoint2);
        }
        adjustBorders();
    }
}

void DmEntityContainer::moveRef(const DmVector& ref, const DmVector& offset)
{
    for (auto e : entities)
    {
        e->moveRef(ref, offset);
    }
    adjustBorders();
}

void DmEntityContainer::moveSelectedRef(const DmVector& ref, const DmVector& offset)
{
    for (auto e : entities)
    {
        e->moveSelectedRef(ref, offset);
    }
    adjustBorders();
}

bool DmEntityContainer::isOwner() const
{
    return m_bAutoDelete;
}

void DmEntityContainer::setOwner(bool owner)
{
    m_bAutoDelete = owner;
}

QList<DmEntity*>::const_iterator DmEntityContainer::begin() const
{
    return entities.begin();
}

QList<DmEntity*>::const_iterator DmEntityContainer::end() const
{
    return entities.end();
}

QList<DmEntity*>::iterator DmEntityContainer::begin()
{
    return entities.begin();
}

QList<DmEntity*>::iterator DmEntityContainer::end()
{
    return entities.end();
}

DmEntity* DmEntityContainer::first() const
{
    return entities.first();
}

DmEntity* DmEntityContainer::last() const
{
    return entities.last();
}

const QList<DmEntity*>& DmEntityContainer::getEntityList()
{
    return entities;
}

std::list<DmEntity*> DmEntityContainer::getSubEntities() const
{
    std::list<DmEntity*> subEnts;
    for (auto e : entities)
    {
        if (e)
        {
            if (e->isContainer())
            {
                subEnts.splice(subEnts.end(), e->getSubEntities());
            }
            else
            {
                subEnts.emplace_back(std::move(e));
            }
        }
    }
    return subEnts;
}
