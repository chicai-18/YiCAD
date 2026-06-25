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


/// @file ActionDrawLine.h
/// @brief 直线绘制交互动作，处理用户鼠标和键盘事件绘制直线
#ifndef ACTIONDRAWLINE_H
#define ACTIONDRAWLINE_H

#include "PreviewActionInterface.h"

/// @brief 直线绘制动作类
///
/// 处理用户鼠标事件绘制直线的交互动作类。
/// 支持起点和终点的设置，角度吸附（Shift 键），命令输入，以及撤销/重做操作。
class ActionDrawLine : public PreviewActionInterface
{
    Q_OBJECT

public:

    /// 动作状态枚举
    enum Status
    {
        SetStartpoint,  ///< 设置起点
        SetEndpoint     ///< 设置终点
    };

    /// 历史动作枚举
    enum HistoryAction
    {
        HA_SetStartpoint,  ///< 设置起点
        HA_SetEndpoint,    ///< 设置终点
        HA_Close,          ///< 闭合线段组
        HA_Next            ///< 开始新线段组
    };

public:

    /// @brief 构造函数
    /// @param doc CAD 文档指针
    /// @param docView 文档视图指针
    ActionDrawLine(DmDocument* doc, GuiDocumentView* docView);

    /// @brief 析构函数
    ~ActionDrawLine() override;

    /// @brief 重置绘制状态
    void reset();

    /// @brief 初始化动作
    /// @param status 初始状态（默认为 0）
    void init(const int status = 0) override;

    /// @brief 触发动作，创建直线实体
    void trigger() override;

    /// @brief 鼠标移动事件处理
    /// @param e 鼠标事件
    void mouseMoveEvent(QMouseEvent* e) override;

    /// @brief 鼠标释放事件处理
    /// @param e 鼠标事件
    void mouseReleaseEvent(QMouseEvent* e) override;

    /// @brief 坐标输入事件处理
    /// @param e 坐标事件
    void coordinateEvent(GuiCoordinateEvent* e) override;

    /// @brief 命令输入事件处理
    /// @param e 命令事件
    void commandEvent(GuiCommandEvent* e) override;

    /// @brief 获取可用命令列表
    /// @return 可用命令字符串列表
    QStringList getAvailableCommands() override;

    /// @brief 显示选项面板
    void showOptions() override;

    /// @brief 隐藏选项面板
    void hideOptions() override;

    /// @brief 更新鼠标按钮提示
    void updateMouseButtonHints() override;

    /// @brief 更新鼠标光标样式
    void updateMouseCursor() override;

    /// @brief 添加历史记录
    /// @param a 历史动作类型
    /// @param p 前一坐标点
    /// @param c 当前坐标点
    /// @param s 起点偏移量
    void addHistory(const ActionDrawLine::HistoryAction a, const DmVector& p, const DmVector& c, const int s);

    /// @brief 闭合当前线段组
    void close();

    /// @brief 开始下一线段组
    void next();

    /// @brief 撤销操作
    void undo();

    /// @brief 重做操作
    void redo();

private:

    /// @brief 添加直线段
    /// @param endPt 终点坐标
    void addLine(const DmVector& endPt);

protected:
    struct History;
    struct Points;
    std::unique_ptr<Points> pPoints; ///< 绘制点数据
};

#endif
