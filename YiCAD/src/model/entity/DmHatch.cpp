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


/// @file DmHatch.cpp
/// @brief 填充（Hatch）实体实现，支持实体填充和图案填充

#include <iostream>
#include <cmath>
#include <memory>

#include <QPainterPath>
#include <QBrush>
#include <QString>
#include "InfoArea.h"
#include "Information.h"
#include "DmPattern.h"
#include "DmPatternList.h"
#include "Math2d.h"
#include "Debug.h"
#include "DmConstructionLine.h"
#include "DmLine.h"
#include "DmArc.h"
#include "DmCircle.h"
#include "DmPoint.h"
#include "DmPolyline.h"
#include "DmEllipse.h"
#include "DmHatch.h"
#include "DmSolid.h"
#include "DmEntityHelper.h"
#include "ConstrainedDelaunayTriangulation.h"
#include "DmDocument.h"
#include "GeometryMethods.h"
#include "IGiGeometry.h"
#include "HatchPatternClipper.h"
#include "IGiSubEntityTraits.h"

TYPESYSTEM_SOURCE(DmHatch, DmEntity, 0);

DmHatch::DmHatch(DmEntity* parent, const HatchData& hatchdata)
    : DmEntity(parent)
    , data(hatchdata)
    , m_filledEntities(new DmEntityContainer())
{
    calculateBorders();
}

DmHatch::DmHatch(DmEntity* parent, DmHatch& hatchdata)
    : DmEntity(parent)
    , m_filledEntities(new DmEntityContainer())
{
    setData(hatchdata.getData());
    calculateBorders();
}

DmHatch::DmHatch(const DmHatch& hatch)
{
    data = hatch.data;
    if (hatch.getBoundary())
    {
        DmRegion* boundary =
            static_cast<DmRegion*>(hatch.data.getBoundary()->clone());
        data.setBoundary(DmRegionPtr(boundary));
    }
    if (hatch.m_filledEntities)
    {
        auto c = static_cast<DmEntityContainer*>(
            hatch.m_filledEntities->clone());
        m_filledEntities = std::make_shared<DmEntityContainer>(c);
    }
    m_patternRuns = hatch.m_patternRuns;
    m_patternLines = hatch.m_patternLines;
}

DmHatch::~DmHatch()
{
}

DmHatch* DmHatch::clone() const
{
    DmHatch* t = new DmHatch(*this);
    t->m_ulID = DmId();
    return t;
}

DM::EntityType DmHatch::getEntityType() const
{
    return DM::EntityHatch;
}

bool DmHatch::isContainer() const
{
    return false;
}

bool DmHatch::isSolid() const
{
    return data.isSolid();
}

void DmHatch::setSolid(bool solid)
{
    data.setIsSolid(solid);
}

void DmHatch::setData(const HatchData& hdata)
{
    data = hdata;
}

QString DmHatch::getPattern() const
{
    return QString::fromStdWString(data.getPatternName());
}

void DmHatch::setPattern(const QString& pattern)
{
    data.setPatternName(pattern.toStdWString());
}

double DmHatch::getScale() const
{
    return data.getPatternScale();
}

void DmHatch::setScale(double scale)
{
    data.setPatternScale(scale);
}

double DmHatch::getAngle() const
{
    return data.getPatternAngle();
}

void DmHatch::setAngle(double angle)
{
    data.setPatternAngle(angle);
}

void DmHatch::setBoundary(DmRegionPtr boundary)
{
    data.setBoundary(boundary);
}

DmRegionPtr DmHatch::getBoundary() const
{
    return data.getBoundary();
}

void DmHatch::transferReferences(DmDocumentTransfer& transfer)
{
    if (DmRegionPtr boundary = data.getBoundary())
    {
        boundary->transferTo(transfer);
    }
}

DmEntityContainerPtr DmHatch::getFilledEntities() const
{
    return m_filledEntities;
}

HatchData& DmHatch::getDataRef()
{
    return data;
}

HatchData DmHatch::getData() const
{
    // 克隆边界信息
    HatchData d = data;
    d.setBoundary(DmRegionPtr()); // 重置边界
    auto boundary = data.getBoundary();
    if (boundary)
    {
        auto boundaryData = boundary->getCloneData();
        boundary->setData(boundaryData);
        d.setBoundary(boundary);
    }
    return d;
}

void DmHatch::calculateBorders()
{
    if (!data.getBoundary())
    {
        return;
    }

    minV = data.getBoundary()->getMin();
    maxV = data.getBoundary()->getMax();
}

