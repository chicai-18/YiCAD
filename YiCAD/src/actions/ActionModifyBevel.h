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


/// @file ActionModifyBevel.h
/// @brief 倒角修改 Action 类，处理用户事件以倒角实体

#ifndef ACTIONMODIFYBEVEL_H
#define ACTIONMODIFYBEVEL_H

#include <memory>

#include "PreviewActionInterface.h"

/// @brief 处理倒角修改的用户交互 Action
class ActionModifyBevel : public PreviewActionInterface
{
    Q_OBJECT

    /// @brief Action 状态枚举
    enum Status
    {
        SetEntity1, ///< 选择第一个实体
        SetEntity2, ///< 选择第二个实体
        SetLength1, ///< 在命令行中设置长度1
        SetLength2  ///< 在命令行中设置长度2
    };

public:
    /// @brief 构造函数
    /// @param doc 文档指针
    /// @param docView 文档视图指针
    ActionModifyBevel(DmDocument* doc, GuiDocumentView* docView);
    ~ActionModifyBevel() override;

    void init(int status) override;
    void trigger() override;

    /// @brief 计算鼠标在实体上的对应点
    /// @param m_p 鼠标位置
    /// @param e 实体指针
    /// @return 实体上最近点的坐标
    DmVector setmousePoint(const DmVector& m_p, DmEntity* e);

    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

    void commandEvent(GuiCommandEvent* e) override;
    QStringList getAvailableCommands() override;

    void hideOptions() override;
    void showOptions() override;

    void updateMouseButtonHints() override;
    void updateMouseCursor() override;

    /// @brief 设置倒角长度1
    /// @param l1 长度1
    void setLength1(double l1);

    /// @brief 获取倒角长度1
    /// @return 长度1
    double getLength1() const;

    /// @brief 设置倒角长度2
    /// @param l2 长度2
    void setLength2(double l2);

    /// @brief 获取倒角长度2
    /// @return 长度2
    double getLength2() const;

    /// @brief 设置是否修剪
    /// @param t 是否修剪
    void setTrim(bool t);

    /// @brief 获取是否修剪
    /// @return 是否修剪
    bool isTrimOn() const;

private:
    /// @brief 倒角计算结果
    struct BevelResult
    {
        bool valid = false;
        DmVector point1;       ///< entity1 上的倒角点（距交点 length1）
        DmVector point2;       ///< entity2 上的倒角点（距交点 length2）
        DmVector intersection; ///< 两实体交点
    };

    /// @brief 计算倒角几何信息
    BevelResult computeBevel(const DmVector& ref1, DmAtomicEntity* e1,
                             const DmVector& ref2, DmAtomicEntity* e2,
                             double l1, double l2) const;

    /// @brief 取消当前高亮实体
    void unhighlightEntity();

    DmEntity* entity1 = nullptr;        ///< 第一个选中实体
    DmEntity* entity2 = nullptr;        ///< 第二个选中实体
    struct Points;
    std::unique_ptr<Points> pPoints;    ///< 倒角计算所需的坐标和数据
    Status lastStatus = SetEntity1;     ///< 进入长度设置前的上一个状态
    bool isEndPt = false;               ///< 鼠标是否在实体端点附近
    DmEntity* prevHighlighted = nullptr; ///< 上次高亮的实体
};

#endif
