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


/// @file DmCharTemplateList.h
/// @brief 文字模板列表类，管理文字模板（DmCharTemplate）

#ifndef DMCHARTEMPLATELIST_H
#define DMCHARTEMPLATELIST_H

#include <map>
#include <QString>

class DmCharTemplate;
class DmFont;

/// @brief 文字模板列表。管理文字模板（DmCharTemplate）的类
class DmCharTemplateList
{
public:
    /// @brief 析构函数，释放所有文字模板
    ~DmCharTemplateList();

    /// @brief 添加文字模板
    /// @param name 文字模板名
    /// @param letter 文字模板指针
    void add(const QString& name, DmCharTemplate* letter);

    /// @brief 查找文字模板
    /// @param name 文字模板名
    /// @return 找到返回模板指针，否则返回nullptr
    DmCharTemplate* find(const QString& name) const;

    /// @brief 获得所属字体
    /// @return 字体指针
    DmFont* getFont() const;

    /// @brief 设置所属字体
    /// @param f 字体指针
    void setFont(DmFont* f);

private:
    std::map<QString, DmCharTemplate*> m_fontChars; ///< 管理的文字模板。里面的DmFontChar由析构函数释放
    DmFont* m_pFont{nullptr}; ///< 文字模板所属字体
};

#endif //!DMCHARTEMPLATELIST_H
