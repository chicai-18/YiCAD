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

/// @file MetaLineTypes.cpp
/// @brief 线型实体序列化容器实现文件

#include "MetaLineTypes.h"

#include "DmLineType.h"
#include "DmLineTypeTable.h"
#include "DmDocument.h"
#include "DmEntityContainer.h"

#include "Reader.h"
#include "Writer.h"

constexpr const char* LINETYPES = "LineTypes";

MetaLineTypesContainer::MetaLineTypesContainer(DmDocument* pDoc)
	: m_pDocument(pDoc)
{
}

void MetaLineTypesContainer::saveXML(Writer& wrt) const
{
	wrt.incInd();

	auto lineTypes = m_pDocument->getLineTypeTable();
	auto active = lineTypes->getActive();

	// Insert lineTypes item
	for (auto i = lineTypes->begin(); i != lineTypes->end(); ++i)
	{
		DmLineType* lineType(*i);
		wrt.Stream() << wrt.ind() << "<LineType" << " name=\"" << lineType->getLineTypeName().toStdString() << "\"" << " active=\"" << (bool)(active == lineType) << "\">" << std::endl;
		auto desp = lineType->getLineTypeDesp();
		wrt.incInd();

		// dash
		wrt.Stream() << wrt.ind() << "<LineTypeDesp" << " desp=\"" << lineType->getLineTypeDesp().toStdString() << "\"/>" << std::endl;

		// data
		auto datas = lineType->getLineTypeData();
		wrt.Stream() << wrt.ind() << "<LineTypeData" << " Count=\"" << datas.size() << "\">" << std::endl;
		wrt.incInd();
		for (auto& data : datas)
		{
			wrt.Stream() << wrt.ind() << "<data" << " value=\"" << data << "\"/>" << std::endl;
		}
		wrt.decInd();
		wrt.Stream() << wrt.ind() << "</LineTypeData>" << std::endl;

		wrt.decInd();

		wrt.Stream() << wrt.ind() << "</LineType>" << std::endl;
	}

	wrt.decInd();
}

void MetaLineTypesContainer::restoreXML(XMLReader& reader)
{
	// restore lineTypes
	auto iCount = (size_t)reader.getAttributeAsInteger("Count");
	for (size_t idx = 0; idx < iCount; idx++)
	{
		reader.readElement("LineType");

		auto name = QString::fromStdString(reader.getAttribute("name"));
		bool active = reader.hasAttribute("active");

		if ((name == "ByLayer" || name == "ByBlock" || name == "Continuous") && m_pDocument->getLineTypeTable()->find(name))
		{
			reader.readEndElement("LineType");
			continue;
		}

		reader.readElement("LineTypeDesp");
		QString desp = QString::fromStdString(reader.getAttribute("desp"));
		QString outward = desp.replace(QRegExp("[a-zA-Z0-9()]"), "");

		reader.readElement("LineTypeData");
		auto iDataCount = (size_t)reader.getAttributeAsInteger("Count");
		auto data = std::vector<double>();
		for (size_t daidx = 0; daidx < iDataCount; daidx++)
		{
			reader.readElement("data");
			data.emplace_back(reader.getAttributeAsFloat("value"));
		}
		reader.readEndElement("LineTypeData");

		reader.readEndElement("LineType");

		DmLineType* linetype = new DmLineType(name);
		linetype->setLineTypeData(data);
		linetype->setLineTypeDesp(desp);
		linetype->setLineTypeName(name);
		linetype->setLineTypeOutWard(outward.trimmed());

		m_pDocument->getLineTypeTable()->add_direct(linetype);

		if (active)
		{
			m_pDocument->getLineTypeTable()->activate_direct(linetype);
		}
	}
}

unsigned int MetaLineTypesContainer::getMemSize() const
{
	return 0;
}
