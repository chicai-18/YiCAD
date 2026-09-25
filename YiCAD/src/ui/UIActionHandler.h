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

/// @file UIActionHandler.h
/// @brief 操作处理器，负责触发菜单、按钮等CAD操作命令

#ifndef UIACTIONHANDLER_H
#define UIACTIONHANDLER_H

#include "ISnapService.h"
#include "MDIWindow.h"
#include "QMdiArea"
#include "UITabDrawWidget.h"
#include "UISnapWidget.h"

#include <functional>

class UISnapWidget;
class DmLayer;

/// @class UIActionHandler
/// @brief 这个类可以触发操作（菜单、按钮等）
class UIActionHandler : public QObject
{
    Q_OBJECT

public:
    using ExternalCommandExecutor =
        std::function<bool(const QString&, const QString&)>;

    UIActionHandler(QObject* parent);
    virtual ~UIActionHandler() = default;

    /// @brief 按字符串命令 ID 启动命令（Ribbon、命令行别名、扩展共用的入口）。
    /// @details 按注册类型分派（CommandRegistry::kind）：交互命令交给视图的命令总线；
    /// 即时命令直接执行；临时视图工具叠在视图的业务栈顶。
    /// @param commandId CommandRegistry 里注册的命令 ID
    /// @param source 触发源，透传为 CommandContext::sender；为空时取 Qt 的 sender()
    void activateCommand(const QString& commandId, QObject* source = nullptr);

    void setSnapToolBar(UISnapWidget* toolbar);
    void setMDIWindow(MDIWindow* m);
    void setMdiArea(QMdiArea* m);
    void setUITabDrawWidget(UITabDrawWidget* tabDrawWidget);

    /// @brief killAllActions kill all actions
    void killAllActions();

    bool keycode(const QString& code);
    bool command(const QString& cmd);

    /// @brief 设置规范外部命令的执行入口。
    /// @param executor 接收 pluginId 和 commandId 的执行器；空执行器表示禁用。
    void setExternalCommandExecutor(ExternalCommandExecutor executor);
    SnapMode getSnaps();
    DM::SnapRestriction getSnapRestriction();
    void set_view(GuiDocumentView* pDocumentView);
    void set_document(DmDocument* document);
    void redrawAll();
    void updateGrids();

public slots:
    void slotEditKillAllActions();
    /// @brief 撤销（编辑扩展的 ext.edit.undo）；没有打开的图纸时什么也不做
    void slotEditUndo();

    void slotSetSnaps(SnapMode const& s);
    void slotSnapFree();
    void slotSnapGrid();
    void slotSnapEndpoint();
    void slotSnapOnEntity();
    void slotSnapCenter();
    void slotSnapMiddle();
    void slotSnapIntersection();

    void slotRestrictNothing();
    void slotRestrictOrthogonal();
    void slotRestrictHorizontal();
    void slotRestrictVertical();

    void disableSnaps();
    void disableRestrictions();

    void slotViewGrid();

    void slotSecectedChanged();

    /// @brief 响应撤销/重做后的块编辑状态变化
    void slotCmdStateChanged();

private:
    /// @brief 解析并执行 pluginId/commandId 形式的外部命令。
    bool executeExternalCommand(const QString& command);

    /// @brief 宿主自己处理的内置命令：结束全部命令（edit.kill_all）与捕捉/约束开关
    ///        （snap.*、restrict.*），keyconfig.xml 可以给它们配别名
    /// @param fromCommandLine 来自命令行（command()）还是按键编码（keycode()）
    /// @return commandId 是内置命令时返回 true
    bool runBuiltinCommand(const QString& commandId, bool fromCommandLine);
    /// @brief 运行 keyconfig.xml 查到的命令：内置命令，或 CommandRegistry 里注册了的命令
    /// @return 运行了时返回 true；命令 ID 为空或没有实现时返回 false
    bool runKeyconfigCommand(const QString& commandId, bool fromCommandLine);

    UISnapWidget*       m_pSnapToolbar = nullptr;
    GuiDocumentView*    m_pView = nullptr;
    DmDocument*         m_pDocument = nullptr;
    MDIWindow*          m_pMdiWin = nullptr;
    QMdiArea*           m_pDrawingArea = nullptr;
    UITabDrawWidget*    m_pTabDrawWidget = nullptr;
    ExternalCommandExecutor m_externalCommandExecutor;
};

#endif