void DmHatch::update()
{
    bumpRevision();
    m_patternRuns.clear();
    m_patternLines.clear();
    DmRegionPtr boundary = data.getBoundary();
    // 没有轮廓不能创建填充
    if (!boundary || boundary->size() == 0)
    {
        return;
    }
    // 清除原始填充
    if (m_filledEntities)
    {
        m_filledEntities->clear();
    }
    else
    {
        m_filledEntities = std::make_shared<DmEntityContainer>(nullptr);
    }

    // 实体填充
    if (data.isSolid())
    {
        fillSolid();
        calculateBorders();
        return;
    }

    // 图案填充
    calculateBorders();
    DmPattern pattern = data.getPattern();
    auto* pat = &pattern;
    if (pat->getPatternData().size() == 0)
    {
        pat = DMPATTERNLIST->requestPattern(
            QString::fromStdWString(data.getPatternName()));
    }
    if (!pat)
    {
        return;
    }

    m_filledEntities.reset(new DmEntityContainer(this));
    m_filledEntities->setPen(DmPen(DmColor(DM::FlagByBlock), DM::Width00, DmLineTypeTable::Continuous));
    m_filledEntities->setLayer(nullptr);
    m_filledEntities->setFlag(DM::FlagTemp);
    pat->scale(data.getPatternScale());
    pat->angle(Math2d::rad2deg(data.getPatternAngle()));
    // 每行 [角度, 基点 x, y, 行距位移在线自身坐标系里的 x, y, 划线...]：换成当前坐标里的方向与位移（GiHatchPatternLine）
    for (const std::vector<double>& row : pat->getPatternData())
    {
        if (row.size() < 5)
        {
            continue;
        }
        GiHatchPatternLine line;
        const double angle = row[0];
        line.direction = DmVector(std::cos(angle), std::sin(angle));
        line.base = DmVector(row[1], row[2]);
        line.offset = DmVector(row[3], row[4]).rotate(angle);
        line.dashes.assign(row.begin() + 5, row.end());
        m_patternLines.push_back(std::move(line));
    }

    // 图案线在边界里切出的整段（与图形系统切的同一份算法），再逐段切成划线实体供选择、捕捉、炸开
    std::vector<GiLoop> loops;
    boundary->getLoops(loops);
    std::vector<HatchPatternRun> runs;
    for (std::size_t i = 0; i < m_patternLines.size(); ++i)
    {
        runs.clear();
        HatchPatternClipper::clip(loops, m_patternLines[i], runs);
        for (const HatchPatternRun& r : runs)
        {
            DmHatchPatternRun run;
            run.start = r.start;
            run.end = r.end;
            run.pattern = static_cast<std::uint32_t>(i);
            run.phase = r.phase;
            m_patternRuns.push_back(run);
            addDashEntities(run);
        }
    }
}

void DmHatch::fillSolid()
{
    if (m_filledEntities)
    {
        m_filledEntities->clear();
    }
    std::vector<DmTriangle*> triangles;
    data.getBoundary()->getTriangles<DmTriangle*>(triangles, true);
    DmPen pen(DmColor(DM::FlagByBlock), DM::Width00,
        DmLineTypeTable::Continuous);
    for (auto tri : triangles)
    {
        tri->setParent(m_filledEntities.get());
        tri->setPen(pen);
        tri->setLayer(nullptr);
        m_filledEntities->addEntity(tri);
    }
}

void DmHatch::addDashEntities(const DmHatchPatternRun& run)
{
    DmPen pen(DmColor(DM::FlagByBlock), DM::Width00, DmLineTypeTable::Continuous);
    auto addLine = [this, &pen](const DmVector& a, const DmVector& b) {
        DmLine* line = new DmLine(m_filledEntities.get(), LineData(a, b));
        line->setPen(pen);
        m_filledEntities->addEntity(line);
    };
    const std::vector<double>& dashes =
        run.pattern < m_patternLines.size() ? m_patternLines[run.pattern].dashes : std::vector<double>();
    // 划线画成直线，点画成点
    HatchPatternRun r;
    r.start = run.start;
    r.end = run.end;
    r.phase = run.phase;
    std::vector<std::pair<DmVector, DmVector>> segments;
    std::vector<DmVector> dots;
    HatchPatternClipper::splitRun(r, dashes, segments, dots);
    for (const auto& [a, b] : segments)
    {
        addLine(a, b);
    }
    for (const DmVector& p : dots)
    {
        DmPoint* point = new DmPoint(m_filledEntities.get(), PointData(p));
        point->setPen(pen);
        m_filledEntities->addEntity(point);
    }
}

void DmHatch::move(const DmVector& offset)
{
    data.getBoundary()->move(offset);
    m_filledEntities->move(offset);
    for (DmHatchPatternRun& run : m_patternRuns)
    {
        run.start += offset;
        run.end += offset;
    }
    for (GiHatchPatternLine& line : m_patternLines)
    {
        line.base += offset;
    }
    DmEntity::moveBorders(offset);
}

