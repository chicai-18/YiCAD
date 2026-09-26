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


/// @file DmAttributeDefinition.cpp
/// @brief DmAttributeDefinition 块属性定义类的实现

#include "DmAttributeDefinition.h"
#include "Debug.h"

TYPESYSTEM_SOURCE(DmAttributeDefinition, DmEntity, 0);

DmAttributeDefinition::DmAttributeDefinition(DmEntity* parent, const TextData& textData, const AttributeDefinitionData& attrData)
	: DmText(parent, textData)
	, data(attrData)
{
}

DmEntity* DmAttributeDefinition::clone() const
{
	DmAttributeDefinition* t = new DmAttributeDefinition(*this);
	t->update();
	return t;
}

DM::EntityType DmAttributeDefinition::getEntityType() const
{
	return DM::EntityAttributeDefinition;
}

AttributeDefinitionData& DmAttributeDefinition::getAttributeDataRef()
{
	return data;
}

AttributeDefinitionData DmAttributeDefinition::getAttributeData() const
{
	return data;
}

void DmAttributeDefinition::update()
{
	clear();

	QString text = getTag().toUpper();
	addEntitiesOfText(text);
}

QString DmAttributeDefinition::getTag() const
{
	return data.getTag();
}

void DmAttributeDefinition::setTag(const QString& tag)
{
	data.setTag(tag);
}

QString DmAttributeDefinition::getPrompt() const
{
	return data.getPrompt();
}

void DmAttributeDefinition::setPrompt(const QString& prompt)
{
	data.setPrompt(prompt);
}

void DmAttributeDefinition::saveStream(OutputStream& wrt) const
{
	DmText::saveStream(wrt);

	auto tag = data.getTag().toStdString();
	auto prompt = data.getPrompt().toStdString();

	wrt << (std::string)tag << (std::string)prompt;
}

void DmAttributeDefinition::restoreStream(InputStream& reader, const std::vector<PAIR>& revs)
{
	int fileRev = getRevisionId("DmAttributeDefinition", revs);
	if (revId > fileRev)
	{
        DmText::restoreStream(reader, revs);
		// 老文件格式
		restoreStreamWithRev(reader, fileRev);
	}
	else
	{
        restoreStream(reader);
	}
}

void DmAttributeDefinition::restoreStreamWithRev(InputStream& rdr, int rev)
{
	if (rev == 0)
	{
	}
	else //big change, e.g. change supper class of DmAttributeDefinition
	{
		//step1.
		// read all legacy data one by one
	}
}

void DmAttributeDefinition::restoreStream(InputStream& rdr)
{
    DmText::restoreStream(rdr);

    std::string tag;
    std::string prompt;
    rdr >> (std::string&)tag >> (std::string&)prompt;
    data.setTag(QString::fromStdString(tag));
    data.setPrompt(QString::fromStdString(prompt));
    update();
}
