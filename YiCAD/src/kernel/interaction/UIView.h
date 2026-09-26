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

/// @file UIView.h
/// @brief 交互视图：在画布 GuiDocumentView 之上装配交互层
///
/// 对应 DS 的 UIView（DimX/Source/View/UIView.h）。DS 把渲染控件 HQWidget 与
/// 交互组装 UIView 拆成基类与派生类，本类沿用这一拆法：GuiDocumentView 是
/// 渲染层（YiCadRender）的画布，不认识交互层的具体类型；本类持有
/// ViewToolControl 与导航（PanZoomTool）、选择（SelectTool）、业务（命令的工具、
/// 编辑模式，以及没有命令时的夹点编辑工具 EditTool）三层工具，以及命令总线
/// ExclusiveCommandBus，接收画布的 Qt 输入事件交给 ViewToolControl 分发
/// （doc/COMMAND_TOOL_MIGRATION_PLAN.md）。夹点编辑工具与选择层是空闲态的工具：
/// 总线通知命令即将启动、已经结束时，由本类让出、收回它们（DS 的 SyncEditActivation）。
///
/// 放在 kernel/interaction/（YiCadInteraction 分区），不和画布同在
/// kernel/view/（YiCadRender 分区）：一个目录归一个分区，渲染层不能依赖交互层。
/// DS 把 HQWidget 与 UIView 同放 View/，是因为它不按目录分库。
///
/// 命令与工具只能经 IDocumentView/GuiDocumentView 认识视图，不能反过来依赖
/// 本类：内核禁止包含 UI* 头文件（tools/check_layering.py），白名单只放行
/// UIView.cpp 包含自身头文件。这一点与 DS 不同：DS 的 EditTool、
/// ExclusiveCommandBus 以 UIView* 构造，命令激活时也拿到 UIView*；这里本类实现
/// ICommandHost，命令与总线只认识接口。
///
/// 启动命令时先按 5.1 节请当前命令让位；即时命令不碰命令总线，执行前按
/// InstantInterrupt 处理正在运行的命令（见 prepareInstantCommand()）。
///
/// 编辑模式（块编辑，IEditMode）也由命令总线持有：启动命令不影响它；结束全部命令、
/// 需要结束全部的即时命令与视图关闭时先问命令、再问模式（ExclusiveCommandBus::approveEndAll）。

#ifndef UIVIEW_H
#define UIVIEW_H

#include <memory>

#include "CommandRegistry.h"
#include "GuiDocumentView.h"
#include "ICommandHost.h"

class EditTool;
class ExclusiveCommandBus;
class IExclusiveCommand;
class ISnapService;
class PanZoomTool;
class Preview;
class SelectTool;
class Snapper;
class ViewToolControl;

/// @brief 交互视图：画布加交互层工具栈与命令总线
class UIView : public GuiDocumentView, public ICommandHost
{
    Q_OBJECT

public:
    /// @param parent 父控件
    /// @param fl 窗口标志
    /// @param doc 关联的文档；为空时不建选择层与命令总线
    UIView(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags(), DmDocument* doc = nullptr);
    ~UIView() override;

    /// @brief 启动交互命令（UIActionHandler 按注册类型分派到这里）
    /// @details 先按 5.1 节请当前命令让位（被否决时丢弃新命令），再交给命令总线激活。
    /// @param command 新命令，视图接管所有权
    /// @return 新命令已激活（包括激活期间就已完成的）时返回 true
    bool startCommand(std::unique_ptr<IExclusiveCommand> command);

    /// @brief 即时命令执行前调用
    /// @param interrupt EndUninterruptible 时结束不可打断的命令
    ///        （IExclusiveCommand::isUninterruptible）；KeepAll 时什么也不做；
    ///        EndAll 时先征求命令与编辑模式同意，再结束全部命令，并复位选择层
    ///        （原排他 Action 启动时的做法）
    /// @return 可以执行时返回 true；EndAll 被否决或处在 5.1 节的回调中时返回 false
    bool prepareInstantCommand(InstantInterrupt interrupt = InstantInterrupt::EndUninterruptible);

    /// @brief 命令总线；没有文档时为空
    ExclusiveCommandBus* commandBus() override { return m_pCommandBus.get(); }

    /// @brief 活动命令的 ID（命令总线上的）；没有时返回空串
    QString activeCommandId() const override;

