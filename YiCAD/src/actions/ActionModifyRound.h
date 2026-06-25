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


/// @file ActionModifyRound.h
/// @brief 圆角修改操作——处理用户鼠标事件以实现圆角功能

#ifndef ACTIONMODIFYROUND_H
#define ACTIONMODIFYROUND_H

#include "PreviewActionInterface.h"

/// @brief 圆角修改操作类，处理用户鼠标事件实现圆角功能
class ActionModifyRound : public PreviewActionInterface
{
    Q_OBJECT

public:
    /// @brief 圆角操作状态枚举
    enum Status
    {
        SetEntity1, ///< 选择第一个实体
        SetEntity2, ///< 选择第二个实体
        SetRadius,  ///< 在命令行中设置半径
        SetTrim     ///< 在命令行中设置裁剪标志
    };

public:
    /// @brief 构造函数
    /// @param [in] doc 文档指针
    /// @param [in] docView 文档视图指针
    ActionModifyRound(DmDocument* doc, GuiDocumentView* docView);

    /// @brief 析构函数
    ~ActionModifyRound() override;

    /// @brief 初始化操作状态，清除捕捉限制
    /// @param [in] status 初始状态值
    void init(int status = 0) override;

    /// @brief 执行圆角操作
    void trigger() override;

    /// @brief 计算鼠标在实体上的对应点
    /// @param [in] m_p 鼠标位置
    /// @param [in] e 实体指针
    /// @return 实体上最近点的坐标
    DmVector setmousePoint(const DmVector& m_p, DmEntity* e);

    /// @brief 处理鼠标移动事件，实时预览圆角效果
    /// @param [in] e 鼠标事件指针
    void mouseMoveEvent(QMouseEvent* e) override;

    /// @brief 处理鼠标释放事件，确认选择或回退操作
    /// @param [in] e 鼠标事件指针
    void mouseReleaseEvent(QMouseEvent* e) override;

    /// @brief 处理命令事件，支持 radius/trim 子命令
    /// @param [in] e 命令事件指针
    void commandEvent(GuiCommandEvent* e) override;

    /// @brief 获取当前状态可用的命令列表
    /// @return 可用命令列表
    QStringList getAvailableCommands() override;

    /// @brief 隐藏选项面板
    void hideOptions() override;

    /// @brief 显示选项面板
    void showOptions() override;

    /// @brief 更新鼠标按钮提示信息
    void updateMouseButtonHints() override;

    /// @brief 更新鼠标光标为选择光标
    void updateMouseCursor() override;

    /// @brief 设置圆角半径
    /// @param [in] r 半径值
    void setRadius(double r);

    /// @brief 获取圆角半径
    /// @return 当前半径值
    double getRadius() const;

    /// @brief 设置裁剪标志
    /// @param [in] t 是否裁剪
    void setTrim(bool t);

    /// @brief 获取裁剪标志
    /// @return 是否启用裁剪
    bool isTrimOn() const;

    /// @brief 禁用拷贝构造
    ActionModifyRound(const ActionModifyRound&) = delete;

    /// @brief 禁用拷贝赋值
    ActionModifyRound& operator=(const ActionModifyRound&) = delete;

private:
    /// @brief 圆角计算结果
    struct FilletResult
    {
        bool valid = false;             ///< 计算是否成功
        DmVector center;                ///< 圆角圆心
        DmVector tangent1;              ///< entity1 上的切点
        DmVector tangent2;              ///< entity2 上的切点
        double startAngle = 0.0;        ///< 圆弧起始角度
        double endAngle = 0.0;          ///< 圆弧终止角度
    };

    /// @brief 计算圆角几何信息
    FilletResult computeFillet(const DmVector& ref1, DmAtomicEntity* e1,
                               const DmVector& ref2, DmAtomicEntity* e2,
                               double radius) const;

    DmEntity* entity1 = nullptr;    ///< 第一个选中实体
    DmEntity* entity2 = nullptr;    ///< 第二个选中实体
    struct Points;
    std::unique_ptr<Points> pPoints; ///< 圆角操作数据点
    /// @brief 进入角度输入前的状态
    Status lastStatus = SetEntity1;
    bool isEndPt = false;           ///< 鼠标是否在实体端点附近
    DmEntity* prevHighlighted = nullptr; ///< 上次高亮的实体

    /// @brief 取消当前高亮实体
    void unhighlightEntity();
};

#endif
