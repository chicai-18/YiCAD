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


/// @file ActionDrawLineTangent2.h
/// @brief 两圆/椭圆公切线绘制交互动作

#ifndef ACTIONDRAWLINETANGENT2_H
#define ACTIONDRAWLINETANGENT2_H

#include "PreviewActionInterface.h"

class DmLine;
class LineData;

/// @brief 通过选择两个圆或椭圆来绘制公切线的交互动作类
class ActionDrawLineTangent2 : public PreviewActionInterface
{
    Q_OBJECT

private:
    enum Status
    {
        SetCircle1, ///< 选择第一个圆或椭圆
        SetCircle2  ///< 选择第二个圆或椭圆
    };

public:
    /// @brief 构造函数
    /// @param [in] doc 文档指针
    /// @param [in] docView 文档视图指针
    ActionDrawLineTangent2(DmDocument* doc, GuiDocumentView* docView);

    ~ActionDrawLineTangent2() override;

    /// @brief 执行绘制动作, 将切线添加到文档
    void trigger() override;

    /// @brief 处理鼠标移动事件, 实时预览切线
    /// @param [in] e 鼠标事件指针
    void mouseMoveEvent(QMouseEvent* e) override;

    /// @brief 处理鼠标释放事件, 选择圆或确认切线
    /// @param [in] e 鼠标事件指针
    void mouseReleaseEvent(QMouseEvent* e) override;

    /// @brief 更新鼠标按钮提示文本
    void updateMouseButtonHints() override;

    /// @brief 完成动作, 清除高亮和预览
    /// @param [in] updateTB 是否更新工具栏状态
    void finish(bool updateTB) override;

    /// @brief 更新鼠标光标样式
    void updateMouseCursor() override;

    /// @brief 根据给定参考点和两个圆/椭圆计算并创建公切线
    /// @param [in] coord 参考点坐标
    /// @param [in] circle1Entity 第一个圆/椭圆实体
    /// @param [in] circle2Entity 第二个圆/椭圆实体
    /// @return 创建的切线对象, 调用者负责内存管理; 无切线时返回 nullptr
    DmLine* createTangent2(const DmVector& coord,
                           DmEntity* circle1Entity,
                           DmEntity* circle2Entity);

private:
    /// @brief 清除所有高亮选中的实体
    void clearHighlighted();

    std::unique_ptr<DmLine> m_tangent;       ///< 最近的一条切线
    std::unique_ptr<LineData> m_lineData;    ///< 切线数据
    DmEntity* m_circle1;                     ///< 第一个被选中的实体
    DmEntity* m_circle2;                     ///< 第二个被选中的实体
    bool m_valid;                            ///< 当前切线是否有效
};

#endif // ACTIONDRAWLINETANGENT2_H
