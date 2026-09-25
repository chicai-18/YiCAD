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

/// @file DrawLinePolygonCommand.h
/// @brief 画正多边形命令：中心+角点 ext.draw.line_polygon_cen_cor（原 ActionDrawLinePolygonCenCor）
///        与中心+切点 ext.draw.line_polygon_cen_tan（原 ActionDrawLinePolygonCenTan）
///
/// 两者共用边数选项条 UILinePolygonOptions。原中心+切点借用了中心+角点的 Action 类型，
/// 选项条靠两个类恰好相同的内存布局才能设置它的边数；现在两者有共同的基类。

#ifndef DRAWLINEPOLYGONCOMMAND_H
#define DRAWLINEPOLYGONCOMMAND_H

#include <vector>

#include <QCoreApplication>
#include <QString>

#include "PlaceCommand.h"

class DmEntityContainer;
class DmLine;
class DmVector;

/// @brief 画正多边形命令的基类：边数选项，预览与提交
class LinePolygonCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(LinePolygonCommand)

public:
    /// @brief 多边形最少边数
    static constexpr int MIN_POLYGON_EDGES = 3;

    // ---- 选项条（UILinePolygonOptions）----

    int getNumber() const { return m_number; }
    void setNumber(int n) { m_number = n; }

    // ---- 工具调用 ----

    /// @brief 第二点的按键提示
    virtual QString secondPointHint() const = 0;
    /// @brief 命令行输入的边数：合法时设为边数，否则给出提示
    virtual void inputNumber(int n) = 0;
    /// @brief 预览多边形
    void previewPolygon(const DmVector& center, const DmVector& point);
    /// @brief 提交多边形（各边为直线）
    void commitPolygon(const DmVector& center, const DmVector& point);

protected:
    LinePolygonCommand() = default;

    /// @brief 由中心与第二点求各边；container 不为空时各边同时加进它
    virtual std::vector<DmLine*> createPolygon(DmEntityContainer* container, const DmVector& center,
                                               const DmVector& point) const = 0;
    /// @brief 提交时的事务名
    virtual QString transactionName() const = 0;

    std::unique_ptr<BasePlaceTool> createTool() override;
    void showOptions() override;
    void hideOptions() override;

    int m_number = MIN_POLYGON_EDGES; ///< 边数
};

/// @brief 画正多边形：中心与一个角点
class DrawLinePolygonCenCorCommand : public LinePolygonCommand
{
    Q_DECLARE_TR_FUNCTIONS(DrawLinePolygonCenCorCommand)

public:
    QString secondPointHint() const override;
    void inputNumber(int n) override;

protected:
    std::vector<DmLine*> createPolygon(DmEntityContainer* container, const DmVector& center,
                                       const DmVector& corner) const override;
    QString transactionName() const override;
};

/// @brief 画正多边形：中心与一条边的切点（边的中点）
class DrawLinePolygonCenTanCommand : public LinePolygonCommand
{
    Q_DECLARE_TR_FUNCTIONS(DrawLinePolygonCenTanCommand)

public:
    QString secondPointHint() const override;
    void inputNumber(int n) override;

protected:
    std::vector<DmLine*> createPolygon(DmEntityContainer* container, const DmVector& center,
                                       const DmVector& tangent) const override;
    QString transactionName() const override;
};

#endif // DRAWLINEPOLYGONCOMMAND_H
