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

/// @file UIActionHandler.cpp
/// @brief 核心动作分发器，负责根据命令类型创建和管理所有CAD操作动作（绘图、修改、标注、捕捉等）

#include <cmath>
#include "UIActionHandler.h"
#include "UISnapWidget.h"
#include "GuiDialogFactory.h"
#include "GuiCommandEvent.h"
#include "Commands.h"
#include "CommandRegistry.h"

#include <utility>

#include "ExclusiveCommandBus.h"

#include "Selection.h"

#include "Debug.h"
#include "DmSettings.h"
#include "IExclusiveCommand.h"
#include "TransientViewTool.h"
#include "MDIWindow.h"
#include "QMdiArea"
#include "GuiDocumentView.h"
#include "UICurrentActivePen.h"
#include "UIView.h"

UIActionHandler::UIActionHandler(QObject* parent)
	:QObject(parent)
{
}


void UIActionHandler::killAllActions()
{

	if (m_pView)
	{
		m_pView->killAllActions();
	}
}


void UIActionHandler::slotEditKillAllActions()
{
	// 结束全部命令并清空选择（原 ActionEditKillAllActions）；它只作用于视图与选择集，
	// 由宿主自己处理，不进 CommandRegistry
	if (m_pView)
	{
		// 被命令否决时（迁移计划 5.1 节）命令继续，选择集也不清空
		if (!m_pView->killAllActions())
		{
			return;
		}

		Selection s(m_pDocument, m_pView);
		s.selectAll(false);
		GUIDIALOGFACTORY->updateSelectionWidget(m_pDocument->getEntityTable()->countSelect());
	}
}

bool UIActionHandler::runBuiltinCommand(const QString& commandId, bool fromCommandLine)
{
	// keyconfig.xml 可以给它们配别名：原先是 DM::ActionEditKillAllActions 与
	// ActionSnap*/ActionRestrict* 这些枚举值，由本类的 switch 特判
	struct Builtin
	{
		const char* id;
		void (UIActionHandler::*slot)();
	};
	static const Builtin builtins[] = {
		{"edit.kill_all", &UIActionHandler::slotEditKillAllActions},
		{"snap.free", &UIActionHandler::slotSnapFree},
		{"snap.grid", &UIActionHandler::slotSnapGrid},
		{"snap.endpoint", &UIActionHandler::slotSnapEndpoint},
		{"snap.on_entity", &UIActionHandler::slotSnapOnEntity},
		{"snap.center", &UIActionHandler::slotSnapCenter},
		{"snap.middle", &UIActionHandler::slotSnapMiddle},
		{"snap.intersection", &UIActionHandler::slotSnapIntersection},
		{"restrict.nothing", &UIActionHandler::slotRestrictNothing},
		{"restrict.orthogonal", &UIActionHandler::slotRestrictOrthogonal},
		{"restrict.horizontal", &UIActionHandler::slotRestrictHorizontal},
		{"restrict.vertical", &UIActionHandler::slotRestrictVertical},
	};
	for (const Builtin& builtin : builtins)
	{
		if (commandId != QLatin1String(builtin.id))
		{
			continue;
		}
		// 与原先一致（既有缺陷）：命令行输入的自由捕捉开关被识别但不起作用，原
		// commandLineActions() 漏了 ActionSnapFree；按键编码（keycode()）照常切换
		if (!(fromCommandLine && commandId == QLatin1String("snap.free")))
		{
			(this->*builtin.slot)();
		}
		return true;
	}
	return false;
}

bool UIActionHandler::runKeyconfigCommand(const QString& commandId, bool fromCommandLine)
{
	if (commandId.isEmpty())
	{
		return false;
	}
	if (runBuiltinCommand(commandId, fromCommandLine))
	{
		return true;
	}
	// keyconfig.xml 认领了、但没有注册的命令（如对应的扩展没有加载）不在这里处理
	if (!CommandRegistry::instance().hasCommand(commandId))
	{
		return false;
	}
	activateCommand(commandId);
	return true;
}

