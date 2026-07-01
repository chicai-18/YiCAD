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

/// @file UIFileDialog.cpp
/// @brief 自定义文件对话框，支持打开和保存CAD文件，记忆最近使用的路径和过滤器

#include "UIFileDialog.h"

#include <QMessageBox>
#include <QSettings>
#ifdef Q_OS_LINUX
    #include <QDialogButtonBox>
    #include <QStyle>
#endif

#include "DmSettings.h"
#include "DmSystem.h"
#include "Debug.h"
#include "Fileio.h"

namespace
{

QString suffixFromNameFilter(const QString& nameFilter)
{
    const auto wildcard = nameFilter.lastIndexOf(QStringLiteral("*."));
    if (wildcard < 0)
    {
        return {};
    }

    const auto end = nameFilter.indexOf(QLatin1Char(')'), wildcard);
    const auto suffix = nameFilter.mid(
        wildcard + 2,
        end < 0 ? -1 : end - wildcard - 2);
    return suffix.contains(QLatin1Char(' ')) ? QString() : suffix;
}

} // namespace

const QString UIFileDialog::m_strDefaultFilter = "";

UIFileDialog::UIFileDialog(QWidget* parent, Qt::WindowFlags f, FileType type)
    : QFileDialog(parent, f)
{
    QSettings settings;
    if (0 == settings.value("Defaults/UseQtFileOpenDialog", 0).toInt())
    {
        setOption(QFileDialog::DontUseNativeDialog, false);
    }
    else
    {
        setOption(QFileDialog::DontUseNativeDialog, true);
    }
    setOption(QFileDialog::HideNameFilterDetails, false);

    switch (type)
    {
    case BlockFile:
        m_strName = "Block";
        break;
    default:
        m_strName = "Drawing";
        break;
    }
}

UIFileDialog::~UIFileDialog()
{
}

QString UIFileDialog::getOpenFile()
{
    setAcceptMode(QFileDialog::AcceptOpen);
    // read default settings:
    DMSETTINGS->beginGroup("/Paths");
    QString defDir = DMSETTINGS->readEntry("/Open", DMSYSTEM->getHomeDir());
    QString open_filter = DMSYSTEM->getCurrentFormatType(); //DMSETTINGS->readEntry("/OpenFilter", DMSYSTEM->getCurrentFormatType());
    DMSETTINGS->endGroup();

    QString fn = "";

    auto typeImportTypes = DMSYSTEM->getImportFormatTypes(open_filter);
    typeImportTypes.append(FileIO::instance()->pluginImportNameFilters());

    setWindowTitle(tr("Open %1").arg(m_strName));
    setNameFilters(typeImportTypes);
    setDirectory(defDir);
    setFileMode(QFileDialog::ExistingFile);
    selectNameFilter(open_filter);

    if (QDialog::Accepted == exec())
    {
        QStringList fl = selectedFiles();
        if (!fl.isEmpty())
        {
            fn = fl[0];
        }
        fn = QDir::toNativeSeparators(QFileInfo(fn).absoluteFilePath());

        // store new default settings:
        DMSETTINGS->beginGroup("/Paths");
        DMSETTINGS->writeEntry("/Open", QFileInfo(fn).absolutePath());
        DMSETTINGS->writeEntry("/OpenFilter", selectedNameFilter());
        DMSETTINGS->endGroup();
    }

    return fn;
}

QString UIFileDialog::getSaveFile(QString& formatType)
{
    setAcceptMode(QFileDialog::AcceptSave);
    // read default settings:
    DMSETTINGS->beginGroup("/Paths");
    QString defDir = DMSETTINGS->readEntry("/Save", DMSYSTEM->getHomeDir());
    QString defFilter = formatType;
    if (defFilter.isEmpty())
    {
        defFilter = DMSETTINGS->readEntry("/SaveFilter", m_strDefaultFilter);
    }
    DMSETTINGS->endGroup();

    if (!defDir.endsWith("/") && !defDir.endsWith("\\"))
    {
        defDir += QDir::separator();
    }

    auto type = DMSYSTEM->getCurrentFormatType();
    auto filters = DMSYSTEM->getExportFormatTypes(type);
    filters.append(FileIO::instance()->pluginExportNameFilters());
    auto subStrs = filters.at(0).split("(*");
    auto suffix = (subStrs[subStrs.size() - 1]).split(")").at(0);

    // when defFilter is added the below should use the default extension.
    // generate an untitled m_strName
    QString fn = "Drawing";
    if (QFile::exists(defDir + fn + suffix))
    {
        int fileCount = 1;
        while (QFile::exists(defDir + fn + QString("%1").arg(fileCount) + suffix))
        {
            ++fileCount;
        }
        fn += QString("%1").arg(fileCount);
    }

    // initialize dialog properties
    setWindowTitle(tr("Save %1 As").arg(m_strName));
    setFileMode(QFileDialog::AnyFile);
    setDirectory(defDir);
    setNameFilters(filters);
    selectNameFilter(type);
    selectFile(fn);

    constexpr int kSuffixLengthWithDot = 4;
    if (kSuffixLengthWithDot == suffix.size())
    {
        if (suffix[0] == '.')
        {
            suffix.remove(0, 1);
        }
    }
    setDefaultSuffix(suffix);
    connect(
        this,
        &QFileDialog::filterSelected,
        this,
        [this](const QString& nameFilter) {
            const auto selectedSuffix = suffixFromNameFilter(nameFilter);
            if (!selectedSuffix.isEmpty())
            {
                setDefaultSuffix(selectedSuffix);
            }
        });

    // only return non empty string when we have a complete, user approved, file m_strName.
    if (exec() != QDialog::Accepted)
    {
        return QString("");
    }

    QStringList fl = selectedFiles();
    if (fl.isEmpty())
    {
        return QString("");
    }

    QFileInfo fi = QFileInfo(fl[0]);
    fn = QDir::toNativeSeparators(fi.absoluteFilePath());

    formatType =
        FileIO::instance()->exportFormatType(selectedNameFilter());

    // store new default settings:
    DMSETTINGS->beginGroup("/Paths");
    DMSETTINGS->writeEntry("/Save", fi.absolutePath());
    DMSETTINGS->writeEntry("/SaveFilter", formatType);
    DMSETTINGS->endGroup();

    return fn;
}
