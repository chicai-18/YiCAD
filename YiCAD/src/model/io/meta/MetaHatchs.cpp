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

/// @file MetaHatchs.cpp
/// @brief 填充(Hatch)实体序列化容器实现文件

#include "MetaHatchs.h"

#include "DmHatch.h"
#include "DmDocument.h"

#include "Reader.h"
#include "Writer.h"

constexpr const char* HATCHS = "Hatchs";
constexpr const char* HATCHS_REV = "rev";
constexpr const char* HATCHS_FILE = "file";
constexpr const char* HATCHS_BIN = "Hatchs.bin";

MetaHatchsContainer::MetaHatchsContainer(DmDocument* pDoc)
    : m_pDocument(pDoc)
{
}

void MetaHatchsContainer::saveXML(Writer& wrt) const
{
    wrt.incInd();

    std::vector<PAIR> revs;
    DmHatch::getRevId(revs);

    // Insert Hatch item
    wrt.Stream() << wrt.ind() << "<Hatchs" << " levels=\"" << revs.size() << "\"" << " Count=\"" << m_entities.size() << "\"" << " file= \"" << wrt.addFile(HATCHS_BIN, this) << "\">" << std::endl;

    // write type and revision ids
    {
        wrt.incInd();
        for (auto itype : revs)
        {
            auto strtype = itype.first;
            wrt.Stream() << wrt.ind() << "<level name=\"" << strtype << "\" id = \"" << itype.second << "\"/>" << std::endl;
        }

        wrt.decInd();
    }
    wrt.Stream() << wrt.ind() << "</" << HATCHS << ">" << std::endl;
    wrt.decInd();
}

void MetaHatchsContainer::restoreXML(XMLReader& reader)
{
    reader.readElement(HATCHS);

    std::string file(reader.getAttribute(HATCHS_FILE));

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

    reader.readEndElement(HATCHS);
}

unsigned int MetaHatchsContainer::getMemSize() const
{
    return 0;
}

void MetaHatchsContainer::saveStream(OutputStream& wrt) const
{
    for (auto& e : m_entities)
    {
        e->saveStream(wrt);
    }
}

void MetaHatchsContainer::restoreStream(InputStream& rdr)
{
    while (!rdr.end())
    {
        auto iHatch = new DmHatch();
        iHatch->setDocument(m_pDocument);
        iHatch->restoreStream(rdr, m_revs);
        iHatch->update();
        m_pDocument->getEntityTable()->add_direct(iHatch);
    }
}

void MetaHatchsContainer::setEntities(std::list<DmEntity*>& entities)
{
    m_entities = entities;
}
