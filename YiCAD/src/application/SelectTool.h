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
/// @brief 选择工具：点选/框选/交叉选
///
/// 从原 `ActionDefault` 抽出（阶段2 第5.4节第3项），吸收其
/// `Neutral`/`Dragging`/`SetCorner2` 三个状态，不是 QObject，可以单独构造与单测。
/// 原先的 `MovingRef`（拖夹点）已拆到夹点编辑工具 `EditTool`（没有命令时在业务栈上），改为单击
/// 夹点激活；按在夹点上的按下归它，本类收不到。`Moving`（空闲态拖动整个实体）已取消，在选中
/// 实体上按住拖动与在空白处一样开始框选（doc/COMMAND_TOOL_MIGRATION_PLAN.md 9.6 节）。
///
/// 由交互视图 `UIView`（kernel/interaction/UIView.h）持有（连同捕捉器与预览容器），注册为
/// `ViewToolControl` 的选择层，业务工具不处理的事件落到本类
/// （doc/COMMAND_TOOL_MIGRATION_PLAN.md 第一步）。与导航层竞争优先级的三处让路：
///   - 中键按下：平移属于导航层（`PanZoomTool`）；
///   - `Neutral` 状态下的 Ctrl/Meta+左键：导航层的平移手势；
///   - 导航层平移中（`PanZoomTool::isPanning()`）的移动与释放。
///
/// "之上有什么在活动"由视图经 `setOverlayQuery()` 告知，见 `Overlay`：
///   - 命令（`IExclusiveCommand`）：`getCursor()` 返回 `nullopt`、按键提示也不更新，
///     光标与提示归命令的工具，选择阶段除外（见下）；
///   - 块编辑模式（`IEditMode`）：本类照常完成块内的选择并给出光标，只是不更新
///     按键提示，提示归编辑模式。
/// `setStatus()`/`init()` 始终直接调用 `setMouseCursor()`。
///
/// 选择阶段（doc/COMMAND_TOOL_MIGRATION_PLAN.md 第二步第 4 项）：先选后建
/// 命令没有选择集时，由本类完成点选与框选，行为复刻原 `ActionSelectMultiple`：
/// 按实体类型过滤、不拖夹点也不拖实体、Ctrl+左键不让给平移、选中后只刷新
/// 选择计数而不发 `selectedChanged`、提示与光标取原 `ActionSelectMultiple` 的。
/// 约束由命令经宿主（ICommandHost）设置，命令结束时由视图保证清除。

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
    /// @brief 内部状态，语义与原 `ActionDefault::Status` 的前三个一致
    enum Status
    {
        Neutral,    ///< 初始状态
        Dragging,   ///< 左键已按下，尚未判定是点选还是框选
        SetCorner2  ///< 设置选择窗口的第二个角点
    };

    /// @brief 选择层之上正在活动的业务，决定本类是否更新提示、是否给出光标
    enum class Overlay
    {
        None,        ///< 空闲：提示与光标都由本类给出
        EditMode,    ///< 编辑模式（块编辑）：提示归编辑模式，光标仍由本类给出
        Command      ///< 命令活动：提示与光标归命令；命令的选择阶段仍由本类给出
    };

    /// @brief 查询选择层之上正在活动的业务
    using OverlayQuery = std::function<Overlay()>;

    /// @brief 启动一个交互命令，作用于 entity（在 point 处）；由视图装配时设置
    /// @return 命令启动成功时返回 true
    using CommandStarter = std::function<bool(const QString& commandId, DmEntity* entity, const DmVector& point)>;

    /// @brief 选择阶段的约束
    struct SelectionPhase
    {
        EntityTypeList entityTypes; ///< 可选的实体类型；为空表示不限
    };

    /// @param doc 文档指针
    /// @param docView 文档视图指针
    /// @param snapService 非持有指针，由视图持有；空闲态的捕捉提示也读它
    /// @param preview 非持有指针，由视图持有的预览容器；本类不往里画，只在选择完成、
    ///                取消与挂起时清除它（与原 `ActionDefault` 相同）
    /// @param panTool 非持有指针，可为空；用于查询导航层是否正在平移中，
    ///                 为空时视为"从不平移"
    SelectTool(DmDocument* doc, IDocumentView* docView, ISnapService* snapService, Preview* preview,
               PanZoomTool* panTool = nullptr);

    /// @brief 复位到 Neutral：清除预览与捕捉点，重新初始化捕捉器
    /// @note 结束全部命令（`UIView::killAllActions()`）时调用
    void init();

    /// @brief 挂起：清除预览与捕捉点
    /// @note 命令启动、或空闲态下鼠标离开画布时调用
    void suspend();
    /// @brief 恢复：刷新按键提示，重绘预览与捕捉点
    /// @note 命令结束回到空闲态、或空闲态下鼠标回到画布时调用
    void resume();

    /// @brief 单点拾取：切换画布坐标处最近实体的选中状态，并刷新选择计数
    /// @param guiX 画布像素 X 坐标
    /// @param guiY 画布像素 Y 坐标
    /// @return 拾取到的实体；未命中返回 nullptr
    /// @note 手写板橡皮擦用，取代原 `ActionSelectSingle`
    DmEntity* pickAt(int guiX, int guiY);

    /// @brief 设置"选择层之上有什么在活动"的查询，由视图装配时设置
    /// @param query 为空时视为空闲（Overlay::None）
    void setOverlayQuery(OverlayQuery query);

    /// @brief 设置启动命令的方式：双击实体时按 CommandRegistry::entityEditor（没有时
    ///        CommandRegistry::propertyEditor）找到的交互命令经它启动（如多行文字的就地编辑）；
    ///        找到的是即时命令（属性对话框）时直接运行
    void setCommandStarter(CommandStarter starter) { m_commandStarter = std::move(starter); }

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

    /// @brief 鼠标进入画布时恢复；有命令时（选择阶段除外）由命令的工具自己恢复
    void enterEvent() override;
    /// @brief 鼠标离开画布时挂起；有命令时（选择阶段除外）由命令的工具自己挂起
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

    /// @brief 选择完成后的通知：空闲态发 selectedChanged；选择阶段只刷新选择计数，
    ///        与原 ActionSelectMultiple 一致（发信号会启动多行文字属性编辑，顶掉当前命令）
    void notifySelectionChanged();

    /// @brief 状态到光标的原始映射，不考虑选择层之上是否有命令活动。
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

    OverlayQuery m_overlayQuery;                 ///< 为空时视为空闲
    CommandStarter m_commandStarter;             ///< 为空时双击不启动编辑命令
    std::optional<SelectionPhase> m_phase;       ///< 有值表示处于选择阶段
};

#endif // SELECTTOOL_H
