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


/// @file DmBlock.cpp
/// @brief 图块定义类的实现，包括实体管理、序列化、嵌套查找等功能

#include <iostream>
#include "DmBlock.h"

#include "DmDocument.h"
#include "DmBlockTable.h"
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
#include "DmAttributeDefinition.h"
#include "DmAttribute.h"
#include "DmDimLinear.h"
#include "DmEntityHelper.h"

TYPESYSTEM_SOURCE(DmBlock, DmObject, 0)

DmBlockData::DmBlockData(const QString& _name, const DmVector& _basePoint, bool _frozen, const QString& path, const QString& modify)
    : name(_name)
    , basePoint(_basePoint)
    , frozen(_frozen)
    , pathName(path)
    , modifyDate(modify)
{
}

bool DmBlockData::isValid() const
{
    return (!name.isEmpty() && basePoint.valid);
}

DmBlock::DmBlock()
{

}

/// @param doc 该块所属的文档
/// @param d 块的定义数据
DmBlock::DmBlock(DmDocument* doc, const DmBlockData& d)
    : DmObject()
    , data(d)
{
    setDocument(doc);
}

DmBlock::~DmBlock()
{
}

DM::EntityType DmBlock::getEntityType() const
{
    return DM::EntityBlock;
}

DmBlock* DmBlock::clone() const
{
    DmBlock* blk = new DmBlock(*this);
    return blk;
}

void DmBlock::setDocument(DmDocument* pDoc)
{
    DmObject::setDocument(pDoc);
    m_entityTable.setDocument(pDoc);
}

QString DmBlock::getName() const
{
    return data.name;
}

QString DmBlock::getPath() const
{
    return data.pathName;
}

void DmBlock::setPath(const QString& path)
{
    data.pathName = path;
}

QString DmBlock::getModifyDate()
{
    return data.modifyDate;
}

void DmBlock::setModifyDate(const QString& date)
{
    data.modifyDate = date;
}

DmVector DmBlock::getBasePoint() const
{
    return data.basePoint;
}

void DmBlock::setBasePoint(const DmVector& pt)
{
    data.basePoint = pt;
}

bool DmBlock::blockIsModify()
{
    return data.isMidify;
}

void DmBlock::setBlockIsModify(bool is)
{
    data.isMidify = is;
}

//bool DmBlock::save(bool isAutoSave)
//{
//    //todo :暂不支持块的保存
//    return false;
//}

//bool DmBlock::saveAs(const QString& filename, const QString& formatType, bool force)
//{
//    DmDocument* g = getDocument();
//    if (g)
//    {
//        return g->saveAs(filename, formatType, force);
//    }
//    else
//    {
//        return false;
//    }
//}

//bool DmBlock::open(const QString&)
//{
//    // do nothing
//    return false;
//}

void DmBlock::setName(const QString& n)
{
    data.name = n;
}

bool DmBlock::isFrozen() const
{
    return data.frozen;
}

void DmBlock::freeze(bool freeze)
{
    data.frozen = freeze;
}

void DmBlock::toggle()
{
    data.frozen = !data.frozen;
}

bool DmBlock::loadTemplate(const QString&)
{
    // 无需处理
    return false;
}

void DmBlock::visibleInBlockList(bool v)
{
    data.visibleInBlockList = v;
}

bool DmBlock::isVisibleInBlockList() const
{
    return data.visibleInBlockList;
}

void DmBlock::selectedInBlockList(bool v)
{
    data.selectedInBlockList = v;
}

bool DmBlock::isSelectedInBlockList() const
{
    return data.selectedInBlockList;
}

QStringList DmBlock::findNestedInsert(const QString& bName)
{
    QStringList bnChain;

    for (DmEntity* e : m_entityTable)
    {
        if (e->getEntityType() == DM::EntityBlockReference)
        {
            DmBlockReference* i = ((DmBlockReference*)e);
            QString iName = i->getName();
            if (iName == bName)
            {
                bnChain << data.name;
                break;
            }
            else
            {
                DmBlockTable* blockTable = getDocument()->getBlockTable();
                if (blockTable)
                {
                    DmBlock* nestedBlock = blockTable->find(iName);
                    if (nestedBlock)
                    {
                        QStringList nestedChain;
                        nestedChain = nestedBlock->findNestedInsert(bName);
                        if (!nestedChain.empty())
                        {
                            bnChain << data.name;
                            bnChain << nestedChain;
                            break;
                        }
                    }
                }
            }
        }
    }

    return bnChain;
}

