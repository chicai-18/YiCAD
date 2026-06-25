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


/// @file DmImage.cpp
/// @brief 光栅图像实体实现，支持图片载入、变换和查询

#include "DmImage.h"

#include <iostream>
#include <fstream>

#include "DmLine.h"
#include "DmSettings.h"
#include "Debug.h"
#include "GuiDocumentView.h"
#include "Math2d.h"

DmImage::DmImage(DmEntity* parent, const ImageData& d)
    : DmAtomicEntity(parent)
    , data(d)
    , isModify(true)
{
    update();
    calculateBorders();
}

DmImage::DmImage(const DmImage& _image)
    : DmAtomicEntity(_image.getParent())
    , data(_image.data)
    , img(_image.img.get() ? new QImage(*_image.img) : nullptr)
    , isModify(true)
{
}

DmImage& DmImage::operator=(const DmImage& _image)
{
    data = _image.data;
    if (_image.img.get())
    {
        img.reset(new QImage(*_image.img));
    }
    else
    {
        img.reset();
    }
    return *this;
}

DmImage::DmImage(DmImage&& _image)
    : DmAtomicEntity(_image.getParent())
    , data(std::move(_image.data))
    , img(std::move(_image.img))
    , isModify(true)
{
}

DmImage& DmImage::operator=(DmImage&& _image)
{
    data = _image.data;
    img = std::move(_image.img);
    return *this;
}

DmEntity* DmImage::clone() const
{
    DmImage* i = new DmImage(*this);
    i->m_ulID = DmId();
    i->setSelected(false);
    i->setHighlighted(false);
    i->setHandle(getHandle());
    i->update();
    return i;
}

DM::EntityType DmImage::getEntityType() const
{
    return DM::EntityImage;
}

void DmImage::updateData(DmVector size, DmVector Uv, DmVector Vv)
{
    data.setSize(size);
    data.setUVector(Uv);
    data.setVVector(Vv);
    update();
    calculateBorders();
}

QString DmImage::getFile() const
{
    return QString::fromStdString(data.getPath());
}

void DmImage::setFile(const QString& file)
{
    data.setPath(file.toStdString());
}

DmVector DmImage::getUVector() const
{
    return data.getUVector();
}

DmVector DmImage::getVVector() const
{
    return data.getVVector();
}

int DmImage::getWidth() const
{
    return (int)data.getSize().x;
}

int DmImage::getHeight() const
{
    return (int)data.getSize().y;
}

int DmImage::getBrightness() const
{
    return data.getBrightness();
}

int DmImage::getContrast() const
{
    return data.getContrast();
}

int DmImage::getFade() const
{
    return data.getFade();
}

int DmImage::getHandle() const
{
    return data.getHandle();
}

void DmImage::setHandle(int h)
{
    data.setHandle(h);
}

int DmImage::getBytesPerLine()
{
    return data.getBytesPerLine();
}

void DmImage::setBytesPerLine(const int& len)
{
    data.setBytesPerLine(len);
}

unsigned char* DmImage::getbits()
{
    return data.getbits();
}

void DmImage::setBits(unsigned char* bit)
{
    data.setBits(bit);
}

double DmImage::getImageWidth()
{
    return data.getSize().x * data.getScale().x;
}

double DmImage::getImageHeight()
{
    return data.getSize().y * data.getScale().y;
}

void DmImage::update()
{
    if (data.getPath() != "")
    {
        img.reset(
            new QImage(QString::fromStdString(data.getPath())));
    }
    else
    {
        img.reset(new QImage(data.getbits(),
            (int)data.getSize().x, (int)data.getSize().y,
            data.getBytesPerLine(), QImage::Format_ARGB32_Premultiplied));
    }
    if (!img->isNull())
    {
        data.setSize(DmVector(img->width(), img->height()));
        data.setBits(img->bits());
        data.setBytesPerLine(img->bytesPerLine());

        // image update need this.
        calculateBorders();
    }
}

