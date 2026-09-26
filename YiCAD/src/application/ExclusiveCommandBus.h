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
///   - 外部结束前先回调命令（5.1 节）：endCommand()/endAll() 先问，都同意才结束；
///     回调期间的启动与结束请求一律忽略；
///   - 以宿主 ICommandHost 构造（DS 是 UIView），命令激活时拿到的也是它；
///   - 持有编辑模式（块编辑，见 IEditMode.h）：模式的工具常驻在业务栈底部，
///     命令叠在它上面；启动命令不影响模式，结束全部命令与视图关闭时先问命令、
///     再问模式（endAll()）；
///   - 命令启停经 commandStarting()/commandFinished() 通知视图，视图据此让出、收回
///     空闲态的工具（夹点编辑工具、选择层）。DS 只有一个 signal_activeChanged，在激活
///     之后发；这里让出要早于激活，所以分成两个。

#ifndef EXCLUSIVECOMMANDBUS_H
#define EXCLUSIVECOMMANDBUS_H

#include <memory>
#include <vector>

#include <QObject>
#include <QString>

#include "IExclusiveCommand.h"

class ICommandHost;
class IEditMode;

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

    /// @param host 所在视图：命令激活时拿到它，编辑模式的工具经它的工具控制器常驻栈底
    explicit ExclusiveCommandBus(ICommandHost& host);
    /// @brief 析构时结束活动命令（不回调 onEndRequested()，照常发 commandFinished()）并退出编辑模式
    ~ExclusiveCommandBus() override;

    ExclusiveCommandBus(const ExclusiveCommandBus&) = delete;
    ExclusiveCommandBus& operator=(const ExclusiveCommandBus&) = delete;

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

    /// @brief 从外部结束活动命令：先征求它同意（5.1 节），同意后结束
    /// @param reason 结束原因；ViewClosing 忽略否决
    /// @return 已经没有活动命令时返回 true；被否决或处在回调中时返回 false，什么也不结束
    bool endCommand(CommandEndReason reason);

    /// @brief 结束全部：先征求活动命令、再征求编辑模式同意（5.1 节），都同意后结束命令并退出编辑模式
    /// @param reason Cancelled（结束全部命令）、Replaced（需要结束全部的即时命令，如新建、打开图纸）
    ///        或 ViewClosing（忽略否决）
    /// @return 都已结束时返回 true；被否决或处在回调中时返回 false，什么也不结束
    bool endAll(CommandEndReason reason);

    /// @brief 是否正在回调 onEndRequested()；期间的启动与结束请求一律忽略
    bool isInCallback() const { return m_inCallback; }

    // ---- 编辑模式（块编辑）----

    /// @brief 进入编辑模式：模式的工具常驻在业务栈底部
    /// @details 没有活动命令时立即恢复模式的界面；有活动命令时（如正在结束的编辑块
    ///          命令）等它结束时恢复。已有编辑模式时先退出它。
    /// @param mode 编辑模式，总线接管所有权
    void enterEditMode(std::unique_ptr<IEditMode> mode);
    /// @brief 当前编辑模式；没有时返回 nullptr
    IEditMode* editMode() const { return m_mode.get(); }
    /// @brief 模式请求退出自己（右键确认、选项条"完成"）；延迟规则同 requestFinish()
    void requestExitEditMode(IEditMode* mode);
    /// @brief 立即退出编辑模式，不征求同意（撤销/重做后文档已离开块编辑）
    /// @note 不能在模式自己的调用栈里调用，模式请求退出自己用 requestExitEditMode()
    void exitEditMode();

signals:
    /// @brief 命令即将激活：它已是活动命令，activate() 尚未调用
    /// @note 视图在这里让出空闲态的工具：激活的夹点要在命令改动选择集或实体之前取消
    void commandStarting();
    /// @brief 命令已结束：deactivate() 已调用，它已不是活动命令
    /// @note 视图在这里清除残留的选择阶段约束、恢复选择层、把夹点编辑工具放回业务栈
    void commandFinished();

private:
    /// @brief 回调活动命令的 onEndRequested()；没有活动命令时同意，ViewClosing 忽略否决
    bool approveCommand(CommandEndReason reason);
    /// @brief 回调编辑模式的 onEndRequested()；没有编辑模式时同意，ViewClosing 忽略否决
    bool approveMode(CommandEndReason reason);
    /// @brief 结束活动命令：deactivate()，发 commandFinished()，恢复编辑模式，销毁命令
    void finishActive();
    void enterScope();
    void leaveScope();

    ICommandHost& m_host;

    std::unique_ptr<IExclusiveCommand> m_active;
    std::unique_ptr<IEditMode> m_mode;
    /// @brief 分发范围内结束的命令，范围结束时销毁（它的工具可能还在调用栈上）
    std::vector<std::unique_ptr<IExclusiveCommand>> m_retired;
    /// @brief 分发范围内退出的编辑模式，范围结束时销毁
    std::vector<std::unique_ptr<IEditMode>> m_retiredModes;
    int m_scopeDepth = 0;
    bool m_finishPending = false;
    bool m_exitModePending = false;
    bool m_inCallback = false;
    /// @brief 每启动一个命令加一，定时器据此确认结束的还是同一个命令
    unsigned m_generation = 0;
};

#endif // EXCLUSIVECOMMANDBUS_H