void UIActionHandler::activateCommand(const QString& commandId, QObject* source)
{
	CommandRegistry& registry = CommandRegistry::instance();
	const CommandContext ctx{m_pDocument, m_pView, source ? source : sender()};

	switch (registry.kind(commandId))
	{
	case CommandKind::Exclusive:
		// 交互命令由视图的命令总线运行，没有打开图纸时不启动
		if (UIView* view = qobject_cast<UIView*>(m_pView))
		{
			if (std::unique_ptr<IExclusiveCommand> command = registry.createCommand(commandId, ctx))
			{
				view->startCommand(std::move(command));
			}
		}
		return;

	case CommandKind::Instant:
		// 即时命令不占命令总线，没有打开图纸时也执行（document/view 为空）
		if (UIView* view = qobject_cast<UIView*>(m_pView))
		{
			if (!view->prepareInstantCommand(registry.instantInterrupt(commandId)))
			{
				return;
			}
		}
		registry.runInstant(commandId, ctx);
		return;

	case CommandKind::ViewTool:
		// 临时视图工具（平移模式）不占命令总线，叠在视图的业务栈顶；没有打开图纸时不启动
		if (UIView* view = qobject_cast<UIView*>(m_pView))
		{
			if (std::unique_ptr<TransientViewTool> tool = registry.createViewTool(commandId, ctx))
			{
				view->startViewTool(std::move(tool));
			}
		}
		return;

	case CommandKind::None:
		return;
	}
}

//get snap mode from snap toolbar
SnapMode UIActionHandler::getSnaps()
{
	if (m_pSnapToolbar)
	{
		return m_pSnapToolbar->getSnaps();
	}
	//return a free snap mode
	return SnapMode();
}



/**
 * Launches the command represented by the given keycode if possible.
 *
 * @return true: the command was recognized.
 *         false: the command is not known and was probably intended for a
 *         running action.
 */
bool UIActionHandler::keycode(const QString& code)
{
	// if the current action can't deal with the keycode,
	// it might be intended to launch a new keycode

	// keycode for new action:
	const QString configured = COMMANDS->keycodeToCommand(code);
	if (runKeyconfigCommand(configured, false))
	{
		return true;
	}

	// keyconfig.xml 里没有的别名，按注册表登记的别名（扩展命令）再查一次。
	const QString commandId = CommandRegistry::instance().commandForAlias(code);
	if (!commandId.isEmpty())
	{
		activateCommand(commandId);
		return true;
	}

	// keyconfig.xml 认领了、但宿主没有实现的命令（如对应的扩展没有加载）：
	// 保持原行为，按已识别处理。
	return !configured.isEmpty();
}

/**
 * Launches the given command if possible.
 *
 * @return true: the command was recognized.
 *         false: the command is not known and was probably intended for a
 *            running action.
 */
bool UIActionHandler::command(const QString& cmd)
{
	if (!m_pView)
	{
		return false;
	}

	if (cmd.isEmpty())
	{
		if (DMSETTINGS->readNumEntry("/Keyboard/ToggleFreeSnapOnSpace", true))
		{
			slotSnapFree();
		}
		return true;
	}

	QString c = cmd.toLower();

	if (c == tr("escape", "escape, go back from action steps"))
	{
		m_pView->back();
		return true;
	}

	// pass command on to running action:
	GuiCommandEvent e(cmd);

	m_pView->commandEvent(&e);

	// if the current action can't deal with the command,
	// it might be intended to launch a new command
	if (!e.isAccepted()) 
	{
		// 解析顺序：keyconfig.xml 里有实现的命令 > 注册表登记的别名
		// （扩展命令）> keyconfig.xml 认领但没有实现的条目 > 插件命令。
		const QString configured = COMMANDS->cmdToCommand(cmd);
		if (runKeyconfigCommand(configured, true))
		{
			return true;
		}

		const QString commandId = CommandRegistry::instance().commandForAlias(cmd);
		if (!commandId.isEmpty())
		{
			activateCommand(commandId);
			return true;
		}

		if (!configured.isEmpty())
		{
			return true;
		}

		// 外部命令最后解析，保持活动 Action 和内置命令的既有优先级。
		return executeExternalCommand(cmd);
	}
	else 
	{
		return true;
	}
	return false;
}

