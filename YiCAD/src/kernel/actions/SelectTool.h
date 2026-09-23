/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/// @file SelectTool.h
/// @brief 选择工具：点选/框选/交叉选，以及拖拽实体与夹点
///
/// 从 `ActionDefault` 抽出（阶段2 第5.4节第3项），吸收其原本的
/// `Neutral`/`Dragging`/`SetCorner2`/`Moving`/`MovingRef` 五个状态。
/// `ActionDefault` 现在是本类的一个薄适配器：真正的状态机搬到这里，
/// 得以脱离 Action 体系（`ActionInterface`/`QObject`）单独构造与单测。
///
/// 未把 `Moving`/`MovingRef` 拆成方案里提到的独立 `GripEditTool`——
/// 三者共享同一次拖拽手势：鼠标刚按下时还不知道最终是框选还是拖动
/// 实体/夹点，要等 `Dragging` 状态下移动超过阈值后才能判定。拆成两个
/// 类需要在它们之间转移这次"未决"的拖拽状态，边界不清晰而收益有限，
/// 遂保留在同一个类里；方案本身也把这个拆分标注为"评估后决定"。
///
/// 尚未注册进 `ViewToolControl` 的选择层：那需要先完成方案第6项——把
/// `GuiEventHandler` 的业务 Action 栈包成业务工具参与统一分发。当前
/// `GuiEventHandler` 对业务 Action 的分发语义是"只要活着就总是处理"
/// （没有 NotHandled 的概念），若现在就把本类注册为选择层，会在导航层
/// 之前抢先"处理"每个事件，业务 Action（画线、修改等）将再也收不到
/// 鼠标事件。见 doc/ARCHITECTURE_EVOLUTION_PLAN.md 阶段2 5.7 节。

#ifndef SELECTTOOL_H
#define SELECTTOOL_H

#include "DmVector.h"
#include "IViewTool.h"

class DmDocument;
class DmEntity;
class IDocumentView;
class ISnapService;
class Preview;
class QKeyEvent;
class QMouseEvent;

class SelectTool : public IViewTool
{
public:
    /// @brief 内部状态，语义与原 `ActionDefault::Status` 完全一致
    enum Status
    {
        Neutral,    ///< 初始状态
        Dragging,   ///< 拖拽中（实体或选择窗口）
        SetCorner2, ///< 设置选择窗口的第二个角点
        Moving,     ///< 移动实体
        MovingRef   ///< 移动选中实体的参考点
    };

    /// @param doc 文档指针
    /// @param docView 文档视图指针
    /// @param snapService 非持有指针，与拥有者（`ActionDefault`）共享
    ///                     同一个捕捉器实例，保证捕捉模式/捕捉结果一致
    /// @param preview 非持有指针，与拥有者共享同一个预览容器
    SelectTool(DmDocument* doc, IDocumentView* docView, ISnapService* snapService, Preview* preview);

    /// @brief 复位到 Neutral，供 `ActionDefault::init()` 调用
    void init();

    ViewToolResult mousePressEvent(QMouseEvent* e) override;
    ViewToolResult mouseReleaseEvent(QMouseEvent* e) override;
    ViewToolResult mouseMoveEvent(QMouseEvent* e) override;
    ViewToolResult mouseDoubleClickEvent(QMouseEvent* e) override;
    ViewToolResult keyPressEvent(QKeyEvent* e) override;
    ViewToolResult keyReleaseEvent(QKeyEvent* e) override;

    std::optional<DM::CursorType> getCursor() const override;

    void onActivate() override;
    void onDeactivate() override;

    /// @brief 更新鼠标按钮提示文本，供 `ActionDefault::updateMouseButtonHints()` 调用
    void updateButtonHints() const;

    int getStatus() const { return m_status; }

private:
    struct Points
    {
        DmVector v1;
        DmVector v2;
    };

    void setStatus(int status);
    void deletePreview();
    void drawPreview();

    DmDocument* m_pDocument = nullptr;
    IDocumentView* m_docView = nullptr;
    ISnapService* m_snapService = nullptr;
    Preview* m_preview = nullptr;

    int m_status = Neutral;
    bool m_hasPreview = false;
    Points m_points;
    DM::SnapRestriction m_restrictionBak = DM::RestrictNothing;
};

#endif // SELECTTOOL_H
