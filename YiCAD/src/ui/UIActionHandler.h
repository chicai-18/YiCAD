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

#include "ActionInterface.h"
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

    ActionInterface* getCurrentAction();
    ActionInterface* setCurrentAction(DM::ActionType id);

    /// @brief 按字符串命令 ID 启动命令（Ribbon、命令行别名、扩展共用的入口）。
    /// @param commandId CommandRegistry 里注册的命令 ID
    /// @param source 触发源，透传为 CommandContext::sender；为空时取 Qt 的 sender()
    /// @return 已交给视图管理的 Action；命令未注册、工厂未构造 Action，或
    /// 没有打开文档时构造后立即触发并删除的，都返回 nullptr。
    ActionInterface* activateCommand(const QString& commandId, QObject* source = nullptr);

    void setSnapToolBar(UISnapWidget* toolbar);
    void setMDIWindow(MDIWindow* m);
    void setMdiArea(QMdiArea* m);
    void setUITabDrawWidget(UITabDrawWidget* tabDrawWidget);

    /// @brief Kills all running selection actions. Called when a selection action is launched to reduce confusion.
    void killSelectActions();
    /// @brief killAllActions kill all actions
    void killAllActions();

    bool keycode(const QString& code);
    // special handling of actions issued from command line, currently used for snap actions
    // return true if handled
    bool commandLineActions(DM::ActionType id);
    bool command(const QString& cmd);

    /// @brief 设置规范外部命令的执行入口。
    /// @param executor 接收 pluginId 和 commandId 的执行器；空执行器表示禁用。
    void setExternalCommandExecutor(ExternalCommandExecutor executor);
    QStringList getAvailableCommands();
    SnapMode getSnaps();
    DM::SnapRestriction getSnapRestriction();
    void set_view(GuiDocumentView* pDocumentView);
    void set_document(DmDocument* document);
    void redrawAll();
    void updateGrids();

public slots:
    void slotZoomIn();
    void slotZoomOut();
    void slotZoomPan();

    void slotEditKillAllActions();
    void slotEditUndo();

    void slotDrawPoint();

    void slotModifyDelete();

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

    void slotIndoSelected();

    void slotLayersFreezeAll();
    void slotLayersLockAll();

    void slotBlocksSave();
    void slotBlocksInsert();

    void slotViewGrid();

    void slotSecectedChanged();

    /// @brief 响应撤销/重做后的块编辑状态变化
    void slotCmdStateChanged();

private:
    /// @brief 解析并执行 pluginId/commandId 形式的外部命令。
    bool executeExternalCommand(const QString& command);

    /// @brief keyconfig.xml 查到的枚举在宿主侧是否有实现：注册表桥接、
    /// KillAllActions，或捕捉/约束开关。
    static bool hasBuiltinHandler(DM::ActionType type);

    UISnapWidget*       m_pSnapToolbar = nullptr;
    GuiDocumentView*    m_pView = nullptr;
    DmDocument*         m_pDocument = nullptr;
    MDIWindow*          m_pMdiWin = nullptr;
    QMdiArea*           m_pDrawingArea = nullptr;
    UITabDrawWidget*    m_pTabDrawWidget = nullptr;
    ExternalCommandExecutor m_externalCommandExecutor;
};

#endif
