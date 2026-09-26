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

/// @file MetaDimLeaders.cpp
/// @brief 引线标注实体序列化容器实现文件

#include "MetaDimLeaders.h"

#include "DmLeader.h"
#include "DmDocument.h"

#include "Reader.h"
#include "Writer.h"

constexpr auto DIMLEADERS = "DimLeaders";
constexpr auto DIMLEADER_REV = "rev";
constexpr auto DIMLEADER_FILE = "file";
constexpr auto DIMLEADER_BIN = "DimLeaders.bin";

MetaDimLeadersContainer::MetaDimLeadersContainer(DmDocument* pDoc)
    : m_pDocument(pDoc)
{
}

/// @brief 保存为XML格式
/// @param wrt Writer对象
void MetaDimLeadersContainer::saveXML(Writer& wrt) const
{
    wrt.incInd();

    std::vector<PAIR> revs;
    DmLeader::getRevId(revs);

    // Insert leaders item
    wrt.Stream() << wrt.ind() << "<DimLeaders" << " levels=\"" << revs.size() << "\"" << " Count=\"" << m_entities.size() << "\"" << " file= \"" << wrt.addFile(DIMLEADER_BIN, this) << "\">" << std::endl;

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
    wrt.Stream() << wrt.ind() << "</" << DIMLEADERS << ">" << std::endl;
    wrt.decInd();
}

/// @brief 从XML恢复
/// @param reader XMLReader对象
void MetaDimLeadersContainer::restoreXML(XMLReader& reader)
{
    reader.readElement(DIMLEADERS);

    std::string file(reader.getAttribute(DIMLEADER_FILE));

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

    reader.readEndElement(DIMLEADERS);
}

/// @brief 获取内存大小
/// @return 内存大小（字节）
unsigned int MetaDimLeadersContainer::getMemSize() const
{
    return 0;
}

/// @brief 保存为流格式
/// @param wrt 输出流
void MetaDimLeadersContainer::saveStream(OutputStream& wrt) const
{
    for (auto& e : m_entities)
    {
        e->saveStream(wrt);
    }
}

/// @brief 从流恢复
/// @param rdr 输入流
void MetaDimLeadersContainer::restoreStream(InputStream& rdr)
{
    while (!rdr.end())
    {
        // read all leaders
        auto iLeader = new DmLeader();
        iLeader->setDocument(m_pDocument);
        iLeader->restoreStream(rdr, m_revs);
        m_pDocument->getEntityTable()->add_direct(iLeader);
    }
}

/// @brief 设置实体列表
/// @param entities 实体列表
void MetaDimLeadersContainer::setEntities(std::list<DmEntity*>& entities)
{
    m_entities = entities;
}
