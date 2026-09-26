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


/// @file DmBlockReference.cpp
/// @brief 块参照类的实现，包括子实体管理、几何变换、捕捉计算和序列化

#include "DmBlockReference.h"
#include "DmAttribute.h"

#include <iostream>
#include <cmath>

#include "DmArc.h"
#include "DmCircle.h"
#include "DmEllipse.h"
#include "DmBlock.h"
#include "DmDocument.h"
#include "DmLayer.h"
#include "Math2d.h"
#include "Debug.h"
#include "DmSettings.h"

TYPESYSTEM_SOURCE(DmBlockReference, DmEntity, 0);

DmBlockReferenceData::DmBlockReferenceData(const QString& _name, DmVector _insertionPoint, DmVector _scaleFactor, double _angle,
    int _cols, int _rows, DmVector _spacing, DmBlockTable* _blockSource, DM::UpdateMode _updateMode, QString _path)
    : name(_name)
    , insertionPoint(_insertionPoint)
    , scaleFactor(_scaleFactor)
    , angle(_angle)
    , cols(_cols)
    , rows(_rows)
    , spacing(_spacing)
    , blockSource(_blockSource)
    , updateMode(_updateMode)
    , path(_path)
{
}

/// @param parent 该块参照所属的父实体
DmBlockReference::DmBlockReference(DmEntity* parent, const DmBlockReferenceData& d)
    : DmEntity(parent)
    , data(d)
{
    block = nullptr;

    if (data.updateMode != DM::NoUpdate)
    {
        update();
    }
}

DmBlockReference::~DmBlockReference()
{
    qDeleteAll(m_subEntities);
    m_subEntities.clear();
}

DmEntity* DmBlockReference::clone() const
{
    DmBlockReference* i = new DmBlockReference(*this);
    i->block = nullptr;
    // 深拷贝子实体
    i->m_subEntities.clear();
    for (auto e : m_subEntities)
    {
        DmEntity* ce = e->clone();
        ce->setParent(i);
        i->m_subEntities.append(ce);
    }
    return i;
}

DM::EntityType DmBlockReference::getEntityType() const
{
    return DM::EntityBlockReference;
}

DmBlockReferenceData DmBlockReference::getData() const
{
    return data;
}

