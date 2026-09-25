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
/// ViewToolControl 与导航（PanZoomTool）、选择（SelectTool）、业务
/// （命令的工具与编辑模式）三层工具，以及命令总线 ExclusiveCommandBus，接收
/// 画布的 Qt 输入事件交给 ViewToolControl 分发（doc/COMMAND_TOOL_MIGRATION_PLAN.md）。
///
/// 放在 kernel/interaction/（YiCadInteraction 分区），不和画布同在
/// kernel/view/（YiCadRender 分区）：一个目录归一个分区，渲染层不能依赖交互层。
/// DS 把 HQWidget 与 UIView 同放 View/，是因为它不按目录分库。
///
/// 命令与工具只能经 IDocumentView/GuiDocumentView 认识视图，不能反过来依赖
/// 本类：内核禁止包含 UI* 头文件（tools/check_layering.py），白名单只放行
/// UIView.cpp 包含自身头文件。这一点与 DS 不同：DS 的 EditTool、
/// ExclusiveCommandBus 以 UIView* 构造；这里命令经总线拿到宿主能力。
///
/// 启动命令时先按 5.1 节请当前命令让位；即时命令不碰命令总线，执行前按
/// InstantInterrupt 处理正在运行的命令（见 prepareInstantCommand()）。
///
/// 临时视图工具（平移模式，TransientViewTool）也由本类持有：它不占命令总线，
/// 叠在业务栈顶；启动时挂起其下各层（命令或编辑模式、选择层），结束时恢复
/// （迁移计划第三步）。启动命令、结束全部命令与视图关闭时结束它。
///
/// 编辑模式（块编辑，IEditMode）也由命令总线持有：启动命令不影响它；结束全部命令、
/// 需要结束全部的即时命令与视图关闭时先问命令、再问模式（ExclusiveCommandBus::approveEndAll）。

#ifndef UIVIEW_H
#define UIVIEW_H

#include <memory>

#include "CommandRegistry.h"
#include "GuiDocumentView.h"

class ExclusiveCommandBus;
class IExclusiveCommand;
class ISnapService;
class PanZoomTool;
class Preview;
class SelectTool;
class Snapper;
class TransientViewTool;
class ViewToolControl;

/// @brief 交互视图：画布加交互层工具栈与命令总线
class UIView : public GuiDocumentView
{
    Q_OBJECT

public:
    /// @param parent 父控件
    /// @param fl 窗口标志
    /// @param doc 关联的文档；为空时不建选择层与命令总线
    UIView(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags(), DmDocument* doc = nullptr);
    ~UIView() override;

    /// @brief 启动交互命令（UIActionHandler 按注册类型分派到这里）
    /// @details 先按 5.1 节请当前命令让位（被否决时丢弃新命令），再结束平移模式，
    ///          最后交给命令总线激活。
    /// @param command 新命令，视图接管所有权
    /// @return 新命令已激活（包括激活期间就已完成的）时返回 true
    bool startCommand(std::unique_ptr<IExclusiveCommand> command);

    /// @brief 即时命令执行前调用
    /// @param interrupt EndUninterruptible 时结束不可打断的命令
    ///        （IExclusiveCommand::isUninterruptible）；KeepAll 时什么也不做；
    ///        EndAll 时先征求命令与编辑模式同意，再结束全部命令与平移模式，并复位选择层
    ///        （原排他 Action 启动时的做法）
    /// @return 可以执行时返回 true；EndAll 被否决或处在 5.1 节的回调中时返回 false
    bool prepareInstantCommand(InstantInterrupt interrupt = InstantInterrupt::EndUninterruptible);

    /// @brief 启动临时视图工具（平移模式）：结束已有的，挂起其下各层，叠在业务栈顶
    /// @param tool 新工具，视图接管所有权
    /// @return 没有文档或处于结束前回调期间时返回 false，工具被丢弃
    bool startViewTool(std::unique_ptr<TransientViewTool> tool);
    /// @brief 结束临时视图工具并恢复其下各层；没有时什么也不做
    void endViewTool();
    /// @brief 当前的临时视图工具；没有时返回 nullptr
    TransientViewTool* viewTool() const { return m_pViewTool.get(); }

