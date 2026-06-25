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


/// @file DmDocument.cpp
/// @brief 文档类实现

#include "DmDocument.h"

#include <iostream>
#include <cmath>
#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>
#include <unordered_map>

#include "GuiDialogFactory.h"
#include "Debug.h"
#include "Fileio.h"
#include "Math2d.h"
#include "DmUnits.h"
#include "DmSettings.h"
#include "DmLayer.h"
#include "DmBlock.h"
#include "DmText.h"
#include "DmMText.h"
#include "DmDimension.h"
#include "MD5.h"
#include "ApplicationWindow.h"
#include "UITabDrawWidget.h"
#include "GuiDocumentView.h"

DmDocument::DmDocument()
    : m_idManager(DmIdManager())
{
    DMSETTINGS->beginGroup("/Defaults");
    setUnit(DmUnits::stringToUnit(DMSETTINGS->readEntry("/Unit", "None")));
    DMSETTINGS->endGroup();
    DMSETTINGS->beginGroup("/Appearance");
    addVariable("$SNAPSTYLE", static_cast<int>(DMSETTINGS->readNumEntry("/IsometricGrid", 0)), 70);
    DMSETTINGS->endGroup();

    m_filename = "";
    m_formatType = DOCDEFAULTFORMAT;

    m_activePen = DmPen(DmColor(DM::FlagByLayer), DM::WidthByLayer, DmLineTypeTable::ByLayer);

    // ****** 初始化 *******
    // 线型表
    m_LineTypeTable = new DmLineTypeTable();
    m_LineTypeTable->setDocument(this);
    // 层表
    m_layerTable = new DmLayerTable();
    m_layerTable->setDocument(this);
    // 字体样式表
    m_textStyleTable = new DmTextStyleTable();
    m_textStyleTable->setDocument(this);
    // 块表
    m_blockTable = new DmBlockTable();
    m_blockTable->setDocument(this);
    // 标注样式表
    m_dimStyleTable = new DmDimensionStyleTable();
    m_dimStyleTable->setDocument(this);
    // 实体表
    m_entityTable = new EntityTable();
    m_entityTable->setDocument(this);
    // 命令管理器
    m_cmdManager = new CmdManager();
    m_cmdManager->setDocument(this);

    m_savedUndoCount = 0;

    m_bHasAutoSaved = false;
    m_timer.reset(new QTimer());
    QObject::connect(m_timer.get(), &QTimer::timeout, [this]() { this->autoSave(); });
    DMSETTINGS->beginGroup("/Defaults");
    bool isAutoSave = DMSETTINGS->readNumEntry("/AutoBackupDocument", 1) != 0;
    int min = DMSETTINGS->readNumEntry("/AutoSaveTime", 10);
    DMSETTINGS->endGroup();
    enableAutoSave(isAutoSave, min);

    m_documentView = nullptr;
    //QObject::connect(&m_cmdManager, SIGNAL(signalCmdCommitted(bool , const std::string& , bool , const std::string&)), )
}

DmDocument::~DmDocument()
{
    // 释放顺序很重要，因为存在依赖关系
    if (m_cmdManager)
    {
        delete m_cmdManager;
        m_cmdManager = nullptr;
    }
    if (m_entityTable)
    {
        delete m_entityTable;
        m_entityTable = nullptr;
    }
    if (m_dimStyleTable)
    {
        delete m_dimStyleTable;
        m_dimStyleTable = nullptr;
    }
    if (m_blockTable)
    {
        delete m_blockTable;
        m_blockTable = nullptr;
    }
    if (m_textStyleTable)
    {
        delete m_textStyleTable;
        m_textStyleTable = nullptr;
    }
    if (m_layerTable)
    {
        delete m_layerTable;
        m_layerTable = nullptr;
    }
    if (m_LineTypeTable)
    {
        delete m_LineTypeTable;
        m_LineTypeTable = nullptr;
    }
}

DmLayerTable* DmDocument::getLayerTable()
{
    return m_layerTable;
}

DmTextStyleTable* DmDocument::getTextStyleTable()
{
    return m_textStyleTable;
}

