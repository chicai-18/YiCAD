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

/// @file DrawCloudLineCommand.h
/// @brief 云线命令：矩形 draw.cloud_line_rectangle（原 ActionDrawCloudLineRectangle）、
///        多边形 draw.cloud_line_polygon（原 ActionDrawCloudLinePolygon）、自由
///        draw.cloud_line_free（原 ActionDrawCloudLineFree）
///
/// 三者共用选项条 UICloudLineOptions：矩形、多边形设最小/最大弧长，自由云线设是否反向，
/// 多边形还有撤销。

#ifndef DRAWCLOUDLINECOMMAND_H
#define DRAWCLOUDLINECOMMAND_H

#include <QCoreApplication>

#include "PlaceCommand.h"

class DmPolyline;

/// @brief 云线命令的基类：预览容器与提交
class CloudLineCommand : public PlaceCommand
{
public:
    /// @brief 提交云线，文档接管它
    void commitCloudLine(DmPolyline* polyline, const QString& transactionName);

protected:
    CloudLineCommand() = default;
    void showOptions() override;
    void hideOptions() override;
};

/// @brief 矩形云线命令
class DrawCloudLineRectangleCommand : public CloudLineCommand
{
    Q_DECLARE_TR_FUNCTIONS(DrawCloudLineRectangleCommand)

public:
    void setMinLength(double minLength) { m_minArcLen = minLength; }
    void setMaxLength(double maxLength) { m_maxArcLen = maxLength; }
    double getMinLength() const { return m_minArcLen; }
    double getMaxLength() const { return m_maxArcLen; }

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;

private:
    double m_minArcLen = 5.0;  ///< 最小弧长
    double m_maxArcLen = 10.0; ///< 最大弧长
};

/// @brief 多边形云线命令
class DrawCloudLinePolygonCommand : public CloudLineCommand
{
    Q_DECLARE_TR_FUNCTIONS(DrawCloudLinePolygonCommand)

public:
    void setMinLength(double minLength) { m_minArcLen = minLength; }
    void setMaxLength(double maxLength) { m_maxArcLen = maxLength; }
    double getMinLength() const { return m_minArcLen; }
    double getMaxLength() const { return m_maxArcLen; }
    /// @brief 撤销上一点
    void undo();

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;

private:
    // 原 Action 没有初始化这两个值，选项条显示时才设置；这里取矩形云线的默认值
    double m_minArcLen = 5.0;  ///< 最小弧长
    double m_maxArcLen = 10.0; ///< 最大弧长
};

/// @brief 自由云线命令
class DrawCloudLineFreeCommand : public CloudLineCommand
{
    Q_DECLARE_TR_FUNCTIONS(DrawCloudLineFreeCommand)

public:
    bool getReversed() const { return m_isReversed; }
    void setReversed(bool reversed) { m_isReversed = reversed; }

protected:
    std::unique_ptr<BasePlaceTool> createTool() override;

private:
    bool m_isReversed = false; ///< 是否反转弧线方向
};

#endif // DRAWCLOUDLINECOMMAND_H
