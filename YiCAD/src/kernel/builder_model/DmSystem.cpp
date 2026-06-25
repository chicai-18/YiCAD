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


/// @file DmSystem.cpp
/// @brief DmSystem 系统管理类的实现，包括路径查找、字体扫描、翻译加载等

#include "DmSystem.h"

#include <iostream>

#include <QMap>
#include <QApplication>
#include <QTextCodec>
#include <QTranslator>
#include <QFileInfo>
#include <QStandardPaths>
#include <qdebug.h>

#include "DmSettings.h"
#include "Datamodel.h"
#include "Debug.h"
#include "DmBlockReference.h"
#include "DmLine.h"
#include "DmCircle.h"
#include "DmPoint.h"
#include "DmPolyline.h"
#include "DmArc.h"
#include "DmEllipse.h"
#include "DmSolid.h"
#include "DmSpline.h"
#include "DmRay.h"
#include "DmXline.h"
#include "DmBlockReference.h"
#include "DmText.h"
#include "DmMText.h"
#include "DmAttributeDefinition.h"
#include "DmAttribute.h"
#include "DmDimLinear.h"
#include "DmDimAligned.h"
#include "DmDimAngular.h"
#include "DmDimRadial.h"
#include "DmDimDiametric.h"
#include "DmLeader.h"
#include "DmImage.h"
#include "DmHatch.h"
#include "DmPolyline.h"
#include "DmBlock.h"

#if defined(WIN32)
#include "dwrite.h"
///@brief 获得windows下所有字体文件路径。
// 参考：https://stackoverflow.com/questions/4577784/get-a-font-filename-based-on-font-name-and-style-bold-italic
QStringList getFontFilesForWindows()
{
	QStringList fonts_filename_list;
	HRESULT hr;

	IDWriteFactory* dwrite_factory;
	hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_ISOLATED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&dwrite_factory));
	if (FAILED(hr))
	{
		return fonts_filename_list;
	}

	IDWriteFontCollection* sys_collection;
	hr = dwrite_factory->GetSystemFontCollection(&sys_collection, false);
	if (FAILED(hr))
	{
		dwrite_factory->Release();
		return fonts_filename_list;
	}

	UINT32 familyCount = sys_collection->GetFontFamilyCount();
	if (familyCount == 0)
	{
		return fonts_filename_list;
	}
	IDWriteFontFamily* font_family;
	for (UINT32 i = 0; i < familyCount; i++)
	{
		font_family = nullptr;
		hr = sys_collection->GetFontFamily(i, &font_family);
		if (FAILED(hr))
		{
			continue;
		}
		UINT32 fontCount = font_family->GetFontCount();
		if (fontCount == 0)
		{
			continue;
		}
		for (UINT32 j = 0; j < fontCount; j++)
		{
			IDWriteFont* font = nullptr;
			hr = font_family->GetFont(j, &font);
			if (FAILED(hr))
			{
				continue;
			}
			IDWriteFontFace* font_face;
			hr = font->CreateFontFace(&font_face);
			if (FAILED(hr))
			{
				font->Release();
				continue;
			}
			UINT file_count;
			hr = font_face->GetFiles(&file_count, NULL);
			if (FAILED(hr))
			{
				font->Release();
				font_face->Release();
				continue;
			}
			IDWriteFontFile** font_files = new IDWriteFontFile * [file_count];
			hr = font_face->GetFiles(&file_count, font_files);
			if (FAILED(hr))
			{
				font->Release();
				font_face->Release();
				continue;
			}
			for (int k = 0; k < file_count; k++)
			{
				LPCVOID font_file_reference_key;
				UINT font_file_reference_key_size;
				hr = font_files[k]->GetReferenceKey(&font_file_reference_key, &font_file_reference_key_size);
				if (FAILED(hr))
				{
					font_files[k]->Release();
					continue;
				}
				IDWriteFontFileLoader* loader;
				hr = font_files[k]->GetLoader(&loader);
				if (FAILED(hr))
				{
					font_files[k]->Release();
					continue;
				}

				IDWriteLocalFontFileLoader* local_loader;
				hr = loader->QueryInterface(__uuidof(IDWriteLocalFontFileLoader), (void**)&local_loader);
				if (FAILED(hr))
				{
					loader->Release();
					font_files[k]->Release();
					continue;
				}

				UINT32 path_length;
				hr = local_loader->GetFilePathLengthFromKey(font_file_reference_key, font_file_reference_key_size, &path_length);
				if (FAILED(hr))
				{
					local_loader->Release();
					loader->Release();
					font_files[k]->Release();
					continue;
				}

				WCHAR* path = new WCHAR[path_length + 1];
				hr = local_loader->GetFilePathFromKey(font_file_reference_key, font_file_reference_key_size, path, path_length + 1);
				if (FAILED(hr))
				{
					local_loader->Release();
					loader->Release();
					font_files[k]->Release();
					continue;
				}
				QString qpath = QString::fromWCharArray(path);
                delete[] path;
				if (fonts_filename_list.indexOf(qpath) == -1)
				{
					fonts_filename_list.append(qpath);
				}
			}
            delete[] font_files;
		}
	}
	return fonts_filename_list;
}

