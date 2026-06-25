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

/// @file MetaDimRadials.cpp
/// @brief 半径标注实体序列化容器实现文件

#include "MetaDimRadials.h"

#include "DmDimRadial.h"
#include "DmDocument.h"

#include "Reader.h"
#include "Writer.h"

constexpr auto DIMRADIALS = "DimRadials";
constexpr auto DIMRADIAL_REV = "rev";
constexpr auto DIMRADIAL_FILE = "file";
constexpr auto DIMRADIAL_BIN = "DimRadials.bin";

MetaDimRadialsContainer::MetaDimRadialsContainer(DmDocument* pDoc)
    : m_pDocument(pDoc)
{
}

/// @brief 保存为XML格式
/// @param wrt Writer对象
void MetaDimRadialsContainer::saveXML(Writer& wrt) const
{
    wrt.incInd();

    std::vector<PAIR> revs;
    DmDimRadial::getRevId(revs);

    // Insert dimRadials item
    wrt.Stream() << wrt.ind() << "<DimRadials" << " levels=\"" << revs.size() << "\"" << " Count=\"" << m_entities.size() << "\"" << " file= \"" << wrt.addFile(DIMRADIAL_BIN, this) << "\">" << std::endl;

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
    wrt.Stream() << wrt.ind() << "</" << DIMRADIALS << ">" << std::endl;
    wrt.decInd();
}

/// @brief 从XML恢复
/// @param reader XMLReader对象
void MetaDimRadialsContainer::restoreXML(XMLReader& reader)
{
    reader.readElement(DIMRADIALS);

    std::string file(reader.getAttribute(DIMRADIAL_FILE));

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

    reader.readEndElement(DIMRADIALS);
}

/// @brief 获取内存大小
/// @return 内存大小（字节）
unsigned int MetaDimRadialsContainer::getMemSize() const
{
    return 0;
}

/// @brief 保存为流格式
/// @param wrt 输出流
void MetaDimRadialsContainer::saveStream(OutputStream& wrt) const
{
    for (auto& e : m_entities)
    {
        e->saveStream(wrt);
    }
}

/// @brief 从流恢复
/// @param rdr 输入流
void MetaDimRadialsContainer::restoreStream(InputStream& rdr)
{
    while (!rdr.end())
    {
        // read all dimRadials
        auto iDimRadial = new DmDimRadial();
        iDimRadial->setDocument(m_pDocument);
        iDimRadial->restoreStream(rdr, m_revs);
        m_pDocument->getEntityTable()->add_direct(iDimRadial);
    }
}

/// @brief 设置实体列表
/// @param entities 实体列表
void MetaDimRadialsContainer::setEntities(std::list<DmEntity*>& entities)
{
    m_entities = entities;
}
