/****************************************************************************
**
 * Draw a tangential circle of two given circles, with given radius

Copyright (C) 2012-2015 Dongxu Li (dongxuli2011@gmail.com)

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


/// @file ActionDrawCircleTan2.h
/// @brief 绘制与两个给定圆相切且具有指定半径的圆的交互动作类

#ifndef ACTIONDRAWCIRCLETAN2_H
#define ACTIONDRAWCIRCLETAN2_H

#include "PreviewActionInterface.h"

class DmAtomicEntity;
struct DmCircleData;

/// @brief 绘制与两个给定圆/线相切且具有指定半径的圆
class ActionDrawCircleTan2 : public PreviewActionInterface
{
    Q_OBJECT
public:
    /// @brief 动作状态枚举
    enum Status
    {
        SetCircle1, ///< 选择第一个圆/线
        SetCircle2, ///< 选择第二个圆/线
        SetCenter   ///< 选择最近的相切圆圆心
    };

public:
    ActionDrawCircleTan2(DmDocument* doc, GuiDocumentView* docView);
    ~ActionDrawCircleTan2() override;

    void init(int status = 0) override;

    void trigger() override;
    bool getCenters();
    bool preparePreview() const;

    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

    QStringList getAvailableCommands() override;
    void finish(bool updateTB = true) override;
    void updateMouseButtonHints() override;
    void updateMouseCursor() override;

    void showOptions() override;
    void hideOptions() override;
    void setRadius(const double& r);
    double getRadius() const;

protected:
    DmEntity* catchCircle(QMouseEvent* e);

private:
    struct Points;
    std::unique_ptr<Points> pPoints;
};

#endif