#endif

DmSystem* DmSystem::m_pUniqueInstance = NULL;

DmSystem::DmSystem()
	: m_pTranslatorQt(nullptr)
	, m_pTranslatorYiCAD(nullptr)
	, m_pTranslatorPlugIns(nullptr)
{
	initialized = false;

	m_importFormatTypes["ycd"] = QStringList({ "Drawing Exchange YCD (*.ycd)" });		// 添加yicad默认导入格式
	m_exportFormatTypes["ycd"] = QStringList({ "Drawing Exchange YCD 2023 (*.ycd)" });	// 添加yicad默认导出格式
	m_currentFormatType = "ycd";														// 设置文件格式
}

DmSystem::~DmSystem()
{
}

DmSystem* DmSystem::instance()
{
	if (m_pUniqueInstance == NULL)
	{
		m_pUniqueInstance = new DmSystem();
	}
	return m_pUniqueInstance;
}

void DmSystem::deleteDmSystem()
{
	if (m_pTranslatorQt)
	{
		delete m_pTranslatorQt;
		m_pTranslatorQt = nullptr;
	}
	if (m_pTranslatorYiCAD)
	{
		delete m_pTranslatorYiCAD;
		m_pTranslatorYiCAD = nullptr;
	}
	if (m_pTranslatorPlugIns)
	{
		delete m_pTranslatorPlugIns;
		m_pTranslatorPlugIns = nullptr;
	}

	if (m_pUniqueInstance)
	{
		delete m_pUniqueInstance;
		m_pUniqueInstance = nullptr;
	}
}

/// @brief Initializes the system.
/// @param appName Application name (e.g. "YiCAD II")
/// @param appVersion Application version (e.g. "1.2.3")
/// @param appDirName Application directory name used for subdirectories in /usr, /etc ~/.
/// @param appDir Absolute application directory (e.g. /opt/qcad) defaults to current directory.
void DmSystem::init(const QString& appName, const QString& appVersion, const QString& appDirName, const QString& appDir)
{
	this->appName = appName;
	this->appVersion = appVersion;
	this->appDirName = appDirName;
	if (appDir == "")
	{
		this->appDir = QDir::currentPath();
	}
	else
	{
		this->appDir = appDir;
	}

	initialized = true;

	Type::initialize();
	MetaType::initialize();
	Persistence::initialize();
	DmFlags::initialize();
    DmObject::initialize();
	DmEntity::initialize();
	DmAtomicEntity::initialize();
	DmBlock::initialize();
	// 注册所有实体
	entityInitialize();
}