// 更新该块参照的子实体缓存。
// 当其依赖的块定义发生变化时，需要调用此方法。
void DmBlockReference::update()
{
    if (updateEnabled == false)
    {
        return;
    }

    // 把属性留下，其余踢掉
    std::list<DmEntity*> otherEnts;
    std::list<DmObserver*> ob;
    bool obModify = false;
    for (auto e : m_subEntities)
    {
        if (e->getEntityType() != DM::EntityAttribute)
        {
            otherEnts.emplace_back(e);
        }
        if (e->getObserver().size() > 0)
        {
            obModify = true;
            ob = e->getObserver();
            e->clearObserver();
        }
    }
    for (auto e : otherEnts)
    {
        m_subEntities.removeOne(e);
        delete e;
    }

    DmBlock* blk = getBlockForInsert();
    if (!blk)
    {
        return;
    }

    constexpr double MIN_SCALE_EPSILON = 1.0e-6;
    if (fabs(data.scaleFactor.x) < MIN_SCALE_EPSILON || fabs(data.scaleFactor.y) < MIN_SCALE_EPSILON)
    {
        return;
    }

    DmPen tmpPen;

    for (auto e : blk->getEntityTable())
    {
        for (int c = 0; c < data.cols; ++c)
        {
            for (int r = 0; r < data.rows; ++r)
            {
                if (e->getEntityType() == DM::EntityAttributeDefinition)
                {
                    continue;
                }
                if (e->getEntityType() == DM::EntityBlockReference && data.updateMode != DM::PreviewUpdate)
                {
                    static_cast<DmBlockReference*>(e)->update();
                }
                DmEntity* ne = nullptr;
                // todo : 此处代码会导致椭圆绘制反向,后面确实不需要可以删掉
                constexpr double ARC_TO_ELLIPSE_EPSILON = 1.0e-6;
                if ((data.scaleFactor.x - data.scaleFactor.y) > ARC_TO_ELLIPSE_EPSILON)
                {
                    if (e->getEntityType() == DM::EntityArc)
                    {
                        DmArc* a = static_cast<DmArc*>(e);
                        // TODO : 圆弧reverse去除了，需要改逻辑
                        ne = new DmEllipse(this,{a->getCenter(), {a->getRadius(), 0.}, DmVector(0.0,0.0,1.0), 1,false, a->getStartAngle(), a->getEndAngle()});
                        ne->setLayer(e->getLayer());
                        ne->setPen(e->getPen(false));
                    }
                    else if (e->getEntityType() == DM::EntityCircle)
                    {
                        DmCircle* a = static_cast<DmCircle*>(e);
                        ne = new DmEllipse(this,{ a->getCenter(), {a->getRadius(), 0.}, DmVector(0.0,0.0,1.0), 1,true, 0., 2. * M_PI});
                        ne->setLayer(e->getLayer());
                        ne->setPen(e->getPen(false));
                    }
                    else
                    {
                        ne = e->clone();
                    }
                }
                else
                {
                    ne = e->clone();
                    ne->clearObserver();
                }
                ne->setUpdateEnabled(false);
                DmLayer* l = ne->getLayer(); // 特殊字体字符块可能没有图层
//              if (l && ne->getLayer()->getName() == "0")
//              {
//                  ne->setLayer(this->getLayer());
//              }
                ne->setParent(this);
                ne->setVisible(getFlag(DM::FlagVisible));
                if (fabs(data.scaleFactor.x) > MIN_SCALE_EPSILON && fabs(data.scaleFactor.y) > MIN_SCALE_EPSILON)
                {
                    ne->move(data.insertionPoint + DmVector(data.spacing.x / data.scaleFactor.x * c, data.spacing.y / data.scaleFactor.y * r));
                }
                else
                {
                    ne->move(data.insertionPoint);
                }
                ne->move(blk->getBasePoint() * -1);
                ne->scale(data.insertionPoint, data.scaleFactor);
                ne->rotateAngle(data.insertionPoint, data.angle);
                ne->setSelected(isSelected());
                tmpPen = ne->getPen(false);

                // setColor已删除。genPen时根据规则会获取自身或者父辈(祖辈)画笔颜色
                // 子实体画笔不应该根据父类去设置,应在绘制时根据规则拿取正确画笔

                // todo ：不应在此处修改线宽及线型
                // 处理来自块的线宽（独立存储）
                if (tmpPen.getWidth() == DM::WidthByBlock)
                {
                    tmpPen.setWidth(getPen().getWidth());
                }

                // 处理来自块的线型（独立存储）
                if (tmpPen.getLineType() == DmLineTypeTable::ByBlock)
                {
                    tmpPen.setLineType(getPen().getLineType());
                }
                ne->setPen(tmpPen);

                ne->setUpdateEnabled(true);

                if (data.updateMode != DM::PreviewUpdate || ne->getEntityType() == DM::EntityBlockReference)
                {
                    ne->update();
                }
                m_subEntities.append(ne);
            }
        }
    }
    calculateBorders();

    if (obModify)
    {
        for (auto e : m_subEntities)
        {
            (*e).setObserver(ob);
        }
    }
}

bool DmBlockReference::setSelected(bool select)
{
    bool result = DmEntity::setSelected(select);
    if (result)
    {
        // 同步子实体的选中状态
        for (auto e : m_subEntities)
        {
            e->setSelected(select);
        }
    }
    return result;
}

QString DmBlockReference::getName() const
{
    return data.name;
}

void DmBlockReference::setName(const QString& newName)
{
    data.name = newName;
    update();
}

DmVector DmBlockReference::getInsertionPoint() const
{
    return data.insertionPoint;
}

void DmBlockReference::setInsertionPoint(const DmVector& i)
{
    data.insertionPoint = i;
}

DmVector DmBlockReference::getScale() const
{
    return data.scaleFactor;
}

void DmBlockReference::setScale(const DmVector& s)
{
    data.scaleFactor = s;
}

double DmBlockReference::getAngle() const
{
    return data.angle;
}

void DmBlockReference::setAngle(double a)
{
    data.angle = a;
}

int DmBlockReference::getCols() const
{
    return data.cols;
}

void DmBlockReference::setCols(int c)
{
    data.cols = c;
}

int DmBlockReference::getRows() const
{
    return data.rows;
}

void DmBlockReference::setRows(int r)
{
    data.rows = r;
}

DmVector DmBlockReference::getSpacing() const
{
    return data.spacing;
}

void DmBlockReference::setSpacing(const DmVector& s)
{
    data.spacing = s;
}

