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

/// @file MetaCustomEntities.h
/// @brief 自定义实体（含代理实体）的序列化容器（RENDER_PLAN.md 第 4.8.3 节）
/// @details 每个实体一条 DmCustomEntity::writeRecord 记录：类名、代理权限、公共属性与数据字节、代理图形。
///          读回时类没有注册的建代理实体，再存盘原样写回

#ifndef PERSISTENCE_META_CUSTOM_ENTITIES_H
#define PERSISTENCE_META_CUSTOM_ENTITIES_H

#include <list>
#include <vector>

#include "DmEntity.h"
#include "Persistence.h"

class DmDocument;

class MetaCustomEntitiesContainer : Persistence
{
public:
    /// @param pDoc 文档指针
    explicit MetaCustomEntitiesContainer(DmDocument* pDoc);

    unsigned int getMemSize() const override;
    void saveXML(Writer& wrt) const override;
    void restoreXML(XMLReader& reader) override;
    void saveStream(OutputStream& wrt) const override;
    void restoreStream(InputStream& rdr) override;

    /// @brief 设置要写出的实体（都是 DM::EntityCustom）
    void setEntities(std::list<DmEntity*>& entities);

private:
    DmDocument*             m_pDocument = nullptr; ///< 文档指针
    std::vector<PAIR>       m_revs;                ///< 文件里 DmObject 到 DmCustomEntity 各层的版本
    std::list<DmEntity*>    m_entities;            ///< 要写出的实体
};

#endif // PERSISTENCE_META_CUSTOM_ENTITIES_H
