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
#include "ISnapService.h"

class DmEntity;
class IDocumentView;
class DmVector;
class QMouseEvent;

/// @brief 画布中的鼠标捕捉，ISnapService 的具体实现
class Snapper : public ISnapService
{
public:
    Snapper() = delete;
    Snapper(DmDocument* doc, IDocumentView* docView);
    ~Snapper() override;

    void init() override;
    /// @brief 结束捕捉
    void finish() override;

    void finishOrthogonal() override;
    void resetOrthogonal() override;

    DmEntity* getKeyEntity() const override;

    /// @brief 设置新的捕捉模式
    /// @param snapMode 捕捉模式
    void setSnapMode(const SnapMode& snapMode) override;

    SnapMode const* getSnapMode() const override;
    SnapMode* getSnapMode() override;

    /// @brief 获取当前捕捉结果类型
    SnapResultType getSnapResult() const override;
    /// @brief 获取当前捕捉点坐标（世界坐标）
    DmVector getSnapSpot() const override;
    /// @brief 获取当前最终捕捉坐标
    DmVector getSnapCoord() const override;

    /// @brief 设置新的捕捉限制
    /// @param snapRes 捕捉限制
    void setSnapRestriction(DM::SnapRestriction /*snapRes*/) override;

    DmVector snapPoint(const DmVector& coord, bool setSpot = false) override;
    DmVector snapPoint(QMouseEvent* e) override;
    DmVector snapFree(QMouseEvent* e) override;

    DmVector snapFree(const DmVector& coord) override;
    DmVector snapGrid(const DmVector& coord) override;

    /// @brief 捕捉到最近的端点
    /// @param coord 鼠标的坐标位置
    /// @return 返回点坐标或者无效的点
    DmVector snapEndpoint(const DmVector& coord) override;
    /// @brief 实体上的最近点
    DmVector snapOnEntity(const DmVector& coord) override;
    /// @brief 捕捉到最近的圆心
    /// @param coord 鼠标的坐标位置
    /// @return 返回点坐标或者无效的点
    DmVector snapCenter(const DmVector& coord) override;
    DmVector snapMiddle(const DmVector& coord) override;
    DmVector snapSubsection(const DmVector& coord) override;
    DmVector snapIntersection(const DmVector& coord) override;
    DmVector snapToAngle(const DmVector& coord, const DmVector& ref_coord, const double ang_res) override;

    DmVector snapByFunc(const DmVector& coord, const std::function<DmVector(DmEntity*, const DmVector&, double*)>& func) override;

    DmVector restrictOrthogonal(const DmVector& coord) override;
    DmVector restrictHorizontal(const DmVector& coord) override;
    DmVector restrictVertical(const DmVector& coord) override;

    DmEntity* catchEntity(const DmVector& pos, DM::ResolveLevel level = DM::ResolveNone) override;
    DmEntity* catchEntity(QMouseEvent* e, DM::ResolveLevel level = DM::ResolveNone) override;
    DmEntity* catchEntity(const DmVector& pos, DM::EntityType enType, DM::ResolveLevel level = DM::ResolveNone) override;
    /// @brief 捕捉指定类型的实体
    /// TODO ：多段线不再能捕获子实体，需要修改所有ResolveAll的catchEntity()引用处
    DmEntity* catchEntity(QMouseEvent* e, DM::EntityType enType, DM::ResolveLevel level = DM::ResolveNone) override;
    DmEntity* catchEntity(QMouseEvent* e, const EntityTypeList& enTypeList, DM::ResolveLevel level = DM::ResolveNone) override;

    // 另一个操作发生时暂停此捕捉器
    void suspend() override;

    // 暂停后恢复此捕捉器
    void resume() override;

    void hideOptions() override;
    void showOptions() override;

    void drawSnapper() override;

    void deleteSnapper() override;

protected:
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
    IDocumentView*      docView = nullptr;                  ///< 文档视图
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
