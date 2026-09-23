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

#include "ActionBlocksEdit.h"

#include "Selection.h"

#include "Debug.h"
#include "DmSettings.h"
#include "MDIWindow.h"
#include "QMdiArea"
#include "GuiDocumentView.h"
#include "GuiEventHandler.h"
#include "UICurrentActivePen.h"

UIActionHandler::UIActionHandler(QObject* parent)
	:QObject(parent)
{
}


// Kills all running selection actions. Called when a selection action is launched to reduce confusion.
void UIActionHandler::killSelectActions()
{
	if (m_pView)
	{
		m_pView->killSelectActions();
	}
}

void UIActionHandler::killAllActions()
{

	if (m_pView)
	{
		m_pView->killAllActions();
	}
}


// @return Current action or NULL.
ActionInterface* UIActionHandler::getCurrentAction()
{
	if (m_pView)
	{
		return m_pView->getCurrentAction();
	}
	else
	{
		return nullptr;
	}
}


// Sets current action.
// @return Pointer to the created action or NULL.
ActionInterface* UIActionHandler::setCurrentAction(DM::ActionType id)
{
	// ActionEditKillAllActions 不构造任何 Action，只做副作用；killAllActions()
	// 只在具体类 GuiDocumentView 上，不在 IDocumentView 接口上，没法进
	// CommandRegistry 的工厂签名，原样保留为显式分支（阶段4第一部分，
	// 见 doc/ARCHITECTURE_EVOLUTION_PLAN.md 阶段4）。
	if (id == DM::ActionEditKillAllActions)
	{
		if (m_pView)
		{
			// DO we need to call some form of a 'clean' function?
			m_pView->killAllActions();

			Selection s(m_pDocument, m_pView);
			s.selectAll(false);
			GUIDIALOGFACTORY->updateSelectionWidget(m_pDocument->getEntityTable()->countSelect());
		}
		return nullptr;
	}

	// Snap/Restrict 类型直接调用 setCurrentAction 时的兜底：commandLineActions()
	// 已经是这批类型的权威实现（command() 早就在调用它），这里只是让
	// setCurrentAction 自身对这批类型保持定义行为，不需要再进注册表或 switch。
	if (commandLineActions(id))
	{
		return nullptr;
	}

	// 全部 153 个原 case 已分批迁移到 CommandRegistry（阶段4第一至八部分，
	// 见 doc/ARCHITECTURE_EVOLUTION_PLAN.md 阶段4）。未命中注册表的类型
	// （枚举里从未进入过这个 switch 的保留值，如 ActionFileExport/Print/
	// Quit、ActionView* 系列，以及已搬进扩展、不再有枚举桥接的命令）维持
	// 原 default 行为：不构造任何 Action。
	const QString commandId = CommandRegistry::instance().commandId(id);
	if (commandId.isEmpty())
	{
		return nullptr;
	}
	return activateCommand(commandId, sender());
}

ActionInterface* UIActionHandler::activateCommand(const QString& commandId, QObject* source)
{
	ActionInterface* a = CommandRegistry::instance().create(
		commandId, CommandContext{m_pDocument, m_pView, this, source ? source : sender()});

	if (a)
	{
		if (m_pView)
		{
			m_pView->setCurrentAction(a);
		}
		//在没有打开文档的情况，Action无法被管理，但是需要触发一下（例如：ActionFileNew）
		else
		{
			a->trigger();
			delete a;
			a = nullptr;
		}
	}

	return a;
}


