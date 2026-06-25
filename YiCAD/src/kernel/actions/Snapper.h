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

/// @file Snapper.h
/// @brief 画布鼠标捕捉功能，支持多种捕捉模式（端点、圆心、中点等）

#ifndef SNAPPER_H
#define SNAPPER_H

#include <memory>
#include "DmVector.h"
#include "Datamodel.h"
#include "DmLineTypeTable.h"
#include "DmDocument.h"

class DmEntity;
class GuiDocumentView;
class DmVector;
class QMouseEvent;

// 设置鼠标捕捉时吸附的距离(屏幕像素)
constexpr int ADSORPTIONDISTANCE = 20;
// 捕获实体时距离（屏幕像素）
constexpr int CATCH_ENTITY_DISTANCE = 5;

/// @brief 捕捉结果类型，记录本次捕捉实际命中哪种点
enum class SnapResultType
{
    SnapNone,
    SnapEndpoint,
    SnapCenter,
    SnapMiddle,
    SnapIntersection,
    SnapOnEntity,
    SnapGrid,
    SnapSubsection,
    SnapFree
};

// 鼠标捕捉模式
struct SnapMode
{
    enum SnapModes
    {
        SnapIntersection = 1 << 0,
        SnapOnEntity = 1 << 1,
        SnapCenter = 1 << 2,
        SnapMiddle = 1 << 4,
        SnapEndpoint = 1 << 5,
        SnapGrid = 1 << 6,
        SnapFree = 1 << 7,
        RestrictHorizontal = 1 << 8,
        RestrictVertical = 1 << 9,
        RestrictOrthogonal = RestrictHorizontal | RestrictVertical,
        SnapAngle = 1 << 10,
        SnapSubsection = 1 << 11
    };

    bool snapIntersection = false;  ///< 是否捕捉到交点
    bool snapOnEntity = false;      ///< 是否捕捉到实体
    bool snapCenter = false;        ///< 是否捕捉到圆心
    bool snapMiddle = false;        ///< 是否捕捉到中点
    bool snapSubsection = false;    ///< 是否捕捉分段
    bool snapEndpoint = false;      ///< 是否捕捉到端点
    bool snapGrid = false;          ///< 是否捕捉到栅格交点
    bool snapFree = false;          ///< 是否自由捕捉
    bool snapAngle = false;         ///< 是否在一定角度下沿直线捕捉

    DM::SnapRestriction restriction{DM::RestrictNothing};  ///< 对自由捕捉的限制

    SnapMode const& clear(void);
    bool operator==(SnapMode const& rhs) const;

    static unsigned int toInt(const SnapMode& s);  // 转换为整型保存设置
    static SnapMode fromInt(unsigned int);         // 从整型转换为枚举型还原设置
};

typedef std::list<DM::EntityType> EntityTypeList;

/// @brief 画布中的鼠标捕捉
class Snapper
{
public:
    Snapper() = delete;
    Snapper(DmDocument* doc, GuiDocumentView* docView);
    virtual ~Snapper();

    void init();
    /// @brief 结束捕捉
    void finish();

    void finishOrthogonal();
    void resetOrthogonal();

    DmEntity* getKeyEntity() const;

    /// @brief 设置新的捕捉模式
    /// @param snapMode 捕捉模式
    void setSnapMode(const SnapMode& snapMode);

    SnapMode const* getSnapMode() const;
    SnapMode* getSnapMode();

    /// @brief 获取当前捕捉结果类型
    SnapResultType getSnapResult() const;
    /// @brief 获取当前捕捉点坐标（世界坐标）
    DmVector getSnapSpot() const;
    /// @brief 获取当前最终捕捉坐标
    DmVector getSnapCoord() const;

    /// @brief 设置新的捕捉限制
    /// @param snapRes 捕捉限制
    void setSnapRestriction(DM::SnapRestriction /*snapRes*/);

    DmVector snapPoint(const DmVector& coord, bool setSpot = false);
    DmVector snapPoint(QMouseEvent* e);
    DmVector snapFree(QMouseEvent* e);

    DmVector snapFree(const DmVector& coord);
    DmVector snapGrid(const DmVector& coord);

    /// @brief 捕捉到最近的端点
    /// @param coord 鼠标的坐标位置
    /// @return 返回点坐标或者无效的点
    DmVector snapEndpoint(const DmVector& coord);
    /// @brief 实体上的最近点
    DmVector snapOnEntity(const DmVector& coord);
    /// @brief 捕捉到最近的圆心
    /// @param coord 鼠标的坐标位置
    /// @return 返回点坐标或者无效的点
    DmVector snapCenter(const DmVector& coord);
    DmVector snapMiddle(const DmVector& coord);
    DmVector snapSubsection(const DmVector& coord);
    DmVector snapIntersection(const DmVector& coord);
    DmVector snapToAngle(const DmVector& coord, const DmVector& ref_coord, const double ang_res);

    DmVector snapByFunc(const DmVector& coord, const std::function<DmVector(DmEntity*, const DmVector&, double*)>& func);

    DmVector restrictOrthogonal(const DmVector& coord);
    DmVector restrictHorizontal(const DmVector& coord);
    DmVector restrictVertical(const DmVector& coord);

    DmEntity* catchEntity(const DmVector& pos, DM::ResolveLevel level = DM::ResolveNone);
    DmEntity* catchEntity(QMouseEvent* e, DM::ResolveLevel level = DM::ResolveNone);
    DmEntity* catchEntity(const DmVector& pos, DM::EntityType enType, DM::ResolveLevel level = DM::ResolveNone);
    /// @brief 捕捉指定类型的实体
    /// TODO ：多段线不再能捕获子实体，需要修改所有ResolveAll的catchEntity()引用处
    DmEntity* catchEntity(QMouseEvent* e, DM::EntityType enType, DM::ResolveLevel level = DM::ResolveNone);
    DmEntity* catchEntity(QMouseEvent* e, const EntityTypeList& enTypeList, DM::ResolveLevel level = DM::ResolveNone);

    // 另一个操作发生时暂停此捕捉器
    virtual void suspend();

    // 暂停后恢复此捕捉器
    virtual void resume();

    virtual void hideOptions();
    virtual void showOptions();

    void drawSnapper();

protected:
    void deleteSnapper();
    double getSnapRange() const;

private:
    /// @brief 获得指定点的吸附（点）范围
    void getAdsorptionExtentOfPoint(const DmVector& pos, DmVector& min, DmVector& max);
    /// @brief 获得指定点的捕捉（实体）范围
    void getCatchExtentOfPoint(const DmVector& pos, DmVector& min, DmVector& max);
    /// @brief 递归获得子实体
    void getSubEntitiesOfContainerRecrusive(DmEntityContainer* container, std::vector<DmEntity*>& subEntities);

protected:
    DmDocument*         pDocument = nullptr;                ///< 关联文档
    GuiDocumentView*    docView = nullptr;                  ///< 文档视图
    DmEntity*           keyEntity = nullptr;                ///< 关键实体
    SnapMode            snapMode;                           ///< 当前捕捉模式
    double              snapDistance = 1.0;                 ///< 捕捉距离，用于捕捉到与端点具有给定距离的点
    int                 snapSubsectionPts = 3;              ///< 捕捉到等距分段点、默认三段
    int                 snapRange = 0;                      ///< 捕捉实体的捕捉范围，TODO ：准备删除
    bool                finished = false;                   ///< 是否为结束的

private:
    struct ImpData;
    std::unique_ptr<ImpData>    m_pImpData;

    DmVector                    m_orthogonalPoint;
};

#endif
