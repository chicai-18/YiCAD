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


/// @file DmLineType.cpp
/// @brief 线型类实现，管理线型的持久化和数据操作

#include "DmLineType.h"
#include "Tools.h"

TYPESYSTEM_SOURCE(DmLineType, DmObject, 0)

DmLineTypeData::DmLineTypeData(const QString& _name,
    const QString& _desp, const QString& _outward,
    const std::vector<double>& _pattern)
    : m_LineTypeName(_name)
    , m_LineTypeDesp(_desp)
    , m_LineTypeOutward(_outward)
    , m_LineTypePattern(_pattern)
{
}

DmLineType::DmLineType()
    : data()
{
}

DmLineType::DmLineType(const QString& _name)
{
    data.m_LineTypeName = _name;
    data.m_LineTypeDesp = nullptr;
    data.m_LineTypeOutward = nullptr;
    data.m_LineTypePattern = {0};
}

DmLineType::DmLineType(DmLineTypeData _data)
    : data(_data)
{
    setLineTypePattern(_data.m_LineTypePattern);
}

DmLineType::DmLineType(DmLineType* _linetype)
{
    data = _linetype->data;
}

QString DmLineType::getLineTypeName()
{
    return data.m_LineTypeName;
}

QString DmLineType::getLineTypeOutWard()
{
    return data.m_LineTypeOutward;
}

QString DmLineType::getLineTypeDesp()
{
    return data.m_LineTypeDesp;
}

std::vector<double>& DmLineType::getLineTypeData()
{
    return data.m_LineTypePattern;
}

size_t DmLineType::getNum()
{
    return data.m_LineTypePattern.size();
}

void DmLineType::clear()
{
    data.m_LineTypeName.clear();
    data.m_LineTypeDesp.clear();
    data.m_LineTypeOutward.clear();
    data.m_LineTypePattern.clear();

    if (data.m_LineTypePattern.size() > 0)
    {
        data.m_LineTypePattern.clear();
    }
}

void DmLineType::setLineTypeName(QString name)
{
    data.m_LineTypeName = name;
}

void DmLineType::setLineTypeOutWard(QString outward)
{
    data.m_LineTypeOutward = outward;
}

void DmLineType::setLineTypeDesp(QString desp)
{
    data.m_LineTypeDesp = desp;
}

void DmLineType::setLineTypeData(QString _data)
{
    data.m_LineTypePattern.clear();

    QStringList tmp = _data.split(",");
    data.m_LineTypePattern.reserve(tmp.count());

    for (int i = 0; i < tmp.count(); i++)
    {
        data.m_LineTypePattern.emplace_back(tmp[i].toDouble());
    }
    setLineTypePattern(data.m_LineTypePattern);
}

void DmLineType::setLineTypeData(std::vector<double> _pattern)
{
    data.m_LineTypePattern = _pattern;
    setLineTypePattern(_pattern);
}

void DmLineType::setLineTypePattern(std::vector<double> _pattern)
{
    data.m_LineTypePattern = _pattern;
}

void DmLineType::saveStream(OutputStream& wrt) const
{
    DmObject::saveStream(wrt);
    wrt << (std::string)data.m_LineTypeName.toStdString()
        << (std::string)data.m_LineTypeOutward.toStdString()
        << (std::string)data.m_LineTypeDesp.toStdString();
    wrt << (int)data.m_LineTypePattern.size();
    for (int i = 0; i < (int)data.m_LineTypePattern.size(); i++)
    {
        wrt << (double)data.m_LineTypePattern.at(i);
    }
}

void DmLineType::restoreStream(InputStream& reader,
    const std::vector<PAIR>& revs)
{
    int fileRev = getRevisionId("DmObject", revs);
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

void DmLineType::restoreStreamWithRev(InputStream& rdr, int rev)
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

void DmLineType::restoreStream(InputStream& rdr)
{
    DmObject::restoreStream(rdr);
    std::string name;
    std::string outward;
    std::string desr;
    rdr >> (std::string&)name >> (std::string&)outward >> (std::string&)desr;
    data.m_LineTypeName = QString::fromStdString(name);
    data.m_LineTypeOutward = QString::fromStdString(outward);
    data.m_LineTypeDesp = QString::fromStdString(desr);

    int size = 0;
    rdr >> (int&)size;
    data.m_LineTypePattern.resize(size);
    if (size > 0)
    {
        double val = 0.0;
        for (int i = 0; i < size; i++)
        {
            rdr >> (double&)val;
            data.m_LineTypePattern.at(i) = val;
        }
    }
    else
    {
        // 无模式数据
    }
}
