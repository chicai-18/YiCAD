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


/// @file ActionEditCopy.h
/// @brief 编辑复制/剪切 Action 类，处理复制或剪切实体到剪贴板

#ifndef ACTIONEDITCOPY_H
#define ACTIONEDITCOPY_H

#include "ActionInterface.h"

/// @brief 处理复制/剪切到剪贴板的用户交互 Action
class ActionEditCopy : public ActionInterface
{
    Q_OBJECT

    /// @brief Action 状态枚举
    enum Status
    {
        SetReferencePoint ///< 设置参考点
    };

public:
    /// @brief 构造函数
    /// @param copy 为 true 时复制，为 false 时剪切
    /// @param doc 文档指针
    /// @param docView 文档视图指针
    ActionEditCopy(bool copy, DmDocument* doc, GuiDocumentView* docView);
    ~ActionEditCopy() override;

    /// @brief 初始化 Action 状态
    /// @param status 初始状态
    void init(int status) override;

    void trigger() override;

    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

    void coordinateEvent(GuiCoordinateEvent* e) override;

    void updateMouseButtonHints() override;
    void updateMouseCursor() override;

protected:
    bool copy;                              ///< true为复制，false为剪切
    std::unique_ptr<DmVector> referencePoint; ///< 参考点
};

#endif // ACTIONEDITCOPY_H
