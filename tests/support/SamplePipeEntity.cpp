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

/// @file SamplePipeEntity.cpp
/// @brief 示例自定义实体"管道"的实现

#include "support/SamplePipeEntity.h"

#include <cmath>

#include "DmColor.h"
#include "IGiGeometry.h"
#include "IGiSubEntityTraits.h"
#include "Stream.h"

TYPESYSTEM_SOURCE_NAMED(SamplePipeEntity, DmCustomEntity, "ext.sample.Pipe", 1)

namespace
{
/// @brief 第一段的单位方向；折点不够或重合时为 X 轴
DmVector firstDirection(const std::vector<DmVector>& vertices)
{
    if (vertices.size() < 2)
    {
        return DmVector(1.0, 0.0);
    }
    const DmVector d = vertices[1] - vertices[0];
    const double len = d.magnitude();
    return len > 0.0 ? d / len : DmVector(1.0, 0.0);
}
}  // namespace

SamplePipeEntity::SamplePipeEntity(const std::vector<DmVector>& vertices, double diameter)
    : m_vertices(vertices)
    , m_diameter(diameter)
{
    update();
}

DmEntity* SamplePipeEntity::clone() const
{
    auto* copy = new SamplePipeEntity(*this);
    copy->m_ulID = DmId();
    return copy;
}

void SamplePipeEntity::setVertices(const std::vector<DmVector>& vertices)
{
    m_vertices = vertices;
    update();
}

void SamplePipeEntity::setDiameter(double diameter)
{
    m_diameter = diameter;
    update();
}

void SamplePipeEntity::worldDraw(IGiWorldDraw& wd) const
{
    if (m_vertices.size() < 2)
    {
        return;
    }
    IGiGeometry& g = wd.geometry();
    const double r = m_diameter * 0.5;
    // 中心线
    g.polyline(m_vertices, {}, {}, GiPolylineFlags::None);
    // 每段两条边线
    for (std::size_t i = 0; i + 1 < m_vertices.size(); ++i)
    {
        const DmVector d = m_vertices[i + 1] - m_vertices[i];
        const double len = d.magnitude();
        if (!(len > 0.0))
        {
            continue;
        }
        const DmVector n = DmVector(-d.y, d.x) / len * r;
        const DmVector left[2] = {m_vertices[i] + n, m_vertices[i + 1] + n};
        const DmVector right[2] = {m_vertices[i] - n, m_vertices[i + 1] - n};
        g.polyline(left, {}, {}, GiPolylineFlags::None);
        g.polyline(right, {}, {}, GiPolylineFlags::None);
    }
    // 朝外的半圆端头：起点从左侧法向转到右侧（经过背向第一段的一侧），终点从右侧法向转到左侧
    const DmVector d0 = firstDirection(m_vertices);
    const double a0 = std::atan2(d0.y, d0.x);
    g.arc(m_vertices.front(), r, a0 + M_PI_2, M_PI);
    const DmVector dn = m_vertices.back() - m_vertices[m_vertices.size() - 2];
    const double an = std::atan2(dn.y, dn.x);
    g.arc(m_vertices.back(), r, an - M_PI_2, M_PI);
    // 第一段中点的红色实心箭头
    const DmVector mid = (m_vertices[0] + m_vertices[1]) * 0.5;
    const DmVector n0(-d0.y, d0.x);
    const DmVector arrow[4] = {mid + d0 * r, mid - d0 * r + n0 * (r * 0.5), mid - d0 * (r * 0.5),
                               mid - d0 * r - n0 * (r * 0.5)};
    const std::uint32_t indices[6] = {0, 1, 2, 0, 2, 3};
    wd.traits().setColor(DmColor(255, 0, 0));
    g.triangles(arrow, indices);
}

void SamplePipeEntity::move(const DmVector& offset)
{
    for (DmVector& v : m_vertices)
    {
        v.move(offset);
    }
    update();
}

void SamplePipeEntity::rotate(const DmVector& center, const DmVector& angleVector)
{
    for (DmVector& v : m_vertices)
    {
        v.rotate(center, angleVector);
    }
    update();
}

void SamplePipeEntity::scale(const DmVector& center, const DmVector& factor)
{
    for (DmVector& v : m_vertices)
    {
        v.scale(center, factor);
    }
    // 管径按面积比例缩放（等比时即缩放比例）
    m_diameter *= std::sqrt(std::fabs(factor.x * factor.y));
    update();
}

void SamplePipeEntity::mirror(const DmVector& axisPoint1, const DmVector& axisPoint2)
{
    for (DmVector& v : m_vertices)
    {
        v.mirror(axisPoint1, axisPoint2);
    }
    update();
}

DmVector SamplePipeEntity::diameterGrip() const
{
    if (m_vertices.empty())
    {
        return DmVector(false);
    }
    const DmVector d0 = firstDirection(m_vertices);
    return m_vertices.front() + DmVector(-d0.y, d0.x) * (m_diameter * 0.5);
}

DmVectorSolutions SamplePipeEntity::getRefPoints() const
{
    DmVectorSolutions points;
    for (const DmVector& v : m_vertices)
    {
        points.push_back(v);
    }
    if (!m_vertices.empty())
    {
        points.push_back(diameterGrip());
    }
    return points;
}

void SamplePipeEntity::moveRef(const DmVector& ref, const DmVector& offset)
{
    constexpr double kTolerance = 1.0e-6;
    if (!m_vertices.empty() && ref.distanceTo(diameterGrip()) < kTolerance)
    {
        // 管径 = 起点到拖到的位置的距离的两倍
        m_diameter = 2.0 * (ref + offset).distanceTo(m_vertices.front());
        update();
        return;
    }
    for (DmVector& v : m_vertices)
    {
        if (v.distanceTo(ref) < kTolerance)
        {
            v.move(offset);
        }
    }
    update();
}

void SamplePipeEntity::saveData(OutputStream& out) const
{
    out << static_cast<std::uint32_t>(m_vertices.size());
    for (const DmVector& v : m_vertices)
    {
        out << v.x << v.y;
    }
    out << m_diameter;
}

bool SamplePipeEntity::restoreData(InputStream& in, std::uint32_t version)
{
    if (version > kVersion)
    {
        // 比程序新的数据读不了：宿主改建代理，数据原样保留
        return false;
    }
    std::uint32_t count = 0;
    in >> count;
    m_vertices.assign(count, DmVector(0.0, 0.0));
    for (DmVector& v : m_vertices)
    {
        in >> v.x >> v.y;
    }
    in >> m_diameter;
    return true;
}
