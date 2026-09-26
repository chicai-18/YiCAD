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

/// @file Debug.h
/// @brief 调试工具类头文件

#ifndef DEBUG_H
#define DEBUG_H

#include <iosfwd>
#ifdef __hpux
#include <sys/_size_t.h>
#endif

class QString;

// print out a debug header
#define DEBUG_HEADER debugHeader(__FILE__, __func__, __LINE__);
void debugHeader(char const* file, char const* func, int line);
#define DEBUG Debug::instance()
#define DEBUG_VERBOSE \
        DEBUG_HEADER \
        Debug::instance()

/// @brief Debugging facilities.
class Debug
{
public:
	enum DebugLevel
	{
		D_NOTHING,
		D_CRITICAL,
		D_ERROR,
		D_WARNING,
		D_NOTICE,
		D_INFORMATIONAL,
		D_DEBUGGING
	};

private:
	Debug();
	Debug(const Debug&) = delete;
	Debug& operator=(const Debug&) = delete;
	Debug(Debug&&) = delete;
	Debug& operator=(Debug&&) = delete;

public:
	static Debug* instance();

	static void deleteInstance();
	void setLevel(DebugLevel level);
	DebugLevel getLevel();
	void print(DebugLevel level, const char* format...);
	void print(const char* format,...);
	void printUnicode(const QString& text);
	void timestamp();
	void setStream(FILE* s);

private:
	static Debug* uniqueInstance;

	DebugLevel debugLevel;
	FILE* stream;
};

#endif
