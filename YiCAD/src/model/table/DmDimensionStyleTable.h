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

/// @file DmDimensionStyleTable.h
/// @brief 标注样式表，管理文档中所有标注样式

#ifndef DMDIMENSIONSTYLETABLE_H
#define DMDIMENSIONSTYLETABLE_H

#include <vector>
#include <memory>
#include <QString>
#include "DmDimensionStyle.h"
#include "DmBlockTable.h"
#include "DmPen.h"
#include "TableBase.h"

class DmBlock;
class DmDocument;

/// @brief 标注样式表
/// @details 表中可能包含名字相同但 id 不同的样式，但相同名字的样式只能有一个是未删除的，这需要外部保证
class DmDimensionStyleTable : public ITable
{
public:
    using iterator = FilterIterator<std::vector<DmDimensionStyle*>::iterator>;
public:
    DmDimensionStyleTable() = default;
    ~DmDimensionStyleTable();

    void setDocument(DmDocument *pDoc) override;
    void startModify(DmObject* e) override;

    /// @brief 补上新文档自带而表里缺少的条目：标注箭头块与 "ISO-25" 标注样式
    /// @details 已有的同名条目不动；没有当前标注样式时以 "ISO-25" 为当前。"ISO-25" 取
    ///          "Standard" 文字样式，所以要在文字样式表补齐之后调用
    void addMissingDefaults();

    /// @brief 直接删除全部标注样式（不产生命令），当前标注样式随之置空；箭头块在块表里，不在此处理
    void clear_direct();
    /// @brief 添加标注样式
    void add(DmDimensionStyle* e);
    /// @brief 通过 id 移除标注样式
    void remove(DmId id);
    /// @brief 移除标注样式
    void remove(DmDimensionStyle* e);
    /// @brief 查找标注样式（不能获得已删除的标注样式）
    DmDimensionStyle* find(const QString& name);
    /// @brief 通过 id 查找标注样式，采用此方法可获得已删除的（未从内存删除）标注样式
    DmDimensionStyle* find(const DmId& id);
    /// @brief 表中不存在该实体，直接添加
    bool add_direct(DmDimensionStyle* e);
    /// @brief 直接删除从表中实体
    bool remove_direct(DmDimensionStyle* e);

    /// @brief 获取当前激活的标注样式
    DmDimensionStyle* getActive();
    /// @brief 按名称激活标注样式
    void activate(const QString& name);
    /// @brief 激活标注样式
    void activate(DmDimensionStyle* dimStyle);
    /// @brief 直接激活标注样式（不产生命令）
    void activate_direct(DmDimensionStyle* dimStyle);

    iterator begin();
    iterator end();
    unsigned int count() const;

    /// @brief 获取箭头块表
    DmBlockTable* getArrowBlocks();
private:
    /// @brief 初始化箭头块（块表里已有的同名块保留）
    void initArrowBlocks();
    /// @brief 把箭头块放进块表；块表里已有同名的块时丢弃它
    void addArrowBlock(DmBlock* blk);
    /// @brief 创建填充圆
    void createFillCircle(DmBlock* blk, const DmPen& pen, const DmVector& center, double r);

private:
    std::unordered_map<DmId, DmDimensionStyle*>    m_dimStyleMap;      ///< 标注样式字典
    std::vector<DmDimensionStyle*>                  m_styles;           ///< 标注样式列表
    DmDimensionStyle*                               m_pActiveStyle = nullptr; ///< 当前激活的标注样式

};
#endif