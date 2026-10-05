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

/// @file MetaCustomEntities.cpp
/// @brief 自定义实体序列化容器的实现

#include "MetaCustomEntities.h"

#include "DmCustomEntity.h"
#include "DmDocument.h"
#include "EntityTable.h"
#include "Reader.h"
#include "Writer.h"

constexpr const char* CUSTOM_ENTITIES     = "CustomEntities";
constexpr const char* CUSTOM_ENTITY_FILE  = "file";
constexpr const char* CUSTOM_ENTITIES_BIN = "CustomEntities.bin";

MetaCustomEntitiesContainer::MetaCustomEntitiesContainer(DmDocument* pDoc)
    : m_pDocument(pDoc)
{
}

void MetaCustomEntitiesContainer::saveXML(Writer& wrt) const
{
    wrt.incInd();

    // 公共部分（DmObject 到 DmCustomEntity）的版本；各类自己的数据版本随记录存
    std::vector<PAIR> revs;
    DmCustomEntity::getRevId(revs);

    wrt.Stream() << wrt.ind() << "<" << CUSTOM_ENTITIES << " levels=\"" << revs.size() << "\"" << " Count=\""
                 << m_entities.size() << "\"" << " file= \"" << wrt.addFile(CUSTOM_ENTITIES_BIN, this) << "\">"
                 << std::endl;
    wrt.incInd();
    for (const PAIR& rev : revs)
    {
        wrt.Stream() << wrt.ind() << "<level name=\"" << rev.first << "\" id = \"" << rev.second << "\"/>" << std::endl;
    }
    wrt.decInd();
    wrt.Stream() << wrt.ind() << "</" << CUSTOM_ENTITIES << ">" << std::endl;

    wrt.decInd();
}

void MetaCustomEntitiesContainer::restoreXML(XMLReader& reader)
{
    reader.readElement(CUSTOM_ENTITIES);

    std::string file(reader.getAttribute(CUSTOM_ENTITY_FILE));
    if (!file.empty())
    {
        reader.addFile(file.c_str(), this);
    }

    const auto levels = static_cast<std::size_t>(reader.getAttributeAsInteger("levels"));
    for (std::size_t i = 0; i < levels; ++i)
    {
        reader.readElement("level");
        auto type = reader.getAttribute("name");
        auto rev = reader.getAttributeAsInteger("id");
        m_revs.push_back(std::make_pair(type, rev));
    }

    reader.readEndElement(CUSTOM_ENTITIES);
}

unsigned int MetaCustomEntitiesContainer::getMemSize() const
{
    return 0;
}

void MetaCustomEntitiesContainer::saveStream(OutputStream& wrt) const
{
    for (DmEntity* e : m_entities)
    {
        DmCustomEntity::writeRecord(wrt, *static_cast<const DmCustomEntity*>(e));
    }
}

void MetaCustomEntitiesContainer::restoreStream(InputStream& rdr)
{
    while (!rdr.end())
    {
        if (DmCustomEntity* entity = DmCustomEntity::readRecord(rdr, m_pDocument, m_revs))
        {
            m_pDocument->getEntityTable()->add_direct(entity);
        }
    }
}

void MetaCustomEntitiesContainer::setEntities(std::list<DmEntity*>& entities)
{
    m_entities = entities;
}
