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


/// @file DmLayer.cpp
/// @brief 图层类实现，管理图层的持久化和属性操作

#include "DmLayer.h"
#include "DmDocument.h"

#include <iostream>
#include <QString>

TYPESYSTEM_SOURCE(DmLayer, DmObject, 0)

DmLayerData::DmLayerData(const QString& name, const DmPen& pen,
    bool frozen, bool locked)
    : name(name)
    , pen(pen)
    , frozen(frozen)
    , locked(locked)
{
}

DmLayer::DmLayer()
{
    m_data = std::shared_ptr<DmLayerData>(new DmLayerData());
}

DmLayer::DmLayer(const QString& name)
{
    m_data = std::make_shared<DmLayerData>(name,
        DmPen(DmColor(255, 255, 255), DM::Width00,
            DmLineTypeTable::Continuous),
        false, false);
}

DmLayer* DmLayer::clone() const
{
    auto l = new DmLayer(*this);
    l->m_ulID = DmId();
    l->m_data = std::shared_ptr<DmLayerData>(
        new DmLayerData(*m_data));
    return l;
}

void DmLayer::setName(const QString& name)
{
    m_data->name = name;
}

QString DmLayer::getName() const
{
    return m_data->name;
}

void DmLayer::setPen(const DmPen& pen)
{
    m_data->pen = pen;
}

DmPen DmLayer::getPen() const
{
    return m_data->pen;
}

bool DmLayer::isFrozen() const
{
    return m_data->frozen;
}

void DmLayer::toggle()
{
    m_data->frozen = !m_data->frozen;
}

void DmLayer::freeze(bool freeze)
{
    m_data->frozen = freeze;
}

void DmLayer::lock(bool l)
{
    m_data->locked = l;
}

bool DmLayer::isLocked() const
{
    return m_data->locked;
}

bool DmLayer::setPrint(const bool print)
{
    return m_data->print = print;
}

bool DmLayer::isPrint() const
{
    return m_data->print;
}

DmLayerData DmLayer::getData() const
{
    return *m_data;
}

void DmLayer::setData(const DmLayerData& d)
{
    m_data.reset(new DmLayerData(d));
}

void DmLayer::saveStream(OutputStream& wrt) const
{
    DmObject::saveStream(wrt);

    std::string layerName = getName().toStdString();
    wrt << (std::string)layerName
        << (bool)m_data->frozen
        << (bool)m_data->locked
        << (bool)m_data->print;
    m_data->pen.getColor().saveStream(wrt);
    std::string lineTypeName =
        m_data->pen.getLineType()->getLineTypeName().toStdString();
    int lineWidth = (int)m_data->pen.getWidth();
    wrt << (std::string)lineTypeName << lineWidth;
}

void DmLayer::restoreStream(InputStream& reader,
    const std::vector<PAIR>& revs)
{
    int fileRev = getRevisionId("DmLayer", revs);
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

void DmLayer::restoreStreamWithRev(InputStream& rdr, int rev)
{
    if (0 == rev)
    {
        // 基本版本，无需额外处理
    }
    else // big change, e.g. change super class of DmEntity
    {
        // step1.
        // read all legacy data one by one
    }
}

void DmLayer::restoreStream(InputStream& rdr)
{
    DmObject::restoreStream(rdr);

    std::string lineTypeName;
    std::string layerName;
    int lineWidth = 0;
    bool frozen = false;
    bool locked = false;
    bool print = false;
    rdr >> (std::string&)layerName
        >> (bool&)frozen
        >> (bool&)locked
        >> (bool&)print;
    (*m_data).name = QString::fromStdString(layerName);
    (*m_data).frozen = frozen;
    (*m_data).locked = locked;
    (*m_data).print = print;
    DmColor color;
    color.restoreStream(rdr);
    rdr >> (std::string&)lineTypeName >> (int&)lineWidth;
    DmDocument* doc = getDocument();
    DmLineType* lineType = DmLineTypeTable::Continuous;
    if (doc)
    {
        lineType = doc->getLineTypeTable()->find(
            QString::fromStdString(lineTypeName));
    }
    auto pen = DmPen(color, (DM::LineWidth)lineWidth, lineType);
    this->setPen(std::move(pen));
}
