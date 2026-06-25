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


/// @file ActionInfoSelected.h
/// @brief 选中实体信息 Action 类，显示当前选中实体的详细信息

#ifndef ACTIONINFOSELECTED_H
#define ACTIONINFOSELECTED_H

#include "ActionInterface.h"

class DmSpline;

/// @brief 显示选中实体信息的 Action
class ActionInfoSelected : public ActionInterface
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param doc 文档指针
    /// @param docView 文档视图指针
    ActionInfoSelected(DmDocument* doc, GuiDocumentView* docView);

    /// @brief 初始化并立即触发
    /// @param status 初始状态
    void init(int status = 0) override;

    /// @brief 触发信息显示
    void trigger() override;

private:
    /// @brief 获取实体信息
    /// @param e 实体指针
    /// @param info 输出信息字符串
    void getInfo(const DmEntity* e, QString& info);

    /// @brief 格式化向量信息
    /// @param vec 向量
    /// @param info 输出信息字符串
    void infoVector(const DmVector& vec, QString& info);

    /// @brief 获取样条曲线专用信息
    /// @param spline 样条曲线指针
    /// @param info 输出信息字符串
    void getInfoForSpline(const DmSpline* spline, QString& info);
};

#endif // ACTIONINFOSELECTED_H
