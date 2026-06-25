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


/// @file DmDocument.h
/// @brief 文档类，YiCAD的核心文档模型，管理实体、图层、块、样式等

#ifndef DMDOCUMENT_H
#define DMDOCUMENT_H

#include <QDateTime>
#include <QTimer>

#include "DmEntityContainer.h"
#include "EntityTable.h"
#include "DmBlockTable.h"
#include "DmLayerTable.h"
#include "DmUnits.h"
#include "DmVariableDict.h"
#include "DmTextStyleTable.h"
#include "DmDimensionStyleTable.h"

#include "DmIdManager.h"
#include "CmdManager.h"

class DmVariableDict;
class DmTextStyle;
class DmLineTypeTable;
class DmCacheDrawData;
class GuiDocumentView;

constexpr const char* DOCDEFAULTFORMAT = "Drawing Exchange YCD 2023 (*.ycd)";

/// @brief 文档
class DmDocument : public DmFlags
{
public:
    DmDocument();
    virtual ~DmDocument();

    /// @brief 获取实体表
    /// @return 实体表指针（编辑块时返回块的实体表）
    EntityTable* getEntityTable();

    /// @brief 获取文档的实体表（始终返回文档级别，不路由到块表）
    /// @return 文档实体表指针
    EntityTable* getDocumentEntityTable() { return m_entityTable; }

    /// @brief 获取图层表
    /// @return 图层表指针
    DmLayerTable* getLayerTable();

    /// @brief 获取文字样式表
    /// @return 文字样式表指针
    DmTextStyleTable* getTextStyleTable();

    /// @brief 获取标注样式表
    /// @return 标注样式表指针
    DmDimensionStyleTable* getDimStyleTable();

    /// @brief 获取块表
    /// @return 块表指针
    DmBlockTable* getBlockTable();

    /// @brief 获取线型表
    /// @return 线型表指针
    DmLineTypeTable* getLineTypeTable();

    /// @brief 搜索包围框与指定区域有重叠的实体
    /// @param min 区域最小点
    /// @param max 区域最大点
    /// @param ents 输出：搜索结果
    /// @param onlyVisible 是否仅搜索可见实体
    /// @param searchSubEnts 是否搜索子实体
    void searchEntities(const DmVector& min, const DmVector& max, std::vector<DmEntity*>& ents, bool onlyVisible = true, bool searchSubEnts = true);

    /// @brief 指示已修改某个实体，需要更新视图及空间树
    /// @param modifiedEnt 已修改的实体
    void specifyModifiedEntity(DmEntity* modifiedEnt);

    /// @brief 指示已修改颜色（图层，线形，线宽，或线宽）
    void specifyPenModified();

    /// @brief 初始化文档
    void initDoc();

    /// @brief 保存文件
    /// @param isAutoSave 是否为自动保存
    /// @return 保存是否成功
    bool save(bool isAutoSave = false, bool force = false);

    /// @brief 另存为
    /// @param filename 目标文件名
    /// @param formatType 文件格式类型
    /// @param force 是否强制保存
    /// @return 保存是否成功
    bool saveAs(const QString& filename, const QString& formatType, bool force = false);

    /// @brief 打开文件
    /// @param filename 文件名
    /// @return 打开是否成功
    bool open(const QString& filename);

    /// @brief 撤销操作
    void undo();

    /// @brief 重做操作
    void redo();

    /// @brief 获得undo，redo信息
    /// @param hasUndo 输出：是否有可撤销操作
    /// @param undoName 输出：撤销操作名称
    /// @param hasRedo 输出：是否有可重做操作
    /// @param redoName 输出：重做操作名称
    void getCmdData(bool& hasUndo, std::string& undoName, bool& hasRedo, std::string& redoName);

    /// @brief 重新生成视图
    void regenerate();

    /// @brief 获取文档保存路径
    /// @return 文件路径
    QString getFilename() const;

    /// @brief 设置文档保存路径
    /// @param filename 文件路径
    void setFilename(const QString& filename);

    /// @brief 获取保存文件格式
    /// @return 格式字符串
    QString getFormatType() const;

    /// @brief 设置文件保存格式
    /// @param ft 格式字符串
    void setFormatType(const QString& ft);

    /// @brief 是否已自动保存
    /// @return 如果已自动保存则返回true
    bool hasAutoSaved() const;

    /// @brief 启动或关闭自动保存
    /// @param enableAutoSave 是否启用自动保存
    /// @param saveMinute 自动保存间隔（分钟）
    void enableAutoSave(bool enableAutoSave, int saveMinute);