DmDimensionStyleTable* DmDocument::getDimStyleTable()
{
    return m_dimStyleTable;
}

DmBlockTable* DmDocument::getBlockTable()
{
    return m_blockTable;
}

DmLineTypeTable* DmDocument::getLineTypeTable()
{
    return m_LineTypeTable;
}

void DmDocument::searchEntities(const DmVector& min, const DmVector& max, std::vector<DmEntity*>& ents, bool onlyVisible /*=true*/, bool searchSubEnts/* = true*/)
{
    getEntityTable()->searchEntities(min, max, ents, onlyVisible, searchSubEnts);
}

void DmDocument::specifyModifiedEntity(DmEntity* modifiedEnt)
{
    getEntityTable()->notifyEntityModified(modifiedEnt);
    if (m_documentView)
    {
        m_documentView->specifyDocumentModified();
    }
}

void DmDocument::specifyPenModified()
{
    if (m_documentView)
    {
        m_documentView->specifyDocumentModified();
    }
}

void DmDocument::initDoc()
{
    m_savedUndoCount = 0;
}

void DmDocument::autoSave()
{
    save(true);
    m_bHasAutoSaved = true;
}

bool DmDocument::save(bool isAutoSave, bool force)
{
    // 判断后缀名和保存格式是否一致
    QFileInfo fileInfo = QFileInfo(getFilename());
    auto fileSuffix = "." + fileInfo.suffix().toLower();
    auto formatLower = getFormatType().toLower();
    if (!formatLower.contains(fileSuffix))
    {
        GUIDIALOGFACTORY->commandMessage(QObject::tr("File format mismatch. Please use 'Save As' to choose a compatible format."));
        return false;
    }

    bool ret = false;

    if (!isModified() && !force)
    {
        return true;
    }
    QString actualName;
    QString actualType = getFormatType();
    QString tmpDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);

    //获得文件的名字
    if (isAutoSave)
    {
        //自动保存
        if (hasAutoSaved())
        {
            return true;
        }
        if (m_filename.isEmpty())   //从未保存过的文件
        {
            //获得选项卡的名字
            SingleTabDrawDataRibbon* drawData = ApplicationWindow::getAppWindow()->getTabDrawWidget()->getTabDrawDataOfDocument(this);
            QString sName = QString::fromStdString(MD5::getMD5(drawData->name.toStdString())).left(8);
            actualName = QDir::cleanPath(tmpDir + QDir::separator() + drawData->name + "_" + sName + ".ycd");
        }
        else
        {
            QFileInfo fileInfo(m_filename);
            QString sName = QString::fromStdString(MD5::getMD5(m_filename.toStdString())).left(8);
            actualName = QDir::cleanPath(tmpDir + QDir::separator() + fileInfo.baseName() + "_" + sName + ".ycd");
        }
    }
    else
    {
        //手动保存
        QFileInfo   finfo(m_filename);
        QDateTime m = finfo.lastModified();
        //被其他程序修改了
        if (m_strCurrentFileName == QString(m_filename) && m_modifiedTime.isValid() && m != m_modifiedTime)
        {
            GUIDIALOGFACTORY->commandMessage(QObject::tr("File on disk modified. Please save to another file to avoid data loss! File modified: %1").arg(m_filename));
            return false;
        }
        actualName = m_filename;
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
        ret = FileIO::instance()->fileExport(*this, tempFileName, actualType);
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
        m_savedUndoCount = m_cmdManager->getUndoCount();
    }
    return ret;
}

bool DmDocument::saveAs(const QString& filename, const QString& formatType, bool force)
{
    bool ret = false;

    // Check/memorize if file name we want to use as new file
    // name is the same as the actual file name.
    bool fn_is_same = filename == this->m_filename;
    auto const filenameSaved = this->m_filename;
    auto const formatTypeSaved = getFormatType();

    this->m_filename = filename;
    setFormatType(formatType);
    QFileInfo   finfo(filename);

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
        this->m_filename = filenameSaved;
        setFormatType(formatTypeSaved);
    }

    return ret;
}

QString DmDocument::getFilename() const
{
    return m_filename;
}

