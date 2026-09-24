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

/// @file IExclusiveCommand.h
/// @brief 交互命令接口，对应 DS 的 Application/IExclusiveCommand.h
///
/// 命令管生命周期（启动、预览、提交、结束），事件由它激活的放置工具
/// （IViewTool）处理。同一视图同一时刻最多一个活动命令，由
/// ExclusiveCommandBus 管理（doc/COMMAND_TOOL_MIGRATION_PLAN.md 第 1 节）。
///
/// 与 DS 的差异：
///   - activate() 拿到的是命令总线而不是 UIView：命令与工具只能经
///     IDocumentView 认识视图（tools/check_layering.py），总线向命令提供
///     文档、视图、工具控制器与选择层；
///   - 总线持有命令（CommandRegistry 每次启动都新建实例）；
///   - 增加 onEndRequested()：命令被外部结束前的回调（5.1 节）；
///   - 增加 suspend()/resume()：过渡期旧 Action 叠在命令之上时挂起命令。

#ifndef IEXCLUSIVECOMMAND_H
#define IEXCLUSIVECOMMAND_H

#include <QString>

class ExclusiveCommandBus;
class ISnapService;

/// @brief 命令被外部结束的原因（doc/COMMAND_TOOL_MIGRATION_PLAN.md 5.1 节）
enum class CommandEndReason
{
    Replaced,    ///< 启动了新命令
    Cancelled,   ///< 用户结束全部命令
    ViewClosing, ///< 视图或文档关闭
};

/// @brief 交互命令接口
/// @details 生命周期：activate() → 任一结束路径上由总线调用一次 deactivate()，
///          随后总线销毁命令。结束路径：命令自己请求结束、被新命令替换、
///          结束全部命令、视图关闭、激活失败。
class IExclusiveCommand
{
public:
    virtual ~IExclusiveCommand() = default;

    /// @brief 命令 ID（CommandRegistry 里的字符串 ID）
    virtual const QString& commandId() const = 0;
    /// @brief 记录命令 ID，只由 CommandRegistry::createCommand 调用
    virtual void setCommandId(const QString& commandId) = 0;

    /// @brief 进入活动态
    /// @param bus 所在视图的命令总线
    /// @return false 表示启动失败，总线随即调用 deactivate() 并销毁命令
    /// @note 允许在激活期间直接完成（如已有选择集时的分解）：命令请求结束后
    ///       返回 true，总线在激活返回后结束它
    virtual bool activate(ExclusiveCommandBus& bus) = 0;
    /// @brief 离开活动态，由总线在任一结束路径上调用
    virtual void deactivate() = 0;
    /// @brief 是否处于活动态
    virtual bool isActive() const = 0;

    /// @brief 命令被外部结束前调用，命令在此保存或放弃未提交的修改
    /// @param reason 结束原因
    /// @return false 表示否决：命令继续运行；ViewClosing 时返回值被忽略
    /// @note 可以弹模态对话框；回调期间总线忽略新的启动与结束请求
    virtual bool onEndRequested(CommandEndReason reason)
    {
        (void)reason;
        return true;
    }

    /// @brief 过渡期旧 Action 叠在命令之上时挂起：停用自己的工具，清除预览
    /// @note 第四步随旧 Action 体系删除
    virtual void suspend() {}
    /// @brief 叠在上面的旧 Action 全部结束后恢复
    virtual void resume() {}

    /// @brief 命令当前使用的捕捉器，画布的捕捉标记与捕捉提示读它
    /// @return 没有捕捉器时返回 nullptr
    virtual ISnapService* snapService() const { return nullptr; }
};

#endif // IEXCLUSIVECOMMAND_H
