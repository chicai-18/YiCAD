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


/// @file DmOverlayEntity.cpp
/// @brief 前景层覆盖实体实现：线、圆、点

#include "DmOverlayEntity.h"
#include "Tools.h"

DmOverlayLine::DmOverlayLine(DmEntity* parent, const LineData& d)
    : DmLine(parent, d)
{
}

DM::EntityType DmOverlayLine::getEntityType() const
{
    return DM::EntityOverlayLine;
}

std::list<DmEntity*> DmOverlayLine::getSubEntities() const
{
    return std::list<DmEntity*>();
}

DmOverlayCircle::DmOverlayCircle(DmEntity* parent, const CircleData& d)
    : DmCircle(parent, d)
{
}

DM::EntityType DmOverlayCircle::getEntityType() const
{
    return DM::EntityOverlayCircle;
}

std::list<DmEntity*> DmOverlayCircle::getSubEntities() const
{
    return std::list<DmEntity*>();
}

DmOverlayPoint::DmOverlayPoint(DmEntity* parent, const PointData& d)
    : DmPoint(parent, d)
{
}

DM::EntityType DmOverlayPoint::getEntityType() const
{
    return DM::EntityOverlayPoint;
}

std::list<DmEntity*> DmOverlayPoint::getSubEntities() const
{
    return std::list<DmEntity*>();
}
