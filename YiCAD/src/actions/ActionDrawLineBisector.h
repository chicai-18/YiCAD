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


/// @file ActionDrawLineBisector.h
/// @brief 角平分线绘制操作类，处理用户鼠标事件以选择两条直线并生成其角平分线

#ifndef ACTIONDRAWLINEBISECTOR_H
#define ACTIONDRAWLINEBISECTOR_H

#include "PreviewActionInterface.h"

class DmLine;

/// @brief 角平分线绘制操作类，处理用户事件以绘制角平分线。
/// 支持选择两条直线，计算其交点并生成指定长度和数量的角平分线。
class ActionDrawLineBisector : public PreviewActionInterface
{
    Q_OBJECT

private:
    enum Status
    {
        SetLine1,  ///< 选择第一条线
        SetLine2,  ///< 选择第二条线
        SetLength, ///< 在命令行中设置长度
        SetNumber  ///< 在命令行中设置数量
    };

public:
    /// @brief 构造函数
    /// @param [in] doc 文档指针
    /// @param [in] docView 文档视图指针
    ActionDrawLineBisector(DmDocument* doc, GuiDocumentView* docView);

    /// @brief 析构函数
    ~ActionDrawLineBisector() override;

    /// @brief 初始化操作状态
    /// @param [in] status 初始状态值，默认为0
    void init(int status = 0) override;

    /// @brief 执行绘制操作，生成角平分线并添加到文档
    void trigger() override;

    /// @brief 处理鼠标移动事件
    /// @param [in] e 鼠标事件指针
    void mouseMoveEvent(QMouseEvent* e) override;

    /// @brief 处理鼠标释放事件
    /// @param [in] e 鼠标事件指针
    void mouseReleaseEvent(QMouseEvent* e) override;

    /// @brief 处理命令行事件
    /// @param [in] e 命令事件指针
    void commandEvent(GuiCommandEvent* e) override;

    /// @brief 获取可用命令列表
    /// @return 可用命令的字符串列表
    QStringList getAvailableCommands() override;

    /// @brief 隐藏选项界面
    void hideOptions() override;

    /// @brief 显示选项界面
    void showOptions() override;

    /// @brief 更新鼠标按钮提示
    void updateMouseButtonHints() override;

    /// @brief 更新鼠标光标样式
    void updateMouseCursor() override;

    /// @brief 设置角平分线长度
    /// @param [in] l 长度值
    void setLength(double l);

    /// @brief 获取角平分线长度
    /// @return 当前长度值
    double getLength() const;

    /// @brief 设置角平分线数量
    /// @param [in] n 数量值
    void setNumber(int n);

    /// @brief 获取角平分线数量
    /// @return 当前数量值
    int getNumber() const;

private:
    /// @brief 创建角平分线实体
    /// @param [in] container 实体容器
    /// @param [in] coord1 第一条线的鼠标坐标
    /// @param [in] coord2 第二条线的鼠标坐标
    /// @param [in] length 角平分线长度
    /// @param [in] num 角平分线数量
    /// @param [in] l1 第一条线
    /// @param [in] l2 第二条线
    /// @return 生成的角平分线列表
    std::vector<DmLine*> createBisector(DmEntityContainer* container,
                                        const DmVector& coord1,
                                        const DmVector& coord2,
                                        double length,
                                        int num,
                                        DmLine* l1,
                                        DmLine* l2);

private:
    DmLine* bisector = nullptr;     ///< 最近的角平分线
    DmLine* line1 = nullptr;        ///< 第一条选中的线
    DmLine* line2 = nullptr;        ///< 第二条选中的线
    double length = 10.0;           ///< 角平分线的长度
    int number = 1;                 ///< 要创建的角平分线数量
    struct Points;
    std::unique_ptr<Points> pPoints; ///< 鼠标选择点数据
    Status lastStatus = SetLine1;   ///< 进入长度或数量设置前的上一个状态
};

#endif
