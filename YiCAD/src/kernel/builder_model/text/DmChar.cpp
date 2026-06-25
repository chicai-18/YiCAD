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


/// @file DmChar.cpp
/// @brief DmChar 文字字符实体类的实现

#include "TextConsts.h"
#include "DmChar.h"
#include "DmCharTemplate.h"
#include "DmLine.h"
#include "DmPolyline.h"

DmChar::DmChar(DmCharTemplate* fontChar, DmEntity* parent)
	:DmEntity(parent) 
	, m_pCharTemplate(fontChar)
	, m_name("")
	, m_dHeight(1.0)
	, m_dWidth(1.0)
	, m_dWidthFactor(1.0)
	, m_dSlashAngle(0.0)
	, m_dNominalHeight(1.0)
	, m_isBold(false)
	, m_isItalic(false)
	, m_hasOverline(false)
	, m_hasStrikethrough(false)
	, m_hasUnderline(false)
	, m_overline(nullptr)
	, m_underline(nullptr)
	, m_strikethrough(nullptr)
	, m_pos(0.0,0.0)
{
	if (m_pCharTemplate)
	{
		m_name = m_pCharTemplate->getName();
	}
}

DmEntity* DmChar::clone() const
{
	DmChar* t = new DmChar(*this);
	t->entities.clear();

	// 设置下划线等
	for (auto e : entities)
	{
		DmEntity* newEnt = e->clone();
		if (e == m_overline)
		{
			t->m_overline = static_cast<DmLine*>(newEnt);
		}
		if (e == m_underline)
		{
			t->m_underline = static_cast<DmLine*>(newEnt);
		}
		if (e == m_strikethrough)
		{
			t->m_strikethrough = static_cast<DmLine*>(newEnt);
		}
		t->entities.emplace_back(newEnt);
	}
	return t;
}

DmChar::~DmChar()
{
	for (auto e : entities)
	{
		delete e;
	}
	entities.clear();
}

DM::EntityType DmChar::getEntityType() const
{
	return DM::EntityChar;
}

bool DmChar::isWriteSpace() const
{
	static std::set<QString> whiteSpaces = {
	QString(0x20) /*空格*/,
	"\n",
	"\t"
	};
	auto it = whiteSpaces.find(m_name);
	if (whiteSpaces.end() == it)	
	{
		return false;
	}
	return true;
}

void DmChar::addEntity(DmEntity* e)
{
	if (e)
	{
		entities.emplace_back(e);
	}
}

void DmChar::removeEntity(DmEntity* e)
{
	if (e)
	{
		auto it = std::find(entities.begin(), entities.end(), e);
		if (it != entities.end())
		{
			delete* it;
			entities.erase(it);
		}
	}
}

QString DmChar::getName() const
{
	return m_name;
}

void DmChar::setName(const QString& name)
{
	m_name = name;
}

double DmChar::getWidth() const
{
	return m_dWidth;
}

void DmChar::setWidth(const double width)
{
	m_dWidth = width;
}

double DmChar::getHeight() const
{
	return m_dHeight;
}

void DmChar::setHeight(const double height)
{
	m_dHeight = height;
}

double DmChar::getAscender() const
{
	return m_pCharTemplate->getAscenderFactor() * getHeight();
}

double DmChar::getDescender() const
{
	return  (1.0 - m_pCharTemplate->getAscenderFactor()) * getHeight();
}

double DmChar::getWidthFactor() const
{
	return m_dWidthFactor;
}

void DmChar::setWidthFactor(const double widthFactor)
{
	m_dWidthFactor = widthFactor;
}

double DmChar::getSlashAngle() const
{
	return m_dSlashAngle;
}

void DmChar::setSlashAngle(const double slashAngle)
{
	m_dSlashAngle = slashAngle;
}

double DmChar::getNominalHeight() const
{
	return m_dNominalHeight;
}

void DmChar::setNominalHeight(const double nominalHeight)
{
	m_dNominalHeight = nominalHeight;
}

DmVector DmChar::getPosition() const
{
	return m_pos;
}

void DmChar::setPosition(const DmVector& pos)
{
	m_pos = pos;
}

