/****************************************************************************
**
 * Draw a common tangent circle of 3 existing circles
 * Problem of Appollonius

Copyright (C) 2012 Dongxu Li (dongxuli2011@gmail.com)

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


/// @file ActionDrawCircleTan3.h
/// @brief 绘制三个给定圆的公切圆（阿波罗尼奥斯问题）的交互动作类

#ifndef ACTIONDRAWCIRCLETAN3_H
#define ACTIONDRAWCIRCLETAN3_H

#include <vector>

#include "PreviewActionInterface.h"

struct DmCircleData;
class DmAtomicEntity;

/// @brief 绘制三个给定圆/线的公切圆（阿波罗尼奥斯问题的解）
class ActionDrawCircleTan3 : public PreviewActionInterface
{
    Q_OBJECT
public:
    /// @brief 动作状态枚举
    enum Status
    {
        SetCircle1, ///< 选择第一个圆/线
        SetCircle2, ///< 选择第二个圆/线
        SetCircle3, ///< 选择第三个圆/线
        SetCenter   ///< 选择最近的公切圆圆心
    };

public:
    ActionDrawCircleTan3(DmDocument* doc, GuiDocumentView* docView);
    ~ActionDrawCircleTan3() override;

    void init(int status = 0) override;

    void trigger() override;
    bool getData();
    bool preparePreview();

    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

    QStringList getAvailableCommands() override;
    void finish(bool updateTB = true) override;
    void updateMouseButtonHints() override;
    void updateMouseCursor() override;

private:
    /// @brief 验证圆心是否有效
    std::vector<double> verifyCenter(const DmVector& center) const;
    /// @brief 获取实体到圆心的半径列表
    std::vector<double> getRadii(DmAtomicEntity* entity, const DmVector& center) const;
    DmEntity* catchCircle(QMouseEvent* e);

    struct Points;
    std::unique_ptr<Points> pPoints;
};

#endif
