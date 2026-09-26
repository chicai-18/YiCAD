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

/// @file MetaTextStyles.cpp
/// @brief 文字样式实体序列化容器实现文件

#include "MetaTextStyles.h"

#include "DmTextStyle.h"
#include "DmTextStyleTable.h"
#include "DmDocument.h"
#include "DmEntityContainer.h"

#include "Reader.h"
#include "Writer.h"

constexpr const char* TEXTSTYLES = "TextStyles";
constexpr const char* TEXTSTYLE_REV = "rev";
constexpr const char* TEXTSTYLE_FILE = "file";
constexpr const char* TEXTSTYLE_BIN = "TextStyles.bin";

MetaTextStylesContainer::MetaTextStylesContainer(DmDocument* pDoc)
	: m_pDocument(pDoc)
	, m_strActiveStyle("Standard")
{
}

void MetaTextStylesContainer::saveXML(Writer& wrt) const
{
	std::vector<PAIR> revs;
	DmTextStyle::getRevId(revs);

	auto activeName = m_pDocument->getTextStyleTable()->getActive()->getName().toStdString();

	// Insert Texts item
	wrt.Stream() << wrt.ind() << "<TextStyles" << " levels=\"" << revs.size() << "\"" << " Count=\"" << m_textStyleList.size() << "\"" << " active=\"" << activeName << "\"" << " file= \"" << wrt.addFile(TEXTSTYLE_BIN, this) << "\">" << std::endl;

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
	wrt.Stream() << wrt.ind() << "</" << TEXTSTYLES << ">" << std::endl;
}

void MetaTextStylesContainer::restoreXML(XMLReader& reader)
{
	reader.readElement(TEXTSTYLES);

	m_strActiveStyle = QString::fromStdString(reader.getAttribute("active"));

	std::string file(reader.getAttribute(TEXTSTYLE_FILE));
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

	reader.readEndElement(TEXTSTYLES);
}

unsigned int MetaTextStylesContainer::getMemSize() const
{
	return 0;
}

void MetaTextStylesContainer::saveStream(OutputStream& wrt) const
{
	for (auto& e : m_textStyleList)
	{
		e->saveStream(wrt);
	}
}

void MetaTextStylesContainer::restoreStream(InputStream& rdr)
{
	while (!rdr.end())
	{
		// read all textStyles
		auto iTextStyle = new DmTextStyle();
		iTextStyle->restoreStream(rdr, m_revs);
		m_pDocument->getTextStyleTable()->add_direct(iTextStyle);
	}
	auto textStyle = m_pDocument->getTextStyleTable()->find(m_strActiveStyle);
	m_pDocument->getTextStyleTable()->activate_direct(textStyle);
}

void MetaTextStylesContainer::setTextStyles(std::vector<DmTextStyle*>& styles)
{
	m_textStyleList = styles;
}
