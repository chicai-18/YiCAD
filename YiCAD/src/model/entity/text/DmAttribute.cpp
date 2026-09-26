/**
 * Copyright (c) 2011-2018 by Andrew Mustun. All rights reserved.
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is part of the YiCAD project.
 *
 * YiCAD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YiCAD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */


/// @file DmAttribute.cpp
/// @brief DmAttribute 块属性实体类的实现

#include "DmAttribute.h"

TYPESYSTEM_SOURCE(DmAttribute, DmEntity, 0);

DmAttribute::DmAttribute(DmEntity* parent, const TextData& textData, const AttributeData& attrData)
	:DmText(parent, textData), data(attrData)
{
}

DmEntity* DmAttribute::clone() const
{
	DmAttribute* t = new DmAttribute(*this);
	t->update();
	return t;
}

DM::EntityType DmAttribute::getEntityType() const
{
	return DM::EntityAttribute;
}

AttributeData& DmAttribute::getAttributeDataRef()
{
	return data;
}

AttributeData DmAttribute::getAttributeData() const
{
	return data;
}

QString DmAttribute::getTag() const
{
	return data.getTag();
}

void DmAttribute::setTag(const QString& tag)
{
	data.setTag(tag);
}

void DmAttribute::saveStream(OutputStream& wrt) const
{
	DmText::saveStream(wrt);

	auto tag = data.getTag().toStdString();

	wrt << (std::string)tag;
}

void DmAttribute::restoreStream(InputStream& reader, const std::vector<PAIR>& revs)
{
	DmText::restoreStream(reader, revs);

	int fileRev = getRevisionId("DmAttribute", revs);
	if (revId > fileRev)
	{
		// 老文件格式
		restoreStreamWithRev(reader, fileRev);
	}
	else
	{
		std::string tag;
		reader >> (std::string&)tag;

		data.setTag(QString::fromStdString(tag));
		update();
	}
}

void DmAttribute::restoreStreamWithRev(InputStream& rdr, int rev)
{
	if (rev == 0)
	{
	}
	else //big change, e.g. change supper class of DmAttribute
	{
		//step1.
		// read all legacy data one by one
	}
}
