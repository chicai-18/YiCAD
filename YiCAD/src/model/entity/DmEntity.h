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


/// @file DmEntity.h
/// @brief 实体基类，所有CAD实体的抽象基类

#ifndef DBENTITY_H
#define DBENTITY_H

#include <map>

#include "DmObject.h"
#include "DmPen.h"
#include "DmVector.h"
#include "DmObserver.h"
#include "DmId.h"

class DmArc;
class DmBlock;
class DmCircle;
class DmEntityContainer;
class DmDocument;
class DmIdManager;
class GuiDocumentView;
class DmBlockReference;
class DmLine;
class DmPoint;
class DmPolyline;
class DmText;
class DmLayer;
class Quadratic;
class DmVector;
class DmVectorSolutions;
class EntityData;
class QString;
class Writer;
class Reader;
class OutputStream;
class InputStream;

using DmEntityPtr = std::shared_ptr<DmEntity>;

/// @brief 实体基类
class DmEntity : public DmObject
{
    TYPESYSTEM_HEADER();

public:
    DmEntity(DmEntity* parent = nullptr);
    virtual ~DmEntity() = default;

    void init();

    virtual DmEntity* clone() const = 0;

    void resetBorders();
    void moveBorders(const DmVector& offset);
    void scaleBorders(const DmVector& center, const DmVector& factor);

    /// @brief 获取实体类型
    /// @return 实体类型枚举值
    virtual DM::EntityType getEntityType() const;

    /// @brief 获取实体id
    /// @return 实体ID
    DmId getId() const;

    /// @brief 获取实体总长度
    /// @return 实体长度
    virtual double getLength() const;

    /// @brief 获取父实体
    /// @return 父实体指针
    DmEntity* getParent() const;

    /// @brief 设置父实体
    /// @param p 父实体指针
    void setParent(DmEntity* p);

    virtual DmVector getCenter() const;
    virtual double getRadius() const;

    /// @brief 实体创建要添加到画布必须设置此属性
    /// @param pDoc 文档指针
    void setDocument(DmDocument* pDoc) override;

    void setLayer(const QString& name);
    void setLayer(DmLayer* l);
    void setLayerToActive();
    DmLayer* getLayer(bool resolve = true) const;

    void setPen(const DmPen& pen);
    void setPenToActive();
    DmPen getPen(bool resolve = true) const;

    /// @brief 实体是否为容器
    /// @return 如果是容器则返回true
    virtual bool isContainer() const = 0;

    virtual bool setSelected(bool select);
    virtual bool toggleSelected();      // 切换选中状态
    virtual bool isSelected() const;
    bool isParentSelected() const;
    virtual bool isProcessed() const;
    virtual void setProcessed(bool on);
    bool isInWindow(DmVector v1, DmVector v2) const;
    virtual bool hasEndpointsWithinWindow(const DmVector& /*v1*/, const DmVector& /*v2*/);

    /// @brief 是否可见
    virtual bool isVisible() const;

    virtual void setVisible(bool v);
    virtual void setHighlighted(bool on);
    virtual bool isHighlighted() const;

    bool isLocked() const;

    virtual void update();

    virtual void setUpdateEnabled(bool on);

    DmVector getMin() const;
    DmVector getMax() const;
    DmVector getWidthHeight() const;

    virtual DmVector getStartpoint() const;
    virtual DmVector getEndpoint() const;

    /// @brief 在端点处计算方向，如果实体类型支持该方向，则派生实体必须实现该方向
    virtual double getDirection1() const;
    virtual double getDirection2() const;

    /// @brief 给定点求切线
    virtual DmVectorSolutions getTangentPoint(const DmVector& /*point*/) const;

    /// @brief 过指定点获得切向。不一定返回单位向量
    virtual DmVector getTangentDirection(const DmVector& /*point*/) const;

    /// @brief 获取实体参考点
    virtual DmVectorSolutions getRefPoints() const;

    /// @brief 获取最近端点
    virtual DmVector getNearestEndpoint(const DmVector& coord, double* dist = nullptr) const;

    /// @brief 获取实体上的最近点
    /// @param onEntity 点是否在实体上。对于圆弧，为false时可获得对应圆上最近点，为true时仅获得该圆弧上的点
    virtual DmVector getNearestPointOnEntity(const DmVector& /*coord*/, bool onEntity = true, double* dist = nullptr, DmEntity** entity = nullptr) const = 0;