    /// @brief 命令总线；没有文档时为空
    ExclusiveCommandBus* commandBus() const { return m_pCommandBus.get(); }

    /// @brief 活动命令的 ID：有临时视图工具时是它的，否则是命令总线上的；没有时返回空串
    QString activeCommandId() const override;

    /// @brief 主窗口转交的按键，经 ViewToolControl 分发
    /// @return 某一层工具处理了该事件时返回 true
    bool processKeyEvent(QKeyEvent* e) override;

    /// @brief 相当于右键：交给平移模式、命令的工具或编辑模式
    void back() override;
    /// @brief 命令行输入：解析坐标，经 ViewToolControl 交给平移模式或命令的工具
    void commandEvent(GuiCommandEvent* e) override;
    /// @brief 按 5.1 节先征求命令、再征求编辑模式同意（Cancelled），被否决时什么也不做；
    ///        否则结束全部命令与平移模式，并复位选择层
    bool killAllActions() override;
    /// @brief 视图关闭：回调命令与编辑模式（ViewClosing，不能否决）后结束全部
    void killAllActionsOnClose() override;
    /// @brief 有活动命令、处于编辑模式或有临时视图工具
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
    void enterEvent(QEvent* e) override;
    void focusInEvent(QFocusEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;

    /// @brief 有命令时取命令的捕捉器，空闲态取选择层的
    SnapResultType currentSnapResult() override;
    /// @brief 与 currentSnapResult() 取自同一个捕捉器
    DmVector currentSnapSpot() override;

private:
    /// @brief 活动命令的捕捉器：命令未挂起、不在选择阶段且有捕捉器时返回它
    ISnapService* commandSnapService() const;

    /// @brief 右键释放（含 back() 合成的）：经 ViewToolControl 交给平移模式、命令的工具
    ///        或编辑模式
    void routeBack(QMouseEvent* e);

    /// @brief 命令总线上有活动命令或编辑模式
    bool hasBusinessOnBus() const;

    /// @brief 临时视图工具启动时挂起其下各层：命令（或编辑模式）与选择层
    void suspendUnderViewTool();
    /// @brief 临时视图工具结束时恢复其下各层
    void resumeUnderViewTool();
    /// @brief 结束全部命令后复位选择层（原先由旧 Action 栈的 killAllActions() 完成）
    void resetSelectTool();

    // 注意声明顺序：成员按声明的逆序析构。m_pCommandBus 最后声明、最先析构
    // （析构函数里还会提前显式释放），结束活动命令时它的工具、选择层与
    // ViewToolControl 都还在；m_pViewToolControl 随后析构，向各层工具发
    // onDeactivate()，被它引用的工具此时都还在；工具之间的裸指针（SelectTool
    // 引用 PanZoomTool、捕捉器与预览容器）也按被引用者在前排列。全部成员都在
    // 基类析构之前析构，基类持有的预览容器这时仍然有效。
    std::unique_ptr<PanZoomTool>            m_pPanZoomTool;         ///< 导航层：中键/Ctrl+左键平移
    std::unique_ptr<Snapper>                m_pSelectSnapper;       ///< 选择层的捕捉器，空闲态的捕捉提示也读它
    std::unique_ptr<Preview>                m_pSelectPreview;       ///< 选择层拖动实体时的预览容器
    std::unique_ptr<SelectTool>             m_pSelectTool;          ///< 选择层；没有文档时为空
    std::unique_ptr<TransientViewTool>      m_pViewTool;            ///< 临时视图工具（平移模式）；没有时为空
    std::unique_ptr<ViewToolControl>        m_pViewToolControl;     ///< 交互层工具控制器
    std::unique_ptr<ExclusiveCommandBus>    m_pCommandBus;          ///< 命令总线；没有文档时为空
    unsigned                                m_viewToolGeneration = 0; ///< 每启动一个临时视图工具加一
};

#endif // UIVIEW_H