QString DmBlockReference::getPath() const
{
    return data.path;
}

void DmBlockReference::setPath(const QString path)
{
    data.path = path;
}

void DmBlockReference::getEntities(std::list<DmEntity*> ents, std::list<DmEntity*>& refEnts, std::list<DmEntity*>& specialEntities)
{
    for (auto subEnts : ents)
    {
        if (subEnts->getEntityType() == DM::EntityBlockReference)
        {
            auto listEnts = static_cast<DmBlockReference*>(subEnts)->getEntityList();
            auto sents = std::list<DmEntity*>(listEnts.begin(), listEnts.end());
            getEntities(sents, refEnts, specialEntities);
        }
        else if (subEnts->getEntityType() == DM::EntityHatch)
        {
            specialEntities.emplace_back(subEnts);
        }
        else if (subEnts->getEntityType() == DM::EntityText || subEnts->getEntityType() == DM::EntityMText)
        {
            specialEntities.emplace_back(subEnts);
        }
        else if (subEnts->getEntityType() == DM::EntitySolid)
        {
            specialEntities.emplace_back(subEnts);
        }
        else if (subEnts->getEntityType() == DM::EntityDimAligned || subEnts->getEntityType() == DM::EntityDimAngular ||
            subEnts->getEntityType() == DM::EntityDimDiametric || subEnts->getEntityType() == DM::EntityDimLeader ||
            subEnts->getEntityType() == DM::EntityDimLinear || subEnts->getEntityType() == DM::EntityDimRadial)
        {
            auto dimEnts = ((DmEntityContainer*)subEnts)->getEntityList();
            for (auto subEnt : dimEnts)
            {
                if (subEnt->getEntityType() == DM::EntityContainer)
                {
                    auto textEnts = ((DmEntityContainer*)subEnt)->getEntityList();
                    for (auto text : textEnts)
                    {
                        if (text->getEntityType() == DM::EntityMText || text->getEntityType() == DM::EntityText)
                        {
                            specialEntities.emplace_back(text);
                        }
                    }
                    continue;
                }
                else if (subEnt->getEntityType() == DM::EntityBlockReference)
                {
                    auto solidEnts = static_cast<DmBlockReference*>(subEnt)->getEntityList();
                    for (auto solid : solidEnts)
                    {
                        if (solid->getEntityType() == DM::EntitySolid)
                        {
                            specialEntities.emplace_back(solid);
                        }
                    }
                    continue;
                }

                // 绘制标注里除特殊实体以外的实体
                refEnts.emplace_back(subEnt);
            }
        }
        else
        {
            refEnts.emplace_back(subEnts);
        }
    }
}

std::list<DmEntity*> DmBlockReference::getSubEntities() const
{
    std::list<DmEntity*> subEnts = std::list<DmEntity*>();

    for (auto ent : m_subEntities)
    {
        auto entSub = ent->getSubEntities();
        if (entSub.size() > 0)
        {
            subEnts.splice(subEnts.end(), entSub);
        }
        else
        {
            subEnts.emplace_back(std::move(ent));
        }
    }
    return subEnts;
}

void DmBlockReference::addAttributes(const std::list<DmAttribute*>& attrs)
{
    for (auto& attr : attrs)
    {
        attr->setParent(this);
        m_subEntities.append(attr);
    }

    forcedCalculateBorders();
}

std::list<DmAttribute*> DmBlockReference::getAttributes() const
{
    std::list<DmAttribute*> attrs;
    for (auto e : m_subEntities)
    {
        if (e->getEntityType() == DM::EntityAttribute)
        {
            attrs.emplace_back(static_cast<DmAttribute*>(e));
        }
    }
    return attrs;
}

void DmBlockReference::setParent(DmEntityContainer* parent)
{
    DmEntity::setParent(parent);
    block = NULL;
}

/// @return 返回与该块参照关联的块定义指针；若未找到则返回 nullptr。
///         若指定了 blockSource，则优先从其中查找；否则从最近的父文档中查找。
DmBlock* DmBlockReference::getBlockForInsert() const
{
    DmBlock* blk = nullptr;
    if (block)
    {
        blk = block;
        return blk;
    }

    DmBlockTable* blockTable = nullptr;

    if (!data.blockSource)
    {
        if (getDocument())
        {
            blockTable = getDocument()->getBlockTable();
        }
        else
        {
            blockTable = nullptr;
        }
    }
    else
    {
        blockTable = data.blockSource;
    }

    if (blockTable)
    {
        blk = blockTable->find(data.name);
    }

    block = blk;

    return blk;
}

