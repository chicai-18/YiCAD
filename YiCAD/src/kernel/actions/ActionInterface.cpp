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

/// @file ActionInterface.cpp
/// @brief Action接口基类实现

#include "ActionInterface.h"

#include <QKeyEvent>

#include "GuiDocumentView.h"
#include "Commands.h"
#include "GuiDialogFactory.h"
#include "GuiCoordinateEvent.h"
#include "Debug.h"

/// @brief 构造函数，初始化Action基本属性
/// @param [in] name Action名称，主要用于调试
/// @param [in] doc 文档指针，Action操作的对象
/// @param [in] docView 文档视图实例，Action归属于此视图
ActionInterface::ActionInterface(const char* name, DmDocument* doc,
                                 GuiDocumentView* docView)
    : Snapper(doc, docView)
{
    this->name = name;
    m_status = 0;
    finished = false;
    actionType = DM::ActionNone;
}

/// @brief 获取此Action的实体类型ID
/// @return 当前Action的ActionType
DM::ActionType ActionInterface::getEntityType() const
{
    return actionType;
}

/// @brief 获取Action名称
/// @return Action名称字符串
QString ActionInterface::getName()
{
    return name;
}

/// @brief 设置Action名称
/// @param [in] _name 新名称
void ActionInterface::setName(const char* _name)
{
    this->name = _name;
}

/// @brief 初始化Action，设置状态并更新UI
/// @param [in] status 初始状态值，默认为0开始操作
void ActionInterface::init(int status)
{
    setStatus(status);
    if (status >= 0)
    {
        Snapper::init();
    }
    else
    {
        deleteSnapper();
    }
}

/// @brief 鼠标移动事件处理（默认空实现，由子类重写）
/// @param [in] event 鼠标事件指针
void ActionInterface::mouseMoveEvent(QMouseEvent* event)
{
}

/// @brief 鼠标按下事件处理（默认空实现，由子类重写）
/// @param [in] event 鼠标事件指针
void ActionInterface::mousePressEvent(QMouseEvent* event)
{
}

/// @brief 鼠标释放事件处理（默认空实现，由子类重写）
/// @param [in] event 鼠标事件指针
void ActionInterface::mouseReleaseEvent(QMouseEvent* event)
{
}

/// @brief 鼠标双击事件处理（默认空实现，由子类重写）
/// @param [in] event 鼠标事件指针
void ActionInterface::mouseDoubleClickEvent(QMouseEvent* event)
{
}

/// @brief 键盘按下事件处理，默认忽略事件
/// @param [in] e 键盘事件指针
void ActionInterface::keyPressEvent(QKeyEvent* e)
{
    e->ignore();
}

/// @brief 键盘释放事件处理，默认忽略事件
/// @param [in] e 键盘事件指针
void ActionInterface::keyReleaseEvent(QKeyEvent* e)
{
    e->ignore();
}

/// @brief 坐标事件处理（默认空实现，由子类重写）
/// @param [in] event 坐标事件指针
void ActionInterface::coordinateEvent(GuiCoordinateEvent* event)
{
}

/// @brief 命令事件处理（默认空实现，由子类重写）
/// @param [in] event 命令事件指针
void ActionInterface::commandEvent(GuiCommandEvent* event)
{
}

/// @brief 获取当前可用的命令列表（默认返回空列表）
/// @return 空字符串列表
QStringList ActionInterface::getAvailableCommands()
{
    return QStringList{};
}

/// @brief 设置当前状态（进度）
/// 负数状态将结束该Action
/// @param [in] status 状态编号
void ActionInterface::setStatus(int status)
{
    if (m_status == status)
    {
        if (status < 0)
        {
            finish();
        }
        return;
    }
    this->m_status = status;
    updateMouseButtonHints();
    updateMouseCursor();
    if (status < 0)
    {
        finish();
    }
}

/// @brief 获取当前状态
/// @return 当前状态值
int ActionInterface::getStatus()
{
    return m_status;
}

/// @brief 触发此Action（默认空实现，由子类重写）
void ActionInterface::trigger()
{
}

/// @brief 更新鼠标按钮提示文本（默认空实现，由子类重写）
void ActionInterface::updateMouseButtonHints()
{
}

/// @brief 更新鼠标光标样式（默认空实现，由子类重写）
void ActionInterface::updateMouseCursor()
{
}

/// @brief 检查Action是否已完成
/// @return true表示已完成，可以删除
bool ActionInterface::isFinished()
{
    return finished;
}

/// @brief 强制终止Action，不进行清理
void ActionInterface::setFinished()
{
    m_status = -1;
    finished = true;
}

/// @brief 正常结束Action
/// 拒绝退出默认Action（ActionDefault）
/// @param [in] updateTB 是否更新工具栏
void ActionInterface::finish(bool /*updateTB*/)
{
    // 拒绝退出默认Action
    if (getEntityType() != DM::ActionDefault)
    {
        m_status = -1;
        finished = true;
        hideOptions();
        Snapper::finish();
    }

    if (getEntityType() != DM::ActionNone)
    {
        Snapper::resetOrthogonal();
    }
}

/// @brief 挂起此Action，允许其他Action执行
void ActionInterface::suspend()
{
    Snapper::suspend();
}

/// @brief 从挂起状态恢复Action
void ActionInterface::resume()
{
    updateMouseCursor();
    updateMouseButtonHints();
    Snapper::resume();
}

/// @brief 是否可以被打断
/// @return 默认返回true，表示可以被其他Action打断
bool ActionInterface::canBeInterrupt()
{
    return true;
}

/// @brief 是否为视图操作命令
/// @return 默认返回false
bool ActionInterface::isViewAction()
{
    return false;
}

/// @brief 是否为排他的命令
/// @return 默认返回false
bool ActionInterface::isExclusive()
{
    return false;
}

/// @brief 是否为子命令
/// @return 默认返回false
bool ActionInterface::isSubAction()
{
    return false;
}

/// @brief 隐藏工具选项
void ActionInterface::hideOptions()
{
    Snapper::hideOptions();
}

/// @brief 显示工具选项
void ActionInterface::showOptions()
{
    Snapper::showOptions();
}

/// @brief 设置Action类型
/// @param [in] actionType ActionType枚举值
void ActionInterface::setActionType(DM::ActionType actionType)
{
    this->actionType = actionType;
}

/// @brief 检查命令字符串是否匹配（委托给COMMANDS模块）
/// @param [in] cmd 要检查的命令
/// @param [in] str 用户输入的字符串
/// @param [in] action ActionType
/// @return true表示匹配
bool ActionInterface::checkCommand(const QString& cmd, const QString& str,
                                   DM::ActionType action)
{
    return COMMANDS->checkCommand(cmd, str, action);
}

/// @brief 获取命令的翻译文本（委托给COMMANDS模块）
/// @param [in] cmd 英文命令
/// @return 翻译后的命令
QString ActionInterface::command(const QString& cmd)
{
    return COMMANDS->command(cmd);
}

/// @brief 获取"可用命令"消息（委托给COMMANDS模块）
/// @return 可用命令消息字符串
QString ActionInterface::msgAvailableCommands()
{
    return COMMANDS->msgAvailableCommands();
}
