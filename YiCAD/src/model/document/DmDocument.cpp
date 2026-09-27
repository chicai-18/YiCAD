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

#include <algorithm>
#include <exception>
#include <iostream>
#include <cmath>
#include <memory>
#include <unordered_map>

#include "DmDocumentListener.h"
#include "Debug.h"
#include "Math2d.h"
#include "DmUnits.h"
#include "DmSettings.h"
#include "DmLayer.h"
#include "DmBlock.h"
#include "DmText.h"
#include "DmMText.h"
#include "DmDimension.h"
#include "FilterInterface.h"
#include "FilterOcdIO.h"
#include "FilterRegistry.h"
#include "ScopedTimer.h"

namespace
{
/// @brief 调过滤器读或写；过滤器返回 false 时为 Failed，抛出的异常转成 Failed 并取异常信息
template <typename Call>
DmFileResult runFilter(Call&& call)
{
    try
    {
        return call() ? DmFileResult{} : DmFileResult{DmFileStatus::Failed, QString()};
    }
    catch (const std::exception& e)
    {
        return {DmFileStatus::Failed, QString::fromUtf8(e.what())};
    }
    catch (...)
    {
        return {DmFileStatus::Failed, QStringLiteral("unknown exception")};
    }
}

/// @brief 用给定的过滤器读入文档，成功后记为已保存
DmFileResult importInto(DmDocument& document, FilterInterface& filter, const QString& file)
{
    const DmFileResult result = runFilter([&]() { return filter.fileImport(document, file); });
    if (result.ok())
    {
        document.markSaved();
    }
    return result;
}
}  // namespace

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
    notifyDocumentModified();
}

void DmDocument::specifyPenModified()
{
    notifyDocumentModified();
}

void DmDocument::initDoc()
{
    m_savedUndoCount = 0;
}

DmFileResult DmDocument::readFile(const QString& file)
{
    YICAD_SCOPED_TIMER(yicad::counters::openDocument());
    initDoc();
    std::unique_ptr<FilterInterface> filter = FilterRegistry::instance().importFilter(file);
    if (!filter)
    {
        return {DmFileStatus::NoFilter, QString()};
    }
    return importInto(*this, *filter, file);
}

DmFileResult DmDocument::readNativeFile(const QString& file)
{
    initDoc();
    FilterOcdIO filter;
    return importInto(*this, filter, file);
}

DmFileResult DmDocument::writeFile(const QString& file, const QString& formatType)
{
    std::unique_ptr<FilterInterface> filter = FilterRegistry::instance().exportFilter(formatType);
    if (!filter)
    {
        return {DmFileStatus::NoFilter, QString()};
    }
    return runFilter([&]() { return filter->fileExport(*this, file, formatType); });
}

void DmDocument::markSaved()
{
    m_savedUndoCount = m_cmdManager->getUndoCount();
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

void DmDocument::addListener(DmDocumentListener* listener)
{
    if (listener && std::find(m_listeners.begin(), m_listeners.end(), listener) == m_listeners.end())
    {
        m_listeners.push_back(listener);
    }
}

void DmDocument::removeListener(DmDocumentListener* listener)
{
    m_listeners.erase(std::remove(m_listeners.begin(), m_listeners.end(), listener), m_listeners.end());
}

void DmDocument::notifyDocumentModified()
{
    for (DmDocumentListener* listener : m_listeners)
    {
        listener->documentModified();
    }
}

void DmDocument::requestRedraw()
{
    for (DmDocumentListener* listener : m_listeners)
    {
        listener->redrawRequested();
    }
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
    if (prev != block)
    {
        DmEntityContainer* container = block ? block->getEntityTable().getEntityContainer()
                                             : m_entityTable->getEntityContainer();
        for (DmDocumentListener* listener : m_listeners)
        {
            listener->paintContainerChanged(container);
        }
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
    notifyDocumentModified();
    requestRedraw();
}

EntityTable *DmDocument::getEntityTable() {
    if (m_editingBlock)
    {
        return &m_editingBlock->getEntityTable();
    }
    return m_entityTable;
}