void UIActionHandler::setExternalCommandExecutor(
	ExternalCommandExecutor executor)
{
	m_externalCommandExecutor = std::move(executor);
}

bool UIActionHandler::executeExternalCommand(const QString& command)
{
	if (!m_externalCommandExecutor)
	{
		return false;
	}

	const int separator = command.indexOf(QLatin1Char('/'));
	if (separator <= 0 || separator == command.size() - 1)
	{
		return false;
	}

	return m_externalCommandExecutor(
		command.left(separator),
		command.mid(separator + 1));
}

void UIActionHandler::slotZoomIn() 
{
	activateCommand(QStringLiteral("zoom.in"));
}

void UIActionHandler::slotZoomOut() 
{
	activateCommand(QStringLiteral("zoom.out"));
}

void UIActionHandler::slotZoomPan() 
{
	activateCommand(QStringLiteral("zoom.pan"));
}

void UIActionHandler::slotEditUndo() 
{
	//to avoid operation on deleted entities, Undo action invalid all suspended
	//actions
	MDIWindow* mdi = m_pTabDrawWidget->getCurrentMdiWindow();
	if (mdi == nullptr)
	{
		return;
	}
	activateCommand(QStringLiteral("edit.undo"));
}

void UIActionHandler::slotDrawPoint() 
{
	activateCommand(QStringLiteral("draw.point"));
}

void UIActionHandler::slotModifyDelete() 
{
	activateCommand(QStringLiteral("modify.delete"));
}

void UIActionHandler::slotSetSnaps(SnapMode const& s) 
{
	if (m_pSnapToolbar) 
	{
		m_pSnapToolbar->setSnaps(s);
	}
	if (m_pView) 
	{
		m_pView->setDefaultSnapMode(s);
	}
}

void UIActionHandler::slotSnapFree() 
{
	SnapMode s = getSnaps();
	s.snapFree = !s.snapFree;
	slotSetSnaps(s);
}

void UIActionHandler::slotSnapGrid() 
{
	SnapMode s = getSnaps();
	s.snapGrid = !s.snapGrid;
	slotSetSnaps(s);
}

void UIActionHandler::slotSnapEndpoint() 
{
	SnapMode s = getSnaps();
	s.snapEndpoint = !s.snapEndpoint;

	slotSetSnaps(s);
}

void UIActionHandler::slotSnapOnEntity() 
{
	SnapMode s = getSnaps();
	s.snapOnEntity = !s.snapOnEntity;

	slotSetSnaps(s);
}

void UIActionHandler::slotSnapCenter() 
{
	SnapMode s = getSnaps();
	s.snapCenter = !s.snapCenter;
	slotSetSnaps(s);
}

void UIActionHandler::slotSnapMiddle() 
{
	SnapMode s = getSnaps();
	s.snapMiddle = !s.snapMiddle;

	slotSetSnaps(s);
}

void UIActionHandler::slotSnapIntersection() 
{
	SnapMode s = getSnaps();
	s.snapIntersection = !s.snapIntersection;

	slotSetSnaps(s);
}

void UIActionHandler::disableSnaps() 
{
	slotSetSnaps(SnapMode());
}

void UIActionHandler::slotRestrictNothing() 
{
	SnapMode s = getSnaps();
	s.restriction = DM::RestrictNothing;
	slotSetSnaps(s);
}

void UIActionHandler::slotRestrictOrthogonal() 
{
	SnapMode s = getSnaps();
	s.restriction = DM::RestrictOrthogonal;
	slotSetSnaps(s);
}

void UIActionHandler::slotRestrictHorizontal() 
{
	SnapMode s = getSnaps();
	s.restriction = DM::RestrictHorizontal;
	slotSetSnaps(s);
}

void UIActionHandler::slotRestrictVertical() 
{
	SnapMode s = getSnaps();
	s.restriction = DM::RestrictVertical;
	slotSetSnaps(s);
}

// find snap restriction from menu
DM::SnapRestriction UIActionHandler::getSnapRestriction() 
{
	return getSnaps().restriction;
}

