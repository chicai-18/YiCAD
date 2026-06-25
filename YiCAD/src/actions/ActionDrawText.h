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


/// @file ActionDrawText.h
/// @brief 文本绘制 Action 类，处理用户事件以绘制文本实体

#ifndef ACTIONDRAWTEXT_H
#define ACTIONDRAWTEXT_H

#include "PreviewActionInterface.h"

class TextData;

/// @brief 处理文本绘制的用户交互 Action
class ActionDrawText : public PreviewActionInterface
{
    Q_OBJECT

public:
    /// @brief Action 状态枚举
    enum Status
    {
        ShowDialog, ///< 显示文本对话框
        SetPos,     ///< 设置位置
        SetSecPos,  ///< 设置对齐/布满文本的第二点
        SetText     ///< 在命令行中设置文本
    };

public:
    /// @brief 构造函数
    /// @param doc 文档指针
    /// @param docView 文档视图指针
    ActionDrawText(DmDocument* doc, GuiDocumentView* docView);
    ~ActionDrawText() override;

    /// @brief 初始化 Action 状态
    /// @param status 初始状态
    void init(int status = 0) override;

    /// @brief 重置数据
    void reset();

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
    void updateMouseCursor() override;

    /// @brief 设置文本内容
    /// @param t 文本字符串
    void setText(const QString& t);

    /// @brief 获取文本内容
    /// @return 文本字符串
    QString getText() const;

    /// @brief 设置文本角度
    /// @param a 角度值
    void setAngle(double a);

    /// @brief 获取文本角度
    /// @return 角度值
    double getAngle() const;

private:
    /// @brief 对于只需要1点定位的情况，设置对齐及位置坐标
    void setDataWithOnePoint();

private:
    struct Points;
    std::unique_ptr<Points>     pPoints;
    std::unique_ptr<TextData>   data;
    bool                        textChanged;
};

#endif
