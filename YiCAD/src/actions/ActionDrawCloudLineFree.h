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


/// @file ActionDrawCloudLineFree.h
/// @brief 自由绘制云线（修订云线）的交互动作类

#ifndef ACTIONDRAWCLOUDLINEFREE_H
#define ACTIONDRAWCLOUDLINEFREE_H

#include "PreviewActionInterface.h"
#include <QList>

class DmPolyline;
class DmVector;

/// @brief 通过鼠标自由移动轨迹绘制修订云线
class ActionDrawCloudLineFree : public PreviewActionInterface
{
    Q_OBJECT
public:
    /// @brief 动作状态枚举
    enum Status
    {
        SetStartPoint, ///< 设置起点
        SetEndPoint    ///< 设置终点/移动路径
    };

public:
    ActionDrawCloudLineFree(DmDocument* doc, GuiDocumentView* docView);
    ~ActionDrawCloudLineFree() override;

    void reset();
    void init(int status = 0) override;
    void trigger() override;

    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;

    void coordinateEvent(GuiCoordinateEvent* e) override;
    void commandEvent(GuiCommandEvent* e) override;
    QStringList getAvailableCommands() override;

    void updateMouseButtonHints() override;
    void updateMouseCursor() override;

    void showOptions() override;
    void hideOptions() override;

    /// @brief 根据历史轨迹点生成云线
    /// @param container[in] 云线所在容器
    /// @return 成功返回云线，否则返回nullptr
    DmPolyline* getPoly(DmEntityContainer* container);

    /// @brief 绘制云线预览
    /// @param container[in] 云线所在容器
    void drawPoly(DmEntityContainer* container);

    bool getReversed() { return m_isReversed; }
    void setReversed(bool reversed) { m_isReversed = reversed; }

protected:
    bool m_isReversed{ false }; ///< 是否反转弧线方向

    struct Points;
    std::unique_ptr<Points> pPoints;
};

#endif
