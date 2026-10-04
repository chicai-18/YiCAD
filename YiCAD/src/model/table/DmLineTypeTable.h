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

/// @file DmLineTypeTable.h
/// @brief 线型表，管理文档中所有线型

#ifndef DMLINETYPETABLE_H
#define DMLINETYPETABLE_H

#include <QString>
#include <QFile>
#include <QList>
#include <QByteArray>
#include <QStringList>

#include "DmLineType.h"
#include "DmSystem.h"
#include "TableBase.h"


/// @brief 线型文件相关常量与保留线型名
namespace LineType
{
    const QString LineTypeFile = "acad.lin";
    const QString LineTypeIsoFile = "acadiso.lin";
    const QString ByLayer = "ByLayer";
    const QString ByBlock = "ByBlock";
    const QString Continuous = "Continuous";
    const QString DashLine = "DashLine";

    /// @brief name 是否为保留的 ByLayer 线型名，不区分大小写（同 ODA 的 OdDbSymUtil::isLinetypeByLayerName）
    bool isLinetypeByLayerName(const QString& name);
    /// @brief name 是否为保留的 ByBlock 线型名，不区分大小写（同 OdDbSymUtil::isLinetypeByBlockName）
    bool isLinetypeByBlockName(const QString& name);
    /// @brief name 是否为保留的 Continuous 线型名，不区分大小写（同 OdDbSymUtil::isLinetypeContinuousName）
    bool isLinetypeContinuousName(const QString& name);
}

class DmDocument;

/// @brief 线型表
/// @details 与 AutoCAD/ODA 相同，ByLayer、ByBlock、Continuous 是每个文档线型表里的保留记录，建表时创建、不能删除。
///          随层、随块线型就是前两条记录：它们只有名字，没有图案等参数；实体引用它们即为随层、随块，
///          判断时与所属文档的这两条记录比较（ODA 比较实体的 linetypeId 与 getLinetypeByLayerId()），
///          见 getLineTypeByLayer()、isByLayer()
class DmLineTypeTable : public ITable
{
public:
    using iterator = FilterIterator<std::vector<DmLineType*>::iterator>;
public:
    DmLineTypeTable();
    virtual ~DmLineTypeTable();

    void setDocument(DmDocument *pDoc) override;
    void startModify(DmObject* e) override;
    /// @brief 添加线型
    void add(DmLineType* e);
    /// @brief 通过 id 移除线型
    void remove(DmId id);
    /// @brief 移除线型
    void remove(DmLineType* e);
    /// @brief 查找线型（不能获得已删除的线型）
    DmLineType* find(const QString& name);
    /// @brief 通过 id 查找线型，采用此方法可获得已删除的线型
    DmLineType* find(const DmId& id);
    /// @brief 表中不存在该实体，直接添加
    bool add_direct(DmLineType* e);
    /// @brief 直接删除从表中实体
    bool remove_direct(DmLineType* e);

    iterator begin();
    iterator end();

    unsigned int count() const;

    /// @brief 按名称激活线型
    void activate(const QString& name);
    /// @brief 激活线型
    void activate(DmLineType* lineType);
    /// @brief 直接激活线型
    void activate_direct(DmLineType* lineType);
    /// @brief 获取当前激活的线型
    DmLineType* getActive();

    /// @brief 保留的 ByLayer 记录（同 ODA 的 OdDbLinetypeTable::getLinetypeByLayerId()）；setDocument 之前为空
    DmLineType* getLineTypeByLayer() const { return m_byLayer; }
    /// @brief 保留的 ByBlock 记录（同 OdDbLinetypeTable::getLinetypeByBlockId()）；setDocument 之前为空
    DmLineType* getLineTypeByBlock() const { return m_byBlock; }
    /// @brief 保留的 Continuous 记录（同 OdDbDatabase::getLinetypeContinuousId()）；setDocument 之前为空
    DmLineType* getLineTypeContinuous() const { return m_continuous; }

    /// @brief lineType 是否为它所属文档的 ByLayer 记录
    /// @details 由记录找到所属文档（记录加入线型表时记下），再与该文档的 ByLayer 记录比较，
    ///          如同 ODA 由对象 ID 找到数据库再比较 getLinetypeByLayerId()；不属于任何文档的线型都不是
    static bool isByLayer(const DmLineType* lineType);
    /// @brief lineType 是否为它所属文档的 ByBlock 记录，见 isByLayer()
    static bool isByBlock(const DmLineType* lineType);

    /// @brief 删除静态线型
    static void deleteStaticLineTypes();
    static DmLineType* Continuous;  ///< 不属于任何文档的实线（文字、填充等内部生成的子实体用）
    static DmLineType* DashLine;

private:
    /// @brief lineType 是否为本表的保留记录
    bool isReserved(const DmLineType* lineType) const;

    std::unordered_map<DmId, DmLineType*>   m_lineTypeMap;      ///< 线型字典
    std::vector<DmLineType*>                m_lineTypes;        ///< 线型列表
    DmLineType*                             m_pActLineType;     ///< 当前激活的线型
    DmLineType*                             m_byLayer = nullptr;     ///< 保留记录 ByLayer
    DmLineType*                             m_byBlock = nullptr;     ///< 保留记录 ByBlock
    DmLineType*                             m_continuous = nullptr;  ///< 保留记录 Continuous
};

#endif // !DMLINETYPETABLE_H