bool DmChar::hasOverline() const
{
	return m_hasOverline;
}

DmLine* DmChar::getOverline() const
{
	return m_overline;
}

void DmChar::setOverline(DmLine* overline)
{
	m_overline = overline;
}

void DmChar::addOverline()
{
	if (m_overline)
	{
		return;
	}
	if (!isNewLine())	//换行不能有真实的上划线
	{
		DmVector p1(m_pos.x - CHARGAPFACTOR * m_dNominalHeight, m_pos.y + m_dNominalHeight * 1.2);
		DmVector p2(m_pos.x + m_dWidth + CHARGAPFACTOR * m_dNominalHeight, m_pos.y + m_dNominalHeight * 1.2);
		DmLine* line = new DmLine(this, p1, p2);
		DmPen pen(DmColor(DM::FlagByBlock), DM::LineWidth::Width00, DmLineTypeTable::Continuous);
		line->setPen(pen);

		addEntity(line);
		m_overline = line;
	}
	m_hasOverline = true;
}

void DmChar::removeOverline()
{
	if (!m_overline)
	{
		return;
	}
	removeEntity(m_overline);
	m_overline = nullptr;
	m_hasOverline = false;
}

bool DmChar::hasUnderline() const
{
	return m_hasUnderline;
}

DmLine* DmChar::getUnderline() const
{
	return m_underline;
}

void DmChar::setUnderline(DmLine* underline)
{
	m_underline = underline;
}

void DmChar::addUnderline()
{
	if (m_underline)
	{
		return;
	}
	if (!isNewLine())
	{
		DmVector p1(m_pos.x - CHARGAPFACTOR * m_dNominalHeight, m_pos.y - m_dNominalHeight * 0.3);
		DmVector p2(m_pos.x + m_dWidth + CHARGAPFACTOR * m_dNominalHeight, m_pos.y - m_dNominalHeight * 0.3);
		DmLine* line = new DmLine(this, p1, p2);
		DmPen pen(DmColor(DM::FlagByBlock), DM::LineWidth::Width00, DmLineTypeTable::Continuous);
		line->setPen(pen);

		addEntity(line);
		m_underline = line;
	}
	m_hasUnderline = true;
}

void DmChar::removeUnderline()
{
	if (!m_underline)
	{
		return;
	}
	removeEntity(m_underline);
	m_underline = nullptr;
	m_hasUnderline = false;
}

bool DmChar::hasStrikethrough() const
{
	return m_hasStrikethrough;
}

DmLine* DmChar::getStrikethrough() const
{
	return m_strikethrough;
}

void DmChar::setStrikethrough(DmLine* strikethrough)
{
	m_strikethrough = strikethrough;
}

void DmChar::addStrikethrough()
{
	if (m_strikethrough)
	{
		return;
	}
	if (!isNewLine())
	{
		DmVector p1(m_pos.x - CHARGAPFACTOR * m_dNominalHeight, m_pos.y + m_dNominalHeight * 0.5);
		DmVector p2(m_pos.x + m_dWidth + CHARGAPFACTOR * m_dNominalHeight, m_pos.y + m_dNominalHeight * 0.5);
		DmLine* line = new DmLine(this, p1, p2);
		DmPen pen(DmColor(DM::FlagByBlock), DM::LineWidth::Width00, DmLineTypeTable::Continuous);
		line->setPen(pen);

		addEntity(line);
		m_strikethrough = line;
	}
	m_hasStrikethrough = true;
}

void DmChar::removeStrikethrough()
{
	if (!m_strikethrough)
	{
		return;
	}
	removeEntity(m_strikethrough);
	m_strikethrough = nullptr;
	m_hasStrikethrough = false;
}

bool DmChar::isBold() const
{
	return m_isBold;
}

void DmChar::setBold(bool bold)
{
	m_isBold = bold;
}

bool DmChar::isItalic() const
{
	return m_isItalic;
}

void DmChar::setItalic(bool italic)
{
	m_isItalic = italic;
}

bool DmChar::isContainer() const
{
	return false;
}

