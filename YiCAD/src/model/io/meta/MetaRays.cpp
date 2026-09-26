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

/// @file MetaRays.cpp
/// @brief 射线实体序列化容器实现文件

#include "MetaRays.h"

#include "DmRay.h"
#include "DmDocument.h"

#include "Reader.h"
#include "Writer.h"

constexpr const char* RAYS     = "Rays";
constexpr const char* RAY_REV  = "rev";
constexpr const char* RAY_FILE = "file";
constexpr const char* RAY_BIN  = "Rays.bin";

MetaRaysContainer::MetaRaysContainer(DmDocument* pDoc)
	: m_pDocument(pDoc)
{
}

void MetaRaysContainer::saveXML(Writer& wrt) const
{
	wrt.incInd();

	std::vector<PAIR> revs;
	DmRay::getRevId(revs);

	// Insert Rays item
	wrt.Stream() << wrt.ind() << "<Rays" << " levels=\"" << revs.size() << "\"" << " Count=\"" << m_entities.size() << "\"" << " file= \"" << wrt.addFile(RAY_BIN, this) << "\">" << std::endl;

	// write type and revision ids
	{
		wrt.incInd();

		for (auto& itype : revs)
		{
			auto strtype = itype.first;

			// wrt.Stream() << wrt.ind() << "<leve name=\"" << PersistentTools::base64_encode(strtype.c_str(), itype.first.length()) << "\" id = \"" << itype.second << "\"/>" << std::endl;
			wrt.Stream() << wrt.ind() << "<level name=\"" << strtype << "\" id = \"" << itype.second << "\"/>" << std::endl;
		}

		wrt.decInd();
	}

	wrt.Stream() << wrt.ind() << "</" << RAYS << ">" << std::endl;
	wrt.decInd();
}

void MetaRaysContainer::restoreXML(XMLReader& reader)
{
	reader.readElement(RAYS);

	std::string file(reader.getAttribute(RAY_FILE));

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

	reader.readEndElement(RAYS);
}

unsigned int MetaRaysContainer::getMemSize() const
{
	return 0;
}

void MetaRaysContainer::saveStream(OutputStream& wrt) const
{
	// give each entity a chance to add entry or a file
	for (auto& e : m_entities)
	{
		e->saveStream(wrt);
	}
}

void MetaRaysContainer::restoreStream(InputStream& rdr)
{
	while (!rdr.end())
	{
		// read all rays
		auto iRay = new DmRay();
		iRay->setDocument(m_pDocument);
		iRay->restoreStream(rdr, m_revs);
		m_pDocument->getEntityTable()->add_direct(iRay);
	}
}

void MetaRaysContainer::setEntities(std::list<DmEntity*>& entities)
{
	m_entities = entities;
}
