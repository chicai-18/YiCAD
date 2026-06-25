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


/// @file ActionDrawImage.h
/// @brief 图片插入交互动作类

#ifndef ACTIONDRAWIMAGE_H
#define ACTIONDRAWIMAGE_H

#include <memory>

#include "PreviewActionInterface.h"

class ImageData;
class QImage;

/// @brief 通过指定插入点和缩放参数将位图插入当前绘图
class ActionDrawImage : public PreviewActionInterface
{
    Q_OBJECT

    /// @brief 动作状态枚举
    enum Status
    {
        ShowDialog,     ///< 显示图片选择对话框
        SetTargetPoint, ///< 设置插入参考点
        SetAngle,       ///< 在命令行设置旋转角度
        SetFactor,      ///< 在命令行设置缩放因子
        SetDPI          ///< 在命令行设置 DPI
    };

public:
    ActionDrawImage(DmDocument* doc, GuiDocumentView* docView);
    ~ActionDrawImage() override;

    void init(int status = 0) override;
    void reset();
    void trigger() override;

    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

    void coordinateEvent(GuiCoordinateEvent* e) override;
    void commandEvent(GuiCommandEvent* e) override;
    QStringList getAvailableCommands() override;

    void showOptions() override;
    void hideOptions() override;

    void updateMouseButtonHints() override;
    void updateMouseCursor() override;

    /// @brief 获取当前旋转角度（弧度）
    /// @return 旋转角度
    double getAngle() const;

    /// @brief 设置旋转角度
    /// @param a 角度值（弧度）
    void setAngle(double a) const;

    /// @brief 获取当前缩放因子
    /// @return 缩放因子
    double getFactor() const;

    /// @brief 设置缩放因子
    /// @param f 缩放因子
    void setFactor(double f) const;

    /// @brief DPI 值转换为缩放因子
    /// @param dpi DPI 值
    /// @return 对应的缩放因子
    double dpiToScale(double dpi) const;

    /// @brief 缩放因子转换为 DPI 值
    /// @param scale 缩放因子
    /// @return 对应的 DPI 值
    double scaleToDpi(double scale) const;

protected:
    struct AImageData;
    std::unique_ptr<AImageData> pImg;

    Status m_lastStatus{ ShowDialog }; ///< 进入选项前的上一个状态
};

#endif
