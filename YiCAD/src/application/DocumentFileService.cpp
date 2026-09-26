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

/// @file DocumentFileService.cpp
/// @brief 文档文件服务实现；save/saveAs/open 从 DmDocument 整段搬来

#include "DocumentFileService.h"

#include <algorithm>
#include <vector>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QObject>
#include <QStandardPaths>

#include "DmDocument.h"
#include "DmSettings.h"
#include "GuiDialogFactory.h"
#include "MD5.h"
#include "YiCadLog.h"

namespace
{
/// @brief 活着的服务，find() 在这里找
std::vector<DocumentFileService*>& instances()
{
    static std::vector<DocumentFileService*> services;
    return services;
}

/// @brief 读写失败时把过滤器的异常信息记进日志；提示用户的文字不变，不带这条信息
void logFailure(const char* what, const QString& file, const DmFileResult& result)
{
    if (!result.ok() && !result.message.isEmpty())
    {
        YICAD_LOG(yicad::log::persistence(), yicad::LogLevel::Warning)
            << what << " failed: " << file.toStdString() << ": " << result.message.toStdString();
    }
}
}  // namespace

DocumentFileService::DocumentFileService(DmDocument& document)
    : m_document(document)
{
    QObject::connect(&m_timer, &QTimer::timeout, [this]() { this->autoSave(); });
    DMSETTINGS->beginGroup("/Defaults");
    bool isAutoSave = DMSETTINGS->readNumEntry("/AutoBackupDocument", 1) != 0;
    int min = DMSETTINGS->readNumEntry("/AutoSaveTime", 10);
    DMSETTINGS->endGroup();
    enableAutoSave(isAutoSave, min);

    instances().push_back(this);
}

DocumentFileService::~DocumentFileService()
{
    std::vector<DocumentFileService*>& services = instances();
    services.erase(std::remove(services.begin(), services.end(), this), services.end());
}

DocumentFileService* DocumentFileService::find(const DmDocument* document)
{
    for (DocumentFileService* service : instances())
    {
        if (&service->m_document == document)
        {
            return service;
        }
    }
    return nullptr;
}

void DocumentFileService::autoSave()
{
    save(true);
    m_bHasAutoSaved = true;
}