void DmChar::calculateBorders()
{
	resetBorders();
	for (auto c : entities)
	{
		c->calculateBorders();
		minV = DmVector::minimum(c->getMin(), minV);
		maxV = DmVector::maximum(c->getMax(), maxV);
	}
}

double DmChar::getDistanceToPoint(const DmVector& coord, DmEntity** entity, DM::ResolveLevel level) const
{
	if (coord.x < maxV.x && coord.y < maxV.y && coord.x > minV.x && coord.y > minV.y)
	{
		double minDist = DM_MAXDOUBLE;
		double curDist = 0.0;
		for (auto e : entities)
		{
			curDist = e->getDistanceToPoint(coord);
			if (curDist < minDist)
			{
				minDist = curDist;
			}
		}
		return minDist;
	}
	return DM_MAXDOUBLE;
}

DmVector DmChar::getNearestEndpoint(const DmVector& coord, double* dist) const
{
	return DmVector(false);
}

DmVector DmChar::getNearestPointOnEntity(const DmVector&, bool onEntity, double* dist, DmEntity** entity) const
{
	return DmVector(false);
}

DmVector DmChar::getNearestCenter(const DmVector& coord, double* dist) const
{
	return DmVector(false);
}

DmVector DmChar::getNearestMiddle(const DmVector& coord, double* dist, int middlePoints) const
{
	return DmVector(false);
}

void DmChar::setVisible(bool v)
{
	DmEntity::setVisible(v);
	for (auto e : entities)
	{
		e->setVisible(v);
	}
}

bool DmChar::setSelected(bool select)
{
	if (DmEntity::setSelected(select))
	{
		for (auto e : entities)
		{
			if (e->isVisible())
			{
				e->setSelected(select);
			}
		}
		return true;
	}
	else
	{
		return false;
	}
}

void DmChar::setHighlighted(bool highlight)
{
	DmEntity::setHighlighted(highlight);
	for (auto e : entities)
	{
		if (e->isVisible())
		{
			e->setHighlighted(highlight);
		}
	}
}

void DmChar::rotate(const DmVector& center, const DmVector& angleVector)
{
	for (auto e : entities)
	{
		e->rotate(center, angleVector);
	}
	calculateBorders();
}

void DmChar::mirror(const DmVector& axisPoint1, const DmVector& axisPoint2)
{
	if (axisPoint1.distanceTo(axisPoint2) > DM_TOLERANCE)
	{
		for (auto e : entities)
		{
			e->mirror(axisPoint1, axisPoint2);
		}
	}
	calculateBorders();
}

std::list<DmEntity*> DmChar::getSubEntities() const
{
	std::list<DmEntity*> subEnts;
	for (auto e : entities)
	{
		if (e->getEntityType() == DM::EntityPolyline)
		{
			std::list<DmEntity*> polySubEnts = ((DmPolyline*)e)->getSubEntities();
			subEnts.splice(subEnts.end(), polySubEnts);
		}
		else
		{
			subEnts.emplace_back(e);
		}
	}
	return subEnts;
}

DmCharTemplate* DmChar::getCharTemplate() const
{
	return m_pCharTemplate;
}

void DmChar::setCharTemplate(DmCharTemplate* templ)
{
	m_pCharTemplate = templ;
}

bool DmChar::isNewLine() const
{
	return m_name == "\n";
}

bool DmChar::isSpace() const
{
	return m_name == QString(0x20);
}

bool DmChar::isTab() const
{
	return m_name == "\t";
}

void DmChar::scale(const DmVector& center, const DmVector& factor)
{
	for (auto e : entities)
	{
		e->scale(center, factor);
	}
	m_dWidth *= factor.x;
	m_dHeight *= factor.y;
	m_dNominalHeight *= factor.y;
	m_pos.scale(center, factor);
	calculateBorders();
}

void DmChar::move(const DmVector& offset)
{
	for (auto e : entities)
	{
		e->move(offset);
	}
	m_pos.move(offset);
	moveBorders(offset);
}

void DmChar::moveTo(const DmVector& newPos)
{
	DmVector offset = newPos - m_pos;
	move(offset);
}
