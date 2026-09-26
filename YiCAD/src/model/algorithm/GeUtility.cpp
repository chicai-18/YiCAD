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

/// @file GeUtility.cpp
/// @brief 几何工具类实现

#include "GeUtility.h"

#include "DmEntity.h"
#include "Datamodel.h"

bool GeUtility::isEntityArc(const DmEntity* e)
{
    switch (e->getEntityType())
    {
    case DM::EntityArc:
    case DM::EntityCircle:
    case DM::EntityEllipse:
        return true;
    default:
        return false;
    }
}
