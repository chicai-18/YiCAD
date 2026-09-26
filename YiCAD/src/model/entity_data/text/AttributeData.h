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

/// @file AttributeData.h
/// @brief 属性数据类

#ifndef ATTRIBUTEDATA_H
#define ATTRIBUTEDATA_H

#include "EntityData.h"
#include <QString>

/// @brief 属性数据
class AttributeData : public EntityData
{
public:
    AttributeData();

    /// @brief 使用标签构造
    /// @param [in] tag 属性标签
    AttributeData(const QString& tag);

    /// @brief 获取标签
    /// @return 标签字符串
    QString getTag() const;

    /// @brief 设置标签
    /// @param [in] tag 标签字符串
    void setTag(const QString& tag);

private:
    QString m_strTag; ///< 标记
    // 默认是基类的text，不另外保存
};

#endif // ATTRIBUTEDATA_H
