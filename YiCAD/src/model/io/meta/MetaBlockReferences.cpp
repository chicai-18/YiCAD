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
/// @file MetaBlockReferences.cpp
/// @brief 块引用元数据容器实现文件

#include "MetaBlockReferences.h"

#include "DmBlockReference.h"
#include "DmBlockTable.h"
#include "DmDocument.h"
#include "DmEntityContainer.h"

#include "Reader.h"
#include "Writer.h"

constexpr const char* BLOCKREFERENCES = "BlockReferences";
constexpr const char* BLOCKREFERENCE_REV = "rev";
constexpr const char* BLOCKREFERENCE_FILE = "file";
constexpr const char* BLOCKREFERENCE_BIN = "BlockReferences.bin";

MetaBlcokReferencesContainer::MetaBlcokReferencesContainer(DmDocument* pDoc)
	: m_pDocument(pDoc)
{
}

void MetaBlcokReferencesContainer::saveXML(Writer& wrt) const
{
	wrt.incInd();

	std::vector<PAIR> revs;
	DmBlockReference::getRevId(revs);

	// 写入块参照节点
	wrt.Stream() << wrt.ind() << "<BlockReferences" << " levels=\"" << revs.size() << "\"" << " Count=\"" << m_entities.size() << "\"" << " file= \"" << wrt.addFile(BLOCKREFERENCE_BIN, this) << "\">" << std::endl;

	// 写入类型及版本号信息
	{
		wrt.incInd();
		for (auto itype : revs)
		{
			auto strtype = itype.first;

			//wrt.Stream() << wrt.ind() << "<leve name=\"" << PersistentTools::base64_encode(strtype.c_str(), itype.first.length()) << "\" id = \"" << itype.second << "\"/>" << std::endl;
			wrt.Stream() << wrt.ind() << "<level name=\"" << strtype << "\" id = \"" << itype.second << "\"/>" << std::endl;
		}

		wrt.decInd();
	}
	wrt.Stream() << wrt.ind() << "</" << BLOCKREFERENCES << ">" << std::endl;
	wrt.decInd();
}

void MetaBlcokReferencesContainer::restoreXML(XMLReader& reader)
{
	reader.readElement(BLOCKREFERENCES);

	std::string file(reader.getAttribute(BLOCKREFERENCE_FILE));
	// 恢复关联文件
	if (!file.empty())
	{
		reader.addFile(file.c_str(), this);
	}

	// 恢复版本层级信息
	auto ilevels = (size_t)reader.getAttributeAsInteger("levels");
	for (size_t idx = 0; idx < ilevels; idx++)
	{
		reader.readElement("level");
		auto itype = reader.getAttribute("name");
		auto irev = reader.getAttributeAsInteger("id");
		m_revs.push_back(std::make_pair(itype, irev));
	}

	reader.readEndElement(BLOCKREFERENCES);
}

unsigned int MetaBlcokReferencesContainer::getMemSize() const
{
	return 0;
}

void MetaBlcokReferencesContainer::saveStream(OutputStream& wrt) const
{
	for (auto e : m_entities)
	{
		e->saveStream(wrt);
	}
}

void MetaBlcokReferencesContainer::restoreStream(InputStream& rdr)
{
	while (!rdr.end())
	{
		// 读取所有块参照
		auto iBlcokReference = new DmBlockReference();
		iBlcokReference->setDocument(m_pDocument);
		iBlcokReference->restoreStream(rdr, m_revs);
		m_pDocument->getEntityTable()->add_direct(iBlcokReference);
	}
}

void MetaBlcokReferencesContainer::setEntities(std::list<DmEntity*>& entities)
{
	m_entities = entities;
}
