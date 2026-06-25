/****************************************************************************
**
 * Draw ellipse by foci and a point on ellipse

Copyright (C) 2011 Dongxu Li (dongxuli2011@gmail.com)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/


/// @file ActionDrawEllipseInscribe.h
/// @brief 通过内切四边形边线绘制椭圆的交互动作类

#ifndef ACTIONDRAWELLIPSEINSCRIBE_H
#define ACTIONDRAWELLIPSEINSCRIBE_H

#include "PreviewActionInterface.h"

/// @brief 通过选择四条直线作为内切四边形的边来绘制椭圆
class ActionDrawEllipseInscribe : public PreviewActionInterface
{
    Q_OBJECT
public:
    /// @brief 动作状态枚举
    enum Status
    {
        SetLine1, ///< 选择第一条边线
        SetLine2, ///< 选择第二条边线
        SetLine3, ///< 选择第三条边线
        SetLine4  ///< 选择第四条边线
    };

public:
    ActionDrawEllipseInscribe(DmDocument* doc, GuiDocumentView* docView);
    ~ActionDrawEllipseInscribe() override;

    void init(int status = 0) override;
    void trigger() override;

    /// @brief 准备预览数据（根据已选边计算内切椭圆）
    /// @return 成功返回 true，无法确定唯一椭圆时返回 false
    bool preparePreview();

    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

    QStringList getAvailableCommands() override;
    void finish(bool updateTB = true) override;
    void updateMouseButtonHints() override;
    void updateMouseCursor() override;

private:
    /// @brief 清除已选中的边线
    /// @param checkStatus 为 true 时保留当前状态对应的边
    void clearLines(bool checkStatus = false);

    struct Points;
    std::unique_ptr<Points> pPoints;
};

#endif
