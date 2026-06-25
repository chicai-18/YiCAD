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
#include "Math2d.h"
#include "Debug.h"

GuiEventHandler::GuiEventHandler(QObject* parent) : QObject(parent)
{
    connect(parent, SIGNAL(relative_zero_changed(const DmVector&)), this, SLOT(setRelativeZero(const DmVector&)));
}

GuiEventHandler::~GuiEventHandler()
{
    delete m_pDefaultAction;
    m_pDefaultAction = nullptr;

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

/// @brief 向当前操作发送回车确认事件
void GuiEventHandler::enter()
{
    QKeyEvent e(QEvent::KeyPress, Qt::Key_Enter, 0);
    keyPressEvent(&e);
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
        if (m_pDefaultAction)
        {
            m_pDefaultAction->mousePressEvent(e);
            e->accept();
        }
        else
        {
            e->ignore();
        }
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
        if (m_pDefaultAction)
        {
            m_pDefaultAction->mouseReleaseEvent(e);
        }
        else
        {
            e->ignore();
        }
    }
}

/// @brief 处理鼠标移动事件（由 GuiDocumentView 调用）
void GuiEventHandler::mouseMoveEvent(QMouseEvent* e)
{
    if (hasAction())
    {
        m_currentActions.last()->mouseMoveEvent(e);
    }
    else if (m_pDefaultAction)
    {
        m_pDefaultAction->mouseMoveEvent(e);
    }
}

/// @brief 处理鼠标双击事件
void GuiEventHandler::mouseDoubleClickEvent(QMouseEvent* e)
{
    if (hasAction())
    {
        m_currentActions.last()->mouseDoubleClickEvent(e);
    }
    else if (m_pDefaultAction)
    {
        m_pDefaultAction->mouseDoubleClickEvent(e);
    }
}

/// @brief 处理鼠标离开事件（由 GuiDocumentView 调用）
void GuiEventHandler::mouseLeaveEvent()
{
    if (hasAction())
    {
        m_currentActions.last()->suspend();
    }
    else
    {
        if (m_pDefaultAction)
        {
            m_pDefaultAction->suspend();
        }
    }
}

/// @brief 处理鼠标进入事件（由 GuiDocumentView 调用）
void GuiEventHandler::mouseEnterEvent()
{
    if (hasAction())
    {
        m_currentActions.last()->resume();
    }
    else
    {
        if (m_pDefaultAction)
        {
            m_pDefaultAction->resume();
        }
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
        if (m_pDefaultAction)
        {
            m_pDefaultAction->keyPressEvent(e);
        }
        else
        {
            e->ignore();
        }
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
        if (m_pDefaultAction)
        {
            m_pDefaultAction->keyReleaseEvent(e);
        }
        else
        {
            e->ignore();
        }
    }
}