// Loads a different translation for the application GUI.
void DmSystem::loadTranslation(const QString& lang)
{
	QString langLower("");
	QString langUpper("");
	int i0 = lang.indexOf('_');
	if (i0 >= 2 && lang.size() - i0 >= 2)
	{
		// contains region code
		langLower = lang.left(i0) + '_' + lang.mid(i0 + 1).toLower();
		langUpper = lang.left(i0) + '_' + lang.mid(i0 + 1).toUpper();
	}
	else
	{
		langLower = lang;
		langUpper.clear();
	}
	// search in various directories for translations
	QStringList lst = getDirectoryList("qm");

	DMSETTINGS->beginGroup("/Paths");
	lst += (DMSETTINGS->readEntry("/Translations", "")).split(";", Qt::SkipEmptyParts);
	DMSETTINGS->endGroup();

	QString langFileLower = "YiCAD_" + langLower + ".qm";
	QString	langFileUpper = "YiCAD_" + langUpper + ".qm";
	QString	langPlugInsLower = "plugins_" + langLower + ".qm";
	QString	langPlugInsUpper = "plugins_" + langUpper + ".qm";
	//QString	langQtLower = "qt_" + langLower + ".qm";
	//QString	langQtUpper = "qt_" + langUpper + ".qm";
	QString	langQtLower = "qtbase_" + langLower + ".qm";
	QString	langQtUpper = "qtbase_" + langUpper + ".qm";

	QTranslator* t = new QTranslator(0);
	for (QStringList::Iterator it = lst.begin(); it != lst.end(); ++it)
	{
		if (nullptr == m_pTranslatorYiCAD)
		{
			if (t->load(langFileLower, *it) == true || (!langUpper.isEmpty() && t->load(langFileUpper, *it) == true))
			{
				m_pTranslatorYiCAD = t;
				qApp->installTranslator(m_pTranslatorYiCAD);
				t = new QTranslator(0);
			}
		}

		// load PlugIns translations
		if (nullptr == m_pTranslatorPlugIns)
		{
			if (t->load(langPlugInsLower, *it) == true || (!langUpper.isEmpty() && t->load(langPlugInsUpper, *it) == true))
			{
				m_pTranslatorPlugIns = t;
				qApp->installTranslator(m_pTranslatorPlugIns);
				t = new QTranslator(0);
			}
		}

		// load Qt standard dialog translations
		if (nullptr == m_pTranslatorQt)
		{
			if (t->load(langQtLower, *it) == true || (!langUpper.isEmpty() && t->load(langQtUpper, *it) == true))
			{
				m_pTranslatorQt = t;
				qApp->installTranslator(m_pTranslatorQt);
				t = new QTranslator(0);
			}
		}
		if (nullptr != m_pTranslatorYiCAD && nullptr != m_pTranslatorPlugIns && nullptr != m_pTranslatorQt)
		{
			break;
		}
	}
	if (nullptr != t)
	{
		delete t;
	}
}

void DmSystem::entityInitialize()
{
	DmLine::initialize();
	DmCircle::initialize();
	DmArc::initialize();
	DmSolid::initialize();
	DmRay::initialize();
	DmXline::initialize();
	DmPoint::initialize();
	DmEllipse::initialize();
	// 组合实体
	DmSpline::initialize();
	DmPolyline::initialize();
	DmBlockReference::initialize();
	DmHatch::initialize();
	DmText::initialize();
	DmMText::initialize();
	DmAttribute::initialize();
	DmAttributeDefinition::initialize();
	DmDimension::initialize();
	DmDimAligned::initialize();
	DmDimLinear::initialize();
	DmDimRadial::initialize();
	DmDimDiametric::initialize();
	DmDimAngular::initialize();
	DmLeader::initialize();

	// 图片 等
}

// Checks if the system has been initialized and prints a warning otherwise to stderr.
bool DmSystem::checkInit()
{
	return initialized;
}

// Creates all given directories in the user's home.
bool DmSystem::createPaths(const QString& directory)
{
	QDir dir;
	dir.cd(QDir::homePath());
	dir.mkpath(directory);
	return true;
}

QString DmSystem::getHomeDir()
{
	return QDir::homePath();
}

QString DmSystem::getCurrentDir()
{
	return QDir::currentPath();
}

QString DmSystem::getAppDir()
{
	return appDir;
}

/// @brief Create if not exist and return the Application data directory.
/// In OS_WIN32 "c:\documents&settings\<user>\local configuration\application data\YiCAD"
/// In OS_MAC "/Users/<user>/Library/Application Support/YiCAD"
/// In OS_LINUX "/home/<user>/.local/share/data/YiCAD"
/// @return Application data directory.
QString DmSystem::getAppDataDir()
{
	QString appData = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
	QDir dir(appData);
	if (!dir.exists())
	{
		if (!dir.mkpath(appData))
		{
			return QString();
		}
	}
	return appData;
}

