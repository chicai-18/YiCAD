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

/// @file DrawCircleTan2Command.h
/// @brief 两切圆命令 ext.draw.circle_tan2，取代原 ActionDrawCircleTan2：选两条直线、圆弧或圆，
///        以给定半径在求出的公切圆里选最靠近鼠标的一个

#ifndef DRAWCIRCLETAN2COMMAND_H
#define DRAWCIRCLETAN2COMMAND_H

#include <QCoreApplication>

#include "PlaceCommand.h"

class CircleData;
class DrawCircleTan2Tool;

/// @brief 两切圆命令；交互由 DrawCircleTan2Tool 驱动
class DrawCircleTan2Command : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(DrawCircleTan2Command)

public:
    DrawCircleTan2Command();
    ~DrawCircleTan2Command() override;

    // ---- 选项条（UICircleTan2Options）----

    /// @brief 设置半径；已选完两个实体时重求圆心（半径记在工具的圆数据里，与原 Action 一致）
    void setRadius(double r);
    double getRadius() const;

    /// @brief 提交圆（工具调用）
    void commitCircle(const CircleData& data);

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
    void showOptions() override;
    void hideOptions() override;

private:
    DrawCircleTan2Tool* tool() const;
};

#endif // DRAWCIRCLETAN2COMMAND_H
