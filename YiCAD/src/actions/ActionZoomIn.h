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


/// @file ActionZoomIn.h
/// @brief 缩放Action，支持不同方向和轴向的缩放操作

#ifndef ACTIONZOOMIN_H
#define ACTIONZOOMIN_H

#include "ActionInterface.h"

/// @brief 默认缩放因子
constexpr double DEFAULT_ZOOM_FACTOR = 1.25;

/// @brief 缩放操作Action，支持放大/缩小及轴向控制
class ActionZoomIn : public ActionInterface
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] doc 文档指针
    /// @param [in] docView 文档视图指针
    /// @param [in] direction 缩放方向（放大/缩小）
    /// @param [in] axis 受影响的轴向
    /// @param [in] pCenter 缩放中心点，为空则使用当前视图中心
    /// @param [in] factor 缩放因子
    ActionZoomIn(DmDocument* doc, GuiDocumentView* docView,
                 DM::ZoomDirection direction = DM::In,
                 DM::Axis axis = DM::Both,
                 DmVector const* pCenter = nullptr,
                 double factor = DEFAULT_ZOOM_FACTOR);

    /// @brief 析构函数
    ~ActionZoomIn() override;

    /// @brief 初始化操作状态
    /// @param [in] status 初始状态值，默认为0
    void init(int status = 0) override;

    /// @brief 执行缩放操作
    void trigger() override;

    /// @brief 是否为视图操作命令
    /// @return true，缩放属于视图操作
    bool isViewAction() override { return true; }

protected:
    double zoom_factor = DEFAULT_ZOOM_FACTOR;         ///< 缩放因子
    DM::ZoomDirection direction = DM::In;             ///< 缩放方向
    DM::Axis axis = DM::Both;                         ///< 受影响的轴向
    std::unique_ptr<DmVector> center;                 ///< 缩放中心点
};

#endif // ACTIONZOOMIN_H