// @return Available commands of the application or the current action.
QStringList UIActionHandler::getAvailableCommands()
{
	ActionInterface* currentAction = getCurrentAction();

	if (currentAction)
	{
		return currentAction->getAvailableCommands();
	}
	else
	{
		QStringList cmd;
		cmd += "line";
		cmd += "rectangle";
		return cmd;
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
	DM::ActionType type = COMMANDS->keycodeToAction(code);
	if (type != DM::ActionNone && hasBuiltinHandler(type))
	{
		// some actions require special handling (GUI update):
		switch (type)
		{
		case DM::ActionSnapFree:
			slotSnapFree();
			break;
		case DM::ActionSnapCenter:
			slotSnapCenter();
			break;
			break;
		case DM::ActionSnapEndpoint:
			slotSnapEndpoint();
			break;
		case DM::ActionSnapGrid:
			slotSnapGrid();
			break;
		case DM::ActionSnapIntersection:
			slotSnapIntersection();
			break;
		case DM::ActionSnapMiddle:
			slotSnapMiddle();
			break;
		case DM::ActionSnapOnEntity:
			slotSnapOnEntity();
			break;
		case DM::ActionRestrictNothing:
			slotRestrictNothing();
			break;
		case DM::ActionRestrictOrthogonal:
			slotRestrictOrthogonal();
			break;
		case DM::ActionRestrictHorizontal:
			slotRestrictHorizontal();
			break;
		case DM::ActionRestrictVertical:
			slotRestrictVertical();
			break;

		default:
			setCurrentAction(type);
			break;
		}
		return true;
	}

	// 纯字符串命令（扩展命令）没有枚举值，也不在 keyconfig.xml 里，按注册表
	// 登记的别名再查一次。
	const QString commandId = CommandRegistry::instance().commandForAlias(code);
	if (!commandId.isEmpty())
	{
		activateCommand(commandId);
		return true;
	}

	// keyconfig.xml 认领了、但宿主没有实现的枚举（如用户目录下旧 keyconfig
	// 里残留的、已搬进扩展的命令）：保持原行为，按已识别处理。
	return type != DM::ActionNone;
}

bool UIActionHandler::hasBuiltinHandler(DM::ActionType type)
{
	switch (type)
	{
	case DM::ActionEditKillAllActions:
	case DM::ActionSnapFree:
	case DM::ActionSnapCenter:
	case DM::ActionSnapEndpoint:
	case DM::ActionSnapGrid:
	case DM::ActionSnapIntersection:
	case DM::ActionSnapMiddle:
	case DM::ActionSnapOnEntity:
	case DM::ActionRestrictNothing:
	case DM::ActionRestrictOrthogonal:
	case DM::ActionRestrictHorizontal:
	case DM::ActionRestrictVertical:
		return true;
	default:
		return CommandRegistry::instance().hasLegacyMapping(type);
	}
}


// toggle snap modes when calling from command line
bool UIActionHandler::commandLineActions(DM::ActionType type)
{
	// snap actions require special handling (GUI update)
	//more special handling of actions can be added here
	switch (type) 
	{
	case DM::ActionSnapCenter:
		slotSnapCenter();
		return true;
	case DM::ActionSnapEndpoint:
		slotSnapEndpoint();
		return true;
	case DM::ActionSnapGrid:
		slotSnapGrid();
		return true;
	case DM::ActionSnapIntersection:
		slotSnapIntersection();
		return true;
	case DM::ActionSnapMiddle:
		slotSnapMiddle();
		return true;
	case DM::ActionSnapOnEntity:
		slotSnapOnEntity();
		return true;

	case DM::ActionRestrictNothing:
		slotRestrictNothing();
		return true;
	case DM::ActionRestrictOrthogonal:
		slotRestrictOrthogonal();
		return true;
	case DM::ActionRestrictHorizontal:
		slotRestrictHorizontal();
		return true;
	case DM::ActionRestrictVertical:
		slotRestrictVertical();
		return true;

	default:
		return false;
	}

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
		// 解析顺序：keyconfig.xml 里有实现的内置命令 > 注册表登记的别名
		// （扩展命令）> keyconfig.xml 认领但没有实现的条目 > 插件命令。
		DM::ActionType type = COMMANDS->cmdToAction(cmd);
		if (type != DM::ActionNone && hasBuiltinHandler(type))
		{
			//special handling, currently needed for snap actions
			if (!commandLineActions(type))
			{
				//not handled yet
				setCurrentAction(type);
			}
			return true;
		}

		const QString commandId = CommandRegistry::instance().commandForAlias(cmd);
		if (!commandId.isEmpty())
		{
			activateCommand(commandId);
			return true;
		}

		if (type != DM::ActionNone)
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
	setCurrentAction(DM::ActionZoomIn);
}

void UIActionHandler::slotZoomOut() 
{
	setCurrentAction(DM::ActionZoomOut);
}

void UIActionHandler::slotZoomPan() 
{
	setCurrentAction(DM::ActionZoomPan);
}

void UIActionHandler::slotEditKillAllActions() 
{
	setCurrentAction(DM::ActionEditKillAllActions);
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
	setCurrentAction(DM::ActionEditUndo);
}

void UIActionHandler::slotDrawPoint() 
{
	setCurrentAction(DM::ActionDrawPoint);
}

void UIActionHandler::slotModifyDelete() 
{
	setCurrentAction(DM::ActionModifyDelete);
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
    setCurrentAction(DM::ActionInfoSelected);
}

void UIActionHandler::slotLayersFreezeAll() 
{
	setCurrentAction(DM::ActionLayersFreezeAll);
}

void UIActionHandler::slotLayersLockAll() 
{
	setCurrentAction(DM::ActionLayersLockAll);
}

void UIActionHandler::slotBlocksSave() 
{
	setCurrentAction(DM::ActionBlocksSave);
}

void UIActionHandler::slotBlocksInsert() 
{
	setCurrentAction(DM::ActionBlocksInsert);
}

void UIActionHandler::slotSecectedChanged()
{
	setCurrentAction(DM::ActionSelectedChanged);
}

void UIActionHandler::slotCmdStateChanged()
{
	if (!m_pDocument || !m_pView)
		return;

	DmBlock* editingBlock = m_pDocument->getEditingBlock();
	// 扫描完整动作栈，而不是只看 getCurrentAction()。
	// 当命令动作（如 ActionEditUndo、ActionDrawLine）压在
	// ActionBlocksEdit 之上时，getCurrentAction() 返回的是栈顶动作，
	// 会把块编辑动作隐藏掉。
	ActionInterface* blockEditAction = nullptr;
	for (auto* a : m_pView->getEventHandler()->getCurrentActionsRef())
	{
		if (!a->isFinished()
			&& a->getEntityType() == DM::ActionBlocksEdit)
		{
			blockEditAction = a;
			break;
		}
	}
	bool inBlockEdit = (blockEditAction != nullptr);

	if (editingBlock && !inBlockEdit)
	{
		// 撤销/重做后重新进入块编辑：找到匹配的块参照，
		// 重新创建一个 ActionBlocksEdit
		// 这里使用 getDocumentEntityTable()，因为 editingBlock 已设置时，
		// getEntityTable() 会路由到块自己的实体表
		DmBlockReference* ref = nullptr;
		for (auto e : *m_pDocument->getDocumentEntityTable())
		{
			if (e && !e->isErased()
				&& e->getEntityType() == DM::EntityBlockReference)
			{
				DmBlockReference* br = static_cast<DmBlockReference*>(e);
				if (br->getName() == editingBlock->getName())
				{
					ref = br;
					break;
				}
			}
		}
		auto* action = new ActionBlocksEdit(m_pDocument, m_pView, ref);
		m_pView->setCurrentAction(action);
	}
	else if (!editingBlock && inBlockEdit)
	{
		// 撤销/重做后退出了块编辑：结束 ActionBlocksEdit
		blockEditAction->finish();
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
