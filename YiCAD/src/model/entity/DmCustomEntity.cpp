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

/// @file DmCustomEntity.cpp
/// @brief 自定义实体基类的实现：默认实现、存盘记录

#include "DmCustomEntity.h"

#include <cmath>
#include <sstream>

#include "DmCustomEntityRegistry.h"
#include "DmDocument.h"
#include "DmGiReferenceCodec.h"
#include "DmProxyEntity.h"
#include "GiStream.h"
#include "Information.h"
#include "Stream.h"

TYPESYSTEM_SOURCE_ABSTRACT(DmCustomEntity, DmEntity, 0)

namespace
{
/// @brief 没有文档时写代理图形用：引用都换不出名字
class NullReferenceCodec final : public IGiReferenceCodec
{
public:
    std::string layerName(const DmLayer*) const override { return {}; }
    const DmLayer* findLayer(const std::string&) const override { return nullptr; }
    std::string lineTypeName(const DmLineType*) const override { return {}; }
    const DmLineType* findLineType(const std::string&) const override { return nullptr; }
    std::string drawableName(const IGiDrawable*) const override { return {}; }
    const IGiDrawable* findDrawable(const std::string&) const override { return nullptr; }
    std::string fontName(const IGiFont*) const override { return {}; }
    const IGiFont* findFont(const std::string&) const override { return nullptr; }
    std::string imageName(const QImage*) const override { return {}; }
    const QImage* findImage(const std::string&) const override { return nullptr; }
};

/// @brief 写一段带长度的字节（Stream 的字符串以换行结尾，装不了任意字节）
void writeBytes(OutputStream& out, const std::string& bytes)
{
    out << static_cast<std::uint64_t>(bytes.size());
    for (char c : bytes)
    {
        out << static_cast<std::uint8_t>(c);
    }
}

/// @brief 读 writeBytes 写的一段字节
std::string readBytes(InputStream& in)
{
    std::uint64_t size = 0;
    in >> size;
    std::string bytes(static_cast<std::size_t>(size), '\0');
    for (char& c : bytes)
    {
        std::uint8_t value = 0;
        in >> value;
        c = static_cast<char>(value);
    }
    return bytes;
}

/// @brief 角度是否可以当作 0
bool isZeroAngle(double angle)
{
    return std::fabs(std::remainder(angle, 2.0 * M_PI)) < 1.0e-12;
}
}  // namespace

DmCustomEntity::DmCustomEntity(DmEntity* parent)
    : DmEntity(parent)
{
}

DmCustomEntity::DmCustomEntity(const DmCustomEntity& other)
    : DmEntity(other)
{
    // 默认实现缓存的基本实体属原对象，副本用到时自己生成
}

DmCustomEntity::~DmCustomEntity() = default;

DM::EntityType DmCustomEntity::getEntityType() const
{
    return DM::EntityCustom;
}

QString DmCustomEntity::className() const
{
    return QString::fromUtf8(getTypeId().getName());
}

std::uint32_t DmCustomEntity::classVersion() const
{
    const DmCustomEntityClass* entityClass = DmCustomEntityRegistry::instance().find(className());
    return entityClass ? entityClass->version : 0;
}

DmProxyFlags DmCustomEntity::proxyFlags() const
{
    const DmCustomEntityClass* entityClass = DmCustomEntityRegistry::instance().find(className());
    return entityClass ? entityClass->proxyFlags : DmProxyFlags::None;
}

GiStream DmCustomEntity::proxyGraphics() const
{
    return GiStreamRecorder::record(*this, GiRegenType::ProxyGraphics);
}

void DmCustomEntity::update()
{
    bumpRevision();
    m_hasQueryEntities = false;
    m_queryEntities.clear();
    calculateBorders();
}

const std::vector<DmGiExplode::Item>& DmCustomEntity::queryEntities() const
{
    if (!m_hasQueryEntities || m_queryRevision != revision())
    {
        m_queryEntities = DmGiExplode::run(*this, getDocument(), DmGiExplode::Purpose::Query);
        for (DmGiExplode::Item& item : m_queryEntities)
        {
            // 子实体的父实体是本实体：按子实体搜索（EntityTable::searchEntities）找到的仍归它，不能单独修剪、修改
            item.entity->setParent(const_cast<DmCustomEntity*>(this));
        }
        m_queryRevision = revision();
        m_hasQueryEntities = true;
    }
    return m_queryEntities;
}

void DmCustomEntity::calculateBorders()
{
    resetBorders();
    for (const DmGiExplode::Item& item : queryEntities())
    {
        minV = DmVector::minimum(minV, item.entity->getMin());
        maxV = DmVector::maximum(maxV, item.entity->getMax());
    }
}

double DmCustomEntity::getDistanceToPoint(const DmVector& coord, DmEntity** entity, DM::ResolveLevel /*level*/) const
{
    // 与块参照相同，总是交出本实体：拾取、捕捉都对整个自定义实体
    if (entity)
    {
        *entity = const_cast<DmCustomEntity*>(this);
    }
    double best = DM_MAXDOUBLE;
    for (const DmGiExplode::Item& item : queryEntities())
    {
        best = std::min(best, item.entity->getDistanceToPoint(coord, nullptr, DM::ResolveNone));
    }
    return best;
}

