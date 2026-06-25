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


/// @file ActionDrawSpline.h
/// @brief 样条曲线绘制交互动作类，处理用户鼠标和键盘事件绘制样条曲线

#ifndef ACTIONDRAWSPLINE_H
#define ACTIONDRAWSPLINE_H

#include "PreviewActionInterface.h"

struct DmSplineData;
class DmSpline;

/// @brief 处理样条曲线绘制的用户交互动作
///
/// 该类继承自 PreviewActionInterface，管理样条曲线的控制点输入、
/// 预览渲染、撤销操作以及闭合/阶数设置。
class ActionDrawSpline : public PreviewActionInterface
{
    Q_OBJECT

    ///< 交互状态枚举
    enum Status
    {
        SetStartpoint, ///< 设置起始点
        SetNextPoint   ///< 设置下一个控制点
    };

public:
    /// @brief 构造函数
    /// @param [in] doc 文档对象指针
    /// @param [in] docView 文档视图指针
    ActionDrawSpline(DmDocument* doc, GuiDocumentView* docView);

    /// @brief 析构函数
    ~ActionDrawSpline() override;

    /// @brief 重置交互状态，清除当前样条线和历史控制点
    void reset();

    /// @brief 初始化交互状态
    /// @param [in] status 初始状态，默认为 0
    void init(int status = 0) override;

    /// @brief 提交当前样条线到文档
    void trigger() override;

    /// @brief 处理鼠标移动事件
    /// @param [in] e 鼠标事件指针
    void mouseMoveEvent(QMouseEvent* e) override;

    /// @brief 处理鼠标释放事件
    /// @param [in] e 鼠标事件指针
    void mouseReleaseEvent(QMouseEvent* e) override;

    /// @brief 处理坐标输入事件
    /// @param [in] e 坐标事件指针
    void coordinateEvent(GuiCoordinateEvent* e) override;

    /// @brief 处理命令输入事件
    /// @param [in] e 命令事件指针
    void commandEvent(GuiCommandEvent* e) override;

    /// @brief 获取当前状态下可用的命令列表
    /// @return 可用命令字符串列表
    QStringList getAvailableCommands() override;

    /// @brief 显示选项面板
    void showOptions() override;

    /// @brief 隐藏选项面板
    void hideOptions() override;

    /// @brief 更新鼠标按键提示
    void updateMouseButtonHints() override;

    /// @brief 更新鼠标光标样式
    void updateMouseCursor() override;

    /// @brief 撤销最后一个控制点
    virtual void undo();

    /// @brief 设置样条曲线的阶数
    /// @param [in] deg 阶数值
    virtual void setDegree(int deg);

    /// @brief 获取样条曲线的阶数
    /// @return 当前阶数
    int getDegree();

    /// @brief 设置样条曲线是否闭合
    /// @param [in] c 是否闭合
    virtual void setClosed(bool c);

    /// @brief 查询样条曲线是否闭合
    /// @return 是否闭合
    virtual bool isClosed();

    /// @brief 根据闭合状态及动态点设置样条曲线控制点和节点
    /// @param [in,out] spline 要设置的样条曲线对象
    /// @param [in] isClosed 是否闭合
    /// @param [in] dynamicPt 动态控制点，默认为无效点
    void setControlPointsKnotsByClose(
        DmSpline* spline,
        bool isClosed,
        DmVector dynamicPt = DmVector(false));

protected:
    struct Points;

    std::unique_ptr<Points> pPoints; ///< 样条曲线数据（PIMPL 模式）
};

#endif
