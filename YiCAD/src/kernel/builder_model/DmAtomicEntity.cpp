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


/// @file DmAtomicEntity.cpp
/// @brief 原子实体基类的默认实现

#include "DmAtomicEntity.h"

TYPESYSTEM_SOURCE_ABSTRACT(DmAtomicEntity, DmEntity, 0)

DmAtomicEntity::DmAtomicEntity(DmEntity* parent)
    : DmEntity(parent)
{
}

bool DmAtomicEntity::isContainer() const
{
    return false;
}

DmVector DmAtomicEntity::getEndpoint() const
{
    return DmVector(false);
}

DmVector DmAtomicEntity::getStartpoint() const
{
    return DmVector(false);
}

double DmAtomicEntity::getDirection1() const
{
    return 0.0;
}

double DmAtomicEntity::getDirection2() const
{
    return 0.0;
}

DmVector DmAtomicEntity::getCenter() const
{
    return DmVector(false);
}

double DmAtomicEntity::getRadius() const
{
    return 0.;
}

DmVector DmAtomicEntity::getNearestCenter(const DmVector& /*coord*/, double* /*dist*/) const
{
    return DmVector(false);
}

void DmAtomicEntity::setStartpointSelected(bool select)
{
    if (select)
    {
        setFlag(DM::FlagSelected1);
    }
    else
    {
        delFlag(DM::FlagSelected1);
    }
}

void DmAtomicEntity::setEndpointSelected(bool select)
{
    if (select)
    {
        setFlag(DM::FlagSelected2);
    }
    else
    {
        delFlag(DM::FlagSelected2);
    }
}

bool DmAtomicEntity::isTangent(const CircleData& /* circleData */) const
{
    return false;
}

bool DmAtomicEntity::isStartpointSelected() const
{
    return getFlag(DM::FlagSelected1);
}

bool DmAtomicEntity::isEndpointSelected() const
{
    return getFlag(DM::FlagSelected2);
}

bool DmAtomicEntity::offset(const DmVector& /*position*/, const double& /*distance*/)
{
    return false;
}

void DmAtomicEntity::moveStartpoint(const DmVector& /*pos*/)
{
}

void DmAtomicEntity::moveEndpoint(const DmVector& /*pos*/)
{
}

void DmAtomicEntity::trimStartpoint(const DmVector& pos)
{
    moveStartpoint(pos);
}

void DmAtomicEntity::trimEndpoint(const DmVector& pos)
{
    moveEndpoint(pos);
}

DM::Ending DmAtomicEntity::getTrimPoint(const DmVector& /*coord*/, const DmVector& /*trimPoint*/)
{
    return DM::EndingNone;
}

void DmAtomicEntity::moveSelectedRef(const DmVector& ref, const DmVector& offset)
{
    if (isSelected())
    {
        moveRef(ref, offset);
    }
}

void DmAtomicEntity::saveStream(OutputStream& wrt) const
{
    DmEntity::saveStream(wrt);
}

void DmAtomicEntity::restoreStream(InputStream& reader, const std::vector<PAIR>& revs)
{
    int fileRev = getRevisionId("DmAtomicEntity", revs);
    if (revId > fileRev)
    {
        DmEntity::restoreStream(reader, revs);
        // 老文件格式
        restoreStreamWithRev(reader, fileRev);
    }
    else
    {
        // 新版本格式，在restoreStream中处理
    }
}

void DmAtomicEntity::restoreStreamWithRev(InputStream& rdr, int rev)
{
    DmEntity::restoreStreamWithRev(rdr, rev);
}

void DmAtomicEntity::restoreStream(InputStream& rdr)
{
    DmEntity::restoreStream(rdr);
}
