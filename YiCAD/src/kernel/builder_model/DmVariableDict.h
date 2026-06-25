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


/// @file DmVariableDict.h
/// @brief 变量字典类，以键值对方式存储变量

#ifndef DMVARIABLEDICT_H
#define DMVARIABLEDICT_H

#include <QHash>

#include "DmVariable.h"

class DmVector;
class QString;

/// @brief Dictionary of variables. The variables are stored as key/value pairs (string/string).
class DmVariableDict
{
public:
	DmVariableDict() = default;

	void clear();

	/// @return Number of variables available.
	int count() const;

	void add(const QString& key, const DmVector& value, int code);
	void add(const QString& key, const QString& value, int code);
	void add(const QString& key, int value, int code);
	void add(const QString& key, double value, int code);

	DmVector getVector(const QString& key, const DmVector& def) const;
	QString getString(const QString& key, const QString& def) const;
	int getInt(const QString& key, int def) const;
	double getDouble(const QString& key, double def) const;

	void remove(const QString& key);

	QHash<QString, DmVariable> const& getVariableDict() const;
	QHash<QString, DmVariable>& getVariableDict();

private:
	QHash<QString, DmVariable> m_variables;
};

#endif
