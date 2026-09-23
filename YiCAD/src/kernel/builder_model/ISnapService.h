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

/// @file ISnapService.h
/// @brief 捕捉能力接口，供 ActionInterface 以组合（而非继承）方式持有
///
/// 从 Snapper 中抽出的纯虚接口。ActionInterface 曾经 `public Snapper`
/// 继承捕捉能力，导致捕捉策略不可替换、不可脱离 Action 单测（P9）。
/// 现在 ActionInterface 持有一个 ISnapService*，Snapper 是它当前唯一的
/// 实现。SnapMode / SnapResultType / EntityTypeList 等类型原来定义在
/// Snapper.h，一并迁到这里：它们是接口契约的一部分，且不依赖 DmDocument.h，
/// 迁出后 ActionInterface.h 不再需要拖入 Snapper 的完整实现头。
/// 见 doc/ARCHITECTURE_EVOLUTION_PLAN.md 阶段2 第5.4节第5项。
///
/// 阶段 3 从 kernel/actions/ 移到本目录：IDocumentView（同批移动）的
/// setDefaultSnapMode/setSnapRestriction 用到 SnapMode/DM::SnapRestriction，
/// 而 IDocumentView 又是 Model 分区需要的接口，故本文件与它一起下沉。

#ifndef ISNAPSERVICE_H
#define ISNAPSERVICE_H

#include <functional>
#include <list>

#include "DmVector.h"
#include "Datamodel.h"

class DmEntity;
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

/// @brief 画布中鼠标捕捉能力的接口
class ISnapService
{
public:
    virtual ~ISnapService() = default;

    virtual void init() = 0;
    /// @brief 结束捕捉
    virtual void finish() = 0;

    virtual void finishOrthogonal() = 0;
    virtual void resetOrthogonal() = 0;

    virtual DmEntity* getKeyEntity() const = 0;

    /// @brief 设置新的捕捉模式
    /// @param snapMode 捕捉模式
    virtual void setSnapMode(const SnapMode& snapMode) = 0;

    virtual SnapMode const* getSnapMode() const = 0;
    virtual SnapMode* getSnapMode() = 0;

    /// @brief 获取当前捕捉结果类型
    virtual SnapResultType getSnapResult() const = 0;
    /// @brief 获取当前捕捉点坐标（世界坐标）
    virtual DmVector getSnapSpot() const = 0;
    /// @brief 获取当前最终捕捉坐标
    virtual DmVector getSnapCoord() const = 0;

    /// @brief 设置新的捕捉限制
    /// @param snapRes 捕捉限制
    virtual void setSnapRestriction(DM::SnapRestriction /*snapRes*/) = 0;

    virtual DmVector snapPoint(const DmVector& coord, bool setSpot = false) = 0;
    virtual DmVector snapPoint(QMouseEvent* e) = 0;
    virtual DmVector snapFree(QMouseEvent* e) = 0;

    virtual DmVector snapFree(const DmVector& coord) = 0;
    virtual DmVector snapGrid(const DmVector& coord) = 0;

    /// @brief 捕捉到最近的端点
    virtual DmVector snapEndpoint(const DmVector& coord) = 0;
    /// @brief 实体上的最近点
    virtual DmVector snapOnEntity(const DmVector& coord) = 0;
    /// @brief 捕捉到最近的圆心
    virtual DmVector snapCenter(const DmVector& coord) = 0;
    virtual DmVector snapMiddle(const DmVector& coord) = 0;
    virtual DmVector snapSubsection(const DmVector& coord) = 0;
    virtual DmVector snapIntersection(const DmVector& coord) = 0;
    virtual DmVector snapToAngle(const DmVector& coord, const DmVector& ref_coord, const double ang_res) = 0;

    virtual DmVector snapByFunc(const DmVector& coord, const std::function<DmVector(DmEntity*, const DmVector&, double*)>& func) = 0;

    virtual DmVector restrictOrthogonal(const DmVector& coord) = 0;
    virtual DmVector restrictHorizontal(const DmVector& coord) = 0;
    virtual DmVector restrictVertical(const DmVector& coord) = 0;

    virtual DmEntity* catchEntity(const DmVector& pos, DM::ResolveLevel level = DM::ResolveNone) = 0;
    virtual DmEntity* catchEntity(QMouseEvent* e, DM::ResolveLevel level = DM::ResolveNone) = 0;
    virtual DmEntity* catchEntity(const DmVector& pos, DM::EntityType enType, DM::ResolveLevel level = DM::ResolveNone) = 0;
    virtual DmEntity* catchEntity(QMouseEvent* e, DM::EntityType enType, DM::ResolveLevel level = DM::ResolveNone) = 0;
    virtual DmEntity* catchEntity(QMouseEvent* e, const EntityTypeList& enTypeList, DM::ResolveLevel level = DM::ResolveNone) = 0;

    // 另一个操作发生时暂停此捕捉器
    virtual void suspend() = 0;
    // 暂停后恢复此捕捉器
    virtual void resume() = 0;

    virtual void hideOptions() = 0;
    virtual void showOptions() = 0;

    virtual void drawSnapper() = 0;

    /// @brief 从屏幕上删除捕捉指示器
    virtual void deleteSnapper() = 0;
};

#endif // ISNAPSERVICE_H
