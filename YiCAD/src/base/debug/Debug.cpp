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

/// @file Debug.cpp
/// @brief 调试工具类实现

#include "Debug.h"

#include <iostream>
#include <cstdio>
#include <cstdarg>
#include <QString>

#include <QDateTime>
#include <QDebug>

Debug* Debug::uniqueInstance = nullptr;
void debugHeader(char const* file, char const* func, int line)
{
	std::cout << file << " : " << func << " : line " << line << std::endl;
}

// Gets the one and only Debug instance (creates a new one on first call only) 
// @return Pointer to the single instance of this singleton class
Debug* Debug::instance()
{
	if (!uniqueInstance)
	{
		QDateTime now = QDateTime::currentDateTime();
		QString nowStr;
		nowStr = now.toString("yyyyMMdd_hhmmss");
		QString fName = QString("debug_%1.log").arg(nowStr);

		uniqueInstance = new Debug;
		uniqueInstance->stream = stderr;
	}
	return uniqueInstance;
}

void Debug::deleteInstance()
{
	if (uniqueInstance)
	{
		fclose(uniqueInstance->stream);
		delete uniqueInstance;
	}
}

// Constructor setting the default debug level.
Debug::Debug()
{
#ifdef NDEBUG
	debugLevel = D_NOTHING;
#else
	debugLevel = D_DEBUGGING;
#endif // NDEBUG

}

// Sets the debugging level.
void Debug::setLevel(DebugLevel level)
{
	if (debugLevel == level)
	{
		return;
	}
	debugLevel = level;
	print(D_NOTHING, "DEBUG::setLevel(%d)", level);
	print(D_CRITICAL, "DEBUG: Critical");
	print(D_ERROR, "DEBUG: Errors");
	print(D_WARNING, "DEBUG: Warnings");
	print(D_NOTICE, "DEBUG: Notice");
	print(D_INFORMATIONAL, "DEBUG: Informational");
	print(D_DEBUGGING, "DEBUG: Debugging");
}

// Gets the current debugging level.
Debug::DebugLevel Debug::getLevel()
{
	return debugLevel;
}

// Prints the given message to stdout.
void Debug::print(const char* format ...)
{
	if (debugLevel == D_DEBUGGING)
	{
		va_list ap;
		va_start(ap, format);
		vfprintf(stream, format, ap);
		fprintf(stream, "\n");
		va_end(ap);
		fflush(stream);
	}
}

// Prints the given message to stdout if the current debug level is lower then the given level
// @param level Debug level.
void Debug::print(DebugLevel level, const char* format ...)
{
	if (debugLevel >= level)
	{
		va_list ap;
		va_start(ap, format);
		vfprintf(stream, format, ap);
		fprintf(stream, "\n");
		va_end(ap);
		fflush(stream);
	}
}

// Prints a time stamp in the format yyyyMMdd_hhmmss.
void Debug::timestamp()
{
	QDateTime now = QDateTime::currentDateTime();
	QString nowStr;

	nowStr = now.toString("yyyyMMdd_hh:mm:ss:zzz ");
	fprintf(stream, "%s", nowStr.toLatin1().data());
	fprintf(stream, "\n");
	fflush(stream);
}

void Debug::setStream(FILE* s)
{
	stream = s;
}

// Prints the unicode for every character in the given string.
void Debug::printUnicode(const QString& text)
{
	for (auto const& v : text)
	{
		print("[%X] %c", v.unicode(), v.toLatin1());
	}
}
