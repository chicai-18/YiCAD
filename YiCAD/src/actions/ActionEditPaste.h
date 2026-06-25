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


/// @file ActionEditPaste.h
/// @brief 粘贴 Action 类，处理将剪贴板实体粘贴到当前文档的用户事件

#ifndef ACTIONEDITPASTE_H
#define ACTIONEDITPASTE_H

#include "PreviewActionInterface.h"

/// @brief 处理粘贴操作的用户交互 Action
class ActionEditPaste : public PreviewActionInterface
{
    Q_OBJECT

public:
    /// @brief Action 状态枚举
    enum Status
    {
        SetTargetPoint ///< 设置目标参考点
    };

public:
    /// @brief 构造函数
    /// @param doc 文档指针
    /// @param docView 文档视图指针
    ActionEditPaste(DmDocument* doc, GuiDocumentView* docView);
    ~ActionEditPaste() override;

    /// @brief 初始化 Action 状态
    /// @param status 初始状态
    void init(int status = 0) override;

    void trigger() override;

    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

    void coordinateEvent(GuiCoordinateEvent* e) override;

    void updateMouseButtonHints() override;
    void updateMouseCursor() override;

private:
    /// @brief 将剪贴板中的图层复制到当前文档
    void pasteLayers(DmDocument* source);

protected:
    std::unique_ptr<DmVector> targetPoint; ///< 粘贴目标参考点
};

#endif // ACTIONEDITPASTE_H
