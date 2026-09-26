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


/// @file DmBlock.h
/// @brief 图块定义类，块参照使用的模板，包含实体集合

#ifndef DMBLOCK_H
#define DMBLOCK_H

#include <QSet>

#include "DmObject.h"
#include "EntityTable.h"

class DmAttributeDefinition;

/// @brief 图块定义数据结构
struct DmBlockData
{
    DmBlockData() = default;

    DmBlockData(const QString& name, const DmVector& basePoint, bool frozen, const QString& path = "", const QString& modify = "");

    bool isValid() const;

    QString name;
    DmVector basePoint;

    QString pathName;
    QString modifyDate;
    bool isMidify = false;
    bool frozen = false;               ///< 冻结标志
    bool visibleInBlockList = true;    ///< 是否在块列表中可见
    bool selectedInBlockList = false;  ///< 是否在块列表中被选中
};

/// @brief 图块定义
/// 块参照使用的模板
class DmBlock : public DmObject
{
    TYPESYSTEM_HEADER();
    friend class DmBlockTable;

public:
    DmBlock();
    /// @param doc 该块所属的文档
    /// @param blockData 块的定义数据
    DmBlock(DmDocument* doc, const DmBlockData& d);

    virtual ~DmBlock();

    virtual DM::EntityType getEntityType() const;

    DmBlock* clone() const;
    void setDocument(DmDocument* pDoc) override;

    QString getPath() const;
    void setPath(const QString& path);

    QString getModifyDate();
    void setModifyDate(const QString& date);

    bool blockIsModify();
    void setBlockIsModify(bool is);
    /// @return 该块的名称（名称是该块的唯一标识）
    QString getName() const;

    /// @return 该块的基点
    DmVector getBasePoint() const;
    void setBasePoint(const DmVector& pt);

    //bool save(bool isAutoSave = false);

    //bool saveAs(const QString& filename, const QString& formatType, bool force = false);

    //bool open(const QString&);

    bool loadTemplate(const QString&);

    // 为块设置新名称，仅供块列表调用以保证块名唯一。
    void setName(const QString& n);

    /// @retval true 该块已冻结（不可见）
    /// @retval false 该块未冻结（可见）
    bool isFrozen() const;

    // 切换该块的可见状态。
    // 未冻结则冻结，已冻结则解冻。
    void toggle();

    /// 冻结或解冻该块。
    /// @param freeze true 表示冻结，false 表示解冻
    void freeze(bool freeze);

    /// 设置该块在块列表中的可见性
    /// @param v true 表示可见，false 表示不可见
    void visibleInBlockList(bool v);

    /// @brief 返回该块在块列表中的可见性
    bool isVisibleInBlockList() const;

    /// @brief 设置该块在块列表中的选中状态
    /// @param v true 表示选中，false 表示取消选中
    void selectedInBlockList(bool v);

    /// @brief 返回该块在块列表中的选中状态
    bool isSelectedInBlockList() const;

    QStringList findNestedInsert(const QString& bName);

    /// @brief 递归收集当前块及其所有嵌套块的名称（包括自身）
    /// @param[out] names 输出的块名称列表
    /// @param[in,out] visited 已访问的块名称集合，用于防止循环嵌套
    void collectNestedBlockNames(QStringList& names, QSet<QString>& visited);

    /// @brief 是否包含属性定义
    bool hasAttributeDefinitions() const;

    /// @brief 获得块中的属性定义
    std::list<DmAttributeDefinition*> getAttributeDefinitions() const;

    EntityTable& getEntityTable() { return m_entityTable; }
    const EntityTable& getEntityTable() const { return m_entityTable; }

    // 持久化辅助接口
    virtual void saveStream(OutputStream& wrt) const override;
    virtual void restoreStream(InputStream& reader, const std::vector<PAIR>& revs) override;
    virtual void restoreStreamWithRev(InputStream& rdr, int rev) override;

private:
    void getEntity(InputStream& rdr, const std::vector<PAIR>& revs);
    bool isSaveEntType(const DM::EntityType type) const; // 判断该类型实体是否保存

protected:
    DmBlockData data;

private:
    EntityTable         m_entityTable;                  ///< 图块的实体集

};

#endif
