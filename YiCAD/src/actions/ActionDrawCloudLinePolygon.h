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


/// @file ActionDrawCloudLinePolygon.h
/// @brief 通过控制点绘制多边形修订云线的交互动作类

#ifndef ACTIONDRAWCLOUDLINEPOLYGON_H
#define ACTIONDRAWCLOUDLINEPOLYGON_H

#include "PreviewActionInterface.h"
#include <QList>

class DmPolyline;
class DmVector;

/// @brief 通过控制点生成多边形修订云线
class ActionDrawCloudLinePolygon : public PreviewActionInterface
{
    Q_OBJECT
public:
    /// @brief 动作状态枚举
    enum Status
    {
        SetStartPoint, ///< 设置起点
        SetEndPoint    ///< 设置后续控制点
    };

public:
    ActionDrawCloudLinePolygon(DmDocument* doc, GuiDocumentView* docView);
    ~ActionDrawCloudLinePolygon() override;

    void reset();

    /// @brief 根据控制点创建云线
    /// @param container[in] 放置云线的容器
    /// @param polyPts[in] 控制点。如果控制点为2个，云线不闭合；如果控制点为3个及以上，云线自动闭合
    /// @return 云线创建成功返回云线，否则返回nullptr
    DmPolyline* calculateCloudPoly(DmEntityContainer* container, const std::vector<DmVector>& polyPts);

    void init(int status = 0) override;
    void trigger() override;

    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;

    void coordinateEvent(GuiCoordinateEvent* e) override;
    void commandEvent(GuiCommandEvent* e) override;
    QStringList getAvailableCommands() override;

    void showOptions() override;
    void hideOptions() override;

    void updateMouseButtonHints() override;
    void updateMouseCursor() override;

    void setMaxLength(double maxLength);
    void setMinLength(double minLength);
    double getMaxLength();
    double getMinLength();
    void undo();

    /// @brief 绘制云线预览
    /// @param container[in] 云线所在容器
    void drawPoly(DmEntityContainer* container);

protected:
    double m_minArcLen; ///< 最小弧长
    double m_maxArcLen; ///< 最大弧长

    struct Points;
    std::unique_ptr<Points> pPoints;
};

#endif