bool DmCustomEntity::isPointOnEntity(const DmVector& coord, double tolerance) const
{
    for (const DmGiExplode::Item& item : queryEntities())
    {
        if (item.entity->isPointOnEntity(coord, tolerance))
        {
            return true;
        }
    }
    return false;
}

namespace
{
/// @brief 在不来自字形的基本实体里取 pick 给出的最近点
template <typename Pick>
DmVector nearestOf(const std::vector<DmGiExplode::Item>& items, double* dist, Pick&& pick)
{
    double best = DM_MAXDOUBLE;
    DmVector result(false);
    for (const DmGiExplode::Item& item : items)
    {
        if (item.fromGlyph)
        {
            continue;
        }
        double d = DM_MAXDOUBLE;
        const DmVector p = pick(*item.entity, &d);
        if (p.valid && d < best)
        {
            best = d;
            result = p;
        }
    }
    if (dist)
    {
        *dist = best;
    }
    return result;
}
}  // namespace

DmVector DmCustomEntity::getNearestEndpoint(const DmVector& coord, double* dist) const
{
    return nearestOf(queryEntities(), dist,
                     [&coord](const DmEntity& e, double* d) { return e.getNearestEndpoint(coord, d); });
}

DmVector DmCustomEntity::getNearestPointOnEntity(const DmVector& coord, bool onEntity, double* dist,
                                                 DmEntity** entity) const
{
    if (entity)
    {
        *entity = const_cast<DmCustomEntity*>(this);
    }
    return nearestOf(queryEntities(), dist, [&coord, onEntity](const DmEntity& e, double* d)
                     { return e.getNearestPointOnEntity(coord, onEntity, d, nullptr); });
}

DmVector DmCustomEntity::getNearestCenter(const DmVector& coord, double* dist) const
{
    return nearestOf(queryEntities(), dist,
                     [&coord](const DmEntity& e, double* d) { return e.getNearestCenter(coord, d); });
}

DmVector DmCustomEntity::getNearestMiddle(const DmVector& coord, double* dist, int middlePoints) const
{
    return nearestOf(queryEntities(), dist, [&coord, middlePoints](const DmEntity& e, double* d)
                     { return e.getNearestMiddle(coord, d, middlePoints); });
}

std::list<DmEntity*> DmCustomEntity::getSubEntities() const
{
    std::list<DmEntity*> subEntities;
    for (const DmGiExplode::Item& item : queryEntities())
    {
        subEntities.push_back(item.entity.get());
    }
    return subEntities;
}

DmVectorSolutions DmCustomEntity::intersectWith(const DmEntity* other, bool onEntities) const
{
    DmVectorSolutions result;
    for (const DmGiExplode::Item& item : queryEntities())
    {
        if (item.fromGlyph)
        {
            continue;
        }
        const DmVectorSolutions part = Information::getIntersection(item.entity.get(), other, onEntities);
        for (const DmVector& p : part)
        {
            result.push_back(p);
        }
        if (part.isTangent())
        {
            result.setTangent(true);
        }
    }
    return result;
}

std::vector<DmEntity*> DmCustomEntity::explode() const
{
    std::vector<DmEntity*> entities;
    for (DmGiExplode::Item& item : DmGiExplode::run(*this, getDocument(), DmGiExplode::Purpose::Explode))
    {
        entities.push_back(item.entity.release());
    }
    return entities;
}

void DmCustomEntity::transformBy(const GiTransform& transform)
{
    if (transform.isIdentity())
    {
        return;
    }
    // 线性部分的奇异值分解 A = 旋转(phi)·缩放(sx, sy)·旋转(theta)（Blinn 的 2×2 闭式解）；sy 为负表示含镜像，
    // 缩放(sx, sy) = 缩放(sx, |sy|)·关于 X 轴的镜像。依次作用：旋转 theta、镜像、缩放、旋转 phi、平移
    const double e = (transform.a() + transform.d()) * 0.5;
    const double f = (transform.a() - transform.d()) * 0.5;
    const double g = (transform.b() + transform.c()) * 0.5;
    const double h = (transform.b() - transform.c()) * 0.5;
    const double q = std::hypot(e, h);
    const double r = std::hypot(f, g);
    const double sx = q + r;
    const double sy = q - r;
    const double a1 = std::atan2(g, f);
    const double a2 = std::atan2(h, e);
    const double theta = (a2 - a1) * 0.5;
    const double phi = (a2 + a1) * 0.5;
    const DmVector origin(0.0, 0.0);

    if (!isZeroAngle(theta))
    {
        rotateAngle(origin, theta);
    }
    if (sy < 0.0)
    {
        mirror(origin, DmVector(1.0, 0.0));
    }
    const double absSy = std::fabs(sy);
    if (std::fabs(sx - 1.0) > 1.0e-12 || std::fabs(absSy - 1.0) > 1.0e-12)
    {
        scale(origin, DmVector(sx, absSy));
    }
    if (!isZeroAngle(phi))
    {
        rotateAngle(origin, phi);
    }
    if (transform.tx() != 0.0 || transform.ty() != 0.0)
    {
        move(DmVector(transform.tx(), transform.ty()));
    }
    update();
}