ImageData DmImage::getData() const
{
    return data;
}

DmVector DmImage::getInsertionPoint() const
{
    return data.getInsertionPoint();
}

void DmImage::setInsertionPoint(DmVector ip)
{
    data.setInsertionPoint(ip);
    calculateBorders();
}

void DmImage::calculateBorders()
{
    DmVectorSolutions sol = getCorners();
    minV = DmVector::minimum(
        DmVector::minimum(sol.get(0), sol.get(1)),
        DmVector::minimum(sol.get(2), sol.get(3)));
    maxV = DmVector::maximum(
        DmVector::maximum(sol.get(0), sol.get(1)),
        DmVector::maximum(sol.get(2), sol.get(3)));
}

std::list<DmEntity*> DmImage::getSubEntities() const
{
    return std::list<DmEntity*>();
}

DmVectorSolutions DmImage::getCorners() const
{
    DmVectorSolutions sol(4);

    sol.set(0, data.getInsertionPoint());
    sol.set(1, data.getInsertionPoint()
        + data.getUVector() * Math2d::round(data.getSize().x));
    sol.set(3, data.getInsertionPoint()
        + data.getVVector() * Math2d::round(data.getSize().y));
    sol.set(2, sol.get(3)
        + data.getUVector() * Math2d::round(data.getSize().x));

    return sol;
}

bool DmImage::containsPoint(const DmVector& coord) const
{
    QPolygonF paf;
    DmVectorSolutions corners = getCorners();
    for (const DmVector& vp : corners)
    {
        paf.push_back(QPointF(vp.x, vp.y));
    }
    paf.push_back(paf.at(0));
    return paf.containsPoint(QPointF(coord.x, coord.y), Qt::OddEvenFill);
}

DmVector DmImage::getNearestEndpoint(const DmVector& coord,
    double* dist) const
{
    DmVectorSolutions corners = getCorners();
    return corners.getClosest(coord, dist);
}

DmVector DmImage::getNearestPointOnEntity(const DmVector& coord,
    bool onEntity, double* dist, DmEntity** entity) const
{
    if (entity)
    {
        *entity = const_cast<DmImage*>(this);
    }

    DmVectorSolutions const& corners = getCorners();
    if (containsPoint(coord))
    {
        if (dist)
        {
            *dist = 0.;
        }
        return coord;
    }
    DmVectorSolutions points;
    for (size_t i = 0; i < corners.size(); ++i)
    {
        size_t const j = (i + 1) % corners.size();
        DmLine const l{corners.at(i), corners.at(j)};
        DmVector const vp = l.getNearestPointOnEntity(coord, onEntity);
        points.push_back(vp);
    }

    return points.getClosest(coord, dist);
}

DmVector DmImage::getNearestCenter(const DmVector& coord,
    double* dist) const
{
    DmVectorSolutions const& corners{getCorners()};
    DmVectorSolutions points;
    for (size_t i = 0; i < corners.size(); ++i)
    {
        size_t const j = (i + 1) % corners.size();
        points.push_back((corners.get(i) + corners.get(j)) * 0.5);
    }
    points.push_back((corners.get(0) + corners.get(2)) * 0.5);

    return points.getClosest(coord, dist);
}

// TODO: implement middlePoints
DmVector DmImage::getNearestMiddle(const DmVector& coord,
    double* dist, const int /*middlePoints*/) const
{
    return getNearestCenter(coord, dist);
}

double DmImage::getDistanceToPoint(const DmVector& coord,
    DmEntity** entity, DM::ResolveLevel /*level*/) const
{
    if (entity)
    {
        *entity = const_cast<DmImage*>(this);
    }

    DmVectorSolutions corners = getCorners();

    if (containsPoint(coord))
    {
        DMSETTINGS->beginGroup("/Appearance");
        bool draftMode = (bool)DMSETTINGS->readNumEntry("/DraftMode", 0);
        DMSETTINGS->endGroup();
        if (!draftMode)
        {
            return double(0.);
        }
    }
    // continue to allow selecting by image edges
    double minDist = DM_MAXDOUBLE;

    for (size_t i = 0; i < corners.size(); ++i)
    {
        size_t const j = (i + 1) % corners.size();
        DmLine const l{corners.get(i), corners.get(j)};
        double const dist = l.getDistanceToPoint(coord, nullptr);
        minDist = std::min(minDist, dist);
    }

    return minDist;
}

