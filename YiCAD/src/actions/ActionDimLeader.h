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


/// @file ActionDimLeader.h
/// @brief 引线标注交互操作类头文件

#ifndef ACTIONDIMLEADER_H
#define ACTIONDIMLEADER_H

#include <memory>

#include "PreviewActionInterface.h"

class DmLeader;

/// @brief 处理用户事件以绘制引线标注（箭头）。
class ActionDimLeader : public PreviewActionInterface
{
    Q_OBJECT
public:
    /**
     * Action States.
     */
    enum Status
    {
        SetStartpoint, /**< Setting the startpoint.  */
        SetEndpoint    /**< Setting the endpoint. */
    };

public:
    /// @brief 构造函数
    /// @param [in] doc 文档对象指针
    /// @param [in] docView 文档视图指针
    explicit ActionDimLeader(DmDocument* doc, GuiDocumentView* docView);
    ~ActionDimLeader() override;

    /// @brief 重置操作状态
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

protected:
    DmLeader* leader = nullptr;          ///< 引线标注实体。

    struct Points;
    std::unique_ptr<Points> pPoints;     ///< 已设置的坐标点。
};

#endif
