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


/// @file DmAttributeDefinition.h
/// @brief 块属性定义类

#ifndef DMATTRIBUTEDEFINITION
#define DMATTRIBUTEDEFINITION

#include "DmText.h"
#include "AttributeDefinitionData.h"

class DmAttributeDefinition : public DmText
{
	TYPESYSTEM_HEADER();
public:
	DmAttributeDefinition() = default;
	DmAttributeDefinition(DmEntity* parent, const TextData& textData, const AttributeDefinitionData& attrData);
	virtual ~DmAttributeDefinition() = default;

public:
	virtual DmEntity* clone() const override;
	virtual DM::EntityType getEntityType() const override;
	AttributeDefinitionData& getAttributeDataRef();
	AttributeDefinitionData getAttributeData() const;

	void update() override;

	QString getTag() const;
	void setTag(const QString& tag);

	QString getPrompt() const;
	void setPrompt(const QString& prompt);

	// persistent helper
	virtual void saveStream(OutputStream& wrt) const override;
	virtual void restoreStream(InputStream& reader, const std::vector<PAIR>& revs) override;
	virtual void restoreStreamWithRev(InputStream& rdr, int rev) override;
    virtual void restoreStream(InputStream& rdr) override;

private:
	AttributeDefinitionData data;
};

#endif // !DMATTRIBUTEDEFINITION
