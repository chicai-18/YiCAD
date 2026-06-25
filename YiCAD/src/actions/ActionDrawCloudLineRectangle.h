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


/// @file ActionDrawCloudLineRectangle.h
/// @brief 矩形云线（修订云线）绘制交互动作类

#ifndef ACTIONDRAWCLOUDLINERECTANGLE_H
#define ACTIONDRAWCLOUDLINERECTANGLE_H

#include "PreviewActionInterface.h"

class DmPolyline;
class DmVector;

/// @brief 通过指定矩形对角点生成修订云线
class ActionDrawCloudLineRectangle : public PreviewActionInterface
{
    Q_OBJECT
public:
    /// @brief 动作状态枚举
    enum Status
    {
        SetStartPoint, ///< 设置起点（矩形第一角点）
        SetEndPoint    ///< 设置终点（矩形对角点）
    };

public:
    ActionDrawCloudLineRectangle(DmDocument* doc, GuiDocumentView* docView);
    ~ActionDrawCloudLineRectangle() override;

    void reset();
    void init(int status = 0) override;
    void trigger() override;

    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

    void coordinateEvent(GuiCoordinateEvent* e) override;
    void commandEvent(GuiCommandEvent* e) override;
    QStringList getAvailableCommands() override;

    void showOptions() override;
    void hideOptions() override;

    void updateMouseButtonHints() override;
    void updateMouseCursor() override;

    /// @brief 用矩形框计算代表云线的多段线
    /// @param container [in] 云线的容器
    /// @param startPt [in] 矩形框的起点
    /// @param endPt [in] 矩形框的终点
    /// @return 生成的云线多段线，失败返回 nullptr
    DmPolyline* calculateCloudRect(DmEntityContainer* container, DmVector startPt, DmVector endPt);

    void setMaxLength(double maxLength);
    void setMinLength(double minLength);
    double getMaxLength() const;
    double getMinLength() const;

protected:
    double m_minArcLen{ 5.0 }; ///< 最小弧长
    double m_maxArcLen{ 10.0 }; ///< 最大弧长

    struct Points;
    std::unique_ptr<Points> pPoints;
};

#endif
