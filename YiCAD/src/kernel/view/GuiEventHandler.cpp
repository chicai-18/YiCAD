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

/// @file GuiEventHandler.cpp
/// @brief GUI 事件处理器实现

#include "GuiEventHandler.h"

#include <QRegExp>
#include <QAction>
#include <QMouseEvent>

#include "ActionInterface.h"
#include "GuiDialogFactory.h"
#include "GuiCommandEvent.h"
#include "GuiCoordinateEvent.h"
#include "GuiCoordinateInput.h"
#include "Debug.h"

GuiEventHandler::GuiEventHandler(QObject* parent) : QObject(parent)
{
    connect(parent, SIGNAL(relative_zero_changed(const DmVector&)), this, SLOT(setRelativeZero(const DmVector&)));
}

GuiEventHandler::~GuiEventHandler()
{
    for (auto a : m_currentActions)
    {
        delete a;
    }
    m_currentActions.clear();
}

/// @brief 在当前操作中后退
void GuiEventHandler::back()
{
    QMouseEvent e(QEvent::MouseButtonRelease, QPoint(0, 0), Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    mouseReleaseEvent(&e);
    if (!hasAction() && m_pAction)
    {
        m_pAction->setChecked(false);
        m_pAction = nullptr;
    }
}

/// @brief 处理鼠标按下事件（由 GuiDocumentView 调用）
void GuiEventHandler::mousePressEvent(QMouseEvent* e)
{
    if (hasAction())
    {
        m_currentActions.last()->mousePressEvent(e);
        e->accept();
    }
    else
    {
        e->ignore();
    }
}

/// @brief 处理鼠标释放事件（由 GuiDocumentView 调用）
void GuiEventHandler::mouseReleaseEvent(QMouseEvent* e)
{
    if (hasAction())
    {
        m_currentActions.last()->mouseReleaseEvent(e);
        cleanUp();
        e->accept();
    }
    else
    {
        e->ignore();
    }
}

/// @brief 处理鼠标移动事件（由 GuiDocumentView 调用）
void GuiEventHandler::mouseMoveEvent(QMouseEvent* e)
{
    if (hasAction())
    {
        m_currentActions.last()->mouseMoveEvent(e);
    }
}

/// @brief 处理鼠标双击事件
void GuiEventHandler::mouseDoubleClickEvent(QMouseEvent* e)
{
    if (hasAction())
    {
        m_currentActions.last()->mouseDoubleClickEvent(e);
    }
}

/// @brief 处理鼠标离开事件（经 LegacyActionTool 调用）；空闲态由选择层自己处理
void GuiEventHandler::mouseLeaveEvent()
{
    if (hasAction())
    {
        m_currentActions.last()->suspend();
    }
}

/// @brief 处理鼠标进入事件（经 LegacyActionTool 调用）；空闲态由选择层自己处理
void GuiEventHandler::mouseEnterEvent()
{
    if (hasAction())
    {
        m_currentActions.last()->resume();
    }
}

/// @brief 处理键盘按下事件（由 GuiDocumentView 调用）
void GuiEventHandler::keyPressEvent(QKeyEvent* e)
{
    if (hasAction())
    {
        ActionInterface* action = m_currentActions.last();
        action->keyPressEvent(e);
    }
    else
    {
        e->ignore();
    }
}

/// @brief 处理键盘释放事件（由 GuiDocumentView 调用）
void GuiEventHandler::keyReleaseEvent(QKeyEvent* e)
{
    if (hasAction())
    {
        m_currentActions.last()->keyReleaseEvent(e);
    }
    else
    {
        e->ignore();
    }
}

/// @brief 处理命令行事件：坐标（见 GuiCoordinateInput）转成坐标事件，其余交给当前 Action
void GuiEventHandler::commandEvent(GuiCommandEvent* e)
{
    if (!m_isCoordinateInputEnabled || e->isAccepted() || !hasAction())
    {
        return;
    }

    const GuiCoordinateInput input = GuiCoordinateInput::parse(e->getCommand(), m_relativeZero);
    switch (input.status)
    {
    case GuiCoordinateInput::Status::Ok:
    {
        GuiCoordinateEvent ce(input.position);
        m_currentActions.last()->coordinateEvent(&ce);
        e->accept();
        break;
    }
    case GuiCoordinateInput::Status::SyntaxError:
        GUIDIALOGFACTORY->commandMessage("Expression Syntax Error");
        e->accept();
        break;
    case GuiCoordinateInput::Status::NotCoordinate:
        // send command event directly to current action:
        m_currentActions.last()->commandEvent(e);
        break;
    }
}

/// @brief 启用命令行坐标输入
void GuiEventHandler::enableCoordinateInput()
{
    m_isCoordinateInputEnabled = true;
}

/// @brief 禁用命令行坐标输入
void GuiEventHandler::disableCoordinateInput()
{
    m_isCoordinateInputEnabled = false;
}

bool GuiEventHandler::isCoordinateInputEnabled() const
{
    return m_isCoordinateInputEnabled;
}

/// @brief 获取当前操作
/// @return 当前活动操作
ActionInterface* GuiEventHandler::getCurrentAction()
{
    if (hasAction())
    {
        return m_currentActions.last();
    }
    return nullptr;
}

/// @brief 获取当前操作数量
int GuiEventHandler::getCurrentActionNum()
{
    return m_currentActions.size();
}

/// @brief 设置旧 Action 栈之下的一层
void GuiEventHandler::setStackBase(ILegacyStackBase* base)
{
    m_pStackBase = base;
}

/// @brief 即时命令执行前，结束不可打断的栈顶 Action
void GuiEventHandler::interruptForInstantCommand()
{
    if (!hasAction())
    {
        return;
    }
    ActionInterface* predecessor = m_currentActions.last();
    if (!predecessor->canBeInterrupt())
    {
        predecessor->finish();
        cleanUp();
    }
}

/// @brief 设置当前操作
void GuiEventHandler::setCurrentAction(ActionInterface* action)
{
    if (action == NULL)
    {
        return;
    }

    // 按需要挂起或终止前一个action；从空栈启动时挂起栈下的一层（选择层或命令）
    ActionInterface* predecessor = hasAction() ? m_currentActions.last() : nullptr;
    if (!predecessor && m_pStackBase)
    {
        m_pStackBase->suspendForLegacy();
    }
    if (predecessor)
    {
        bool finishPre = false;
        if (action->isSubAction())
        {
            if (predecessor->isSubAction())
            {
                finishPre = true;
            }
            else
            {
                finishPre = false;
            }
        }
        // action是视图操作，或者前一个可被打断，强行挂起前一个
        else if (action->isViewAction() || predecessor->canBeInterrupt())
        {
            finishPre = false;
        }
        // 否则结束前一个Action
        else
        {
            finishPre = true;
        }
        if (finishPre)
        {
            predecessor->finish();
            cleanUp();
        }
        else
        {
            predecessor->suspend();
            predecessor->hideOptions();
        }
    }

    // 如果是排他的action，删除前面所有action
    if (action->isExclusive())
    {
        killAllActions();
        cleanUp();
    }
    m_currentActions.push_back(action);

    action->init();

    if (action->isFinished() == false)
    {
        m_currentActions.last()->showOptions();
    }

    cleanUp();

    if (m_pAction)
    {
        m_pAction->setChecked(true);
    }
}

/// @brief 终止所有活动操作，并复位栈下的一层
void GuiEventHandler::killAllActions()
{
    if (m_pAction)
    {
        m_pAction->setChecked(false);
        m_pAction = nullptr;
    }

    for (auto p = m_currentActions.rbegin(); p != m_currentActions.rend(); p++)
    {
        if (!(*p)->isFinished())
        {
            (*p)->finish();
        }
    }

    if (m_pStackBase)
    {
        m_pStackBase->resetAfterKill();
    }
}

QList<ActionInterface*>& GuiEventHandler::getCurrentActionsRef()
{
    return m_currentActions;
}

/// @brief 检查操作是否有效
/// @return true 表示该操作在 m_currentActions 中
bool GuiEventHandler::isValid(ActionInterface* action) const
{
    return m_currentActions.indexOf(action) >= 0;
}

/// @brief 检查是否有活动操作
/// @return true 表示操作栈中至少有一个未完成的操作
bool GuiEventHandler::hasAction()
{
    foreach(ActionInterface * a, m_currentActions)
    {
        if (!a->isFinished())
        {
            return true;
        }
    }
    return false;
}

/// @brief 操作垃圾回收
void GuiEventHandler::cleanUp()
{
    for (auto it = m_currentActions.begin(); it != m_currentActions.end();)
    {
        if ((*it)->isFinished())
        {
            delete *it;
            it = m_currentActions.erase(it);
        }
        else
        {
            ++it;
        }
    }
    if (hasAction())
    {
        m_currentActions.last()->resume();
        m_currentActions.last()->showOptions();
    }
    else if (m_pStackBase)
    {
        m_pStackBase->resumeAfterLegacy();
    }
}

/// @brief 为所有当前活动操作设置捕捉模式
void GuiEventHandler::setSnapMode(SnapMode sm)
{
    for (auto a : m_currentActions)
    {
        if (!a->isFinished())
        {
            a->setSnapMode(sm);
        }
    }
}

/// @brief 为所有当前活动操作设置捕捉限制
void GuiEventHandler::setSnapRestriction(DM::SnapRestriction sr)
{
    for (auto a : m_currentActions)
    {
        if (!a->isFinished())
        {
            a->setSnapRestriction(sr);
        }
    }
}

void GuiEventHandler::setQAction(QAction* action)
{
    if (m_pAction)
    {
        m_pAction->setChecked(false);
        killAllActions();
    }
    m_pAction = action;
}

void GuiEventHandler::setRelativeZero(const DmVector& point)
{
    m_relativeZero = point;
}
