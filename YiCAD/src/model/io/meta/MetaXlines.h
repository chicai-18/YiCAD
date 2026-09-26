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

/// @file MetaXlines.h
/// @brief 构造线(Xline)实体序列化容器头文件

#ifndef PERSISTENCE_META_XLINES_H
#define PERSISTENCE_META_XLINES_H

#include <vector>
#include <list>

#include "DmEntity.h"
#include "Persistence.h"

class DmDocument;

class MetaXlinesContainer : Persistence
{
public:
    /// @brief 构造函数
    /// @param [in] pDoc 文档指针
    MetaXlinesContainer(DmDocument* pDoc);

    /// @brief 析构函数
    virtual ~MetaXlinesContainer() {};

    /// @brief 获取内存大小
    /// @return 内存大小
    unsigned int getMemSize() const override;

    /// @brief 保存XML数据
    /// @param [in] wrt 写入器引用
    void saveXML(Writer& wrt) const override;

    /// @brief 从XML恢复数据
    /// @param [in] reader XML读取器引用
    void restoreXML(XMLReader& reader) override;

    /// @brief 保存二进制数据流
    /// @param [in] wrt 输出流引用
    void saveStream(OutputStream& wrt) const override;

    /// @brief 从二进制数据流恢复
    /// @param [in] rdr 输入流引用
    void restoreStream(InputStream& rdr) override;

    /// @brief 设置实体列表
    /// @param [in] entities 实体列表引用
    void setEntities(std::list<DmEntity*>& entities);

private:
    DmDocument*             m_pDocument = nullptr;  ///< 文档指针
    std::vector<PAIR>       m_revs;                 ///< 版本信息列表
    std::list<DmEntity*>    m_entities;             ///< 实体列表
};

#endif // PERSISTENCE_META_XLINES_H
