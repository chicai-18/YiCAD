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

/// @file DrawPolylineCommand.h
/// @brief 画多段线命令 ext.draw.polyline，取代原 ActionDrawPolyline
///
/// 逐点添加直线段或圆弧段（相切、相切定半径、定角度），可设起止线宽、闭合、撤销。
/// 第一段落下时多段线即加入文档，此后每段就地修改。

#ifndef DRAWPOLYLINECOMMAND_H
#define DRAWPOLYLINECOMMAND_H

#include <QCoreApplication>

#include "PlaceCommand.h"

class ArcData;
class DmPolyline;
class DmVector;
class DrawPolylineTool;

/// @brief 画多段线命令；交互由 DrawPolylineTool 驱动，选项条参数在本类
class DrawPolylineCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(DrawPolylineCommand)

public:
    /// @brief 线段模式
    enum SegmentMode
    {
        Line = 0,       ///< 直线
        Tangential = 1, ///< 相切
        TanRad = 2,     ///< 相切定半径
        Ang = 3,        ///< 定角度
    };

    DrawPolylineCommand();
    ~DrawPolylineCommand() override;

    // ---- 选项条（UIPolylineOptions）----

    void setMode(SegmentMode m) { m_mode = m; }
    int getMode() const { return m_mode; }
    void setStartWeight(double start) { m_startWeight = start; }
    double getStartWeight() const { return m_startWeight; }
    void setEndWeight(double end) { m_endWeight = end; }
    double getEndWeight() const { return m_endWeight; }
    void setRadius(double r) { m_radius = r; }
    double getRadius() const { return m_radius; }
    void setAngle(double a) { m_angle = a; }
    double getAngle() const { return m_angle; }
    void setCCW(bool ccw) { m_ccw = ccw; }
    bool isCCW() const { return m_ccw; }
    /// @brief 闭合多段线
    void close();
    /// @brief 撤销上一点
    void undo();

    // ---- 预览与提交（工具调用）----

    /// @brief 预览直线段：没有线宽时画线，有线宽时画填充四边形
    void previewLineSegment(const DmVector& from, const DmVector& to);
    /// @brief 预览圆弧段
    void previewArcSegment(const ArcData& arc);
    /// @brief 第一段落下：把多段线加入文档
    void addPolyline(DmPolyline* polyline);
    /// @brief 后续各段：标记多段线已修改
    void updatePolyline(DmPolyline* polyline);
    /// @brief 撤销到只剩起点：把多段线移出文档
    void removePolyline(DmPolyline* polyline);

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
    void showOptions() override;
    void hideOptions() override;

private:
    DrawPolylineTool* tool() const;

    double m_startWeight = 0.0; ///< 起始线宽
    double m_endWeight = 0.0;   ///< 终止线宽
    SegmentMode m_mode = Line;  ///< 线段模式
    double m_radius = 0.0;      ///< 相切定半径模式的半径
    double m_angle = 0.0;       ///< 定角度模式的包角（度）
    bool m_ccw = true;          ///< 定角度模式是否逆时针
};

#endif // DRAWPOLYLINECOMMAND_H
