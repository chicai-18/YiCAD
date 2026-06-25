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

/// @file DmTextStyleTable.h
/// @brief 文字样式表，管理文档中所有文字样式

#ifndef DMTEXTSTYLETABLE_H
#define DMTEXTSTYLETABLE_H

#include <vector>
#include <memory>
#include <QString>
#include "DmTextStyle.h"
#include "TableBase.h"

class DmDocument;

/// @brief 文字样式表
/// @details 用来管理文档中所有的文字样式
class DmTextStyleTable : public ITable
{
public:
    using iterator = FilterIterator<std::vector<DmTextStyle*>::iterator>;
public:
    DmTextStyleTable() = default;
    ~DmTextStyleTable();

    void setDocument(DmDocument *pDoc) override;
    void startModify(DmObject* e) override;
    /// @brief 添加文字样式
    void add(DmTextStyle* e);
    /// @brief 通过 id 移除文字样式
    void remove(DmId id);
    /// @brief 移除文字样式
    void remove(DmTextStyle* e);
    /// @brief 查找文字样式（不能获得已删除的文字样式）
    DmTextStyle* find(const QString& name);
    /// @brief 通过 id 查找文字样式，采用此方法可获得已删除的文字样式
    DmTextStyle* find(const DmId& id);
    /// @brief 表中不存在该实体，直接添加
    bool add_direct(DmTextStyle* e);
    /// @brief 直接删除从表中实体
    bool remove_direct(DmTextStyle* e);

    /// @brief 获取当前激活的文字样式
    DmTextStyle* getActive();
    /// @brief 按名称激活文字样式
    void activate(const QString& name);
    /// @brief 激活文字样式
    void activate(DmTextStyle* textStyle);
    /// @brief 直接激活文字样式
    void activate_direct(DmTextStyle* textStyle);

    iterator begin();
    iterator end();

    /// @brief 获得默认文字样式，可以绘制问号 "?"
    static DmTextStyle* getDefaultStyle();
    /// @brief 初始化默认文字样式，用来绘制问号
    static void initDefaultStyle();
    /// @brief 检查默认样式是否已初始化
    static bool isDefaultStyleInit();

private:
    std::unordered_map<DmId, DmTextStyle*> m_textStyleMap;       ///< 文字样式字典
    std::vector<DmTextStyle*>              m_styles;             ///< 文字样式列表
    DmTextStyle*                           m_pActiveStyle;       ///< 当前激活的文字样式

    /// 用来绘制未知符号 "?" 的文字样式
    static std::unique_ptr<DmTextStyle>    g_defaultStyle;
    static bool                            g_defaultStyleInit;
};
#endif // DMTEXTSTYLETABLE_H
