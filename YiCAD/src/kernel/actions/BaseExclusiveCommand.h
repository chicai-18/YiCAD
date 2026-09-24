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
/// 封装活动状态与所在总线，提供请求结束（DS 的 RequestCancel）、宿主能力的
/// 取用，以及进入、退出选择阶段的辅助方法（doc/COMMAND_TOOL_MIGRATION_PLAN.md
/// 第二步第 4 项）。

#ifndef BASEEXCLUSIVECOMMAND_H
#define BASEEXCLUSIVECOMMAND_H

#include "IExclusiveCommand.h"
#include "ISnapService.h"

class DmDocument;
class IDocumentView;
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

    /// @brief 记下总线后调用 onActivate()
    bool activate(ExclusiveCommandBus& bus) override;
    /// @brief 活动时调用 onDeactivate()；激活失败的命令不调用
    void deactivate() override;
    bool isActive() const override { return m_active; }

    /// @brief 请求结束本命令（提交或取消之后调用）
    /// @details 在自己工具的事件处理中调用时，总线在这次分发返回后才结束并销毁
    ///          命令，调用之后仍可安全访问成员（见 ExclusiveCommandBus::requestFinish）
    void finish();

protected:
    /// @brief 进入活动态时的命令逻辑：激活自己的工具、显示提示等
    /// @return false 表示启动失败；此时 onDeactivate() 不会被调用，需自行清理
    virtual bool onActivate() = 0;
    /// @brief 离开活动态时的命令逻辑：停用工具、清除预览等
    virtual void onDeactivate() = 0;

    /// @brief 所在总线；只在活动期间有效
    ExclusiveCommandBus* bus() const { return m_bus; }
    /// @brief 视图的文档
    DmDocument* document() const;
    /// @brief 视图
    IDocumentView* view() const;
    /// @brief 视图的工具控制器
    ViewToolControl* viewToolControl() const;

    /// @brief 进入选择阶段：选择层按先选后建的约束完成点选与框选
    /// @param entityTypes 可选的实体类型；为空表示不限
    /// @note 命令结束时总线保证清除约束，命令不必在 onDeactivate() 里退出
    void enterSelectionPhase(const EntityTypeList& entityTypes = {});
    /// @brief 退出选择阶段
    void leaveSelectionPhase();
    /// @brief 视图的选择层是否处于选择阶段
    bool inSelectionPhase() const;

private:
    ExclusiveCommandBus* m_bus = nullptr;
    bool m_active = false;
    QString m_commandId;
};

#endif // BASEEXCLUSIVECOMMAND_H
