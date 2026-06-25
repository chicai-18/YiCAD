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


/// @file ActionModifyCut.h
/// @brief 裁剪实体操作 —— 选择实体并指定剪切点将其一分为二。

#ifndef ACTIONMODIFYCUT_H
#define ACTIONMODIFYCUT_H

#include "ActionInterface.h"

/// @brief 裁剪操作类：处理用户事件以实现实体裁剪分割功能。
class ActionModifyCut : public ActionInterface
{
    Q_OBJECT
public:
    /// @brief 操作状态。
    enum Status
    {
        ChooseCutEntity, ///< 选择要裁剪的实体。
        SetCutCoord      ///< 指定裁剪点坐标。
    };

    /// @brief 构造函数。
    /// @param doc CAD文档对象。
    /// @param docView 文档视图对象。
    ActionModifyCut(DmDocument* doc, GuiDocumentView* docView);

    /// @brief 析构函数。
    ~ActionModifyCut() override;

    /// @brief 初始化操作状态。
    /// @param status 初始状态索引，默认为 ChooseCutEntity。
    void init(int status = 0) override;

    /// @brief 执行裁剪操作，将实体在裁剪点处分割。
    void trigger() override;

    /// @brief 处理鼠标移动事件。
    /// @param e 鼠标事件对象。
    void mouseMoveEvent(QMouseEvent* e) override;

    /// @brief 处理鼠标释放事件，根据当前状态选择实体或设置裁剪点。
    /// @param e 鼠标事件对象。
    void mouseReleaseEvent(QMouseEvent* e) override;

    /// @brief 更新鼠标按钮提示文本。
    void updateMouseButtonHints() override;

    /// @brief 更新鼠标光标样式。
    void updateMouseCursor() override;

    /// @brief 结束操作，清理高亮状态。
    /// @param updateTB 是否更新工具栏。
    void finish(bool updateTB) override;

protected:
    /// @brief 判断实体类型是否支持裁剪。
    /// @param e 待检查的实体指针。
    /// @return 若实体类型为直线、圆弧、椭圆、多段线或样条曲线则返回 true。
    bool entityTrimmable(DmEntity* e) const;

private:
    DmEntity* cutEntity = nullptr;            ///< 当前选中的待裁剪实体。
    std::unique_ptr<DmVector> cutCoord;       ///< 裁剪点坐标。
};

#endif // ACTIONMODIFYCUT_H
