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


/// @file DmFlags.cpp
/// @brief 标志位基类持久化实现

#include "DmFlags.h"

TYPESYSTEM_SOURCE_ABSTRACT(DmFlags, Persistence, 0)
// Constructor with initialisation to the given flags.

void DmFlags::saveStream(OutputStream& wrt) const
{
    wrt << (uint32_t)flags;
}

void DmFlags::restoreStream(InputStream& reader, const std::vector<PAIR>& revs)
{
    int fileRev = getRevisionId("DmFlags", revs);
    if (revId > fileRev)
    {
        // 老文件格式
        restoreStreamWithRev(reader, fileRev);
    }
    else
    {
        restoreStream(reader);
    }
}

void DmFlags::restoreStreamWithRev(InputStream& rdr, int rev)
{
    if (0 == rev)
    {
        // 基本版本，无需额外处理
    }
    else // big change, e.g. change super class of DmFlags
    {
        // step1.
        // read all legacy data one by one
    }
}

void DmFlags::restoreStream(InputStream& rdr)
{
    rdr >> (uint32_t&)flags;
}
