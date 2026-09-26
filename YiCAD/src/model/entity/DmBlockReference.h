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


/// @file DmBlockReference.h
/// @brief 块参照类，通过块定义实例化的实体对象

#ifndef DMBLOCKREFERENCE_H
#define DMBLOCKREFERENCE_H

#include "DmEntity.h"

class DmBlockTable;
class DmAttribute;

/// @brief 块参照数据结构
struct DmBlockReferenceData
{
    DmBlockReferenceData() = default;
    ~DmBlockReferenceData() = default;

    DmBlockReferenceData(const QString& name, DmVector insertionPoint, DmVector scaleFactor, double angle, int cols, int rows,
        DmVector spacing, DmBlockTable* blockSource = NULL, DM::UpdateMode updateMode = DM::Update, QString path = NULL);

    QString         name;
    DmVector        insertionPoint;
    DmVector        scaleFactor;
    double          angle = 0.0;
    int             cols = 0;
    int             rows = 0;
    DmVector        spacing;
    DmBlockTable*   blockSource = nullptr;  ///< 块来源表
    DM::UpdateMode  updateMode = DM::Update;  ///< 更新模式
    QString         path = NULL;
};

/// @brief 块参照
/// 通过块定义实例化的实体对象
class DmBlockReference : public DmEntity
{
    TYPESYSTEM_HEADER();
public:
    DmBlockReference() = default;
    DmBlockReference(DmEntity* parent, const DmBlockReferenceData& d);
    virtual ~DmBlockReference();

    virtual DmEntity* clone() const;

    /// @return DM::EntityBlockReference
    virtual DM::EntityType getEntityType() const;

    /// @return 返回定义该块参照的数据副本
    DmBlockReferenceData getData() const;

    // 重写 setParent，并使块缓存指针失效。
    virtual void setParent(DmEntityContainer* parent);

    DmBlock* getBlockForInsert() const;
    void setBlock(DmBlock* blc);

    virtual void update();

    virtual bool setSelected(bool select) override;

    QString getName() const;
    void setName(const QString& newName);

    DmVector getInsertionPoint() const;
    void setInsertionPoint(const DmVector& i);

    DmVector getScale() const;
    void setScale(const DmVector& s);

    double getAngle() const;
    void setAngle(double a);

    int getCols() const;
    void setCols(int c);

    int getRows() const;
    void setRows(int r);

    DmVector getSpacing() const;
    void setSpacing(const DmVector& s);

    QString getPath() const;
    void setPath(const QString path);

    void getEntities(std::list<DmEntity*> ents, std::list<DmEntity*>& refents, std::list<DmEntity*>& specialEntities);

    std::list<DmEntity*> getSubEntities() const override;

    QList<DmEntity*>& getEntityList();
    const QList<DmEntity*>& getEntityList() const;
    virtual bool isContainer() const override { return false; }
    virtual void calculateBorders() override;
    void forcedCalculateBorders();

    /// @brief 添加属性实体到块参照。
    void addAttributes(const std::list<DmAttribute*>& attrs);

    /// @brief 获得块参照的属性
    std::list<DmAttribute*> getAttributes() const;

    virtual bool isVisible() const;

    virtual DmVectorSolutions getRefPoints() const;
    virtual DmVector getMiddlePoint(void) const;
    virtual DmVector getNearestRef(const DmVector& coord, double* dist = nullptr) const;
    virtual double getDistanceToPoint(const DmVector& coord, DmEntity** entity = nullptr, DM::ResolveLevel level = DM::ResolveNone) const override;
    virtual DmVector getNearestPointOnEntity(const DmVector& coord, bool onEntity = true, double* dist = nullptr, DmEntity** entity = nullptr) const override;
    virtual DmVector getNearestEndpoint(const DmVector& coord, double* dist = nullptr) const override;
    virtual DmVector getNearestCenter(const DmVector& coord, double* dist = nullptr) const override;
    virtual DmVector getNearestMiddle(const DmVector& coord, double* dist = nullptr, int middlePoints = 1) const override;

    virtual void move(const DmVector& offset);
    virtual void moveRef(const DmVector& ref, const DmVector& offset) override;
    virtual void rotate(const DmVector& center, const DmVector& angleVector);
    virtual void scale(const DmVector& center, const DmVector& factor);
    virtual void mirror(const DmVector& axisPoint1, const DmVector& axisPoint2);

    // 持久化辅助接口
    virtual void saveStream(OutputStream& wrt) const override;
    virtual void restoreStream(InputStream& reader, const std::vector<PAIR>& revs) override;
    virtual void restoreStreamWithRev(InputStream& rdr, int rev) override;

protected:
    DmBlockReferenceData        data;
    mutable DmBlock*    block = nullptr; ///< 缓存关联的块定义指针

private:
    QList<DmEntity*> m_subEntities;  ///< 展开的子实体，始终拥有所有权
};

#endif
