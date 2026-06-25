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


/// @file ActionDimRadial.h
/// @brief 径向/直径标注交互操作类头文件

#ifndef ACTIONDIMRADIAL_H
#define ACTIONDIMRADIAL_H

#include "ActionDimension.h"

struct DmDimRadialData;

/// @brief 处理用户事件以绘制径向/直径标注。
class ActionDimRadial : public ActionDimension
{
    Q_OBJECT
private:
    enum Status
    {
        SetEntity, /**< Choose entity. */
        SetPos,    /**< Choose point. */
        SetText    /**< Setting text label in the command line. */
    };

public:
    /// @brief 构造函数
    /// @param [in] doc 文档对象指针
    /// @param [in] docView 文档视图指针
    explicit ActionDimRadial(DmDocument* doc, GuiDocumentView* docView);
    ~ActionDimRadial() override;

    /// @brief 重置操作状态
    void reset() override;

    void trigger() override;
    void preparePreview();

    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

    void coordinateEvent(GuiCoordinateEvent* e) override;
    void commandEvent(GuiCommandEvent* e) override;
    QStringList getAvailableCommands() override;

    void hideOptions() override;
    void showOptions() override;

    void updateMouseButtonHints() override;

private:
    DmEntity* entity = nullptr;                   ///< 已选中的实体（圆弧/圆）。
    std::unique_ptr<DmVector> pos;                 ///< 拾取圆后鼠标移动时的位置。
    std::unique_ptr<DmDimRadialData> edata;        ///< 新标注数据。
    Status lastStatus;                             ///< 进入文字输入前的状态。
};

#endif
