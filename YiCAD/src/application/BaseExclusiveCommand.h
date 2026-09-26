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
#include "DmVector.h"
#include "ISnapService.h"

class DmDocument;
class DmEntity;
class ExclusiveCommandBus;
class ICommandHost;
class IDocumentView;
class QWidget;
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

    /// @brief 结束本命令并启动另一个命令
    /// @details 取代原 Action 里 finish() 之后 setCurrentAction(new ...) 的写法（如三点
    ///          圆弧在命令行切换为圆心圆弧）。按 5.1 节，本命令先被请求让位（Replaced）。
    /// @param commandId 另一个命令在 CommandRegistry 里的 ID；未注册时什么也不做
    void replaceWith(const QString& commandId);
    /// @brief 同上，新命令作用于 entity（在 point 处），如修改实体属性转到多行文字属性面板、
    ///        多行文字属性面板双击转到文字编辑（见 CommandContext）
    void replaceWith(const QString& commandId, DmEntity* entity, const DmVector& point = DmVector(false));

    /// @brief 弹出对话框时的父窗口：视图所在的顶层窗口（主窗口）
    /// @return 视图不是控件（测试用的假视图）时为空
    static QWidget* dialogParentOf(IDocumentView* view);

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
    /// @brief 弹出对话框时的父窗口，见 dialogParentOf()
    QWidget* dialogParent() const { return dialogParentOf(view()); }

private:
    ICommandHost* m_host = nullptr;
    bool m_active = false;
    QString m_commandId;
};

#endif // BASEEXCLUSIVECOMMAND_H
