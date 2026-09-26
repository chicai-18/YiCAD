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
/// @file MetaBlockTableRecords.cpp
/// @brief 块表记录元数据容器实现文件

#include "MetaBlockTableRecords.h"

#include "DmBlock.h"
#include "DmBlockTable.h"
#include "DmDocument.h"
#include "DmEntityContainer.h"
#include "DmLine.h"
#include "DmCircle.h"
#include "DmPoint.h"
#include "DmPolyline.h"
#include "DmArc.h"
#include "DmEllipse.h"
#include "DmSolid.h"
#include "DmSpline.h"
#include "DmRay.h"
#include "DmXline.h"
#include "DmBlockReference.h"

#include "Reader.h"
#include "Writer.h"

constexpr const char* BLOCKS = "Blocks";
constexpr const char* BLOCK_REV = "rev";
constexpr const char* BLOCK_FILE = "file";
constexpr const char* BLOCK_BIN = "Blocks.bin";

MetaBlockTableRecordsContainer::MetaBlockTableRecordsContainer(DmDocument* pDoc)
	: m_pDocument(pDoc)
	, m_Count(0)
{
}

void MetaBlockTableRecordsContainer::saveXML(Writer& wrt) const
{
	std::vector<PAIR> revs;
	DmBlock::getRevId(revs);

	//Insert BlockTableRecords item
	wrt.Stream() << wrt.ind() << "<Blocks" << " levels=\"" << revs.size() << "\"" << " Count=\"" << m_pBlocks.size() << "\"" << " file= \"" << wrt.addFile(BLOCK_BIN, this) << "\">" << std::endl;

	//write type and revision ids
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
	wrt.Stream() << wrt.ind() << "</" << BLOCKS << ">" << std::endl;
}

void MetaBlockTableRecordsContainer::restoreXML(XMLReader& reader)
{
	reader.readElement(BLOCKS);

	m_Count = (int)reader.getAttributeAsInteger("Count");

	std::string file(reader.getAttribute(BLOCK_FILE));
	//restore files
	if (!file.empty())
	{
		reader.addFile(file.c_str(), this);
	}

	//restore levels
	auto ilevels = (size_t)reader.getAttributeAsInteger("levels");
	for (size_t idx = 0; idx < ilevels; idx++)
	{
		reader.readElement("level");
		auto itype = reader.getAttribute("name");
		auto irev = reader.getAttributeAsInteger("id");
		m_revs.push_back(std::make_pair(itype, irev));
	}

	reader.readEndElement(BLOCKS);
}

unsigned int MetaBlockTableRecordsContainer::getMemSize() const
{
	return 0;
}

void MetaBlockTableRecordsContainer::saveStream(OutputStream& wrt) const
{
	for (auto& block : m_pBlocks)
	{
		block->saveStream(wrt);
	}
}

void MetaBlockTableRecordsContainer::restoreStream(InputStream& rdr)
{
	int num = 0;
	while (!rdr.end())
	{
		if (num >= m_Count)
		{
			break;
		}

		// read all blocks
		auto iBlock = new DmBlock();
		iBlock->setDocument(m_pDocument);
		iBlock->restoreStream(rdr, m_revs);
		m_pDocument->getBlockTable()->add_direct(iBlock);

		num++;
	}
}

void MetaBlockTableRecordsContainer::setBlockList(std::vector<DmBlock*>& pBlockList)
{
	m_pBlocks = pBlockList;
}