    /// @brief 主窗口转交的按键，经 ViewToolControl 分发
    /// @return 某一层工具处理了该事件时返回 true
    bool processKeyEvent(QKeyEvent* e) override;

    /// @brief 相当于右键：交给命令的工具或编辑模式
    void back() override;
    /// @brief 命令行输入：解析坐标，经 ViewToolControl 交给命令的工具或编辑模式
    void commandEvent(GuiCommandEvent* e) override;
    /// @brief 按 5.1 节先征求命令、再征求编辑模式同意（Cancelled），被否决时什么也不做；
    ///        否则结束全部命令，并复位选择层
    bool killAllActions() override;
    /// @brief 视图关闭：回调命令与编辑模式（ViewClosing，不能否决）后结束全部
    void killAllActionsOnClose() override;
    /// @brief 有活动命令或处于编辑模式
    bool hasActiveCommand() const override;

    /// @brief 设置默认捕捉模式，并同步给选择层与活动命令的捕捉器
    void setDefaultSnapMode(SnapMode sm) override;
    /// @brief 设置捕捉限制，并同步给选择层与活动命令的捕捉器
    void setSnapRestriction(DM::SnapRestriction sr) override;

protected:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void tabletEvent(QTabletEvent* e) override;
    void leaveEvent(QEvent* e) override;
    void enterEvent(QEnterEvent* e) override;
    void focusInEvent(QFocusEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;

    /// @brief 有命令时取命令的捕捉器，空闲态取选择层的
    SnapResultType currentSnapResult() override;
    /// @brief 与 currentSnapResult() 取自同一个捕捉器
    DmVector currentSnapSpot() override;

private:
    // ---- ICommandHost：命令与总线经接口调用 ----
    DmDocument* document() override { return getDocument(); }
    IDocumentView* view() override { return this; }
    ViewToolControl* viewToolControl() override { return m_pViewToolControl.get(); }
    void beginSelectionPhase(const EntityTypeList& entityTypes) override;
    void endSelectionPhase() override;

    /// @brief 命令即将激活：夹点编辑工具移出业务栈，挂起选择层
    void onCommandStarting();
    /// @brief 命令已结束：清除残留的选择阶段约束，恢复选择层，夹点编辑工具放回业务栈顶
    void onCommandFinished();

    /// @brief 活动命令的捕捉器：不在选择阶段且有捕捉器时返回它
    ISnapService* commandSnapService() const;

    /// @brief 右键释放（含 back() 合成的）：经 ViewToolControl 交给命令的工具或编辑模式
    void routeBack(QMouseEvent* e);

    /// @brief 命令总线上有活动命令或编辑模式
    bool hasBusinessOnBus() const;

    /// @brief 结束全部命令后复位夹点编辑工具与选择层（原先由旧 Action 栈的 killAllActions() 完成）
    void resetIdleTools();

    // 注意声明顺序：成员按声明的逆序析构。m_pCommandBus 最后声明、最先析构
    // （析构函数里还会提前显式释放），结束活动命令时它的工具、选择层与
    // ViewToolControl 都还在；m_pViewToolControl 随后析构，向各层工具发
    // onDeactivate()，被它引用的工具此时都还在；工具之间的裸指针（SelectTool
    // 与 EditTool 引用 PanZoomTool、捕捉器与预览容器）也按被引用者在前排列。全部成员都在
    // 基类析构之前析构，基类持有的预览容器这时仍然有效。
    std::unique_ptr<PanZoomTool>            m_pPanZoomTool;         ///< 导航层：中键/Ctrl+左键平移
    std::unique_ptr<Snapper>                m_pSelectSnapper;       ///< 选择层与夹点编辑工具的捕捉器，空闲态的捕捉提示也读它
    std::unique_ptr<Preview>                m_pSelectPreview;       ///< 夹点编辑工具移动夹点时的预览容器，选择层选择完成时清除它
    std::unique_ptr<SelectTool>             m_pSelectTool;          ///< 选择层；没有文档时为空
    std::unique_ptr<EditTool>               m_pEditTool;            ///< 夹点编辑工具，没有命令时在业务栈上；没有文档时为空
    std::unique_ptr<ViewToolControl>        m_pViewToolControl;     ///< 交互层工具控制器
    std::unique_ptr<ExclusiveCommandBus>    m_pCommandBus;          ///< 命令总线；没有文档时为空
};

#endif // UIVIEW_H