    /// @brief 获取最近的中心点
    virtual DmVector getNearestCenter(const DmVector& coord, double* dist = nullptr) const = 0;

    /// @brief 获取中点
    virtual DmVector getMiddlePoint(void) const;
    virtual DmVector getNearestMiddle(const DmVector& coord, double* dist = nullptr, int middlePoints = 1) const = 0;

    /// @brief 获取最近捕捉点(参考点)
    virtual DmVector getNearestRef(const DmVector& coord, double* dist = nullptr) const;

    /// @brief 获取选中实体的最近捕捉点(参考点)
    virtual DmVector getNearestSelectedRef(const DmVector& coord, double* dist = nullptr) const;

    /// @brief 获取点到实体的距离
    virtual DmVector getNearestOrthTan(const DmVector& /*coord*/, const DmLine& /*normal*/, bool onEntity = false) const;
    virtual double getDistanceToPoint(const DmVector& coord, DmEntity** entity = nullptr, DM::ResolveLevel level = DM::ResolveNone) const;

    /// @brief 判断点是否在实体上
    virtual bool isPointOnEntity(const DmVector& coord, double tolerance = DM_TOLERANCE) const;

    virtual bool offset(const DmVector& /*coord*/, const double& /*distance*/);

    // Implementations must offset the entity by the distance at both directions used to generate tangential circles
    virtual std::vector<DmEntity*> offsetTwoSides(const double& /*distance*/) const;

    // Implementations must move the entity by the given vector.
    virtual void move(const DmVector& offset) = 0;

    // Implementations must rotate the entity by the given angle around the given center.（逆时针旋转一个弧度角）
    virtual void rotateAngle(const DmVector& center, const double& angle);
    virtual void rotate(const DmVector& center, const DmVector& angleVector) = 0;

    // Implementations must scale the entity by the given factors.
    virtual void scale(const DmVector& center, const DmVector& factor) = 0;
    virtual void scale(const DmVector& center, const double& factor);
    virtual void scale(const DmVector& factor);

    // Implementations must mirror the entity by the given axis.
    virtual void mirror(const DmVector& axisPoint1, const DmVector& axisPoint2) = 0;

    // Implementations must drag the reference point(s) of all (sub-)entities that are very close to ref by offset.
    virtual void moveRef(const DmVector& /*ref*/, const DmVector& /*offset*/);

    // Implementations must drag the reference point(s) of selected (sub-)entities that are very close to ref by offset.
    virtual void moveSelectedRef(const DmVector& /*ref*/, const DmVector& /*offset*/);

    // 实体扩展属性集合(类似于AutoCAD的XData)
    QString getUserDefVar(const QString& key) const;
    std::vector<QString> getAllKeys() const;
    void setUserDefVar(QString key, QString val);
    void delUserDefVar(QString key);

    /// @brief 重新计算实体的边界
    virtual void calculateBorders() = 0;
    virtual Quadratic getQuadratic() const;

    /// @brief 获取该实体的子实体
    virtual std::list<DmEntity*> getSubEntities() const = 0;

    void setObserver(DmObserver* ob);
    void setObserver(std::list<DmObserver*>);
    std::list<DmObserver*> getObserver() const;
    void clearObserver();
    void notify();

    // persistent helper
    virtual void saveStream(OutputStream& wrt) const override;
    virtual void restoreStream(InputStream& rdr, const std::vector<PAIR>& revs) override;
    virtual void restoreStreamWithRev(InputStream& rdr, int rev) override;
    virtual void restoreStream(InputStream& rdr) override;

protected:
    DmEntity*                       parent = nullptr;              ///< 该实体的父实体
    DmVector                        minV;                           ///< 实体的最小坐标
    DmVector                        maxV;                           ///< 实体的最大坐标
    DmLayer*                        layer = nullptr;               ///< 实体所属图层
    DmPen                           pen;                            ///< 实体的画笔
    bool                            updateEnabled = true;           ///< 是否启用自动更新。TODO ：不知道干啥用的，块参照和填充里有用到，不需要可删除

private:
    std::map<QString, QString>      m_varList;                      ///< 实体扩展属性集合(类似于AutoCAD的XData)
    std::list<DmObserver*>          m_observerList;                 ///< 观察者列表
};

#endif
