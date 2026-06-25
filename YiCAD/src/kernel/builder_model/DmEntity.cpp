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


/// @file DmEntity.cpp
/// @brief 实体基类实现

#include "DmEntity.h"

#include <QPolygon>
#include <QString>
#include <iostream>
#include <utility>
#include "DmBlock.h"
#include "DmCircle.h"
#include "DmDocument.h"
#include "DmBlockReference.h"
#include "DmLayer.h"
#include "DmLine.h"
#include "DmVector.h"
#include "GuiDocumentView.h"
#include "Information.h"
#include "Quadratic.h"

TYPESYSTEM_SOURCE_ABSTRACT(DmEntity, DmObject, 0)

DmEntity::DmEntity(DmEntity* parent)
{
    this->parent = parent;
    init();
}

void DmEntity::init()
{
    resetBorders();

    setFlag(DM::FlagVisible);
    updateEnabled = true;
}

DM::EntityType DmEntity::getEntityType() const
{
    return DM::EntityUnknown;
}

DmId DmEntity::getId() const
{
    return m_ulID;
}

double DmEntity::getLength() const
{
    return -1.0;
}

DmEntity* DmEntity::getParent() const
{
    return parent;
}

void DmEntity::setParent(DmEntity* p)
{
    parent = p;
}

// Resets the borders of this element.
void DmEntity::resetBorders()
{
    double maxd = DM_MAXDOUBLE;
    double mind = DM_MINDOUBLE;
    minV.set(maxd, maxd);
    maxV.set(mind, mind);
}

void DmEntity::moveBorders(const DmVector& offset)
{
    minV.move(offset);
    maxV.move(offset);
}

void DmEntity::scaleBorders(const DmVector& center, const DmVector& factor)
{
    minV.scale(center, factor);
    maxV.scale(center, factor);
}

/// @param select True to select, false to deselect.
bool DmEntity::setSelected(bool select)
{
    // layer is locked:
    if (select && isLocked())
    {
        return false;
    }

    if (select != isSelected())
    {
        if (select)
        {
            setFlag(DM::FlagSelected);
        }
        else
        {
            delFlag(DM::FlagSelected);
        }
    }

    return true;
}

bool DmEntity::toggleSelected()
{
    return setSelected(!isSelected());
}

bool DmEntity::isSelected() const
{
    return isVisible() && getFlag(DM::FlagSelected);
}

bool DmEntity::isParentSelected() const
{
    DmEntity const* p = this;

    while (p)
    {
        p = p->getParent();
        if (p && p->isSelected() == true)
        {
            return true;
        }
    }

    return false;
}

/// @param on True to set, false to reset.
void DmEntity::setProcessed(bool on)
{
    if (on)
    {
        setFlag(DM::FlagProcessed);
    }
    else
    {
        delFlag(DM::FlagProcessed);
    }
}

/// @return True if the processed flag is set.
bool DmEntity::isProcessed() const
{
    return getFlag(DM::FlagProcessed);
}

void DmEntity::update()
{
}

void DmEntity::setUpdateEnabled(bool on)
{
    updateEnabled = on;
}

DmVector DmEntity::getMin() const
{
    return minV;
}

DmVector DmEntity::getMax() const
{
    return maxV;
}

/// @return True if the entity is in the given range.
bool DmEntity::isInWindow(DmVector v1, DmVector v2) const
{
    double right = 0.0, left = 0.0, top = 0.0, bottom = 0.0;

    right = std::max(v1.x, v2.x);
    left = std::min(v1.x, v2.x);
    top = std::max(v1.y, v2.y);
    bottom = std::min(v1.y, v2.y);

    return (getMin().x >= left && getMax().x <= right && getMin().y >= bottom && getMax().y <= top);
}

bool DmEntity::hasEndpointsWithinWindow(const DmVector& /*v1*/, const DmVector& /*v2*/)
{
    return false;
}

bool DmEntity::isPointOnEntity(const DmVector& coord, double tolerance) const
{
    double dist = getDistanceToPoint(coord, nullptr, DM::ResolveNone);
    return dist <= tolerance;
}

bool DmEntity::offset(const DmVector& /*coord*/, const double& /*distance*/)
{
    return false;
}

std::vector<DmEntity*> DmEntity::offsetTwoSides(const double& /*distance*/) const
{
    return std::vector<DmEntity*>();
}

