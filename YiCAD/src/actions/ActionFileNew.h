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


/// @file ActionFileNew.h
/// @brief 新建文件 Action 类，处理用户创建新空白文档的事件

#ifndef ACTIONFILENEW_H
#define ACTIONFILENEW_H

#include "ActionInterface.h"

/// @brief 新建文件 Action，独占模式，触发后创建新文档
class ActionFileNew : public ActionInterface
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param doc 文档指针
    /// @param docView 文档视图指针
    ActionFileNew(DmDocument* doc, GuiDocumentView* docView);

    /// @brief 判断是否为独占模式
    /// @return 始终返回 true
    bool isExclusive() override;

    /// @brief 初始化并立即触发新建
    /// @param status 初始状态
    void init(int status = 0) override;

    /// @brief 触发新建文件操作
    void trigger() override;

    /// @brief 获取文件名
    /// @return 文件全路径
    QString getFileName() const;

    /// @brief 设置文件名
    /// @param fileName 文件全路径，为空时自动设置
    void setFileName(const QString& fileName);

protected:
    QString m_strFileName; ///< 文件全路径，或者为空（新建时自动设置）
};

#endif // ACTIONFILENEW_H
