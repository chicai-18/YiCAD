/****************************************************************************
**
 * This action class can handle user events to draw tangents normal to lines

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


/// @file ActionDrawLineOrthTan.h
/// @brief 绘制与直线正交的切线交互动作类

#ifndef ACTIONDRAWLINEORTHTAN_H
#define ACTIONDRAWLINEORTHTAN_H

#include "PreviewActionInterface.h"

class DmLine;

/// @brief 处理用户事件以绘制与直线正交的切线
class ActionDrawLineOrthTan : public PreviewActionInterface
{
    Q_OBJECT
private:
    enum Status
    {
        SetLine,  ///< 选择与切线正交的直线
        SetCircle ///< 选择圆弧/圆/椭圆以创建其切线
    };

public:
    /// @brief 构造函数
    /// @param doc 文档指针
    /// @param docView 文档视图指针
    ActionDrawLineOrthTan(DmDocument* doc, GuiDocumentView* docView);

    /// @brief 触发当前动作
    void trigger() override;

    /// @brief 完成动作，清理状态
    /// @param updateTB 是否更新工具栏
    void finish(bool updateTB = true) override;

    /// @brief 处理鼠标移动事件
    /// @param e 鼠标事件指针
    void mouseMoveEvent(QMouseEvent* e) override;

    /// @brief 处理鼠标释放事件
    /// @param e 鼠标事件指针
    void mouseReleaseEvent(QMouseEvent* e) override;

    /// @brief 更新鼠标按钮提示文本
    void updateMouseButtonHints() override;

    /// @brief 更新鼠标光标样式
    void updateMouseCursor() override;

    /// @brief 创建一条与给定法线正交的切线
    /// @param coord 参考坐标点
    /// @param normal 法线实体指针
    /// @param circle 圆弧/圆/椭圆实体指针
    /// @return 创建的切线，失败返回 nullptr
    DmLine* createLineOrthTan(const DmVector& coord, DmLine* normal, DmEntity* circle);

private:
    /// @brief 清除选中的直线和预览
    void clearLines();

    DmLine* normal = nullptr;  ///< 选中的法线
    DmLine* tangent = nullptr; ///< 用于预览的切线
    DmEntity* circle = nullptr; ///< 用于生成切线的圆弧/圆/椭圆
};

#endif
