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

/// @file ExclusiveCommandBus.h
/// @brief 视图作用域的命令总线，对应 DS 的 Application/ExclusiveCommandBus.h
///
/// 每个交互视图 UIView 一个。同一时刻最多一个活动命令；启动新命令前先请当前
/// 命令让位。与 DS 的差异（doc/COMMAND_TOOL_MIGRATION_PLAN.md 第二步第 1 项）：
///   - 总线持有命令：CommandRegistry 每次启动都新建实例，结束后由总线销毁；
///   - 命令在自己工具的事件处理中请求结束时，延迟到这次分发返回后再结束
///     （DS 的 ViewCommandManager::QueueFinishExclusive 用 0 毫秒定时器；这里
///     由视图用 DispatchScope 标出分发的范围，分发返回即结束，不经事件循环，
///     结束时机确定。分发之外的请求，如选项条按钮，仍经 0 毫秒定时器）；
///   - 外部结束前先回调命令（5.1 节）：approveEnd() 只问不改，调用方全部征得
///     同意后再 end()；回调期间的启动与结束请求一律忽略；
///   - 总线就是命令的宿主：命令经它拿到文档、视图、工具控制器与选择层
///     （命令不能认识 UIView，见 IExclusiveCommand.h），并由它保证命令结束时
///     清除选择阶段的约束；
///   - 过渡期旧 Action 叠在命令之上时，经 suspend()/resume() 挂起、恢复命令。
///
/// 视图工具（平移、缩放）不占总线，直接叠在业务栈顶。

#ifndef EXCLUSIVECOMMANDBUS_H
#define EXCLUSIVECOMMANDBUS_H

#include <memory>
#include <vector>

#include <QObject>
#include <QString>

#include "Datamodel.h"
#include "IExclusiveCommand.h"

class DmDocument;
class IDocumentView;
class SelectTool;
class ViewToolControl;
struct SnapMode;

/// @brief 视图作用域的命令总线
/// @note 仅限 UI 主线程访问
class ExclusiveCommandBus : public QObject
{
    Q_OBJECT

public:
    /// @brief 标出一次事件分发的范围：范围内请求的结束延迟到范围结束
    /// @details 可以嵌套（分发中弹出模态对话框时的内层事件循环），最外层结束时
    ///          才结束命令、销毁已结束的命令。
    class DispatchScope
    {
    public:
        /// @param bus 可为空，此时什么也不做
        explicit DispatchScope(ExclusiveCommandBus* bus);
        ~DispatchScope();
        DispatchScope(const DispatchScope&) = delete;
        DispatchScope& operator=(const DispatchScope&) = delete;

    private:
        ExclusiveCommandBus* m_bus;
    };

    /// @param doc 视图的文档
    /// @param view 视图
    /// @param tools 视图的工具控制器，命令在其业务栈上激活自己的工具
    /// @param selectTool 视图的选择层，可为空；选择阶段的约束设在它上面
    ExclusiveCommandBus(DmDocument* doc, IDocumentView* view, ViewToolControl* tools, SelectTool* selectTool);
    /// @brief 析构时结束活动命令，不回调 onEndRequested()
    ~ExclusiveCommandBus() override;

    ExclusiveCommandBus(const ExclusiveCommandBus&) = delete;
    ExclusiveCommandBus& operator=(const ExclusiveCommandBus&) = delete;

    // ---- 命令的宿主能力 ----

    DmDocument* document() const { return m_document; }
    IDocumentView* view() const { return m_view; }
    ViewToolControl* viewToolControl() const { return m_tools; }
    /// @brief 选择层；可为空
    SelectTool* selectTool() const { return m_selectTool; }

    // ---- 命令生命周期 ----

    /// @brief 启动命令
    /// @details 有活动命令时先按 5.1 节请它让位（Replaced），被否决时丢弃新命令；
    ///          否则结束它，再激活新命令。激活失败时结束新命令。
    /// @param command 新命令，总线接管所有权
    /// @return 新命令已激活（包括激活期间就已完成的）时返回 true
    bool start(std::unique_ptr<IExclusiveCommand> command);

    /// @brief 活动命令；没有时返回 nullptr
    IExclusiveCommand* activeCommand() const { return m_active.get(); }
    /// @brief 是否有活动命令
    bool hasActiveCommand() const { return m_active != nullptr; }
    /// @brief 活动命令的 ID；没有时返回空串
    QString activeCommandId() const;

    /// @brief 命令请求结束自己（提交或取消之后）
    /// @details 分发范围内延迟到范围结束；范围外经 0 毫秒定时器，避免在命令
    ///          自己的调用栈里销毁它。不是活动命令时忽略。
    void requestFinish(IExclusiveCommand* command);

    /// @brief 外部结束前征求活动命令同意（5.1 节），不改变任何状态
    /// @param reason 结束原因
    /// @return 没有活动命令或命令同意时返回 true；ViewClosing 忽略否决；
    ///         回调期间的重入请求返回 false
    bool approveEnd(CommandEndReason reason);
    /// @brief 结束活动命令，不再征求同意（调用方已经 approveEnd()）
    void end();

    /// @brief 是否正在回调 onEndRequested()；期间的启动与结束请求一律忽略
    bool isInCallback() const { return m_inCallback; }

    // ---- 过渡期：旧 Action 叠在命令之上 ----

    /// @brief 挂起活动命令（旧 Action 从空栈启动）
    void suspend();
    /// @brief 恢复活动命令（旧 Action 栈清空）
    void resume();
    /// @brief 活动命令是否被挂起
    bool isSuspended() const { return m_suspended; }

    // ---- 捕捉设置同步 ----

    /// @brief 视图的默认捕捉模式变化时同步给活动命令的捕捉器
    void setSnapMode(const SnapMode& snapMode);
    /// @brief 视图的捕捉限制变化时同步给活动命令的捕捉器
    void setSnapRestriction(DM::SnapRestriction restriction);

private:
    /// @brief 结束活动命令：deactivate()，清除选择阶段约束，恢复选择层，销毁命令
    void finishActive();
    void enterScope();
    void leaveScope();

    DmDocument* m_document = nullptr;
    IDocumentView* m_view = nullptr;
    ViewToolControl* m_tools = nullptr;
    SelectTool* m_selectTool = nullptr;

    std::unique_ptr<IExclusiveCommand> m_active;
    /// @brief 分发范围内结束的命令，范围结束时销毁（它的工具可能还在调用栈上）
    std::vector<std::unique_ptr<IExclusiveCommand>> m_retired;
    int m_scopeDepth = 0;
    bool m_finishPending = false;
    bool m_inCallback = false;
    bool m_suspended = false;
    /// @brief 每启动一个命令加一，定时器据此确认结束的还是同一个命令
    unsigned m_generation = 0;
};

#endif // EXCLUSIVECOMMANDBUS_H
