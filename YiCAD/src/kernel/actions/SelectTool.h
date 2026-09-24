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
/// 从原 `ActionDefault` 抽出（阶段2 第5.4节第3项），吸收其
/// `Neutral`/`Dragging`/`SetCorner2`/`Moving`/`MovingRef` 五个状态，
/// 脱离 Action 体系（`ActionInterface`/`QObject`）单独构造与单测。
///
/// 未把 `Moving`/`MovingRef` 拆成独立的 `GripEditTool`：三者共享同一次
/// 拖拽手势，鼠标刚按下时还不知道最终是框选还是拖动实体/夹点，要等
/// `Dragging` 状态下移动超过阈值后才能判定，拆开需要在两个类之间转移
/// 这次"未决"的拖拽状态。
///
/// 由交互视图 `UIView`（kernel/interaction/UIView.h）持有（连同捕捉器与预览容器），注册为
/// `ViewToolControl` 的选择层。没有旧版业务 Action 活动时，`LegacyActionTool`
/// 整体让路，空闲态事件直接落到本类（doc/COMMAND_TOOL_MIGRATION_PLAN.md
/// 第一步）。与导航层竞争优先级的三处让路：
///   - 中键按下：平移属于导航层（`PanZoomTool`）；
///   - `Neutral` 状态下的 Ctrl/Meta+左键：导航层的平移手势；
///   - 导航层平移中（`PanZoomTool::isPanning()`）的移动与释放。
///
/// 有旧版业务 Action 活动时，本类只收到该 Action 经
/// `ActionInterface::passesToSelection()` 交下来的事件（块编辑、多行文字
/// 属性编辑时的双击）。此时 `getCursor()` 返回 `nullopt`、按键提示也不
/// 更新：光标与提示归那个 Action 管（它仍通过 `updateMouseCursor()`/
/// `updateMouseButtonHints()` 直接设置，见主计划 5.7 节）。`setStatus()`/
/// `init()` 仍直接调用 `setMouseCursor()`，块编辑中拖动实体时的光标反馈
/// 靠这条路径。命令（`IExclusiveCommand`）活动时同理，"之上有什么在活动"
/// 由视图经 `setOverlayQuery()` 告知，见 `Overlay`。
///
/// 选择阶段（doc/COMMAND_TOOL_MIGRATION_PLAN.md 第二步第 4 项）：先选后建
/// 命令没有选择集时，由本类完成点选与框选，行为复刻原 `ActionSelectMultiple`：
/// 按实体类型过滤、不拖夹点也不拖实体、Ctrl+左键不让给平移、选中后只刷新
/// 选择计数而不发 `selectedChanged`、提示与光标取原 `ActionSelectMultiple` 的。
/// 约束由命令设置，命令结束时由命令总线保证清除。

#ifndef SELECTTOOL_H
#define SELECTTOOL_H

#include <functional>
#include <optional>

#include <QCoreApplication>