void DmBlock::collectNestedBlockNames(QStringList& names, QSet<QString>& visited)
{
    if (visited.contains(data.name))
        return;

    visited.insert(data.name);
    names.append(data.name);

    DmBlockTable* blockTable = getDocument()->getBlockTable();
    if (!blockTable)
        return;

    for (auto e : m_entityTable)
    {
        if (e->getEntityType() == DM::EntityBlockReference)
        {
            DmBlockReference* ref = static_cast<DmBlockReference*>(e);
            QString refName = ref->getName();
            if (!visited.contains(refName))
            {
                DmBlock* nestedBlock = blockTable->find(refName);
                if (nestedBlock)
                {
                    nestedBlock->collectNestedBlockNames(names, visited);
                }
            }
        }
    }
}

bool DmBlock::hasAttributeDefinitions() const
{
    for (auto ent : m_entityTable)
    {
        if (ent->getEntityType() == DM::EntityAttributeDefinition)
        {
            return true;
        }
    }
    return false;
}

std::list<DmAttributeDefinition*> DmBlock::getAttributeDefinitions() const
{
    std::list<DmAttributeDefinition*> attrDefs;
    for (auto ent : m_entityTable)
    {
        if (ent->getEntityType() == DM::EntityAttributeDefinition)
        {
            attrDefs.emplace_back(static_cast<DmAttributeDefinition*>(ent));
        }
    }
    return attrDefs;
}

void DmBlock::saveStream(OutputStream& wrt) const
{
    DmObject::saveStream(wrt);

    auto name = getName().toStdString();
    auto basePoint = getBasePoint();
    auto frozen = isFrozen();
    wrt << name << (double)basePoint.x << (double)basePoint.y;

    wrt << (uint32_t)m_entityTable.count();
    for (auto ent : m_entityTable)
    {
        if (isSaveEntType(ent->getEntityType()))
        {
            auto strType = DmEntityHelper::getEntityNameByType(ent->getEntityType());
            wrt << strType;
            ent->saveStream(wrt);
        }
    }
}

void DmBlock::restoreStream(InputStream& reader, const std::vector<PAIR>& revs)
{
    DmObject::restoreStream(reader, revs);

    int fileRev = getRevisionId("DmBlock", revs);
    if (revId > fileRev)
    {
        // 老文件格式
        restoreStreamWithRev(reader, fileRev);
    }
    else
    {
        std::string name;
        DmVector basePoint;
        bool frozen = false;
        uint32_t entSize = 0;
        reader >> (std::string&)name >> (double&)basePoint.x >> (double&)basePoint.y >> (uint32_t&)entSize;

        setName(QString::fromStdString(name));
        setBasePoint(basePoint);
        data.frozen = frozen;

        m_entityTable.clear_direct();
        for (uint32_t i = 0; i < entSize; ++i)
        {
            getEntity(reader, revs);
        }
    }
}

void DmBlock::restoreStreamWithRev(InputStream& rdr, int rev)
{
    if (rev == 0)
    {
    }
    else // 发生较大版本变更，例如 DmBlock 的父类发生变化
    {
        // 第一步：逐项读取旧版本数据
    }
}

void DmBlock::getEntity(InputStream& rdr, const std::vector<PAIR>& revs)
{
    std::string strType;
    rdr >> strType;
    DmEntity* pEntity = DmEntityHelper::createEntityByName(strType);

    if (pEntity)
    {
        pEntity->setParent(nullptr);
        pEntity->setDocument(getDocument());
        pEntity->restoreStream(rdr, revs);
        m_entityTable.add_direct(pEntity);
    }
}

bool DmBlock::isSaveEntType(const DM::EntityType type) const
{
    switch (type)
    {
    case DM::EntityLine:
    case DM::EntityCircle:
    case DM::EntityPoint:
    case DM::EntityPolyline:
    case DM::EntityArc:
    case DM::EntityEllipse:
    case DM::EntitySolid:
    case DM::EntitySpline:
    case DM::EntityRay:
    case DM::EntityXline:
    case DM::EntityBlockReference:
    case DM::EntityAttribute:
    case DM::EntityAttributeDefinition:
    case DM::EntityText:
    case DM::EntityDimAligned:
    case DM::EntityDimLinear:
    case DM::EntityDimRadial:
    case DM::EntityDimDiametric:
    case DM::EntityDimAngular:
    case DM::EntityDimLeader:
    case DM::EntityMText:
    case DM::EntityHatch:
    //case DM::EntityImage:
        return true;
    default:
        return false;
    }
}