QStringList DmSystem::getFontFiles()
{
	if (!m_bFontFilesSet)
	{
		//shx字体
		m_fontFiles = getFileList("fonts", "shx");

		//系统字体
#if defined(WIN32)
		// QStandardPaths::standardLocations()无法获得windows用户字体，所以单独处理
		QStringList paths = getFontFilesForWindows();
		for (auto path : paths)
		{
			QString lowerPath = path.toLower();
			if (lowerPath.endsWith(".ttf") || lowerPath.endsWith(".ttc"))
			{
				m_fontFiles.append(path);
			}
		}
#else
		QStringList dirs = QStandardPaths::standardLocations(QStandardPaths::FontsLocation);
		for (auto& dir : dirs)
		{
			//qDebug()<<"diretories: " <<dir;
			getSysFontsRecursive(dir, m_fontFiles);
		}
#endif
		m_bFontFilesSet = true;
	}
	return m_fontFiles;
}

void DmSystem::getSysFontsRecursive(const QString& dir, QStringList& fontPaths)
{
	QStringList names = QDir(dir).entryList();
	for (auto& name : names)
	{
		if (name == "." || name == "..")	//linux有时候获得.，..
			continue;
		QString path = dir + "/" + name;
		QFileInfo fi(path);
		// 文件
		if (fi.isFile())
		{
			QString lowerPath = path.toLower();
			if (lowerPath.endsWith(".ttf") || lowerPath.endsWith(".ttc"))
			{
				fontPaths.append(path);
			}
		}
		// 目录
		else
		{
			getSysFontsRecursive(path, fontPaths);
		}
	}

}

QStringList DmSystem::getPatternList()
{
	QStringList pat = getFileList("patterns", "pat");
	return pat;
	
}

QStringList DmSystem::getLineTypesList()
{
	QStringList lin = getFileList("linetypes", "lin");
	return lin;
}

QStringList DmSystem::getScriptList()
{
	return getFileList("scripts/qsa", "qs");
}

QStringList DmSystem::getMachineList()
{
	return getFileList("machines", "cxm");
}

QString DmSystem::getDocPath()
{
	QStringList lst = getDirectoryList("doc");

	if (!(lst.isEmpty()))
	{
		return lst.first();
	}
	else
	{
		return QString();
	}
}

QString DmSystem::getAppName()
{
	return appName;
}

QString DmSystem::getAppVersion()
{
	return appVersion;
}

/// @brief Searches for files in an application shared directory in the given subdirectory with the given extension.
///	@return List of the absolute paths of the files found.
QStringList DmSystem::getFileList(const QString& subDirectory, const QString& fileExtension)
{
	checkInit();

	QStringList dirList = getDirectoryList(subDirectory);

	QStringList fileList;
	QString path;
	QDir dir;

	for (QStringList::Iterator it = dirList.begin(); it != dirList.end(); ++it)
	{
		path = QString(*it);
		dir = QDir(path);

		if (dir.exists() && dir.isReadable())
		{
			QStringList files = dir.entryList(QStringList("*." + fileExtension));
			for (QStringList::Iterator it2 = files.begin(); it2 != files.end(); it2++)
			{
				fileList += path + "/" + (*it2).toLower();
			}
		}
	}

	return fileList;
}

