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

/// @file MetaDimensionStyles.cpp
/// @brief 标注样式实体序列化容器实现文件

#include "MetaDimensionStyles.h"

#include "DmDimensionStyle.h"
#include "DmDimensionStyleTable.h"
#include "DmDocument.h"
#include "DmEntityContainer.h"

#include "Reader.h"
#include "Writer.h"

constexpr const char* DIMENSIONSTYLES = "DimensionStyles";
constexpr const char* DIMENSIONSTYLE_REV = "rev";
constexpr const char* DIMENSIONSTYLE_FILE = "file";
constexpr const char* DIMENSIONSTYLE_BIN = "DimensionStyles.bin";

MetaDimensionStylesContainer::MetaDimensionStylesContainer(DmDocument* pDoc)
	: m_pDocument(pDoc)
	, m_strActiveStyle("Standard")
{
}

void MetaDimensionStylesContainer::saveXML(Writer& wrt) const
{
	std::vector<PAIR> revs;
	DmDimensionStyle::getRevId(revs);

	auto activeName = m_pDocument->getDimStyleTable()->getActive()->getName().toStdString();

	// Insert styles item
	wrt.Stream() << wrt.ind() << "<DimensionStyles" << " levels=\"" << revs.size() << "\"" << " Count=\"" << m_dimStyleList.size() << "\"" << " active=\"" << activeName << "\"" << " file= \"" << wrt.addFile(DIMENSIONSTYLE_BIN, this) << "\">" << std::endl;

	// write type and revision ids
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
	wrt.Stream() << wrt.ind() << "</" << DIMENSIONSTYLES << ">" << std::endl;
}

void MetaDimensionStylesContainer::restoreXML(XMLReader& reader)
{
	reader.readElement(DIMENSIONSTYLES);

	m_strActiveStyle = QString::fromStdString(reader.getAttribute("active"));

	std::string file(reader.getAttribute(DIMENSIONSTYLE_FILE));
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

	reader.readEndElement(DIMENSIONSTYLES);
}

unsigned int MetaDimensionStylesContainer::getMemSize() const
{
	return 0;
}

void MetaDimensionStylesContainer::saveStream(OutputStream& wrt) const
{
	for (auto& e : m_dimStyleList)
	{
		e->saveStream(wrt);
	}
}

void MetaDimensionStylesContainer::restoreStream(InputStream& rdr)
{
	auto table = m_pDocument->getDimStyleTable();
	while (!rdr.end())
	{
		// read all dimStyles
		auto iDimStyle = new DmDimensionStyle();
		iDimStyle->setDocument(m_pDocument);
		iDimStyle->restoreStream(rdr, m_revs);
		table->add_direct(iDimStyle);
	}

	auto activeStyle = table->find(m_strActiveStyle);
	table->activate_direct(activeStyle);
}

void MetaDimensionStylesContainer::setDimStyles(std::vector<DmDimensionStyle*>& styles)
{
	m_dimStyleList = styles;
}