    /// @brief 给文档设置画布
    /// @param docView 画布视图指针
    void setDocumentView(GuiDocumentView* docView);

    /// @brief 获取文档的画布
    /// @return 画布视图指针
    GuiDocumentView* getDocumentView();

    /// @brief 获取文档当前画笔
    /// @return 当前画笔
    DmPen getActivePen() const;

    /// @brief 设置文档当前画笔
    /// @param pen 画笔
    void setActivePen(DmPen pen);

    /// @brief 根据ID查找对象
    /// @param id 对象ID
    /// @return 找到的对象指针，未找到返回nullptr
    DmObject* findObject(const DmId& id);

    /// @brief 设置当前编辑的块
    /// @param block 块指针
    void setEditBlock(DmBlock* block);

    /// @brief 获取当前编辑的块
    /// @return 当前编辑的块指针
    DmBlock* getEditingBlock() const { return m_editingBlock; }

    // Wrappers for variable functions:
    void clearVariables();
    int countVariables();

    void addVariable(const QString& key, const DmVector& value, int code);
    void addVariable(const QString& key, const QString& value, int code);
    void addVariable(const QString& key, int value, int code);
    void addVariable(const QString& key, double value, int code);

    DmVector getVariableVector(const QString& key, const DmVector& def);
    QString getVariableString(const QString& key, const QString& def);
    int getVariableInt(const QString& key, int def);
    double getVariableDouble(const QString& key, double def);

    void removeVariable(const QString& key);

    QHash<QString, DmVariable>& getVariableDict();

    DM::LinearFormat getLinearFormat();
    DM::LinearFormat getLinearFormat(int f);
    int getLinearPrecision();
    DM::AngleFormat getAngleFormat();
    int getAnglePrecision();

    /// @brief 设置图纸单位
    /// @param u 单位类型
    void setUnit(DM::Unit u);

    /// @brief 获取图纸单位
    /// @return 单位类型
    DM::Unit getUnit();

    /// @brief 判断网格是否开启
    /// @return 如果网格开启则返回true
    bool isGridOn();

    /// @brief 设置网格显示状态
    /// @param on 是否显示网格
    void setGridOn(bool on);

    /// @brief 获取doc是否修改
    /// @return 如果已修改则返回true
    bool isModified() const;

    /// @brief 获取文档上次修改时间
    /// @return 修改时间
    QDateTime getModifyTime(void);

    /// @brief 获取缓存绘制数据
    /// @return 共享指针
    std::shared_ptr<DmCacheDrawData> getCacheDrawData();

public:
    /// @brief 通过计时器自动保存
    void autoSave();

    /// @brief 获取命令管理器
    /// @return 命令管理器指针
    CmdManager* getCmdManager() { return m_cmdManager; }

    /// @brief 获取ID管理器
    /// @return ID管理器指针
    DmIdManager* getIdManager() { return &m_idManager; }

    friend class EntityTable;

private:
    QDateTime                           m_modifiedTime; ///< 文档修改时间
    QString                             m_strCurrentFileName; ///< 保存文件名副本，用于检测外部修改

    DmLineTypeTable*                    m_LineTypeTable = nullptr; ///< 线型表
    DmLayerTable*                       m_layerTable = nullptr; ///< 层表
    DmTextStyleTable*                   m_textStyleTable = nullptr; ///< 文字样式表
    DmDimensionStyleTable*              m_dimStyleTable = nullptr; ///< 标注样式表
    DmBlockTable*                       m_blockTable = nullptr; ///< 块表
    EntityTable*                        m_entityTable = nullptr; ///< 实体表

    DmVariableDict                      m_variableDict; ///< 变量字典
    CmdManager*                         m_cmdManager = nullptr; ///< 命令管理器
    DmBlock*                            m_editingBlock = nullptr; ///< 当前编辑的块

    std::shared_ptr<DmCacheDrawData>    m_pCacheDrawData; ///< 缓存绘制数据

    size_t                              m_savedUndoCount = 0; ///< 保存时的 undo 栈大小，用于判断文档是否需要保存
    bool                                m_bHasAutoSaved = false; ///< 是否已自动保存
    std::shared_ptr<QTimer>             m_timer; ///< 用于自动保存文件的定时器   //不能用unique_ptr，否则编译不过
    GuiDocumentView*                    m_documentView = nullptr; ///< 这个文档对应的画布
    DmPen                               m_activePen; ///< 文档当前的画笔
    QString                             m_filename; ///< 文档保存路径
    QString                             m_formatType; ///< 保存格式名
    DmIdManager                         m_idManager; ///< id管理器
};

#endif
