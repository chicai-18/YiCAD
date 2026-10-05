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

/// @file DmProxyEntity.cpp
/// @brief 代理实体的实现

#include "DmProxyEntity.h"

#include "DmBlock.h"
#include "DmDocumentTransfer.h"
#include "DmLayer.h"
#include "DmLineType.h"
#include "IGiGeometry.h"
#include "Stream.h"

TYPESYSTEM_SOURCE(DmProxyEntity, DmCustomEntity, 0)

DmProxyEntity::DmProxyEntity(const QString& originalClassName, DmProxyFlags proxyFlags)
    : m_className(originalClassName)
    , m_proxyFlags(proxyFlags)
{
}

DmEntity* DmProxyEntity::clone() const
{
    auto* copy = new DmProxyEntity(*this);
    copy->m_ulID = DmId();
    return copy;
}

void DmProxyEntity::worldDraw(IGiWorldDraw& wd) const
{
    // 属性按代理自己的（setAttributes 取公共属性，可以改过）；图元重放代理图形的图元段。
    // 没有变换过时不包变换，输出与原实体的 worldDraw 相同
    const bool transformed = !m_transform.isIdentity();
    if (transformed)
    {
        wd.geometry().pushTransform(m_transform);
    }
    GiStreamDrawable(m_graphics).worldDraw(wd);
    if (transformed)
    {
        wd.geometry().popTransform();
    }
}

void DmProxyEntity::accumulate(const GiTransform& step)
{
    if (!hasFlag(m_proxyFlags, DmProxyFlags::Transform))
    {
        return;
    }
    m_transform = step * m_transform;
    update();
}

void DmProxyEntity::move(const DmVector& offset)
{
    accumulate(GiTransform::translation(offset));
}

void DmProxyEntity::rotate(const DmVector& center, const DmVector& angleVector)
{
    accumulate(GiTransform::rotation(angleVector.angle(), center));
}

void DmProxyEntity::scale(const DmVector& center, const DmVector& factor)
{
    accumulate(GiTransform::scaling(factor, center));
}

void DmProxyEntity::mirror(const DmVector& axisPoint1, const DmVector& axisPoint2)
{
    accumulate(GiTransform::mirroring(axisPoint1, axisPoint2));
}

void DmProxyEntity::saveData(OutputStream& out) const
{
    for (char c : m_data)
    {
        out << static_cast<std::uint8_t>(c);
    }
}

bool DmProxyEntity::restoreData(InputStream& in, std::uint32_t version)
{
    m_data.clear();
    while (!in.end())
    {
        std::uint8_t value = 0;
        in >> value;
        m_data.push_back(static_cast<char>(value));
    }
    m_version = version;
    return true;
}

void DmProxyEntity::saveDataBytes(OutputStream& out) const
{
    out << m_version;
    out << static_cast<std::uint64_t>(m_data.size());
    for (char c : m_data)
    {
        out << static_cast<std::uint8_t>(c);
    }
}

bool DmProxyEntity::restoreDataBytes(const std::string& bytes, std::uint32_t version)
{
    m_data = bytes;
    m_version = version;
    return true;
}

void DmProxyEntity::applyStoredTransform(const GiTransform& transform)
{
    m_transform = transform;
}

void DmProxyEntity::transferReferences(DmDocumentTransfer& transfer)
{
    m_graphics.remapReferences([&transfer](GiReferenceKind kind, const void* object) -> const void*
    {
        if (!object)
        {
            return object;
        }
        switch (kind)
        {
        case GiReferenceKind::Layer:
            return transfer.layer(const_cast<DmLayer*>(static_cast<const DmLayer*>(object)));
        case GiReferenceKind::LineType:
            return transfer.lineType(const_cast<DmLineType*>(static_cast<const DmLineType*>(object)));
        case GiReferenceKind::Drawable:
        {
            const auto* block = dynamic_cast<const DmBlock*>(static_cast<const IGiDrawable*>(object));
            if (!block)
            {
                return object;
            }
            return static_cast<const IGiDrawable*>(transfer.block(const_cast<DmBlock*>(block)));
        }
        case GiReferenceKind::Font:
        case GiReferenceKind::Image:
            // 字体是全进程的；图片像素不属于文档
            return object;
        }
        return object;
    });
}

bool DmProxyEntity::allows(const DmEntity* entity, DmProxyFlags operation)
{
    if (!entity || entity->getEntityType() != DM::EntityCustom)
    {
        return true;
    }
    const auto* custom = static_cast<const DmCustomEntity*>(entity);
    return !custom->isProxy() || hasFlag(custom->proxyFlags(), operation);
}

std::vector<DmEntity*> DmProxyEntity::filterAllowed(const std::vector<DmEntity*>& entities, DmProxyFlags operation,
                                                    int* skipped)
{
    std::vector<DmEntity*> allowed;
    allowed.reserve(entities.size());
    int count = 0;
    for (DmEntity* entity : entities)
    {
        if (allows(entity, operation))
        {
            allowed.push_back(entity);
        }
        else
        {
            ++count;
        }
    }
    if (skipped)
    {
        *skipped = count;
    }
    return allowed;
}