/// @return List of all directories in subdirectory 'subDirectory' in all possible QCad directories.
QStringList DmSystem::getDirectoryList(const QString& _subDirectory)
{
	QStringList dirList;

	QString subDirectory = QDir::fromNativeSeparators(_subDirectory);

#ifdef Q_OS_MAC
	dirList.append(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/" + appDirName + "/" + subDirectory);
#endif

#ifdef Q_OS_WIN32
	dirList.append(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/" + appDirName + "/" + subDirectory);
#endif

	dirList.append(getHomeDir() + "/." + appDirName + "/" + subDirectory);

	if (!appDir.isEmpty() && appDir != "/" && appDir != getHomeDir())
	{
		if (appDir != getCurrentDir() && subDirectory != QString("plugins"))
		{
			dirList.append(appDir + "/" + subDirectory);
		}
	}

	// Ubuntu
	dirList.append("/usr/share/doc/" + appDirName + "/" + subDirectory);

	// Redhat style:
	dirList.append("/usr/share/" + appDirName + "/" + subDirectory);

#ifdef Q_OS_MAC
	// Apple uses the resource directory
	if (!appDir.isEmpty() && appDir != "/")
	{
		dirList.append(appDir + "/../Resources/" + subDirectory);
	}
#endif

#ifndef Q_OS_MAC
	// Add support directory if YiCAD is run-in-place,
	// not for Apple because it uses resources this is more for unix systems
	dirList.append(appDir + "/resources/" + subDirectory);
#endif

	// Individual directories:
	DMSETTINGS->beginGroup("/Paths");
	if (subDirectory == "fonts")
	{
		dirList += (DMSETTINGS->readEntry("/Fonts", "")).split(QRegExp("[;]"), Qt::SkipEmptyParts);
	}
	else if (subDirectory == "patterns")
	{
		dirList += (DMSETTINGS->readEntry("/Patterns", "")).split(QRegExp("[;]"), Qt::SkipEmptyParts);
	}
	else if (subDirectory.startsWith("scripts"))
	{
		dirList += (DMSETTINGS->readEntry("/Scripts", "")).split(QRegExp("[;]"), Qt::SkipEmptyParts);
	}
	else if (subDirectory.startsWith("library"))
	{
		dirList += (DMSETTINGS->readEntry("/Library", "")).split(QRegExp("[;]"), Qt::SkipEmptyParts);
	}
	else if (subDirectory.startsWith("po"))
	{
		dirList += (DMSETTINGS->readEntry("/Translations", "")).split(QRegExp("[;]"), Qt::SkipEmptyParts);
	}
	DMSETTINGS->endGroup();

	QStringList ret;

	for (QStringList::Iterator it = dirList.begin(); it != dirList.end(); ++it)
	{
		if (QFileInfo(*it).isDir())
		{
			ret += (*it);
		}
	}

	return ret;
}

QMap<QString, QStringList> DmSystem::getImportTypes() const
{
	return m_importFormatTypes;
}

QStringList DmSystem::getImportFormatTypes(const QString& type) const
{
	if (m_importFormatTypes.find(type) == m_importFormatTypes.end())
	{
		return QStringList();
	}
	else
	{
		return m_importFormatTypes[type];
	}
}

void DmSystem::setImportTypes(const QMap<QString, QStringList>& formatTypes)
{
	m_importFormatTypes = formatTypes;
}

void DmSystem::addImportFormatType(const QString& key, const QString& formatType)
{
	if (m_importFormatTypes.find(key) == m_importFormatTypes.end())
	{
		m_importFormatTypes[key] = QStringList(formatType);
	}
	else
	{
		m_importFormatTypes.find(key).value().append(formatType);
	}
}

QMap<QString, QStringList> DmSystem::getExportTypes() const
{
	return m_exportFormatTypes;
}

QStringList DmSystem::getExportFormatTypes(const QString& type) const
{
	if (m_exportFormatTypes.find(type) == m_exportFormatTypes.end())
	{
		return QStringList(); 
	}
	else
	{
		return m_exportFormatTypes[type];
	}
}

void DmSystem::setExportTypes(const QMap<QString, QStringList>& formatTypes)
{
	m_exportFormatTypes = formatTypes;
}

void DmSystem::addExportFormatType(const QString& key, const QString& formatType)
{
	if (m_exportFormatTypes.find(key) == m_exportFormatTypes.end())
	{
		m_exportFormatTypes[key] = QStringList(formatType);
	}
	else
	{
		m_exportFormatTypes.find(key).value().append(formatType);
	}
}

QString DmSystem::getCurrentFormatType() const
{
	return m_currentFormatType;
}

void DmSystem::setCurrentFormatType(const QString& formatType)
{
	m_currentFormatType = formatType;
}
