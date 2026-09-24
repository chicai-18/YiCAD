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

/// @file DimensionCommand.h
/// @brief 标注命令的共同基类，取代原 ActionDimension：通用标注数据（DmDimensionData）与
///        标签、公差、直径标志归放置工具，选项条（只有线性标注有）随命令显示、收起
///
/// 业务工具化第三步第⑨批：标注扩展的 Action 改为命令 + 放置工具。各标注的工具从原 Action
/// 机械改写而来，预览与提交仍在工具里（与第④批的修改命令相同）。

#ifndef DIMENSIONCOMMAND_H
#define DIMENSIONCOMMAND_H

#include <memory>

#include <QString>

#include "BasePlaceTool.h"
#include "PlaceCommand.h"

struct DmDimensionData;

/// @brief 标注命令的共同基类
class DimensionCommand : public PlaceCommand
{
protected:
    /// @brief 原 ActionDimension::showOptions：显示选项条并从命令读取
    void showOptions() override;
    void hideOptions() override;
};

/// @brief 标注工具的共同基类（原 ActionDimension 的数据部分）
class DimensionTool : public BasePlaceTool
{
public:
    DimensionTool(DimensionCommand& command, DmDocument* doc, IDocumentView* view);
    ~DimensionTool() override;

    /// @brief 原 ActionDimension 的光标
    std::optional<DM::CursorType> getCursor() const override { return DM::SelectCursor; }

protected:
    /// @brief 通用标注数据复位为当前标注样式（原 ActionDimension::reset）
    void resetDimension();
    /// @brief 回到某一状态（原 init(status)）；status < 0 时结束命令
    void init(int s);
    /// @brief 设置标注文字
    void setText(const QString& t);

    std::unique_ptr<DmDimensionData> data; ///< 通用标注数据
    QString label;                         ///< 标注标签
    QString tol1;                          ///< 上公差
    QString tol2;                          ///< 下公差
    bool diameter = false;                 ///< 直径标志
};

#endif  // DIMENSIONCOMMAND_H
