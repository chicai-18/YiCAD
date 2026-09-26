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


/// @file DmVariable.h
/// @brief 变量类，支持int/double/string/vector类型

#ifndef DMVARIABLE_H
#define DMVARIABLE_H

#include <QString>

#include "Datamodel.h"
#include "DmVector.h"

/// @brief A variable of type int, double, string or vector.
///	The variable also contains its type and an int code which can identify its contents in any way.
class DmVariable
{
	struct DmVariableContents
	{
		QString s;
		int i;
		double d;
		DmVector v;
	};

public:
	DmVariable() = default;
	DmVariable(const DmVector& v, int c)
		: m_iCode(c)
	{
		setVector(v);
	}

	DmVariable(const QString& v, int c) 
		: m_iCode(c)
	{
		setString(v);
	}

	DmVariable(int v, int c) 
		: m_iCode(c)
	{
		setInt(v);
	}

	DmVariable(double v, int c) 
		: m_iCode(c)
	{
		setDouble(v);
	}

	void setString(const QString& str)
	{
		m_contents.s = str;
		m_eType = DM::VariableString;
	}

	void setInt(int i)
	{
		m_contents.i = i;
		m_eType = DM::VariableInt;
	}

	void setDouble(double d)
	{
		m_contents.d = d;
		m_eType = DM::VariableDouble;
	}

	void setVector(const DmVector& v)
	{
		m_contents.v = v;
		m_eType = DM::VariableVector;
	}

	QString getString() const
	{
		return m_contents.s;
	}

	int getInt() const
	{
		return m_contents.i;
	}

	double getDouble() const
	{
		return m_contents.d;
	}

	DmVector getVector() const
	{
		return m_contents.v;
	}

	DM::VariableType getType() const
	{
		return m_eType;
	}

	int getCode() const
	{
		return m_iCode;
	}

private:
	DmVariableContents	m_contents;
	DM::VariableType	m_eType = DM::VariableVoid;
	int					m_iCode = 0;
};

#endif
