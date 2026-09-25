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

/// @file DrawLineCommand.h
/// @brief 画直线命令 ext.draw.line，取代原 ActionDrawLine：连续指定点画线段，可闭合、撤销、重做

#ifndef DRAWLINECOMMAND_H
#define DRAWLINECOMMAND_H

#include <QCoreApplication>

#include "PlaceCommand.h"

class DmVector;
class DrawLineTool;
class LineData;

/// @brief 画直线命令；交互由 DrawLineTool 驱动
class DrawLineCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(DrawLineCommand)

public:
    DrawLineCommand();
    ~DrawLineCommand() override;

    /// @brief 预览从起点到鼠标位置的线段
    void previewLine(const DmVector& start, const DmVector& end);
    /// @brief 提交一条线段
    void commitLine(const LineData& data);

    // ---- 选项条（UILineOptions）----

    /// @brief 闭合当前线段组
    void close();
    /// @brief 撤销上一步
    void undo();
    /// @brief 重做下一步
    void redo();

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
    void showOptions() override;
    void hideOptions() override;

private:
    DrawLineTool* tool() const;
};

#endif // DRAWLINECOMMAND_H