void DmHatch::rotate(const DmVector& center, const DmVector& angleVector)
{
    data.getBoundary()->rotate(center, angleVector);
    m_filledEntities->rotate(center, angleVector);
    for (DmHatchPatternRun& run : m_patternRuns)
    {
        run.start.rotate(center, angleVector);
        run.end.rotate(center, angleVector);
    }
    for (GiHatchPatternLine& line : m_patternLines)
    {
        line.base.rotate(center, angleVector);
        line.direction.rotate(angleVector);
        line.offset.rotate(angleVector);
    }
    data.setPatternAngle(
        Math2d::correctAngle(data.getPatternAngle() + angleVector.angle()));
    calculateBorders();
}

void DmHatch::scale(const DmVector& center, const DmVector& factor)
{
    data.getBoundary()->scale(center, factor);
    m_filledEntities->scale(center, factor);
    // 图案线的定义按仿射变换：基点、位移照常缩放，方向缩放后取单位向量，划线与相位乘方向上的伸缩
    std::vector<double> stretch(m_patternLines.size(), 1.0);
    for (std::size_t i = 0; i < m_patternLines.size(); ++i)
    {
        GiHatchPatternLine& line = m_patternLines[i];
        line.base.scale(center, factor);
        line.offset = DmVector(line.offset.x * factor.x, line.offset.y * factor.y);
        const DmVector dir(line.direction.x * factor.x, line.direction.y * factor.y);
        const double len = std::hypot(dir.x, dir.y);
        if (len > 0.0)
        {
            line.direction = dir / len;
            stretch[i] = len;
            for (double& d : line.dashes)
            {
                d *= len;
            }
        }
    }
    for (DmHatchPatternRun& run : m_patternRuns)
    {
        run.start.scale(center, factor);
        run.end.scale(center, factor);
        if (run.pattern < stretch.size())
        {
            run.phase *= stretch[run.pattern];
        }
    }
    calculateBorders();
    data.setPatternScale(data.getPatternScale() * factor.x);
}

void DmHatch::mirror(const DmVector& axisPoint1, const DmVector& axisPoint2)
{
    data.getBoundary()->mirror(axisPoint1, axisPoint2);
    m_filledEntities->mirror(axisPoint1, axisPoint2);
    for (DmHatchPatternRun& run : m_patternRuns)
    {
        run.start.mirror(axisPoint1, axisPoint2);
        run.end.mirror(axisPoint1, axisPoint2);
    }
    // 镜像是保距变换：沿方向的位置不变，图案与相位照旧，方向与位移按向量镜像
    const DmVector origin = DmVector(0.0, 0.0).mirror(axisPoint1, axisPoint2);
    for (GiHatchPatternLine& line : m_patternLines)
    {
        line.base.mirror(axisPoint1, axisPoint2);
        line.direction = DmVector(line.direction).mirror(axisPoint1, axisPoint2) - origin;
        line.offset = DmVector(line.offset).mirror(axisPoint1, axisPoint2) - origin;
    }
    calculateBorders();
    double ang = axisPoint1.angleTo(axisPoint2);
    data.setPatternAngle(
        Math2d::correctAngle(data.getPatternAngle() + ang * 2.0));
}

std::list<DmEntity*> DmHatch::getSubEntities() const
{
    std::list<DmEntity*> subEnts = std::list<DmEntity*>();
    // 没有填充实体
    if (!data.getBoundary() || !m_filledEntities)
    {
        return subEnts;
    }
    // 图案填充
    auto listEnts = m_filledEntities->getEntityList();
    if (listEnts.size() > 0)
    {
        auto seSub = std::list<DmEntity*>(listEnts.begin(), listEnts.end());
        subEnts.splice(subEnts.end(), seSub);
    }
    return subEnts;
}

DmVectorSolutions DmHatch::getRefPoints() const
{
    if (!data.getBoundary() || !m_filledEntities)
    {
        return DmVectorSolutions();
    }

    DmVector min = data.getBoundary()->getMin();
    DmVector max = data.getBoundary()->getMax();
    DmVector mid = (min + max) / 2.0;
    return DmVectorSolutions({mid});
}

void DmHatch::moveRef(const DmVector& ref, const DmVector& offset)
{
    move(offset);
    calculateBorders();
}

DmVector DmHatch::getNearestRef(const DmVector& coord, double* dist) const
{
    return DmEntity::getNearestRef(coord, dist);
}

DmVector DmHatch::getNearestEndpoint(const DmVector& coord,
    double* dist) const
{
    return DmVector(false);
}