void DmBlockReference::setBlock(DmBlock* blc)
{
    block = blc;
}

QList<DmEntity*>& DmBlockReference::getEntityList()
{
    return m_subEntities;
}

const QList<DmEntity*>& DmBlockReference::getEntityList() const
{
    return m_subEntities;
}

void DmBlockReference::calculateBorders()
{
    resetBorders();
    for (DmEntity* e : m_subEntities)
    {
        DmLayer* layer = e->getLayer();
        if (e->isVisible() && !(layer && layer->isFrozen()))
        {
            e->calculateBorders();
            minV = DmVector::minimum(e->getMin(), minV);
            maxV = DmVector::maximum(e->getMax(), maxV);
        }
    }
}

void DmBlockReference::forcedCalculateBorders()
{
    resetBorders();
    for (DmEntity* e : m_subEntities)
    {
        if (e->isContainer())
        {
            static_cast<DmEntityContainer*>(e)->forcedCalculateBorders();
        }
        else
        {
            e->calculateBorders();
        }
        minV = DmVector::minimum(e->getMin(), minV);
        maxV = DmVector::maximum(e->getMax(), maxV);
    }
}

/// @brief 该块参照是否可见（重写自 DmEntity）
/// @return 仅当实体本身、所属块以及所在图层都可见时返回 true。
///         图层为 nullptr 时忽略图层可见性；块为 nullptr 时忽略块可见性。
bool DmBlockReference::isVisible() const
{
    DmBlock* blk = getBlockForInsert();
    if (blk)
    {
        if (blk->isFrozen())
        {
            return false;
        }
    }

    return DmEntity::isVisible();
}

DmVectorSolutions DmBlockReference::getRefPoints() const
{
    return DmVectorSolutions{ data.insertionPoint };
}

DmVector DmBlockReference::getMiddlePoint(void) const
{
    return DmVector(false);
}

DmVector DmBlockReference::getNearestRef(const DmVector& coord, double* dist) const
{
    return getRefPoints().getClosest(coord, dist);
}

double DmBlockReference::getDistanceToPoint(const DmVector& coord, DmEntity** entity, DM::ResolveLevel level) const
{
    if (entity)
    {
        *entity = const_cast<DmBlockReference*>(this);
    }
    // 计算到子实体几何图形的最近距离，使块参照能被整体选中
    double dist = DM_MAXDOUBLE;
    getNearestPointOnEntity(coord, true, &dist, nullptr);
    return dist;
}

DmVector DmBlockReference::getNearestPointOnEntity(const DmVector& coord, bool onEntity, double* dist, DmEntity** entity) const
{
    DmVector point(false);
    double minDist = DM_MAXDOUBLE;
    double curDist = DM_MAXDOUBLE;

    for (auto en : m_subEntities)
    {
        if (en->isVisible())
        {
            point = en->getNearestPointOnEntity(coord, onEntity, &curDist, entity);
            if (point.valid && curDist < minDist)
            {
                minDist = curDist;
            }
            else if (!point.valid)
            {
                // 文字(DmText/DmMText)的getNearestPointOnEntity返回无效点，
                // 回退到getDistanceToPoint以正确计算到文字的距离
                curDist = en->getDistanceToPoint(coord);
                if (curDist < minDist)
                {
                    minDist = curDist;
                    point = coord;
                }
            }
        }
    }
    if (dist)
    {
        *dist = minDist;
    }

    return point;
}

DmVector DmBlockReference::getNearestEndpoint(const DmVector& coord, double* dist) const
{
    double minDist = DM_MAXDOUBLE;
    double curDist = DM_MAXDOUBLE;
    DmVector closestPoint(false);
    DmVector point;

    for (auto en : m_subEntities)
    {
        if (en->isVisible())
        {
            point = en->getNearestEndpoint(coord, &curDist);
            if (point.valid && curDist < minDist)
            {
                closestPoint = point;
                minDist = curDist;
            }
        }
    }
    if (dist)
    {
        *dist = minDist;
    }

    return closestPoint;
}

