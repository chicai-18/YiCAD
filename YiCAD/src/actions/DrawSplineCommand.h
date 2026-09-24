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

/// @file DrawSplineCommand.h
/// @brief 样条命令：控制点样条 draw.spline（原 ActionDrawSpline）与拟合点样条
///        draw.spline_points（原 ActionDrawSplinePoints）
///
/// 两者共用选项条 UISplineOptions。阶数与是否闭合记在工具正在画的样条数据里
/// （与原 Action 一致），命令把选项条的调用转给工具。

#ifndef DRAWSPLINECOMMAND_H
#define DRAWSPLINECOMMAND_H

#include <QCoreApplication>

#include "PlaceCommand.h"

class DmSpline;

/// @brief 样条命令的基类：选项条的公共部分
class SplineCommand : public PlaceCommand
{
public:
    // ---- 选项条（UISplineOptions）----

    /// @brief 撤销上一点
    virtual void undo() = 0;
    virtual void setClosed(bool c) = 0;
    virtual bool isClosed() const = 0;

    // ---- 预览与提交（工具调用）----

    /// @brief 预览样条的一个副本，可选地连同它的控制点
    void previewSpline(const DmSpline& spline, bool withControlPoints);
    /// @brief 提交样条，文档接管它
    void commitSpline(DmSpline* spline, const QString& transactionName);

protected:
    SplineCommand() = default;
    void showOptions() override;
    void hideOptions() override;
};

/// @brief 控制点样条命令
class DrawSplineCommand : public SplineCommand
{
    Q_DECLARE_TR_FUNCTIONS(DrawSplineCommand)

public:
    void undo() override;
    void setClosed(bool c) override;
    bool isClosed() const override;
    void setDegree(int deg);
    int getDegree() const;

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
};

/// @brief 拟合点样条命令
class DrawSplinePointsCommand : public SplineCommand
{
    Q_DECLARE_TR_FUNCTIONS(DrawSplinePointsCommand)

public:
    void undo() override;
    void setClosed(bool c) override;
    bool isClosed() const override;

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;
};

#endif // DRAWSPLINECOMMAND_H
