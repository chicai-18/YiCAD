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
/// 现已注册为 `ViewToolControl` 的选择层（阶段2第6项 `LegacyActionTool`
/// 落地之后）。这带来两处必须的额外判断，都是"选择层现在要直接与导航层、
/// 业务层竞争优先级"的自然结果：
///   - `mousePressEvent` 的 `Neutral` 分支要在 Ctrl/Meta+左键时让路给
///     导航层（`PanZoomTool` 的平移手势）——这个判断在阶段2第一轮里被
///     挪到了 `GuiDocumentView`/`LegacyActionTool`，选择层若不重新声明，
///     会在导航层之前把这次按下错当成框选/点选的起点抢走。
///   - `mouseMoveEvent`/`mouseReleaseEvent` 在导航层正在平移
///     （`PanZoomTool::isPanning()`）时也要主动让路，否则会在平移的
///     移动/释放事件上抢在导航层结束这次平移之前把事件处理掉。
///
/// `getCursor()` 在"有其它业务 Action 正活动"（`docView->getEventHandler()
/// ->hasAction()`）时返回 `nullopt`：那些 Action 仍然通过
/// `updateMouseCursor()` 直接调用 `setMouseCursor()`（105 个未改造，见
/// 5.7 节"与方案的偏差"），选择层不应该用自己的偏好覆盖它们。
/// `setStatus()`/`init()` 仍然保留直接调用 `setMouseCursor()`——这是
/// `ActionBlocksEdit`/`ActionModifyMText` 通过 `getDefaultAction()`
/// 复用本类时唯一还在起作用的光标更新路径（那时 `hasAction()` 为真，
/// `getCursor()` 的仲裁通道按上一条规则保持沉默）。

#ifndef SELECTTOOL_H
#define SELECTTOOL_H

#include "DmVector.h"
#include "IViewTool.h"

class DmDocument;
class DmEntity;
class IDocumentView;
class ISnapService;
class Preview;
class PanZoomTool;
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
    /// @param panTool 非持有指针，可为空；用于查询导航层是否正在平移中，
    ///                 为空时视为"从不平移"（当前语义不变）
    SelectTool(DmDocument* doc, IDocumentView* docView, ISnapService* snapService, Preview* preview,
               PanZoomTool* panTool = nullptr);

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

    /// @brief 状态到光标的原始映射，不考虑是否有其它业务 Action 活动。
    /// `setStatus()`/`init()` 的直接调用用这个（不受 `getCursor()` 的
    /// `hasAction()` 让路判断影响，见头部说明）；`getCursor()` 在此基础上
    /// 叠加让路判断。
    std::optional<DM::CursorType> cursorForStatus() const;

    DmDocument* m_pDocument = nullptr;
    IDocumentView* m_docView = nullptr;
    ISnapService* m_snapService = nullptr;
    Preview* m_preview = nullptr;
    PanZoomTool* m_panTool = nullptr;

    int m_status = Neutral;
    bool m_hasPreview = false;
    Points m_points;
    DM::SnapRestriction m_restrictionBak = DM::RestrictNothing;
};

#endif // SELECTTOOL_H
