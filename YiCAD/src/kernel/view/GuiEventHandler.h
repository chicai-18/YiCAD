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

/// @file GuiEventHandler.h
/// @brief GUI 事件处理器，管理和分发所有活动操作的事件

#ifndef GUIEVENTHANDLER_H
#define GUIEVENTHANDLER_H

#include <QObject>

#include "DmVector.h"

class ActionInterface;
class QAction;
class QMouseEvent;
class QKeyEvent;
class GuiCommandEvent;
class DmVector;

struct SnapMode;

/// @brief 旧版 Action 栈之下的一层（过渡期接口，第四步随 GuiEventHandler 删除）
/// @details 原默认 Action 承担的三处切换：业务 Action 从空栈启动时挂起这一层、
///          栈空时恢复、killAllActions() 后复位。第一步起这一层是选择层；第二步
///          起命令（IExclusiveCommand）活动时也是它：旧 Action 叠在命令之上时
///          命令被挂起，旧 Action 全部结束后恢复。由交互视图 UIView 实现。
class ILegacyStackBase
{
public:
    virtual ~ILegacyStackBase() = default;

    /// @brief 旧 Action 从空栈启动，挂起这一层
    virtual void suspendForLegacy() = 0;
    /// @brief 旧 Action 栈清空，恢复这一层
    virtual void resumeAfterLegacy() = 0;
    /// @brief killAllActions() 之后复位这一层
    virtual void resetAfterKill() = 0;
};

/// @brief GUI 事件处理器
/// @details 拥有并管理所有当前活跃的旧版业务 Action。视图的事件经
///          LegacyActionTool 转到这里；没有业务 Action 时事件不经过本类，
///          直接由选择层 SelectTool 或命令的工具处理
///          （doc/COMMAND_TOOL_MIGRATION_PLAN.md 第一步、第二步）。
class GuiEventHandler : public QObject
{
    Q_OBJECT

public:
    /// @brief 构造事件处理器
    /// @param parent 父对象
    GuiEventHandler(QObject* parent = 0);
    ~GuiEventHandler();

    /// @brief 设置关联的 QAction
    void setQAction(QAction* action);

    /// @brief 后退操作
    void back();

    /// @brief 处理鼠标按下事件
    void mousePressEvent(QMouseEvent* e);
    /// @brief 处理鼠标释放事件
    void mouseReleaseEvent(QMouseEvent* e);
    /// @brief 处理鼠标移动事件
    void mouseMoveEvent(QMouseEvent* e);
    /// @brief 处理鼠标双击事件
    void mouseDoubleClickEvent(QMouseEvent* e);
    /// @brief 处理鼠标离开事件
    void mouseLeaveEvent();
    /// @brief 处理鼠标进入事件
    void mouseEnterEvent();

    /// @brief 处理键盘按下事件
    void keyPressEvent(QKeyEvent* e);
    /// @brief 处理键盘释放事件
    void keyReleaseEvent(QKeyEvent* e);

    /// @brief 处理命令事件
    void commandEvent(GuiCommandEvent* e);
    /// @brief 启用坐标输入
    void enableCoordinateInput();
    /// @brief 禁用坐标输入
    void disableCoordinateInput();
    /// @brief 命令行坐标输入是否启用（命令的工具也遵守这一开关）
    bool isCoordinateInputEnabled() const;

    /// @brief 设置旧 Action 栈之下的一层，由视图持有
    /// @param base 非持有，可为空（没有文档的视图）
    void setStackBase(ILegacyStackBase* base);

    /// @brief 即时命令执行前，按旧规则处理栈顶 Action
    /// @details 旧 Action 启动时，不可打断（canBeInterrupt() 为 false）的前一个
    ///          Action 被结束，可打断的被挂起再恢复；即时命令立即完成，后者等于
    ///          不变，只需结束前者（如多行文字属性编辑，否则它会继续编辑被删除的文字）。
    void interruptForInstantCommand();

    /// @brief 设置当前操作
    void setCurrentAction(ActionInterface* action);
    /// @brief 获取当前操作
    /// @return 栈顶业务 Action；没有业务 Action 时返回 nullptr
    ActionInterface* getCurrentAction();
    /// @brief 获取当前操作数量
    int getCurrentActionNum();
    /// @brief 检查操作是否有效
    bool isValid(ActionInterface* action) const;

    /// @brief 终止所有操作，并复位选择层
    void killAllActions();

    QList<ActionInterface*>& getCurrentActionsRef();

    /// @brief 检查是否有活动操作
    bool hasAction();
    /// @brief 清理已完成的操作（垃圾回收）
    void cleanUp();
    /// @brief 设置捕捉模式
    void setSnapMode(SnapMode sm);
    /// @brief 设置捕捉限制
    void setSnapRestriction(DM::SnapRestriction sr);

private:
    QAction*                m_pAction = nullptr;                    ///< 关联的 QAction
    ILegacyStackBase*       m_pStackBase = nullptr;                 ///< 旧 Action 栈之下的一层（非持有）
    QList<ActionInterface*> m_currentActions;                       ///< 当前操作栈
    bool                    m_isCoordinateInputEnabled = true;      ///< 是否启用坐标输入
    DmVector                m_relativeZero;                         ///< 相对零点

public slots:
    /// @brief 设置相对零点
    void setRelativeZero(const DmVector&);
};

#endif
