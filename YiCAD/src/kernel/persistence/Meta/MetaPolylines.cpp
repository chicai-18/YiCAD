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

/// @file MetaPolylines.cpp
/// @brief 多段线(Polyline)实体序列化容器实现文件

#include "MetaPolylines.h"

#include "DmPolyline.h"
#include "DmDocument.h"

#include "Reader.h"
#include "Writer.h"

constexpr const char* POLYLINES = "Polylines";
constexpr const char* POLYLINE_REV = "rev";
constexpr const char* POLYLINE_FILE = "file";
constexpr const char* POLYLINE_BIN = "Polylines.bin";

MetaPolylinesContainer::MetaPolylinesContainer(DmDocument* pDoc)
    : m_pDocument(pDoc)
{
}

void MetaPolylinesContainer::saveXML(Writer& wrt) const
{
    wrt.incInd();

    std::vector<PAIR> revs;
    DmPolyline::getRevId(revs);

    // Insert Polylines item
    wrt.Stream() << wrt.ind() << "<Polylines" << " levels=\"" << revs.size() << "\"" << " Count=\"" << m_entities.size() << "\"" << " file= \"" << wrt.addFile(POLYLINE_BIN, this) << "\">" << std::endl;

    // write type and revision ids
    {
        wrt.incInd();
        for (auto itype : revs)
        {
            auto strtype = itype.first;

            // wrt.Stream() << wrt.ind() << "<leve name=\"" << PersistentTools::base64_encode(strtype.c_str(), itype.first.length()) << "\" id = \"" << itype.second << "\"/>" << std::endl;
            wrt.Stream() << wrt.ind() << "<level name=\"" << strtype << "\" id = \"" << itype.second << "\"/>" << std::endl;
        }

        wrt.decInd();
    }
    wrt.Stream() << wrt.ind() << "</" << POLYLINES << ">" << std::endl;
    wrt.decInd();
}

void MetaPolylinesContainer::restoreXML(XMLReader& reader)
{
    reader.readElement(POLYLINES);

    std::string file(reader.getAttribute(POLYLINE_FILE));

    // restore files
    if (!file.empty())
    {
        reader.addFile(file.c_str(), this);
    }

    // restore levels
    auto ilevels = (size_t)reader.getAttributeAsInteger("levels");
    for (size_t idx = 0; idx < ilevels; idx++)
    {
        reader.readElement("level");
        auto itype = reader.getAttribute("name");
        auto irev = reader.getAttributeAsInteger("id");
        m_revs.push_back(std::make_pair(itype, irev));
    }

    reader.readEndElement(POLYLINES);
}

unsigned int MetaPolylinesContainer::getMemSize() const
{
    return 0;
}

void MetaPolylinesContainer::saveStream(OutputStream& wrt) const
{
    // give each entity a chance to add entry or a file
    for (auto& e : m_entities)
    {
        e->saveStream(wrt);
    }
}

void MetaPolylinesContainer::restoreStream(InputStream& rdr)
{
    while (!rdr.end())
    {
        // read all polylines
        auto iPolyline = new DmPolyline();
        iPolyline->setDocument(m_pDocument);
        iPolyline->restoreStream(rdr, m_revs);
        m_pDocument->getEntityTable()->add_direct(iPolyline);
    }
}

void MetaPolylinesContainer::setEntities(std::list<DmEntity*>& entities)
{
    m_entities = entities;
}