DmVector DmBlockReference::getNearestCenter(const DmVector& coord, double* dist) const
{
    double minDist = DM_MAXDOUBLE;
    double curDist = DM_MAXDOUBLE;
    DmVector closestPoint(false);
    DmVector point;

    for (auto en : m_subEntities)
    {
        if (en->isVisible())
        {
            point = en->getNearestCenter(coord, &curDist);
            if (point.valid && curDist < minDist)
            {
                closestPoint = point;
                minDist = curDist;
            }
        }
    }
    if (dist)
    {
        *dist = minDist;
    }

    return closestPoint;
}

DmVector DmBlockReference::getNearestMiddle(const DmVector& coord, double* dist, int middlePoints) const
{
    double minDist = DM_MAXDOUBLE;
    double curDist = DM_MAXDOUBLE;
    DmVector closestPoint(false);
    DmVector point;

    for (auto en : m_subEntities)
    {
        if (en->isVisible())
        {
            point = en->getNearestMiddle(coord, &curDist, middlePoints);
            if (point.valid && curDist < minDist)
            {
                closestPoint = point;
                minDist = curDist;
            }
        }
    }
    if (dist)
    {
        *dist = minDist;
    }

    return closestPoint;
}

void DmBlockReference::move(const DmVector& offset)
{
    data.insertionPoint.move(offset);
    auto attrs = getAttributes();
    for (auto attr : attrs)
    {
        attr->move(offset);
    }
    update();
}

void DmBlockReference::moveRef(const DmVector& ref, const DmVector& offset)
{
    // 对块参照来说，移动锚点等于整体移动
    move(offset);
}

void DmBlockReference::rotate(const DmVector& center, const DmVector& angleVector)
{
    data.insertionPoint.rotate(center, angleVector);
    data.angle = Math2d::correctAngle(data.angle + angleVector.angle());
    auto attrs = getAttributes();
    for (auto attr : attrs)
    {
        attr->rotate(center, angleVector);
    }
    update();
}

void DmBlockReference::scale(const DmVector& center, const DmVector& factor)
{
    data.insertionPoint.scale(center, factor);
    data.scaleFactor.scale(DmVector(0.0, 0.0), factor);
    data.spacing.scale(DmVector(0.0, 0.0), factor);
    auto attrs = getAttributes();
    for (auto attr : attrs)
    {
        attr->scale(center, factor);
    }
    update();
}

void DmBlockReference::mirror(const DmVector& axisPoint1, const DmVector& axisPoint2)
{
    data.insertionPoint.mirror(axisPoint1, axisPoint2);

    DmVector vec = DmVector::polar(1.0, data.angle);
    vec.mirror(DmVector(0.0, 0.0), axisPoint2 - axisPoint1);
    data.angle = Math2d::correctAngle(vec.angle() - M_PI);

    data.scaleFactor.x *= -1;
    auto attrs = getAttributes();
    for (auto attr : attrs)
    {
        attr->mirror(axisPoint1, axisPoint2);
    }
    update();
}

void DmBlockReference::saveStream(OutputStream& wrt) const
{
    DmEntity::saveStream(wrt);

    auto name = data.name.toStdString();
    auto insertionPoint = data.insertionPoint;
    auto scaleFactor = data.scaleFactor;
    DmVector sp = data.spacing;
    double angle = data.angle;

    // 属性
    std::list<DmAttribute*> attrs = getAttributes();

    wrt << name << (double)insertionPoint.x << (double)insertionPoint.y << (double)scaleFactor.x << (double)scaleFactor.y << (double)sp.x << (double)sp.y << (double)angle << (uint32_t)attrs.size();

    for (auto& att : attrs)
    {
        auto text_noConst = att->getData();

        DmVector position = text_noConst.getPosition();
        double height = text_noConst.getTextHeight();
        double angle = text_noConst.getAngle() * 180 / M_PI;
        std::string style = text_noConst.getTextStyle()->getName().toStdString();
        int textHorzMode = (int)text_noConst.getTextHorzMode();
        int textVertMode = (int)text_noConst.getTextVertMode();
        bool isMirrorInX = text_noConst.getUpsideDown();
        bool isMirrorInY = text_noConst.getReverseDirection();
        double widthFactor = text_noConst.getWidthFactor();
        double oblique = text_noConst.getSlashAngle();
        DmVector alignment = text_noConst.getAlignment();
        std::string textString = text_noConst.getTextString().toStdString();
        std::string tag = att->getTag().toStdString();
        std::string layerName = att->getLayer()->getName().toStdString();

        wrt << (double)position.x << (double)position.y << (double)height << (double)angle << style << (int)textHorzMode << (int)textVertMode << (bool)isMirrorInX << (bool)isMirrorInY << (double)widthFactor << (double)oblique << (double)alignment.x << (double)alignment.y << textString << tag << layerName;
    }
}