void DmDocument::setFilename(const QString& filename)
{
    m_filename = filename;
}

QString DmDocument::getFormatType() const
{
    return m_formatType;
}

void DmDocument::setFormatType(const QString& ft)
{
    m_formatType = ft;
}

bool DmDocument::hasAutoSaved() const
{
    return m_bHasAutoSaved;
}

void DmDocument::enableAutoSave(bool enableAutoSave, int saveMinute)
{
    if (enableAutoSave && saveMinute > 0)
    {
        constexpr int kMsecPerMinute = 60 * 1000;
        int msec = saveMinute * kMsecPerMinute;
        m_timer->start(msec);
    }
    else
    {
        m_timer->stop();
    }
}

void DmDocument::setDocumentView(GuiDocumentView* docView)
{
    m_documentView = docView;
}

GuiDocumentView* DmDocument::getDocumentView()
{
    return m_documentView;
}

DmPen DmDocument::getActivePen() const
{
    return m_activePen;
}

void DmDocument::setActivePen(DmPen pen)
{
    m_activePen = pen;
}

DmObject* DmDocument::findObject(const DmId& id)
{
    return m_idManager.getEntity(id);
}

void DmDocument::setEditBlock(DmBlock* block)
{
    DmBlock* prev = m_editingBlock;
    m_editingBlock = block;
    if (prev != block && m_documentView)
    {
        if (block)
            m_documentView->setDocumentPainterContainer(
                block->getEntityTable().getEntityContainer());
        else
            m_documentView->setDocumentPainterContainer(
                m_entityTable->getEntityContainer());
    }
}

void DmDocument::clearVariables()
{
    m_variableDict.clear();
}

int DmDocument::countVariables()
{
    return m_variableDict.count();
}

void DmDocument::addVariable(const QString& key, const DmVector& value, int code)
{
    m_variableDict.add(key, value, code);
}

void DmDocument::addVariable(const QString& key, const QString& value, int code)
{
    m_variableDict.add(key, value, code);
}

void DmDocument::addVariable(const QString& key, int value, int code)
{
    m_variableDict.add(key, value, code);
}

void DmDocument::addVariable(const QString& key, double value, int code)
{
    m_variableDict.add(key, value, code);
}

DmVector DmDocument::getVariableVector(const QString& key, const DmVector& def)
{
    return m_variableDict.getVector(key, def);
}

QString DmDocument::getVariableString(const QString& key, const QString& def)
{
    return m_variableDict.getString(key, def);
}

int DmDocument::getVariableInt(const QString& key, int def)
{
    return m_variableDict.getInt(key, def);
}

double DmDocument::getVariableDouble(const QString& key, double def)
{
    return m_variableDict.getDouble(key, def);
}

void DmDocument::removeVariable(const QString& key)
{
    m_variableDict.remove(key);
}

QHash<QString, DmVariable>& DmDocument::getVariableDict()
{
    return m_variableDict.getVariableDict();
}

bool DmDocument::open(const QString& filename)
{
    bool ret = false;

    this->m_filename = filename;
    QFileInfo finfo(filename);

    // 新建文档
    initDoc();

    // 导入文件
    QString tmpDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    ret = FileIO::instance()->fileImport(*this, filename);
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
                if (QMessageBox::Ok == QMessageBox::critical(nullptr, QObject::tr("Tips"), QObject::tr("Open failed, try to open backup file?"),
                    QMessageBox::Ok, QMessageBox::Cancel))
                {
                    QString curBakName;
                    initDoc();
                    ret = FileIO::instance()->fileImport(*this, newestBakName);
                    if (ret)
                    {
                        curBakName = newestBakName;
                    }
                    else if (!notNewbakName.isEmpty())
                    {
                        initDoc();
                        ret = FileIO::instance()->fileImport(*this, notNewbakName);
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
                        QString appendStr = QDateTime::currentDateTime().date().toString(Qt::ISODate) + QDateTime::currentDateTime().time().toString();
                        QString backedFileName = QDir::cleanPath(bakfinfo.absolutePath() + QDir::separator() + bakfinfo.baseName() + "_" + QDateTime::currentDateTime().toString("dd.MM.yyyy.hh.mm.ss.zzz") + ".ycd");
                        bool copyRes = bakfile.copy(backedFileName);
                        this->m_filename = backedFileName;
                        QFileInfo finfo(backedFileName);
                    }
                }
            }
        }
    }

    if (ret)
    {
        m_savedUndoCount = m_cmdManager->getUndoCount();
        m_modifiedTime = finfo.lastModified();
        m_strCurrentFileName = QString(filename);
    }
    else
    {
        QMessageBox::critical(nullptr, QObject::tr("Tips"), QObject::tr("Open failed, invalid file!"));
    }

    return ret;
}

