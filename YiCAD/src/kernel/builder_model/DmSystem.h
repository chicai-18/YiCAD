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


/// @file DmSystem.h
/// @brief 系统管理单例类，提供文件路径、字体、翻译、导入导出格式等功能

#ifndef DMSYSTEM_H
#define DMSYSTEM_H

#include <QDir>
#include <QList>
#include <QMap>
#include <QSharedPointer>

#include "Debug.h"

class QTranslator;

// TODO: 考虑改为 constexpr 函数
#define DMSYSTEM DmSystem::instance()

// Class for some system methods such as file system operations.
// Implemented as singleton. Use init to Initialize the class before you use it.
class DmSystem
{
protected:
	DmSystem();
	~DmSystem();

public:
	/// @return Instance to the unique system object.
	static DmSystem* instance();

	void deleteDmSystem();

	void init(const QString& appName, const QString& appVersion, const QString& appDirName, const QString& appDir = "");
	//void initAllLanguagesList();
	/// @brief 为IO模块注册所有实体对象
	void entityInitialize();

	bool checkInit();
	bool createPaths(const QString& p);

	/// @return Users home directory.
	QString getHomeDir();

	/// @return Current directory.
	QString getCurrentDir();

	/// @return Application directory.
	QString getAppDir();

	/// @return Application Data directory.
	QString getAppDataDir();

	/// @brief 获得全部shx字体，系统字体路径
	QStringList getFontFiles();

	/// @brief 递归获得字体文件路径
	void getSysFontsRecursive(const QString& dir, QStringList& fontPaths);

	/// @return A list of absolute paths to all hatch pattern files found.
	QStringList getPatternList();

	/// @return A list of absolute paths to all line type files found.
	QStringList getLineTypesList();

	/// @return A list of absolute paths to all script files found.
	QStringList getScriptList();

	/// @return A list of absolute paths to all machine configuration files found.
	QStringList getMachineList();

	/// @return Absolute path to the documentation.
	QString getDocPath();

	/// @return The application name.
	QString getAppName();

	/// @return The application version.
	QString getAppVersion();

	QStringList getFileList(const QString& subDirectory, const QString& fileExtension);

	QStringList getDirectoryList(const QString& subDirectory);

	void loadTranslation(const QString& lang);

	/// @brief 获取导入格式集合
	QMap<QString, QStringList> getImportTypes() const;
	/// @brief 获取指定格式的导入文件集合
	QStringList getImportFormatTypes(const QString& type) const;
	/// @brief 设置导入格式集合
	void setImportTypes(const QMap<QString, QStringList>& formatTypes);
	/// @brief 新增导入格式
	void addImportFormatType(const QString& key, const QString& formatType);
	
	/// @brief 获取导出格式集合
	QMap<QString, QStringList> getExportTypes() const;
	/// @brief 获取指定格式的导入文件集合
	QStringList getExportFormatTypes(const QString& type) const;
	/// @brief 设置导出格式集合
	void setExportTypes(const QMap<QString, QStringList>& formatTypes);
	/// @brief 新增导出格式
	void addExportFormatType(const QString& key, const QString& formatType);

	/// @brief 获取当前格式
	QString getCurrentFormatType() const;
	/// @brief 设置当前格式
	void setCurrentFormatType(const QString& formatType);

protected:
	static DmSystem*					m_pUniqueInstance;

	QString								appName;
	QString								appVersion;
	QString								appDirName;
	QString								appDir;

	QStringList							m_fontFiles;				//字体文件全路径，包括系统字体，shx字体
	bool								m_bFontFilesSet = false;	//字体文件是否已扫描过
	bool								initialized;

	QTranslator*						m_pTranslatorQt;
	QTranslator*						m_pTranslatorYiCAD;
	QTranslator*						m_pTranslatorPlugIns;
	QMap<QString, QStringList>			m_importFormatTypes;		// 导入格式集合
	QMap<QString, QStringList>			m_exportFormatTypes;		// 导出格式集合
	QString								m_currentFormatType;		// 当前文件格式
};

#endif