void DmBlockReference::restoreStream(InputStream& reader, const std::vector<PAIR>& revs)
{
    DmEntity::restoreStream(reader, revs);

    int fileRev = getRevisionId("DmBlockReference", revs);
    if (revId > fileRev)
    {
        // 老文件格式
        restoreStreamWithRev(reader, fileRev);
    }
    else
    {
        std::string name;
        DmVector insertionPoint(true), scaleFactor(true), sp(true);
        double angle = 0.0;
        uint32_t attrCount = 0;

        reader >> (std::string&)name >> (double&)insertionPoint.x >> (double&)insertionPoint.y >> (double&)scaleFactor.x >> (double&)scaleFactor.y >> (double&)sp.x >> (double&)sp.y >> (double&)angle >> (uint32_t&)attrCount;

        auto blockReference = getDocument()->getBlockTable()->find(QString::fromStdString(name));
        data.blockSource = getDocument()->getBlockTable();
        setParent(nullptr);
        setRows(1);
        setCols(1);
        setName(QString::fromStdString(name));
        setInsertionPoint(insertionPoint);
        setScale(scaleFactor);
        setSpacing(sp);
        setAngle(angle);

        auto existingAttrs = getAttributes();
        for (auto attr : existingAttrs)
        {
            m_subEntities.removeOne(attr);
            delete attr;
        }

        auto attrs = std::list<DmAttribute*>();
        for (uint32_t i = 0; i < attrCount; i++)
        {
            DmVector position(true);
            double height = 0.0;
            double angle = 0.0;
            std::string style;
            int textHorzMode = 0;
            int textVertMode = 0;
            bool isMirrorInX = false;
            bool isMirrorInY = false;
            double widthFactor = 0.0;
            double oblique = 0.0;
            DmVector alignment(true);
            std::string textString;
            std::string tag;
            std::string layerName;

            reader >> (double&)position.x >> (double&)position.y >> (double&)height >> (double&)angle >> (std::string&)style >> (int&)textHorzMode >> (int&)textVertMode >> (bool&)isMirrorInX >> (bool&)isMirrorInY >> (double&)widthFactor >> (double&)oblique >> (double&)alignment.x >> (double&)alignment.y >> (std::string&)textString >> (std::string&)tag >> (std::string&)layerName;

            AttributeData attData = AttributeData(QString::fromStdString(tag));
            TextData textData;
            textData.setPosition(position);
            textData.setTextHeight(height);
            textData.setAngle(angle);
            QString sty = QString::fromStdString(style);
            DmTextStyleTable* textStyleTable = getDocument()->getTextStyleTable();
            QString textStr = QString::fromStdString(textString);
            // 使用当前图纸的默认文字样式
            if (sty.isEmpty())
            {
                sty = DEFAULT_TEXTSTYLE_NAME;
            }
            DmTextStyle* pStyle = textStyleTable->find(sty);
            textData.setTextStyle(pStyle);
            textData.setTextHorzMode((ETextHorzMode)textHorzMode);
            textData.setTextVertMode((ETextVertMode)textVertMode);
            textData.setUpsideDown(isMirrorInX);
            textData.setReverseDirection(isMirrorInY);
            textData.setWidthFactor(widthFactor);
            textData.setSlashAngle(oblique);
            textData.setAlignment(alignment);
            textData.setTextString(QString::fromStdString(textString));
            textData.setUpdateMode(EUpdateMode::NoUpdate);

            DmAttribute* att = new DmAttribute(nullptr, textData, attData);
            att->setParent(this);

            auto qStrLyaerName = QString::fromStdString(layerName);
            auto attLayer = getDocument()->getLayerTable()->find(qStrLyaerName);
            att->setLayer(attLayer);
            att->update();
            attrs.emplace_back(att);
        }
        addAttributes(attrs);
        update();
    }
}

void DmBlockReference::restoreStreamWithRev(InputStream& rdr, int rev)
{
    if (rev == 0)
    {
    }
    else // 发生较大版本变更，例如 DmBlockReference 的父类发生变化
    {
        // 第一步：逐项读取旧版本数据
    }
}
