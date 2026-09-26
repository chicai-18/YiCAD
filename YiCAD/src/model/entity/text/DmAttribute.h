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


/// @file DmAttribute.h
/// @brief 块属性实体类

#ifndef DMATTRIBUTE_H
#define DMATTRIBUTE_H

#include "DmText.h"
#include "AttributeData.h"

class DmAttribute : public DmText
{
	TYPESYSTEM_HEADER();
public:
	DmAttribute() = default;
	DmAttribute(DmEntity* parent, const TextData& textData, const AttributeData& attrData);
	virtual ~DmAttribute() = default;

public:
	virtual DmEntity* clone() const override;
	virtual DM::EntityType getEntityType() const override;
	AttributeData& getAttributeDataRef();
	AttributeData getAttributeData() const;

	QString getTag() const;
	void setTag(const QString& tag);

	// persistent helper
	virtual void saveStream(OutputStream& wrt) const override;
	virtual void restoreStream(InputStream& reader, const std::vector<PAIR>& revs) override;
	virtual void restoreStreamWithRev(InputStream& rdr, int rev) override;

private:
	AttributeData data;
};
#endif	//!DMATTRIBUTE_H