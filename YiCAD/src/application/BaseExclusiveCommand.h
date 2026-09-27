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

/// @file BaseExclusiveCommand.h
/// @brief 命令的通用基类，对应 DS 的 Application/BaseExclusiveCommand.h
///
/// 封装活动状态与所在视图（宿主 ICommandHost），提供请求结束（DS 的 RequestCancel）
/// 与宿主能力的取用。

#ifndef BASEEXCLUSIVECOMMAND_H
#define BASEEXCLUSIVECOMMAND_H

#include "IExclusiveCommand.h"

class DmDocument;
class ExclusiveCommandBus;
class HighlightSet;
class ICommandHost;
class IDocumentView;
class SelectionSet;
class ViewToolControl;

/// @brief 命令的通用基类
class BaseExclusiveCommand : public IExclusiveCommand
{
public:
    BaseExclusiveCommand() = default;
    ~BaseExclusiveCommand() override = default;

    BaseExclusiveCommand(const BaseExclusiveCommand&) = delete;
    BaseExclusiveCommand& operator=(const BaseExclusiveCommand&) = delete;

    const QString& commandId() const override { return m_commandId; }
    void setCommandId(const QString& commandId) override { m_commandId = commandId; }

    /// @brief 记下宿主后调用 onActivate()
    bool activate(ICommandHost& host) override;
    /// @brief 活动时调用 onDeactivate()；激活失败的命令不调用
    void deactivate() override;
    bool isActive() const override { return m_active; }

    /// @brief 请求结束本命令（提交或取消之后调用）
    /// @details 在自己工具的事件处理中调用时，总线在这次分发返回后才结束并销毁
    ///          命令，调用之后仍可安全访问成员（见 ExclusiveCommandBus::requestFinish）
    void finish();

    /// @brief 文档的选择集；只在活动期间有效。命令的放置工具经 BasePlaceTool::command() 取用，
    ///        所以公开（工具的文档与视图在构造时传入）
    SelectionSet* selection() const;
    /// @brief 视图的高亮集；只在活动期间有效，命令结束时由视图清空。公开的理由同 selection()
    HighlightSet* highlight() const;

protected:
    /// @brief 进入活动态时的命令逻辑：激活自己的工具、显示提示等
    /// @return false 表示启动失败；此时 onDeactivate() 不会被调用，需自行清理
    virtual bool onActivate() = 0;
    /// @brief 离开活动态时的命令逻辑：停用工具、清除预览等
    virtual void onDeactivate() = 0;

    /// @brief 所在视图；只在活动期间有效
    ICommandHost* host() const { return m_host; }
    /// @brief 所在视图的命令总线；只在活动期间有效
    ExclusiveCommandBus* bus() const;
    /// @brief 视图的文档
    DmDocument* document() const;
    /// @brief 视图
    IDocumentView* view() const;
    /// @brief 视图的工具控制器
    ViewToolControl* viewToolControl() const;

private:
    ICommandHost* m_host = nullptr;
    bool m_active = false;
    QString m_commandId;
};

#endif // BASEEXCLUSIVECOMMAND_H