void UIActionHandler::disableRestrictions() 
{
	SnapMode s = getSnaps();
	s.restriction = DM::RestrictNothing;
	slotSetSnaps(s);
}

void UIActionHandler::slotIndoSelected()
{
    activateCommand(QStringLiteral("info.selected"));
}

void UIActionHandler::slotSecectedChanged()
{
	// 原 ActionSelectedChanged：刷新选择计数；单选多行文字时的属性面板归文字扩展
	if (!m_pDocument)
	{
		return;
	}
	GUIDIALOGFACTORY->updateSelectionWidget(m_pDocument->getEntityTable()->countSelect());
	CommandRegistry::instance().runInstant(QStringLiteral("ext.text.selection_changed"),
	                                       CommandContext{m_pDocument, m_pView});
}

void UIActionHandler::slotCmdStateChanged()
{
	if (!m_pDocument || !m_pView)
		return;

	UIView* view = qobject_cast<UIView*>(m_pView);
	ExclusiveCommandBus* bus = view ? view->commandBus() : nullptr;
	if (!bus)
		return;

	DmBlock* editingBlock = m_pDocument->getEditingBlock();
	const bool inBlockEdit = bus->editMode() != nullptr;

	if (editingBlock && !inBlockEdit)
	{
		// 撤销/重做后重新进入块编辑：文档已处于块编辑，由块扩展恢复编辑模式
		// （只有块扩展的命令会让文档进入块编辑，没有它时不会走到这里）
		CommandRegistry::instance().runInstant(QStringLiteral("ext.block.reenter_edit"),
		                                       CommandContext{m_pDocument, m_pView});
	}
	else if (!editingBlock && inBlockEdit)
	{
		// 撤销/重做后退出了块编辑：只收起编辑模式，不再改动文档
		bus->exitEditMode();
	}
}

void UIActionHandler::set_view(GuiDocumentView* pDocumentView)
{
	m_pView = pDocumentView;
}
void UIActionHandler::set_document(DmDocument* doc)
{
	m_pDocument = doc;
	// 连接 cmdChanged 信号，用于处理撤销/重做导致的块编辑重新进入
	if (m_pDocument && m_pDocument->getCmdManager())
	{
		connect(m_pDocument->getCmdManager(), &CmdManager::cmdChanged,
				this, &UIActionHandler::slotCmdStateChanged);
	}
}

void UIActionHandler::setSnapToolBar(UISnapWidget* toolbar)
{
	m_pSnapToolbar = toolbar;
}

void UIActionHandler::setMDIWindow(MDIWindow* m)
{
	m_pMdiWin = m;
}

void UIActionHandler::setMdiArea(QMdiArea* m)
{
	m_pDrawingArea = m;
}

void UIActionHandler::setUITabDrawWidget(UITabDrawWidget* tabDrawWidget)
{
	m_pTabDrawWidget = tabDrawWidget;
}

void UIActionHandler::slotViewGrid()
{
	MDIWindow* m = m_pTabDrawWidget->getCurrentMdiWindow();
	if (m)
	{
		DmDocument* g = m->getDocument();
		bool toggle = g->isGridOn();
		if (g)
		{
			g->setGridOn(!toggle);
		}
	}

	updateGrids();
	redrawAll();
}

void UIActionHandler::redrawAll()
{
	if (m_pDrawingArea)
	{
		MDIWindow* m = m_pTabDrawWidget->getCurrentMdiWindow();
		GuiDocumentView* gv = m->getDocumentView();
		if (gv)
		{
			gv->redraw();
		}
	}
}

void UIActionHandler::updateGrids()
{
	if (m_pDrawingArea)
	{
		QList<QMdiSubWindow*> windows = m_pDrawingArea->subWindowList();
		for (int i = 0; i < windows.size(); ++i)
		{
			MDIWindow* m = qobject_cast<MDIWindow*>(windows.at(i));
			if (m)
			{
				GuiDocumentView* gv = m->getDocumentView();
				if (gv)
				{
					gv->redraw();
				}
			}
		}
	}
}


// EOF
