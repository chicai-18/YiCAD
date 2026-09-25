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

/// @file DrawLineBisectorCommand.h
/// @brief 画角平分线命令 ext.draw.line_bisector，取代原 ActionDrawLineBisector：选两条直线，
///        在其夹角内等分画若干条给定长度的直线

#ifndef DRAWLINEBISECTORCOMMAND_H
#define DRAWLINEBISECTORCOMMAND_H

#include <QCoreApplication>

#include "PlaceCommand.h"

class DmLine;
class DmVector;

/// @brief 画角平分线命令；交互由 DrawLineBisectorTool 驱动，选项条参数在本类
class DrawLineBisectorCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(DrawLineBisectorCommand)

public:
    // ---- 选项条（UILineBisectorOptions）----

    void setLength(double l) { m_length = l; }
    double getLength() const { return m_length; }
    void setNumber(int n) { m_number = n; }
    int getNumber() const { return m_number; }

    // ---- 预览与提交（工具调用）----

    /// @brief 预览两条线之间的角平分线
    /// @param coord1 选第一条线时的鼠标位置
    /// @param coord2 当前鼠标位置
    void previewBisectors(const DmVector& coord1, const DmVector& coord2, DmLine* line1, DmLine* line2);
    /// @brief 提交角平分线；两条线不相交时提交一个空事务（与原 Action 一致）
    void commitBisectors(const DmVector& coord1, const DmVector& coord2, DmLine* line1, DmLine* line2);

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
    void showOptions() override;
    void hideOptions() override;

private:
    double m_length = 10.0; ///< 角平分线的长度
    int m_number = 1;       ///< 要创建的角平分线数量
};

#endif // DRAWLINEBISECTORCOMMAND_H
