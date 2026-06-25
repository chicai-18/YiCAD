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

/// @brief GUI 事件处理器
/// @details 拥有并管理所有当前活跃的操作。所有从视图到操作的事件都通过此类传递。
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
    /// @brief 前进/确认操作
    void enter();

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

    /// @brief 设置默认操作
    void setDefaultAction(ActionInterface* action);
    /// @brief 获取默认操作
    ActionInterface* getDefaultAction() const;

    /// @brief 设置当前操作
    void setCurrentAction(ActionInterface* action);
    /// @brief 获取当前操作
    ActionInterface* getCurrentAction();
    /// @brief 获取当前操作数量
    int getCurrentActionNum();
    /// @brief 检查操作是否有效
    bool isValid(ActionInterface* action) const;

    /// @brief 终止选择类操作
    void killSelectActions();
    /// @brief 终止所有操作
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

    /// @return true 表示当前操作为选择模式
    bool inSelectionMode();

private:
    QAction*                m_pAction = nullptr;                    ///< 关联的 QAction
    ActionInterface*        m_pDefaultAction = nullptr;             ///< 默认操作
    QList<ActionInterface*> m_currentActions;                       ///< 当前操作栈
    bool                    m_isCoordinateInputEnabled = true;      ///< 是否启用坐标输入
    DmVector                m_relativeZero;                         ///< 相对零点

public slots:
    /// @brief 设置相对零点
    void setRelativeZero(const DmVector&);
};

#endif