void DmImage::move(const DmVector& offset)
{
    data.setInsertionPoint(data.getInsertionPoint().move(offset));
    moveBorders(offset);
}

void DmImage::rotate(const DmVector& center, const DmVector& angleVector)
{
    data.setInsertionPoint(
        data.getInsertionPoint().rotate(center, angleVector));
    data.setUVector(data.getUVector().rotate(angleVector));
    data.setVVector(data.getVVector().rotate(angleVector));
    calculateBorders();
}

void DmImage::scale(const DmVector& center, const DmVector& factor)
{
    data.setInsertionPoint(
        data.getInsertionPoint().scale(center, factor));
    data.setScale(data.getScale().scale(factor));
    data.setUVector(data.getUVector().scale(factor));
    data.setVVector(data.getVVector().scale(factor));
    scaleBorders(center, factor);
}

void DmImage::mirror(const DmVector& axisPoint1,
    const DmVector& axisPoint2)
{
    data.setInsertionPoint(
        data.getInsertionPoint().mirror(axisPoint1, axisPoint2));
    DmVector vp0(0., 0.);
    DmVector vp1(axisPoint2 - axisPoint1);
    data.setUVector(data.getUVector().mirror(vp0, vp1));
    data.setVVector(data.getVVector().mirror(vp0, vp1));
    calculateBorders();
}

void DmImage::saveStream(OutputStream& wrt) const
{
    DmAtomicEntity::saveStream(wrt);

    DmVector ip = data.getInsertionPoint();
    wrt << (double)ip.x << (double)ip.y;
    DmVector uv = data.getUVector();
    wrt << (double)uv.x << (double)uv.y;
    DmVector vv = data.getVVector();
    wrt << (double)vv.x << (double)vv.y;
    DmVector sz = data.getSize();
    wrt << (double)sz.x << (double)sz.y;
    std::string path = data.getPath();
    wrt << path;
    wrt << (int)data.getBrightness();
    wrt << (int)data.getContrast();
    wrt << (int)data.getFade();
    DmVector sc = data.getScale();
    wrt << (double)sc.x << (double)sc.y;
}

void DmImage::restoreStream(InputStream& reader, const std::vector<PAIR>& revs)
{
    DmAtomicEntity::restoreStream(reader, revs);
    restoreStream(reader);
}

void DmImage::restoreStreamWithRev(InputStream& rdr, int rev)
{
    if (rev == 0)
    {
    }
}

void DmImage::restoreStream(InputStream& reader)
{
    DmAtomicEntity::restoreStream(reader);

    double pt_x, pt_y;
    reader >> (double&)pt_x >> (double&)pt_y;
    data.setInsertionPoint(DmVector(pt_x, pt_y));
    reader >> (double&)pt_x >> (double&)pt_y;
    data.setUVector(DmVector(pt_x, pt_y));
    reader >> (double&)pt_x >> (double&)pt_y;
    data.setVVector(DmVector(pt_x, pt_y));
    reader >> (double&)pt_x >> (double&)pt_y;
    data.setSize(DmVector(pt_x, pt_y));
    std::string path;
    reader >> path;
    if (!path.empty())
    {
        data.setPath(path);
    }
    int val;
    reader >> (int&)val; data.setBrightness(val);
    reader >> (int&)val; data.setContrast(val);
    reader >> (int&)val; data.setFade(val);
    reader >> (double&)pt_x >> (double&)pt_y;
    data.setScale(DmVector(pt_x, pt_y));

    update();
    calculateBorders();
    isModify = true;
}