bool DocumentFileService::save(bool isAutoSave, bool force)
{
    const QString filename = m_document.getFilename();

    // 判断后缀名和保存格式是否一致
    QFileInfo fileInfo = QFileInfo(filename);
    auto fileSuffix = "." + fileInfo.suffix().toLower();
    auto formatLower = m_document.getFormatType().toLower();
    if (!formatLower.contains(fileSuffix))
    {
        GUIDIALOGFACTORY->commandMessage(QObject::tr("File format mismatch. Please use 'Save As' to choose a compatible format."));
        return false;
    }

    bool ret = false;

    if (!m_document.isModified() && !force)
    {
        return true;
    }
    QString actualName;
    QString actualType = m_document.getFormatType();
    QString tmpDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);

    //获得文件的名字
    if (isAutoSave)
    {
        //自动保存
        if (hasAutoSaved())
        {
            return true;
        }
        if (filename.isEmpty())   //从未保存过的文件
        {
            //获得选项卡的名字
            QString tabName = GUIDIALOGFACTORY->requestUntitledDocumentName(&m_document);
            QString sName = QString::fromStdString(MD5::getMD5(tabName.toStdString())).left(8);
            actualName = QDir::cleanPath(tmpDir + QDir::separator() + tabName + "_" + sName + ".ycd");
        }
        else
        {
            QFileInfo fileInfo(filename);
            QString sName = QString::fromStdString(MD5::getMD5(filename.toStdString())).left(8);
            actualName = QDir::cleanPath(tmpDir + QDir::separator() + fileInfo.baseName() + "_" + sName + ".ycd");
        }
    }
    else
    {
        //手动保存
        QFileInfo   finfo(filename);
        QDateTime m = finfo.lastModified();
        //被其他程序修改了
        if (m_strCurrentFileName == filename && m_modifiedTime.isValid() && m != m_modifiedTime)
        {
            GUIDIALOGFACTORY->commandMessage(QObject::tr("File on disk modified. Please save to another file to avoid data loss! File modified: %1").arg(filename));
            return false;
        }
        actualName = filename;
    }

    //保存文件
    if (!actualName.isEmpty())
    {
        // 自动保存只保存ocd格式
        if (isAutoSave)
        {
            actualType = DOCDEFAULTFORMAT;
            GUIDIALOGFACTORY->commandMessage(QObject::tr("Auto saving file: %1").arg(actualName));
        }

        QString tempFileName = actualName + ".tmp";
        ret = writeFile(tempFileName, actualType);
        QFileInfo tempFileInfo(tempFileName);
        QFile tempFile(tempFileName);
        if (ret)
        {
            if (isAutoSave)
            {
                //删除原来的备份文件
                bool canTempRename = true;
                QFileInfo originFinfo(actualName);
                if (originFinfo.exists())
                {
                    QFile originFile(actualName);
                    bool res = originFile.remove();
                    canTempRename = res;
                }
                if (canTempRename)
                {
                    //将.tmp文件重命名为actualName
                    tempFile.rename(actualName);
                }
                else
                {
                    tempFile.remove();
                    GUIDIALOGFACTORY->commandMessage(QObject::tr("Can not backup file: %1!").arg(actualName));
                    return false;
                }
            }
            else
            {
                //备份已有文件
                QFileInfo originFinfo(actualName);
                if (originFinfo.exists())
                {
                    QFile file(actualName);
                    QString bakName = QDir::cleanPath(originFinfo.absolutePath() + QDir::separator() + originFinfo.baseName() + ".bak");
                    QFile bakFile(bakName);
                    //移除原来的bak文件
                    if (bakFile.exists())
                    {
                        bool removeRes = bakFile.remove();
                        if (!removeRes)
                        {
                            tempFile.remove();
                            GUIDIALOGFACTORY->commandMessage(QObject::tr("Can not remove origin backup file: %1!").arg(bakName));
                            return false;
                        }
                    }
                    //将原文件重命名为bak文件
                    bool res = file.rename(bakName);
                    if (!res)
                    {
                        tempFile.remove();
                        GUIDIALOGFACTORY->commandMessage(QObject::tr("Can not backup file: %1!").arg(actualName));
                        return false;
                    }
                }

                //将.tmp文件重命名为actualName
                tempFile.rename(actualName);

                //删除临时自动保存文件（另存为的在此处无效）
                QString sName = QString::fromStdString(MD5::getMD5(actualName.toStdString())).left(8);
                QString autoSaveName = QDir::cleanPath(tmpDir + QDir::separator() + originFinfo.baseName() + "_" + sName + ".ycd");
                QFile autoSaveFile(autoSaveName);
                autoSaveFile.remove();

                QFileInfo   finfo(actualName);
                m_modifiedTime = finfo.lastModified();
                m_strCurrentFileName = actualName;
            }
            GUIDIALOGFACTORY->commandMessage(QObject::tr("File saved: %1").arg(actualName));
        }
        else
        {
            GUIDIALOGFACTORY->commandMessage(QObject::tr("File save failed: %1!").arg(actualName));
            return false;
        }
    }

    if (ret && !isAutoSave)
    {
        // Tell that drawing file is no more modified.
        m_document.markSaved();
    }
    return ret;
}

bool DocumentFileService::saveAs(const QString& filename, const QString& formatType, bool force)
{
    bool ret = false;

    // Check/memorize if file name we want to use as new file
    // name is the same as the actual file name.
    bool fn_is_same = filename == m_document.getFilename();
    auto const filenameSaved = m_document.getFilename();
    auto const formatTypeSaved = m_document.getFormatType();

    m_document.setFilename(filename);
    m_document.setFormatType(formatType);

    ret = save(false, !fn_is_same || force); // Save file.

    if (ret)
    {
        //删除临时自动保存文件
        QFileInfo originFinfo(filenameSaved);
        if (originFinfo.exists())
        {
            QString tmpDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
            QString sName = QString::fromStdString(MD5::getMD5(filenameSaved.toStdString())).left(8);
            QString autoSaveName = QDir::cleanPath(tmpDir + QDir::separator() + originFinfo.baseName() + "_" + sName + ".ycd");
            QFile autoSaveFile(autoSaveName);
            autoSaveFile.remove();
        }
    }
    else
    {
        // do not modify filenames:
        m_document.setFilename(filenameSaved);
        m_document.setFormatType(formatTypeSaved);
    }

    return ret;
}