/// @return true if the grid is switched on (visible).
bool DmDocument::isGridOn()
{
    int on = getVariableInt("$GRIDMODE", 1);
    return on != 0;
}

// Enables / disables the grid.
void DmDocument::setGridOn(bool on)
{
    addVariable("$GRIDMODE", (int)on, 70);
}

void DmDocument::setUnit(DM::Unit u)
{
    addVariable("$INSUNITS", (int)u, 70);
}

// Gets the unit of this document
DM::Unit DmDocument::getUnit()
{
    return (DM::Unit)getVariableInt("$INSUNITS", 0);
}

/// @return The linear format type for this document.
/// This is determined by the variable "$LUNITS".
DM::LinearFormat DmDocument::getLinearFormat()
{
    int lunits = getVariableInt("$LUNITS", 2);
    return getLinearFormat(lunits);
}

/// @return The linear format type used by the variable "$LUNITS" & "$DIMLUNIT".
DM::LinearFormat DmDocument::getLinearFormat(int f)
{
    switch (f)
    {
    default:
    case 2:
        return DM::Decimal;
    case 1:
        return DM::Scientific;
    case 3:
        return DM::Engineering;
    case 4:
        return DM::Architectural;
    case 5:
        return DM::Fractional;
    case 6:
        return DM::ArchitecturalMetric;
    }
}

/// @return The linear precision for this document.
/// This is determined by the variable "$LUPREC".
int DmDocument::getLinearPrecision()
{
    return getVariableInt("$LUPREC", 4);
}

/// @return The angle format type for this document.
/// This is determined by the variable "$AUNITS".
DM::AngleFormat DmDocument::getAngleFormat()
{
    int aunits = getVariableInt("$AUNITS", 0);

    switch (aunits)
    {
    default:
    case 0:
        return DM::DegreesDecimal;
    case 1:
        return DM::DegreesMinutesSeconds;
    case 2:
        return DM::Gradians;
    case 3:
        return DM::Radians;
    case 4:
        return DM::Surveyors;
    }
}

/// @return The linear precision for this document.
/// This is determined by the variable "$LUPREC".
int DmDocument::getAnglePrecision()
{
    return getVariableInt("$AUPREC", 4);
}

bool DmDocument::isModified() const
{
    return m_cmdManager->getUndoCount() != m_savedUndoCount;
}

QDateTime DmDocument::getModifyTime(void)
{
    return m_modifiedTime;
}

std::shared_ptr<DmCacheDrawData> DmDocument::getCacheDrawData()
{
    return m_pCacheDrawData;
}

void DmDocument::redo() {
    m_cmdManager->redo();
    regenerate();
}

void DmDocument::undo() {
    m_cmdManager->undo();
    regenerate();
}

void DmDocument::getCmdData(bool &hasUndo, std::string &undoName, bool &hasRedo, std::string &redoName) {
    m_cmdManager->getCmdData(hasUndo, undoName, hasRedo, redoName);
}

void DmDocument::regenerate() {
    if (m_editingBlock)
    {
        m_editingBlock->getEntityTable().updateContainer();
    }
    else
    {
        m_entityTable->updateContainer();
    }
    if (m_documentView)
    {
        m_documentView->specifyDocumentModified();
        m_documentView->redraw();
    }
}

EntityTable *DmDocument::getEntityTable() {
    if (m_editingBlock)
    {
        return &m_editingBlock->getEntityTable();
    }
    return m_entityTable;
}
