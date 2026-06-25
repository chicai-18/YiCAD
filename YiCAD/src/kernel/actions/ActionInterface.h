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

/// @file ActionInterface.h
/// @brief Action接口基类，所有CAD操作Action必须实现此接口

#ifndef ACTIONINTERFACE_H
#define ACTIONINTERFACE_H

#include <QObject>

#include "Snapper.h"

class QKeyEvent;
class GuiCommandEvent;
class GuiCoordinateEvent;
class DmDocument;
class QAction;

/// @brief Action接口基类，所有Action类必须实现此接口
/// Action类处理如画线、移动实体或缩放等操作
/// 继承自QObject以支持Qt翻译功能
class ActionInterface : public QObject, public Snapper
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] name Action名称，主要用于调试
    /// @param [in] doc 文档指针
    /// @param [in] docView 文档视图指针
    ActionInterface(const char* name, DmDocument* doc,
                    GuiDocumentView* docView);

    /// @brief 虚析构函数
    virtual ~ActionInterface() = default;

    /// @brief 获取此Action的实体类型ID
    /// @return ActionType枚举值
    virtual DM::ActionType getEntityType() const;

    /// @brief 设置Action名称
    /// @param [in] _name 新名称
    void setName(const char* _name);

    /// @brief 获取Action名称
    /// @return Action名称
    QString getName();

    /// @brief 初始化Action
    /// @param [in] status 初始状态值，0表示开始操作
    virtual void init(int status = 0);

    /// @brief 鼠标移动事件处理
    /// @param [in] event 鼠标事件指针
    virtual void mouseMoveEvent(QMouseEvent* event);

    /// @brief 鼠标按下事件处理
    /// @param [in] event 鼠标事件指针
    virtual void mousePressEvent(QMouseEvent* event);

    /// @brief 鼠标释放事件处理
    /// @param [in] event 鼠标事件指针
    virtual void mouseReleaseEvent(QMouseEvent* event);

    /// @brief 鼠标双击事件处理
    /// @param [in] event 鼠标事件指针
    virtual void mouseDoubleClickEvent(QMouseEvent* event);

    /// @brief 键盘按下事件处理
    /// @param [in] e 键盘事件指针
    virtual void keyPressEvent(QKeyEvent* e);

    /// @brief 键盘释放事件处理
    /// @param [in] e 键盘事件指针
    virtual void keyReleaseEvent(QKeyEvent* e);

    /// @brief 坐标事件处理（通常来自命令行输入）
    /// @param [in] event 坐标事件指针
    virtual void coordinateEvent(GuiCoordinateEvent* event);

    /// @brief 命令事件处理（来自命令行）
    /// @param [in] event 命令事件指针
    virtual void commandEvent(GuiCommandEvent* event);

    /// @brief 获取当前可用的命令列表
    /// @return 可用命令字符串列表
    virtual QStringList getAvailableCommands();

    /// @brief 设置当前状态（进度）
    /// @param [in] status 状态编号，负数表示结束操作
    virtual void setStatus(int status);

    /// @brief 获取当前状态
    /// @return 当前状态值
    virtual int getStatus();

    /// @brief 触发此Action，在所需数据收集完毕后调用
    virtual void trigger();

    /// @brief 更新鼠标按钮提示文本
    virtual void updateMouseButtonHints();

    /// @brief 更新鼠标光标样式
    virtual void updateMouseCursor();

    /// @brief 检查Action是否已完成
    /// @return true表示已完成，可以删除
    virtual bool isFinished();

    /// @brief 强制终止Action，不进行清理
    virtual void setFinished();

    /// @brief 正常结束Action
    /// @param [in] updateTB 是否更新工具栏
    virtual void finish(bool updateTB = true);

    /// @brief 挂起此Action，允许其他Action执行
    virtual void suspend();

    /// @brief 从挂起状态恢复Action
    virtual void resume();

    /// @brief 是否可以被打断
    /// @return true表示可以被其他Action打断
    virtual bool canBeInterrupt();

    /// @brief 是否为视图操作命令
    /// 该类型的命令可以打断任何命令，不论canBeInterrupt()返回值
    /// 视图命令必须是立即结束的
    /// @return true表示是视图操作命令
    virtual bool isViewAction();

    /// @brief 是否为排他的命令
    /// 排他的命令将删除前面的所有命令
    /// @return true表示是排他命令
    virtual bool isExclusive();

    /// @brief 是否为子命令
    /// 子命令可以打断非子命令，不论canBeInterrupt()返回值
    /// 子命令跟随在父命令后，只有一个在命令列表
    /// @return true表示是子命令
    virtual bool isSubAction();

    /// @brief 隐藏工具选项
    virtual void hideOptions();

    /// @brief 显示工具选项
    virtual void showOptions();

    /// @brief 设置Action类型
    /// @param [in] actionType ActionType枚举值
    virtual void setActionType(DM::ActionType actionType);

    /// @brief 检查命令字符串是否匹配
    /// @param [in] cmd 要检查的命令
    /// @param [in] str 用户输入的字符串
    /// @param [in] action ActionType
    /// @return true表示匹配
    bool checkCommand(const QString& cmd, const QString& str,
                      DM::ActionType action = DM::ActionNone);

    /// @brief 获取命令的翻译文本
    /// @param [in] cmd 英文命令
    /// @return 翻译后的命令
    QString command(const QString& cmd);

    /// @brief 获取"可用命令"消息
    /// @return 可用命令消息字符串
    QString msgAvailableCommands();

private:
    /// @brief 当前状态，-2为未初始化，-1为已终止，>=0为操作步骤
    /// 其他正数值可用于描述操作的不同阶段
    /// 例如窗口缩放：选择第一角(status 0)，选择第二角(status 1)
    int m_status = -2;

protected:
    QString name;               ///< Action名称
    bool finished = false;       ///< 是否已完成
    DM::ActionType actionType;   ///< Action类型标识
    // TODO: actionType未初始化，需在构造函数中设置
};

#endif // ACTIONINTERFACE_H