#include "DmVector.h"
#include "ISnapService.h"
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
    Q_DECLARE_TR_FUNCTIONS(SelectTool)

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

    /// @brief 选择层之上正在活动的业务，决定本类是否更新提示、是否给出光标
    enum class Overlay
    {
        None,        ///< 空闲：提示与光标都由本类给出
        EditMode,    ///< 编辑模式（块编辑）：提示归编辑模式，光标仍由本类给出
        Command,     ///< 命令活动：提示与光标归命令；命令的选择阶段仍由本类给出
        LegacyAction ///< 旧版业务 Action 活动（叠在命令之上时也是它）：提示与光标都归它
    };

    /// @brief 查询选择层之上正在活动的业务
    using OverlayQuery = std::function<Overlay()>;

    /// @brief 选择阶段的约束
    struct SelectionPhase
    {
        EntityTypeList entityTypes; ///< 可选的实体类型；为空表示不限
    };

    /// @param doc 文档指针
    /// @param docView 文档视图指针
    /// @param snapService 非持有指针，由视图持有；空闲态的捕捉提示也读它
    /// @param preview 非持有指针，由视图持有的预览容器
    /// @param panTool 非持有指针，可为空；用于查询导航层是否正在平移中，
    ///                 为空时视为"从不平移"
    SelectTool(DmDocument* doc, IDocumentView* docView, ISnapService* snapService, Preview* preview,
               PanZoomTool* panTool = nullptr);

    /// @brief 复位到 Neutral：清除预览与捕捉点，重新初始化捕捉器
    /// @note 结束全部命令（`GuiEventHandler::killAllActions()`）时调用
    void init();

    /// @brief 挂起：清除预览与捕捉点
    /// @note 旧版业务 Action 从空闲态启动、或空闲态下鼠标离开画布时调用
    void suspend();
    /// @brief 恢复：刷新按键提示，重绘预览与捕捉点
    /// @note 回到空闲态、或空闲态下鼠标回到画布时调用
    void resume();

    /// @brief 单点拾取：切换画布坐标处最近实体的选中状态，并刷新选择计数
    /// @param guiX 画布像素 X 坐标
    /// @param guiY 画布像素 Y 坐标
    /// @return 拾取到的实体；未命中返回 nullptr
    /// @note 手写板橡皮擦用，取代原 `ActionSelectSingle`
    DmEntity* pickAt(int guiX, int guiY);

    /// @brief 设置"选择层之上有什么在活动"的查询，由视图装配时设置
    /// @param query 为空时退回默认：视图的旧版 Action 栈有 Action 即 LegacyAction
    void setOverlayQuery(OverlayQuery query);

    /// @brief 进入选择阶段：复位到 Neutral，按约束工作
    /// @param phase 约束
    void beginSelectionPhase(const SelectionPhase& phase);
    /// @brief 退出选择阶段：清除约束，复位到 Neutral
    void endSelectionPhase();
    /// @brief 是否处于选择阶段
    bool inSelectionPhase() const { return m_phase.has_value(); }

    ViewToolResult mousePressEvent(QMouseEvent* e) override;
    ViewToolResult mouseReleaseEvent(QMouseEvent* e) override;
    ViewToolResult mouseMoveEvent(QMouseEvent* e) override;
    ViewToolResult mouseDoubleClickEvent(QMouseEvent* e) override;
    ViewToolResult keyPressEvent(QKeyEvent* e) override;
    ViewToolResult keyReleaseEvent(QKeyEvent* e) override;

    /// @brief 鼠标进入画布时恢复；有业务 Action 或命令时由它们自己恢复
    void enterEvent() override;
    /// @brief 鼠标离开画布时挂起；有业务 Action 或命令时由它们自己挂起
    void leaveEvent() override;

    std::optional<DM::CursorType> getCursor() const override;

    void onActivate() override;
    void onDeactivate() override;

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

    /// @brief 更新鼠标按钮提示；选择层之上有业务或编辑模式时不更新，见头部说明
    void updateButtonHints() const;

    /// @brief 选择层之上正在活动的业务，见 setOverlayQuery()
    Overlay overlay() const;

    /// @brief 处于选择阶段且没有旧版 Action 叠在命令之上：提示与光标由选择阶段给出
    bool phaseOwnsInput() const;

    /// @brief 选择完成后的通知：空闲态发 selectedChanged；选择阶段只刷新选择计数，
    ///        与原 ActionSelectMultiple 一致（发信号会启动多行文字属性编辑，顶掉当前命令）
    void notifySelectionChanged();

    /// @brief 状态到光标的原始映射，不考虑是否有业务 Action 活动。
    /// `setStatus()`/`init()` 的直接调用用这个；`getCursor()` 在此基础上
    /// 叠加让路判断，见头部说明。
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

    OverlayQuery m_overlayQuery;                 ///< 为空时按旧版 Action 栈判断
    std::optional<SelectionPhase> m_phase;       ///< 有值表示处于选择阶段
};

#endif // SELECTTOOL_H
