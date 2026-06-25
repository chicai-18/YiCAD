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


/// @file ActionModifyExtend.h
/// @brief 延伸实体的交互动作类声明

#ifndef ACTIONMODIFYEXTEND
#define ACTIONMODIFYEXTEND

#include "PreviewActionInterface.h"

class DmPolyline;
class DmLine;
class DmArc;
class DmEllipse;
class DmSpline;

/// @brief 延伸实体的交互动作
///
/// 处理用户延伸直线、圆弧、多段线、椭圆弧和样条曲线到边界实体的操作。
class ActionModifyExtend : public PreviewActionInterface
{
    Q_OBJECT
public:
    /// @brief 动作状态枚举
    enum Status
    {
        ChooseEntity ///< 选择实体状态
    };

    /// @brief 拾取实体端点侧枚举
    enum class PickEntitySide
    {
        Begin, ///< 起始端
        End    ///< 末端
    };

public:
    /// @brief 构造函数
    /// @param [in] doc 文档指针
    /// @param [in] docView 文档视图指针
    ActionModifyExtend(DmDocument* doc, GuiDocumentView* docView);

    /// @brief 析构函数
    ~ActionModifyExtend() override;

    /// @brief 初始化动作
    /// @param [in] status 初始状态，默认为0
    void init(int status = 0) override;

    /// @brief 触发延伸操作
    void trigger() override;

    /// @brief 鼠标移动事件处理
    /// @param [in] e 鼠标事件指针
    void mouseMoveEvent(QMouseEvent* e) override;

    /// @brief 鼠标释放事件处理
    /// @param [in] e 鼠标事件指针
    void mouseReleaseEvent(QMouseEvent* e) override;

    /// @brief 更新鼠标光标样式
    void updateMouseCursor() override;

    /// @brief 计算点距离哪个端更近
    /// @param [in] e 实体指针
    /// @param [in] pt 参考点
    /// @return 拾取的实体端点侧
    ActionModifyExtend::PickEntitySide getEntitySide(DmEntity* e, const DmVector& pt);

    /// @brief 计算点在直线上的哪一端更近
    /// @param [in] e 直线实体指针
    /// @param [in] pt 参考点
    /// @return 拾取的实体端点侧
    ActionModifyExtend::PickEntitySide getEntitySideOfLine(DmLine* e, const DmVector& pt);

    /// @brief 计算点在圆弧上的哪一端更近
    /// @param [in] e 圆弧实体指针
    /// @param [in] pt 参考点
    /// @return 拾取的实体端点侧
    ActionModifyExtend::PickEntitySide getEntitySideOfArc(DmArc* e, const DmVector& pt);

    /// @brief 计算点在多段线上的哪一端更近
    /// @param [in] e 多段线实体指针
    /// @param [in] pt 参考点
    /// @return 拾取的实体端点侧
    ActionModifyExtend::PickEntitySide getEntitySideOfPolyline(DmPolyline* e, const DmVector& pt);

    /// @brief 计算点在椭圆弧上的哪一端更近
    /// @param [in] e 椭圆弧实体指针
    /// @param [in] pt 参考点
    /// @return 拾取的实体端点侧
    ActionModifyExtend::PickEntitySide getEntitySideOfEllipse(DmEllipse* e, const DmVector& pt);

    /// @brief 计算点在样条曲线上的哪一端更近
    /// @param [in] e 样条曲线实体指针
    /// @param [in] pt 参考点
    /// @return 拾取的实体端点侧
    ActionModifyExtend::PickEntitySide getEntitySideOfSpline(DmSpline* e, const DmVector& pt);

    /// @brief 延伸操作
    /// @brief 执行实体延伸
    /// @param [in] e 待延伸实体指针
    /// @param [in] side 延伸端侧
    /// @return 延伸后的新实体指针，失败返回nullptr
    DmEntity* extend(DmEntity* e, ActionModifyExtend::PickEntitySide side);

    /// @brief 延伸直线
    /// @param [in] line 直线实体指针
    /// @param [in] side 延伸端侧
    /// @return 延伸后的新直线指针，失败返回nullptr
    DmLine* extendForLine(DmLine* line, ActionModifyExtend::PickEntitySide side);

    /// @brief 延伸圆弧
    /// @param [in] arc 圆弧实体指针
    /// @param [in] side 延伸端侧
    /// @return 延伸后的新圆弧指针，失败返回nullptr
    DmArc* extendForArc(DmArc* arc, ActionModifyExtend::PickEntitySide side);

    /// @brief 延伸多段线
    /// @param [in] poly 多段线实体指针
    /// @param [in] side 延伸端侧
    /// @return 延伸后的新多段线指针，失败返回nullptr
    DmPolyline* extendForPolyline(DmPolyline* poly, ActionModifyExtend::PickEntitySide side);

    /// @brief 延伸椭圆弧
    /// @param [in] ellipse 椭圆弧实体指针
    /// @param [in] side 延伸端侧
    /// @return 延伸后的新椭圆弧指针，失败返回nullptr
    DmEllipse* extendForEllipse(DmEllipse* ellipse, ActionModifyExtend::PickEntitySide side);

    /// @brief 延伸样条曲线
    /// @param [in] spline 样条曲线实体指针
    /// @param [in] side 延伸端侧
    /// @return 延伸后的新样条曲线指针，失败返回nullptr
    DmSpline* extendForSpline(DmSpline* spline, ActionModifyExtend::PickEntitySide side);

    /// @brief 判断实体是否可被延伸
    /// @param [in] ent 实体指针
    /// @return 可延伸返回true，否则返回false
    static bool isExtendableEntity(DmEntity* ent);

public slots:
    /// @brief 视图变化槽函数
    void slotViewChanged();

    /// @brief 更新当前视图内的实体列表
    void updateEntitiesInView();

private:
    DmEntity* m_entToTrim = nullptr;                       ///< 待延伸的实体
    DmEntity* m_entUnderCursor = nullptr;                  ///< 当前鼠标下的实体
    DmVector m_trimPt{false};                           ///< 确认延伸时，鼠标点下的位置
    bool m_bExtendToSelect = false;                         ///< 是否延伸至选择的实体

    PickEntitySide m_side = PickEntitySide::Begin;         ///< 裁剪实体的端，鼠标移动时记录同一实体端变化以减少延伸计算
    std::unique_ptr<DmEntity> m_extendedEnt = nullptr;     ///< 预览时临时的延伸后的实体

    std::vector<DmEntity*> m_seleltedEnts;                 ///< 选择的实体，作为延伸的边界，也可以是待延伸的实体
    std::vector<DmEntity*> m_entsInView;                   ///< 当前视图内的实体
};

#endif // ACTIONMODIFYEXTEND
