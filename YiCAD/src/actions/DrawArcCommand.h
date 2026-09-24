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

/// @file DrawArcCommand.h
/// @brief 圆心圆弧命令 draw.arc，取代原 ActionDrawArc：圆心、半径、起始角、包角
///
/// 同一文件里还有三点圆弧命令 draw.arc_3p（原 ActionDrawArc3P），它没有选项条，
/// 只在 .cpp 里定义。

#ifndef DRAWARCCOMMAND_H
#define DRAWARCCOMMAND_H

#include <QCoreApplication>

#include "PlaceCommand.h"

class ArcData;
class DmVector;
class DrawArcTool;

/// @brief 圆心圆弧命令；交互由 DrawArcTool 驱动
class DrawArcCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(DrawArcCommand)

public:
    DrawArcCommand();
    ~DrawArcCommand() override;

    // ---- 选项条（UIArcOptions）----

    /// @brief 是否顺时针；方向记在工具正在画的圆弧上（与原 Action 一致，每画完一段复位为逆时针）
    bool isClockwise() const;
    /// @brief 设置方向；已在指定包角时交换起止角
    void setClockwise(bool clockwise);

    // ---- 预览与提交（工具调用）----

    /// @brief 预览圆（指定半径时）
    void previewCircle(const DmVector& center, double radius);
    /// @brief 预览圆弧
    void previewArc(const ArcData& data);
    /// @brief 提交圆弧（顺时针的先换成逆时针），相对零点移到圆心
    void commitArc(const ArcData& data);

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
    void showOptions() override;
    void hideOptions() override;

private:
    DrawArcTool* tool() const;
};

#endif // DRAWARCCOMMAND_H
