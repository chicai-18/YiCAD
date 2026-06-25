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

/// @file MetaDimLinears.h
/// @brief 线性标注实体序列化容器头文件

#ifndef PERSISTENCE_META_DIMLINEARS_H
#define PERSISTENCE_META_DIMLINEARS_H

#include <vector>
#include <list>

#include "DmEntity.h"
#include "Persistence.h"

class DmDocument;

class MetaDimLinearsContainer : Persistence
{
public:
    /// @brief 构造函数
    /// @param pDoc 文档指针
    MetaDimLinearsContainer(DmDocument* pDoc);

    /// @brief 析构函数
    virtual ~MetaDimLinearsContainer()
    {
    }

    // persistent helper

    /// @brief 获取内存大小
    /// @return 内存大小（字节）
    unsigned int getMemSize() const override;

    /// @brief 保存为XML格式
    /// @param wrt Writer对象
    void saveXML(Writer& wrt) const override;

    /// @brief 从XML恢复
    /// @param reader XMLReader对象
    void restoreXML(XMLReader& reader) override;

    /// @brief 保存为流格式
    /// @param wrt 输出流
    void saveStream(OutputStream& wrt) const override;

    /// @brief 从流恢复
    /// @param rdr 输入流
    void restoreStream(InputStream& rdr) override;

    /// @brief 设置实体列表
    /// @param entities 实体列表
    void setEntities(std::list<DmEntity*>& entities);

private:
    DmDocument*             m_pDocument = nullptr;  ///< 文档指针
    std::vector<PAIR>       m_revs;                 ///< 修订版本列表
    std::list<DmEntity*>    m_entities;             ///< 实体列表
};

#endif // PERSISTENCE_META_DIMLINEARS_H
