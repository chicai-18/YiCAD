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

/// @file MetaLayers.cpp
/// @brief 图层实体序列化容器实现文件

#include "MetaLayers.h"

#include "DmLineType.h"
#include "DmLayer.h"
#include "DmLayerTable.h"
#include "DmDocument.h"
#include "DmEntityContainer.h"

#include "Reader.h"
#include "Writer.h"

constexpr const char* LAYERS = "Layers";

MetaLayersContainer::MetaLayersContainer(DmDocument* pDoc)
	: m_pDocument(pDoc)
{
}

void MetaLayersContainer::saveXML(Writer& wrt) const
{
	wrt.incInd();

	auto layerList = m_pDocument->getLayerTable();
	auto active = layerList->getActive();

	// Insert Layer item
	for (auto i = layerList->begin(); i != layerList->end(); ++i)
	{
		DmLayer* layer(*i);

		auto layerName = layer->getName().toStdString();
		std::string id = layer->getId().asString();

		wrt.Stream() << wrt.ind() << "<Layer" << " name=\"" << encode(layerName) << "\"" << " id=\"" << encode(id) << "\""
			<< " active=\"" << (bool)(active == layer) << "\"" << " frozen=\"" << layer->isFrozen() << "\"" << " locked=\"" << layer->isLocked()
			<< "\"" << " print=\"" << layer->isPrint() << "\">" << std::endl;

		wrt.incInd();
		auto pen = layer->getPen();

		// color
		wrt.Stream() << wrt.ind() << "<Color" << " r=\"" << (int)pen.getColor().red() << "\"" << " g=\"" << (int)pen.getColor().green() << "\"" << " b=\"" << (int)pen.getColor().blue() << "\"/>" << std::endl;

		// lineType
		wrt.Stream() << wrt.ind() << "<LineType" << " name=\"" << pen.getLineType()->getLineTypeName().toStdString() << "\"/>" << std::endl;

		// lineWigth
		wrt.Stream() << wrt.ind() << "<LineWigth" << " width=\"" << (int)pen.getWidth() << "\"/>" << std::endl;
		wrt.decInd();

		wrt.Stream() << wrt.ind() << "</Layer>" << std::endl;
	}

	wrt.decInd();
}

void MetaLayersContainer::restoreXML(XMLReader& reader)
{
	// restore layers
	auto iCount = (size_t)reader.getAttributeAsInteger("Count");
	for (size_t idx = 0; idx < iCount; idx++)
	{
		reader.readElement("Layer");

		auto name = QString::fromStdString(decode(reader.getAttribute("name")));
		auto id = decode(reader.getAttribute("id"));
		bool active = (int)reader.getAttributeAsInteger("active");
		bool frozen = (int)reader.getAttributeAsInteger("frozen");
		bool locked = (int)reader.getAttributeAsInteger("locked");
		bool print = (int)reader.getAttributeAsInteger("print");

		reader.readElement("Color");
		auto r = reader.getAttributeAsInteger("r");
		auto g = reader.getAttributeAsInteger("g");
		auto b = reader.getAttributeAsInteger("b");

		reader.readElement("LineType");
		auto linetype = QString::fromStdString(reader.getAttribute("name"));

		reader.readElement("LineWigth");
		auto lineWigth = reader.getAttributeAsInteger("width");

		DmLayer* layer = new DmLayer();
		DmId ID(id);
		m_pDocument->getIdManager()->assignID(layer, ID);
		layer->setDocument(m_pDocument);
		layer->setName(name);
		layer->freeze(frozen);
		layer->lock(locked);
		layer->setPrint(print);
		DmLineType* pLinetype = m_pDocument->getLineTypeTable()->find(linetype);
		DmPen pen = DmPen(DmColor(r, g, b), (DM::LineWidth)lineWigth, pLinetype);
		layer->setPen(std::move(pen));
		m_pDocument->getLayerTable()->add_direct(layer);

		if (active)
		{
			m_pDocument->getLayerTable()->activate_direct(name);
		}

		reader.readEndElement("Layer");
	}
}

unsigned int MetaLayersContainer::getMemSize() const
{
	return 0;
}