double DmEntity::getDistanceToPoint(const DmVector& coord, DmEntity** entity, DM::ResolveLevel /*level*/) const
{
    if (entity)
    {
        *entity = const_cast<DmEntity*>(this);
    }
    double dToEntity = DM_MAXDOUBLE;
    (void)getNearestPointOnEntity(coord, true, &dToEntity, entity);

    if (getCenter().valid)
    {
        double dToCenter = getCenter().distanceTo(coord);
        return std::min(dToEntity, dToCenter);
    }
    else
    {
        return dToEntity;
    }
}

bool DmEntity::isVisible() const
{
    if (!getFlag(DM::FlagVisible))
    {
        return false;
    }

    if (!getLayer())
    {
        return true;
    }

    if (layer)
    {
        if (!layer->isFrozen())
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    if (!layer)
    {
        if (!getLayer())
        {
            return true;
        }
        else
        {
            if (!getLayer()->isFrozen())
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    }

    return false;
}

void DmEntity::setVisible(bool v)
{
    if (v)
    {
        setFlag(DM::FlagVisible);
    }
    else
    {
        delFlag(DM::FlagVisible);
    }
}

void DmEntity::setHighlighted(bool on)
{
    if (on)
    {
        setFlag(DM::FlagHighlighted);
    }
    else
    {
        delFlag(DM::FlagHighlighted);
    }
}

DmVector DmEntity::getStartpoint() const
{
    return {};
}

DmVector DmEntity::getEndpoint() const
{
    return {};
}

double DmEntity::getDirection1() const
{
    return 0.;
}

double DmEntity::getDirection2() const
{
    return 0.;
}

DmVectorSolutions DmEntity::getTangentPoint(const DmVector& /*point*/) const
{
    return {};
}

DmVector DmEntity::getTangentDirection(const DmVector& /*point*/) const
{
    return {};
}

DmVector DmEntity::getMiddlePoint(void) const
{
    return DmVector(false);
}

bool DmEntity::isHighlighted() const
{
    return getFlag(DM::FlagHighlighted);
}

DmVector DmEntity::getWidthHeight() const
{
    return maxV - minV;
}

bool DmEntity::isLocked() const
{
    return getLayer(true) && getLayer()->isLocked();
}

DmVector DmEntity::getCenter() const
{
    return DmVector{};
}

double DmEntity::getRadius() const
{
    return DM_MAXDOUBLE;
}

void DmEntity::setDocument(DmDocument* pDoc)
{
    m_pDocument = pDoc;

    // 初始化实体属性
    this->setLayer(pDoc->getLayerTable()->getActive());
    this->setPen(pDoc->getActivePen());
}

Quadratic DmEntity::getQuadratic() const
{
    return Quadratic{};
}

/// Returns a pointer to the layer this entity is on or nullptr.
/// @param resolve true: if the layer is ByBlock, the layer of the block this entity is in is returned.
/// false: the layer of the entity is returned.
/// @return pointer to the layer this entity is on. If the layer is set to nullptr the layer of the next parent that is not on
/// layer nullptr is returned. If all parents are on layer nullptr, nullptr is returned.
DmLayer* DmEntity::getLayer(bool resolve) const
{
    if (resolve)
    {
        if (!layer)
        {
            if (parent)
            {
                return parent->getLayer(true);
            }
            else
            {
                return nullptr;
            }
        }
    }
    return layer;
}

// Sets the layer of this entity to the layer with the given name
void DmEntity::setLayer(const QString& name)
{
    if (m_pDocument)
    {
        layer = m_pDocument->getLayerTable()->find(name);
    }
    else
    {
        layer = nullptr;
    }
}

// Sets the layer of this entity to the layer given.
void DmEntity::setLayer(DmLayer* l)
{
    layer = l;
}

// Sets the layer of this entity to the current layer of the document this entity is in. If this entity (and none
// of its parents) are in a document the layer is set to nullptr.
void DmEntity::setLayerToActive()
{
    if (!m_pDocument)
    {
        return;
    }
    setLayer(m_pDocument->getLayerTable()->getActive());
}

DmPen DmEntity::getPen(bool resolve) const
{
    if (!resolve)
    {
        return pen;
    }
    else
    {
        DmPen p = pen;
        DmLayer* l = getLayer(true);

        // use parental attributes (e.g. vertex of a polyline, block
        // entities when they are drawn in block documents):
        if (parent)
        {
            // if pen is invalid gets all from parent
            if (!p.isValid())
            {
                p = parent->getPen();
            }
            // pen is valid, verify byBlock parts
            DmEntity* ep = parent;
            // If parent is byblock check parent.parent (nested blocks)
            while (p.getColor().isByBlock())
            {
                if (ep)
                {
                    p.setColor(parent->getPen().getColor());
                    ep = ep->parent;
                }
                else
                {
                    break;
                }
            }
            ep = parent;
            while (p.getWidth() == DM::WidthByBlock)
            {
                if (ep)
                {
                    p.setWidth(parent->getPen().getWidth());
                    ep = ep->parent;
                }
                else
                {
                    break;
                }
            }
            ep = parent;
            while (p.getLineType() == DmLineTypeTable::ByBlock)
            {
                if (ep)
                {
                    p.setLineType(parent->getPen().getLineType());
                    ep = ep->parent;
                }
                else
                {
                    break;
                }
            }
        }
        // check byLayer attributes:
        if (l)
        {
            // use layer's color:
            if (p.getColor().isByLayer())
            {
                p.setColor(l->getPen().getColor());
            }

            // use layer's width:
            if (p.getWidth() == DM::WidthByLayer)
            {
                p.setWidth(l->getPen().getWidth());
            }

            // use layer's linetype:
            if (p.getLineType() == DmLineTypeTable::ByLayer)
            {
                p.setLineType(l->getPen().getLineType());
            }
        }

        return p;
    }
}

void DmEntity::setPen(const DmPen& pen)
{
    this->pen = pen;
}

// Sets the pen of this entity to the current pen of the document this entity is in. If this entity (and none
// of its parents) are in a document the pen is not changed.
void DmEntity::setPenToActive()
{
    if (m_pDocument)
    {
        setPen(m_pDocument->getActivePen());
    }
}

/// @return User defined variable connected to this entity or nullptr if not found.
QString DmEntity::getUserDefVar(const QString& key) const
{
    auto it = m_varList.find(key);
    if (it == m_varList.end())
    {
        return nullptr;
    }
    return m_varList.at(key);
}

/// return a line tangent to entity and orthogonal to the line (*normal)
DmVector DmEntity::getNearestOrthTan(const DmVector& /*coord*/, const DmLine& /*normal*/, bool /*onEntity = false*/) const
{
    return DmVector(false);
}

// Add a user defined variable to this entity.
void DmEntity::setUserDefVar(QString key, QString val)
{
    m_varList.insert(std::make_pair(key, val));
}

// Deletes the given user defined variable.
void DmEntity::delUserDefVar(QString key)
{
    m_varList.erase(key);
}

/// @return A list of all keys connected to this entity.
std::vector<QString> DmEntity::getAllKeys() const
{
    std::vector<QString> ret(0);
    for (auto const& v : m_varList)
    {
        ret.push_back(v.first);
    }
    return ret;
}

DmVectorSolutions DmEntity::getRefPoints() const
{
    return DmVectorSolutions();
}

DmVector DmEntity::getNearestEndpoint(const DmVector& coord, double* dist) const
{
    double minDist = DM_MAXDOUBLE;
    DmVector ret(false);
    DmVector vp1(getStartpoint());
    DmVector vp2(getEndpoint());
    if (!vp1.valid || !vp2.valid)
    {
        if (dist)
        {
            *dist = minDist;
        }
        return ret;
    }

    double d1((coord - vp1).squared());
    double d2((coord - vp2).squared());
    if (d1 < d2)
    {
        ret = vp1;
        minDist = std::sqrt(d1);
    }
    else
    {
        ret = vp2;
        minDist = std::sqrt(d2);
    }
    if (dist)
    {
        *dist = minDist;
    }
    return ret;
}

void DmEntity::rotateAngle(const DmVector& center, const double& angle)
{
    rotate(center, DmVector(angle));
}

void DmEntity::scale(const DmVector& center, const double& factor)
{
    scale(center, DmVector(factor, factor));
}

void DmEntity::scale(const DmVector& factor)
{
    scale(DmVector(0., 0.), factor);
}

void DmEntity::moveRef(const DmVector& /*ref*/, const DmVector& /*offset*/)
{
    return;
}

void DmEntity::moveSelectedRef(const DmVector& /*ref*/, const DmVector& /*offset*/)
{
    return;
}

DmVector DmEntity::getNearestRef(const DmVector& coord, double* dist) const
{
    DmVectorSolutions const&& s = getRefPoints();
    return s.getClosest(coord, dist);
}

DmVector DmEntity::getNearestSelectedRef(const DmVector& coord, double* dist) const
{
    if (isSelected())
    {
        return getNearestRef(coord, dist);
    }
    else
    {
        return DmVector(false);
    }
}

void DmEntity::setObserver(DmObserver* ob)
{
    if (ob->getObserverType() == DM::ArrayRectObserver)
    {
        for (auto observer : m_observerList)
        {
            if ((*observer).getObserverType() == DM::ArrayRectObserver)
            {
                m_observerList.remove(observer);
                break;
            }
        }
    }
    m_observerList.emplace_back(ob);
}

void DmEntity::setObserver(std::list<DmObserver*> ob)
{
    m_observerList.insert(m_observerList.end(), ob.begin(), ob.end());
}

std::list<DmObserver*> DmEntity::getObserver() const
{
    return m_observerList;
}

void DmEntity::clearObserver()
{
    m_observerList.clear();
}

void DmEntity::notify()
{
    for (auto ob : m_observerList)
    {
        (*ob).respond(this);
    }
}

void DmEntity::saveStream(OutputStream& wrt) const
{
    DmObject::saveStream(wrt);
    auto d_layer = getLayer(false);
    if (d_layer == NULL)
    {
        bool bhaveLayer = false;
        wrt << (bool)bhaveLayer;
    }
    else
    {
        bool bhaveLayer = true;
        wrt << (bool)bhaveLayer;
        std::string layerName = getLayer()->getName().toStdString();
        wrt << (std::string)layerName;
    }

    auto pen = getPen(false);
    bool byBlock = pen.getColor().isByBlock();
    bool byLayer = pen.getColor().isByLayer();
    int color_r = pen.getColor().red();
    int color_g = pen.getColor().green();
    int color_b = pen.getColor().blue();
    auto lineTypeName = pen.getLineType()->getLineTypeName().toStdString();
    int lineWidth = (int)pen.getWidth();

    wrt << (bool)byBlock << (bool)byLayer << (int)color_r << (int)color_g << (int)color_b << (std::string)lineTypeName << (int)lineWidth;
}

void DmEntity::restoreStream(InputStream& reader, const std::vector<PAIR>& revs)
{
    int fileRev = getRevisionId("DmEntity", revs);
    if (revId > fileRev)
    {
        DmObject::restoreStream(reader, revs);
        // 老文件格式
        restoreStreamWithRev(reader, fileRev);
    }
    else
    {
        restoreStream(reader);
    }
}

void DmEntity::restoreStreamWithRev(InputStream& rdr, int rev)
{
    if (rev == 0)
    {
        // legacy rev 0 format
    }
    else //big change, e.g. change supper class of DmEntity
    {
        //step1.
        // read all legacy data one by one
    }
}

void DmEntity::restoreStream(InputStream& reader)
{
    DmObject::restoreStream(reader);
    // 为了支持跨平台，必须采用(uint32_t&)layerId，其他类型数据同理
    bool bhaveLayer = false;
    reader >> (bool&)bhaveLayer;
    DmDocument* doc = getDocument();
    if (bhaveLayer)
    {
        std::string layerName;
        reader >> (std::string&)layerName;

        DmLayer* pLayer = nullptr;
        if (doc)
        {
            pLayer = doc->getLayerTable()->find(QString::fromStdString(layerName));
        }
        this->setLayer(pLayer);
    }
    else
    {
        this->setLayer(nullptr);
    }

    bool byBlock = false, byLayer = false;
    int color_r = 0, color_g = 0, color_b = 0;
    std::string lineTypeName;
    int lineWidth = 0;

    reader >> (bool&)byBlock >> (bool&)byLayer >> (int&)color_r >> (int&)color_g >> (int&)color_b >> (std::string&)lineTypeName >> (int&)lineWidth;

    DmColor color = DmColor(color_r, color_g, color_b);
    if (byBlock)
    {
        color.setFlag(DM::FlagByBlock);
    }
    if (byLayer)
    {
        color.setFlag(DM::FlagByLayer);
    }
    DmLineType* lineType = DmLineTypeTable::Continuous;
    if (doc)
    {
        lineType = doc->getLineTypeTable()->find(QString::fromStdString(lineTypeName));
    }
    auto pen = DmPen(color, (DM::LineWidth)lineWidth, lineType);
    this->setPen(std::move(pen));

    // 导入实体不允许高亮和选中状态
    if (this->isHighlighted())
    {
        this->setHighlighted(false);
    }
    if (this->isSelected())
    {
        this->setSelected(false);
    }
}
