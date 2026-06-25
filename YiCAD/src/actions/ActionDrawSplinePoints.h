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


/// @file ActionDrawSplinePoints.h
/// @brief 通过指定控制点绘制样条曲线的交互动作类

#ifndef ACTIONDRAWSPLINEPOINTS_H
#define ACTIONDRAWSPLINEPOINTS_H

#include "ActionDrawSpline.h"
#include "PreviewActionInterface.h"
#include "DmSpline.h"

/// @brief 通过指定控制点绘制样条曲线的交互动作类
class ActionDrawSplinePoints : public PreviewActionInterface
{
    Q_OBJECT
public:
    /// @brief 动作状态枚举
    enum Status
    {
        SetStartPoint, ///< 设置起始点
        SetNextPoint   ///< 设置下一个点
    };

    /// @brief 样条曲线控制点数据结构
    struct Points
    {
        SplineData data;                          ///< 已定义的样条曲线数据
        std::unique_ptr<DmSpline> spline;         ///< 使用的样条曲线实例
        std::vector<DmVector> undoBuffer;         ///< 点历史记录（用于撤消操作）
    };

    /// @brief 构造函数
    /// @param [in] doc 文档指针
    /// @param [in] docView 文档视图指针
    ActionDrawSplinePoints(DmDocument* doc, GuiDocumentView* docView);

    /// @brief 析构函数
    ~ActionDrawSplinePoints() override;

    /// @brief 重置动作状态
    void reset();

    /// @brief 初始化动作
    /// @param [in] status 初始状态值
    void init(int status = 0) override;

    /// @brief 触发动作执行，将样条曲线提交到文档
    void trigger() override;

    /// @brief 鼠标移动事件处理
    /// @param [in] e 鼠标事件指针
    void mouseMoveEvent(QMouseEvent* e) override;

    /// @brief 鼠标释放事件处理
    /// @param [in] e 鼠标事件指针
    void mouseReleaseEvent(QMouseEvent* e) override;

    /// @brief 坐标输入事件处理
    /// @param [in] e 坐标事件指针
    void coordinateEvent(GuiCoordinateEvent* e) override;

    /// @brief 命令行事件处理
    /// @param [in] e 命令事件指针
    void commandEvent(GuiCommandEvent* e) override;

    /// @brief 获取当前状态下可用的命令列表
    /// @return 可用命令字符串列表
    QStringList getAvailableCommands() override;

    /// @brief 显示选项面板
    void showOptions() override;

    /// @brief 隐藏选项面板
    void hideOptions() override;

    /// @brief 更新鼠标按钮提示文本
    void updateMouseButtonHints() override;

    /// @brief 更新鼠标光标样式
    void updateMouseCursor() override;

    /// @brief 撤消上一个控制点
    virtual void undo();

    /// @brief 设置样条曲线是否闭合
    /// @param [in] c 是否闭合
    virtual void setClosed(bool c);

    /// @brief 获取样条曲线是否闭合
    /// @return 闭合状态
    virtual bool isClosed();

    /// @brief 根据控制点拟合样条曲线
    /// @param [in,out] spline 待拟合的样条曲线对象
    /// @param [in] dynamicPt 动态点坐标（用于鼠标移动时的实时预览）
    void fitPoints(DmSpline* spline,
                   DmVector dynamicPt = DmVector(false));

private:
    std::unique_ptr<Points> pPoints; ///< 样条曲线控制点数据
};

#endif