/// @brief 处理命令行事件
void GuiEventHandler::commandEvent(GuiCommandEvent* e)
{
    QString cmd = e->getCommand();

    if (m_isCoordinateInputEnabled)
    {
        if (!e->isAccepted())
        {
            if (hasAction())
            {
                // handle absolute cartesian coordinate input:
                if (cmd.contains(',') && cmd.at(0) != '@')
                {
                    int commaPos = cmd.indexOf(',');
                    bool ok1 = false;
                    bool ok2 = false;
                    double x = Math2d::eval(cmd.left(commaPos), &ok1);
                    double y = Math2d::eval(cmd.mid(commaPos + 1), &ok2);

                    if (ok1 && ok2)
                    {
                        GuiCoordinateEvent ce(DmVector(x, y));
                        m_currentActions.last()->coordinateEvent(&ce);
                    }
                    else
                    {
                        GUIDIALOGFACTORY->commandMessage("Expression Syntax Error");
                    }
                    e->accept();
                }

                // handle relative cartesian coordinate input:
                if (!e->isAccepted())
                {
                    if (cmd.contains(',') && cmd.at(0) == '@')
                    {
                        int commaPos = cmd.indexOf(',');
                        bool ok1 = false;
                        bool ok2 = false;
                        double x = Math2d::eval(cmd.mid(1, commaPos - 1), &ok1);
                        double y = Math2d::eval(cmd.mid(commaPos + 1), &ok2);

                        if (ok1 && ok2)
                        {
                            GuiCoordinateEvent ce(DmVector(x, y) + m_relativeZero);

                            m_currentActions.last()->coordinateEvent(&ce);
                        }
                        else
                        {
                            GUIDIALOGFACTORY->commandMessage("Expression Syntax Error");
                        }
                        e->accept();
                    }
                }

                // handle absolute polar coordinate input:
                if (!e->isAccepted())
                {
                    if (cmd.contains('<') && cmd.at(0) != '@')
                    {
                        int commaPos = cmd.indexOf('<');
                        bool ok1 = false;
                        bool ok2 = false;
                        double r = Math2d::eval(cmd.left(commaPos), &ok1);
                        double a = Math2d::eval(cmd.mid(commaPos + 1), &ok2);

                        if (ok1 && ok2)
                        {
                            DmVector pos{ DmVector::polar(r,Math2d::deg2rad(a)) };
                            GuiCoordinateEvent ce(pos);
                            m_currentActions.last()->coordinateEvent(&ce);
                        }
                        else
                        {
                            GUIDIALOGFACTORY->commandMessage("Expression Syntax Error");
                        }
                        e->accept();
                    }
                }

                // handle relative polar coordinate input:
                if (!e->isAccepted())
                {
                    if (cmd.contains('<') && cmd.at(0) == '@')
                    {
                        int commaPos = cmd.indexOf('<');
                        bool ok1 = false;
                        bool ok2 = false;
                        double r = Math2d::eval(cmd.mid(1, commaPos - 1), &ok1);
                        double a = Math2d::eval(cmd.mid(commaPos + 1), &ok2);

                        if (ok1 && ok2)
                        {
                            DmVector pos = DmVector::polar(r, Math2d::deg2rad(a));
                            GuiCoordinateEvent ce(pos + m_relativeZero);
                            m_currentActions.last()->coordinateEvent(&ce);
                        }
                        else
                        {
                            GUIDIALOGFACTORY->commandMessage("Expression Syntax Error");
                        }
                        e->accept();
                    }
                }

                // send command event directly to current action:
                if (!e->isAccepted())
                {
                    m_currentActions.last()->commandEvent(e);
                }
            }
            else
            {
                //send the command to default action
                if (m_pDefaultAction)
                {
                    m_pDefaultAction->commandEvent(e);
                }
            }
        }
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

/// @brief 获取当前操作
/// @return 当前活动操作
ActionInterface* GuiEventHandler::getCurrentAction()
{
    if (hasAction())
    {
        return m_currentActions.last();
    }
    else
    {
        return m_pDefaultAction;
    }
}

/// @brief 获取当前操作数量
int GuiEventHandler::getCurrentActionNum()
{
    return m_currentActions.size();
}

/// @brief 获取默认操作
/// @return 当前默认操作
ActionInterface* GuiEventHandler::getDefaultAction() const
{
    return m_pDefaultAction;
}

/// @brief 设置默认操作
void GuiEventHandler::setDefaultAction(ActionInterface* action)
{
    if (m_pDefaultAction)
    {
        m_pDefaultAction->finish();
        delete m_pDefaultAction;
    }

    m_pDefaultAction = action;
}

/// @brief 设置当前操作
void GuiEventHandler::setCurrentAction(ActionInterface* action)
{
    if (action == NULL)
    {
        return;
    }

    ActionInterface* predecessor = NULL;

    // 按需要挂起或终止前一个action
    if (hasAction())
    {
        predecessor = m_currentActions.last();
    }
    else
    {
        if (m_pDefaultAction)
        {
            predecessor = m_pDefaultAction;
        }
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

/// @brief 终止选择类操作
void GuiEventHandler::killSelectActions()
{
    for (auto it = m_currentActions.begin(); it != m_currentActions.end();)
    {
        if ((*it)->getEntityType() == DM::ActionSelectSingle)
        {
            if (!(*it)->isFinished())
            {
                (*it)->finish();
            }
            delete *it;
            it = m_currentActions.erase(it);
        }
        else
        {
            it++;
        }
    }
}

/// @brief 终止所有活动操作（窗口关闭时调用）
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

    if (!m_pDefaultAction->isFinished())
    {
        m_pDefaultAction->finish();
    }

    m_pDefaultAction->init(0);
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
    else
    {
        if (m_pDefaultAction)
        {
            m_pDefaultAction->resume();
            m_pDefaultAction->showOptions();
        }
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

    if (m_pDefaultAction)
    {
        m_pDefaultAction->setSnapMode(sm);
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

    if (m_pDefaultAction)
    {
        m_pDefaultAction->setSnapRestriction(sr);
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

/// @brief 检查是否处于选择模式
bool GuiEventHandler::inSelectionMode()
{
    switch (getCurrentAction()->getEntityType())
    {
    case DM::ActionDefault:
    case DM::ActionSelectSingle:
        return true;
    default:
        return false;
    }
}