DmVector DmHatch::getNearestPointOnEntity(const DmVector& coord,
    bool onEntity, double* dist, DmEntity** entity) const
{
    if (entity)
    {
        *entity = const_cast<DmHatch*>(this);
    }
    bool onBoundary = false;
    if (data.getBoundary()->isPointInside(coord, &onBoundary))
    {
        if (dist)
        {
            *dist = 0.0;
            return coord;
        }
    }
    // 区域外
    else
    {
        data.getBoundary()->getNearestPointOnEntity(coord, true, dist, nullptr);
    }

    return DmVector(false);
}

DmVector DmHatch::getNearestCenter(const DmVector& coord,
    double* dist) const
{
    return DmVector(false);
}

DmVector DmHatch::getNearestMiddle(const DmVector& coord,
    double* dist, int middlePoints) const
{
    return DmVector(false);
}

void DmHatch::saveStream(OutputStream& wrt) const
{
    DmEntity::saveStream(wrt);

    auto name = getPattern().toStdString();
    auto angle = getAngle();
    auto scale = getScale();
    bool isSolid = data.isSolid();
    wrt << name << (double)angle << (double)scale << (bool)isSolid;

    // pattern
    auto pattern = data.getPattern().getPatternData();
    auto patternsize = pattern.size();
    wrt << (uint32_t)patternsize;
    for (auto patline : pattern)
    {
        auto patlinesize = patline.size();
        wrt << (uint32_t)patlinesize;
        for (int i = 0; i < patlinesize; i++)
        {
            double dat = patline[i];
            wrt << (double)dat;
        }
    }

    // 边界
    data.getBoundary()->saveStream(wrt);
}

void DmHatch::restoreStream(InputStream& reader,
    const std::vector<PAIR>& revs)
{
    int fileRev = getRevisionId("DmHatch", revs);
    if (revId > fileRev)
    {
        DmEntity::restoreStream(reader, revs);
        // 老文件格式
        restoreStreamWithRev(reader, fileRev);
    }
    else
    {
        restoreStream(reader);
    }
}

void DmHatch::restoreStreamWithRev(InputStream& rdr, int rev)
{
    if (0 == rev)
    {
        // 基本版本，无需额外处理
    }
    else // big change, e.g. change super class of DmCircle
    {
        // step1.
        // read all legacy data one by one
    }
}

void DmHatch::restoreStream(InputStream& reader)
{
    DmEntity::restoreStream(reader);

    std::string name;
    double angle = 0.0;
    double scale = 0.0;
    bool isSolid = false;
    reader >> (std::string&)name >> (double&)angle
           >> (double&)scale >> (bool&)isSolid;
    setPattern(QString::fromStdString(name));
    setAngle(angle);
    setScale(scale);
    data.setIsSolid(isSolid);

    // pattern
    std::vector<std::vector<double>> patdata =
        std::vector<std::vector<double>>{};
    uint32_t patternsize = 0;
    reader >> (uint32_t&)patternsize;
    for (int i = 0; i < patternsize; i++)
    {
        std::vector<double> patline = std::vector<double>{};
        uint32_t patlinesize = 0;
        reader >> (uint32_t&)patlinesize;
        for (int j = 0; j < patlinesize; j++)
        {
            double dat = 0.0;
            reader >> (double&)dat;
            patline.emplace_back(dat);
        }
        patdata.emplace_back(patline);
    }

    std::wstring wname(name.begin(), name.end());
    DmPattern pattern(wname);
    pattern.setPatternData(patdata);
    data.setPattern(pattern);

    // 边界
    data.getBoundary()->restoreStream(reader);
}

void DmHatch::worldDraw(IGiWorldDraw& wd) const
{
    DmRegionPtr boundary = data.getBoundary();
    if (!boundary || boundary->size() == 0)
    {
        return;
    }
    // 边界与孔洞的环（圆弧精确），按奇偶规则填充
    std::vector<GiLoop> loops;
    boundary->getLoops(loops);
    if (loops.empty())
    {
        return;
    }
    if (isSolid())
    {
        wd.geometry().fill(loops, GiFillRule::EvenOdd);
        return;
    }
    if (m_patternLines.empty())
    {
        return;
    }
    // 图案：交出图案线的定义与边界，图案线由接收方切出（与 update() 切的同一份算法），按图案线自己的划线画，
    // 颜色、线宽随填充自己的；图形系统在线太密时改画实心（RENDER_PLAN.md 第 4.3.10 节）。对应 ODA 的 setFill(OdGiHatchPattern)
    GiHatchPattern pattern;
    pattern.lines = m_patternLines;
    wd.traits().setFill(&pattern);
    wd.geometry().fill(loops, GiFillRule::EvenOdd);
    wd.traits().setFill(nullptr);
}