bool DocumentFileService::hasAutoSaved() const
{
    return m_bHasAutoSaved;
}

void DocumentFileService::enableAutoSave(bool enableAutoSave, int saveMinute)
{
    if (enableAutoSave && saveMinute > 0)
    {
        constexpr int kMsecPerMinute = 60 * 1000;
        int msec = saveMinute * kMsecPerMinute;
        m_timer.start(msec);
    }
    else
    {
        m_timer.stop();
    }
}

bool DocumentFileService::open(const QString& filename)
{
    bool ret = false;

    m_document.setFilename(filename);
    QFileInfo finfo(filename);

    // 导入文件（readFile 先 initDoc()，即新建文档）
    QString tmpDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    ret = readFile(filename);
    if (!ret)
    {
        //导入失败(文件损坏等原因)，尝试打开备份文件
        QFileInfo finfo(filename);
        if (finfo.exists())
        {
            QString bakName = QDir::cleanPath(finfo.absolutePath() + QDir::separator() + finfo.baseName() + ".bak");
            QString sName = QString::fromStdString(MD5::getMD5(filename.toStdString())).left(8);
            QString autoBakName = QDir::cleanPath(tmpDir + QDir::separator() + finfo.baseName() + "_" + sName + ".ycd");
            QString newestBakName;  //最新备份文件
            QString notNewbakName;  //非最新备份文件
            QFileInfo bakInfo(bakName);
            if (bakInfo.exists())
            {
                newestBakName = bakName;
            }
            QFileInfo autoBakInfo(autoBakName);
            if (autoBakInfo.exists())
            {
                if (bakInfo.exists())
                {
                    if (autoBakInfo.lastModified() > bakInfo.lastModified())
                    {
                        newestBakName = autoBakName;
                        notNewbakName = bakName;
                    }
                    else
                    {
                        notNewbakName = autoBakName;
                    }
                }
                else
                {
                    newestBakName = autoBakName;
                }
            }

            //先读取最新备份文件，如果失败读取非最新备份文件
            if (!newestBakName.isEmpty())
            {
                if (GUIDIALOGFACTORY->requestConfirmDialog(QObject::tr("Tips"), QObject::tr("Open failed, try to open backup file?")))
                {
                    // 备份按原生格式读：.bak 的后缀没有过滤器接（R8）
                    QString curBakName;
                    ret = readNativeFile(newestBakName);
                    if (ret)
                    {
                        curBakName = newestBakName;
                    }
                    else if (!notNewbakName.isEmpty())
                    {
                        ret = readNativeFile(notNewbakName);
                        if (ret)
                        {
                            curBakName = notNewbakName;
                        }
                    }
                    if (!curBakName.isEmpty())
                    {
                        //备份文件读取成功
                        QFile bakfile(curBakName);
                        QFileInfo bakfinfo(curBakName);
                        QString backedFileName = QDir::cleanPath(bakfinfo.absolutePath() + QDir::separator() + bakfinfo.baseName() + "_" + QDateTime::currentDateTime().toString("dd.MM.yyyy.hh.mm.ss.zzz") + ".ycd");
                        bakfile.copy(backedFileName);
                        m_document.setFilename(backedFileName);
                    }
                }
            }
        }
    }

    if (ret)
    {
        m_document.markSaved();
        m_modifiedTime = finfo.lastModified();
        m_strCurrentFileName = filename;
    }
    else
    {
        GUIDIALOGFACTORY->requestWarningDialog(QObject::tr("Open failed, invalid file!"));
    }

    return ret;
}

bool DocumentFileService::writeFile(const QString& file, const QString& formatType)
{
    const DmFileResult result = m_document.writeFile(file, formatType);
    if (result.status == DmFileStatus::NoFilter)
    {
        QMessageBox::critical(nullptr, QObject::tr("ToolTips"), QObject::tr("Unsupported file format, please use another format to export!"), QMessageBox::Cancel); // 暂不支持的文件格式，请使用其他格式导出
    }
    logFailure("write", file, result);
    return result.ok();
}

bool DocumentFileService::readFile(const QString& file)
{
    const DmFileResult result = m_document.readFile(file);
    logFailure("read", file, result);
    return result.ok();
}

bool DocumentFileService::readNativeFile(const QString& file)
{
    const DmFileResult result = m_document.readNativeFile(file);
    logFailure("read", file, result);
    return result.ok();
}