void DmCustomEntity::saveStream(OutputStream& wrt) const
{
    DmEntity::saveStream(wrt);
    saveDataBytes(wrt);
    const GiTransform t = storedTransform();
    wrt << t.a() << t.b() << t.c() << t.d() << t.tx() << t.ty();
}

void DmCustomEntity::restoreStream(InputStream& rdr, const std::vector<PAIR>& revs)
{
    const int fileRev = getRevisionId("DmCustomEntity", revs);
    if (revId > fileRev)
    {
        DmEntity::restoreStream(rdr, revs);
        restoreStreamWithRev(rdr, fileRev);
    }
    else
    {
        restoreStream(rdr);
    }
}

void DmCustomEntity::restoreStreamWithRev(InputStream& /*rdr*/, int /*rev*/)
{
    // 第 0 版，没有旧格式
}

void DmCustomEntity::restoreStream(InputStream& rdr)
{
    m_restoreFailed = false;
    DmEntity::restoreStream(rdr);
    std::uint32_t version = 0;
    rdr >> version;
    const std::string bytes = readBytes(rdr);
    double a = 1.0, b = 0.0, c = 0.0, d = 1.0, tx = 0.0, ty = 0.0;
    rdr >> a >> b >> c >> d >> tx >> ty;
    if (!restoreDataBytes(bytes, version))
    {
        m_restoreFailed = true;
        return;
    }
    applyStoredTransform(GiTransform(a, b, c, d, tx, ty));
    update();
}

void DmCustomEntity::saveDataBytes(OutputStream& out) const
{
    std::ostringstream oss;
    {
        OutputStream data(oss);
        saveData(data);
    }
    out << classVersion();
    writeBytes(out, oss.str());
}

bool DmCustomEntity::restoreDataBytes(const std::string& bytes, std::uint32_t version)
{
    std::istringstream iss(bytes);
    InputStream data(iss);
    return restoreData(data, version);
}

void DmCustomEntity::applyStoredTransform(const GiTransform& transform)
{
    // 代理被变换过：按累计的变换改动读回的原实体
    transformBy(transform);
}

GiTransform DmCustomEntity::storedTransform() const
{
    return GiTransform();
}

void DmCustomEntity::writeRecord(OutputStream& out, const DmCustomEntity& entity)
{
    out << entity.className().toStdString();
    out << static_cast<std::uint32_t>(entity.proxyFlags());

    std::ostringstream payload;
    {
        OutputStream payloadStream(payload);
        entity.saveStream(payloadStream);
    }
    writeBytes(out, payload.str());

    // 代理图形也带长度：读回时版本不认识只丢掉图形，不会错位
    std::ostringstream graphics;
    {
        OutputStream graphicsStream(graphics);
        const GiStream stream = entity.proxyGraphics();
        if (DmDocument* document = entity.getDocument())
        {
            stream.write(graphicsStream, DmGiReferenceCodec(*document));
        }
        else
        {
            stream.write(graphicsStream, NullReferenceCodec());
        }
    }
    writeBytes(out, graphics.str());
}

DmCustomEntity* DmCustomEntity::readRecord(InputStream& in, DmDocument* document, const std::vector<PAIR>& revs)
{
    std::string name;
    std::uint32_t flags = 0;
    in >> name >> flags;
    const std::string payload = readBytes(in);
    const std::string graphicsBytes = readBytes(in);
    const QString className = QString::fromStdString(name);

    if (DmCustomEntity* entity = DmCustomEntityRegistry::instance().create(className))
    {
        if (document)
        {
            entity->setDocument(document);
        }
        std::istringstream iss(payload);
        InputStream payloadStream(iss);
        entity->restoreStream(payloadStream, revs);
        if (!entity->restoreFailed())
        {
            return entity;
        }
        delete entity;
    }

    // 类没有注册，或这个版本的数据读不了：建代理实体，字节原样保留
    auto* proxy = new DmProxyEntity(className, static_cast<DmProxyFlags>(flags));
    if (document)
    {
        proxy->setDocument(document);
    }
    GiStream graphics;
    {
        std::istringstream iss(graphicsBytes);
        InputStream graphicsStream(iss);
        if (document)
        {
            graphics.read(graphicsStream, DmGiReferenceCodec(*document));
        }
        else
        {
            graphics.read(graphicsStream, NullReferenceCodec());
        }
    }
    proxy->setProxyGraphics(graphics);
    std::istringstream iss(payload);
    InputStream payloadStream(iss);
    proxy->restoreStream(payloadStream, revs);
    return proxy;
}
