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

/// @file DmDocumentTransfer.cpp
/// @brief 实体改归目标文档时解析引用的表项

#include "DmDocumentTransfer.h"

#include "DmBlock.h"
#include "DmBlockTable.h"
#include "DmDimensionStyle.h"
#include "DmDimensionStyleTable.h"
#include "DmDocument.h"
#include "DmLayer.h"
#include "DmLayerTable.h"
#include "DmLineType.h"
#include "DmLineTypeTable.h"
#include "DmPen.h"
#include "DmTextStyle.h"
#include "DmTextStyleTable.h"

namespace
{
/// @brief DmLineTypeTable 的静态线型：全局对象，不属于任何文档
/// @details 随层、随块线型是各文档线型表里的保留记录，与其他线型一样按名字换成目标文档的记录
bool isStaticLineType(const DmLineType* lineType)
{
    return lineType == DmLineTypeTable::Continuous || lineType == DmLineTypeTable::DashLine;
}

/// @brief 按约定把复制出的条目放进目标表：直接放入，或经命令放入
template <typename Table, typename Entry>
void addTo(Table* table, Entry* entry, DmDocumentTransfer::Missing missing)
{
    if (missing == DmDocumentTransfer::Missing::AddDirect)
    {
        table->add_direct(entry);
    }
    else
    {
        table->add(entry);
    }
}
}  // namespace

DmDocumentTransfer::DmDocumentTransfer(DmDocument& target, Missing missing)
    : m_target(target)
    , m_missing(missing)
{
}

DmDocument& DmDocumentTransfer::target() const
{
    return m_target;
}

DmLayer* DmDocumentTransfer::layer(DmLayer* source)
{
    if (!source)
    {
        return nullptr;
    }
    DmLayerTable* table = m_target.getLayerTable();
    if (DmLayer* found = table->find(source->getName()))
    {
        return found;
    }
    if (m_missing == Missing::KeepSource)
    {
        return source;
    }
    DmLayer* copy = source->clone();
    copy->setDocument(&m_target);
    DmPen pen = copy->getPen();
    pen.setLineType(lineType(pen.getLineType()));
    copy->setPen(pen);
    addTo(table, copy, m_missing);
    return copy;
}

DmLineType* DmDocumentTransfer::lineType(DmLineType* source)
{
    if (!source || isStaticLineType(source))
    {
        return source;
    }
    DmLineTypeTable* table = m_target.getLineTypeTable();
    if (DmLineType* found = table->find(source->getLineTypeName()))
    {
        return found;
    }
    if (m_missing == Missing::KeepSource)
    {
        return source;
    }
    auto* copy = new DmLineType(source);  // 只复制线型数据，id 由目标表分配
    copy->setDocument(&m_target);
    addTo(table, copy, m_missing);
    return copy;
}

DmTextStyle* DmDocumentTransfer::textStyle(DmTextStyle* source)
{
    if (!source)
    {
        return nullptr;
    }
    DmTextStyleTable* table = m_target.getTextStyleTable();
    if (DmTextStyle* found = table->find(source->getName()))
    {
        return found;
    }
    if (m_missing == Missing::KeepSource)
    {
        return source;
    }
    DmTextStyle* copy = source->clone();
    copy->resetId();
    copy->setDocument(&m_target);
    addTo(table, copy, m_missing);
    return copy;
}

DmDimensionStyle* DmDocumentTransfer::dimStyle(DmDimensionStyle* source)
{
    if (!source)
    {
        return nullptr;
    }
    DmDimensionStyleTable* table = m_target.getDimStyleTable();
    if (DmDimensionStyle* found = table->find(source->getName()))
    {
        return found;
    }
    if (m_missing == Missing::KeepSource)
    {
        return source;
    }
    auto* copy = new DmDimensionStyle(*source);  // 只复制样式数据，id 由目标表分配
    copy->getDataRef().setTextStyle(textStyle(source->getDataConstRef().textStyle()));
    copy->getDataRef().setDimLineType(lineType(source->getDataConstRef().dimLineType()));
    copy->getDataRef().setBoundLineType(lineType(source->getDataConstRef().boundLineType()));
    copy->setDocument(&m_target);
    addTo(table, copy, m_missing);
    return copy;
}

DmBlock* DmDocumentTransfer::block(DmBlock* source)
{
    if (!source)
    {
        return nullptr;
    }
    DmBlockTable* table = m_target.getBlockTable();
    if (DmBlock* found = table->find(source->getName()))
    {
        return found;
    }
    if (m_missing == Missing::KeepSource)
    {
        return source;
    }
    DmBlock* copy = source->copyInto(*this);
    if (m_missing == Missing::AddDirect)
    {
        // DmBlockTable::add_direct 不分配 id
        m_target.getIdManager()->assignID(copy);
        table->add_direct(copy);
    }
    else
    {
        table->add(copy);
    }
    return copy;
}
