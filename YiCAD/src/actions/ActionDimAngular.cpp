/**
 * Copyright (c) 2011-2018 by Andrew Mustun. All rights reserved.
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is part of the YiCAD project.
 *
 * YiCAD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YiCAD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */


/// @file ActionDimAngular.cpp
/// @brief 角度标注操作类实现文件

#include "ActionDimAngular.h"

#include <cmath>

#include <QAction>
#include <QMouseEvent>

#include "Debug.h"
#include "DmDimAngular.h"
#include "GuiCommandEvent.h"
#include "GuiCoordinateEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Information.h"
#include "Math2d.h"
#include "Preview.h"
#include "Transaction.h"

/// @brief 构造函数
ActionDimAngular::ActionDimAngular(DmDocument* doc, GuiDocumentView* docView)
	: ActionDimension("Draw Angular Dimensions", doc, docView)
{
	reset();
}

/// @brief 析构函数
ActionDimAngular::~ActionDimAngular() = default;

/// @brief 重置操作状态
void ActionDimAngular::reset()
{
	ActionDimension::reset();

	actionType = DM::ActionDimAngular;
	edata.reset(new DmDimAngularData(
		DmVector(false), DmVector(false),
		DmVector(false), DmVector(false), DmVector(false)));
	GUIDIALOGFACTORY->requestOptions(this, true, true);
}

/// @brief 触发创建角度标注
void ActionDimAngular::trigger()
{
	PreviewActionInterface::trigger();

	if (line1.getStartpoint().valid && line2.getStartpoint().valid)
	{
		DmDimAngular* newEntity = new DmDimAngular(nullptr, *data, *edata);
		newEntity->setDocument(pDocument);
		newEntity->update();

		Transaction t(tr("Add dimension angular").toStdString(), pDocument);
		t.start();
		pDocument->getEntityTable()->add(newEntity);
		t.commit();

		DmVector rz{docView->getRelativeZero()};
		setStatus(SetLine1);
		docView->moveRelativeZero(rz);
		Snapper::finish();
	}
}

/// @brief 鼠标移动事件处理
void ActionDimAngular::mouseMoveEvent(QMouseEvent* e)
{
	switch (getStatus())
	{
	case SetPos:
	{
		edata->ptOnArc = docView->toGraph(e->x(), e->y());

		DmDimAngular* d =
			new DmDimAngular(preview->getEntityContainer(), *data, *edata);
		d->setDocument(pDocument);
		deletePreview();
		preview->addEntity(d);
		d->update();
		drawPreview();
	}
	break;

	default:
		break;
	}
}

/// @brief 鼠标释放事件处理
void ActionDimAngular::mouseReleaseEvent(QMouseEvent* e)
{
	if (Qt::LeftButton == e->button())
	{
		switch (getStatus())
		{
		case SetLine1:
		{
			DmEntity* en{catchEntity(e, DM::ResolveAll)};

			if (en && DM::EntityLine == en->getEntityType())
			{
				line1 = *dynamic_cast<DmLine*>(en);
				edata->line1StartPt = line1.getStartpoint();
				edata->line1EndPt = line1.getEndpoint();
				setStatus(SetLine2);
			}
		}
		break;

		case SetLine2:
		{
			DmEntity* en{catchEntity(e, DM::ResolveAll)};

			if (en && en->getEntityType() == DM::EntityLine)
			{
				line2 = *dynamic_cast<DmLine*>(en);
				edata->line2StartPt = line2.getStartpoint();
				edata->line2EndPt = line2.getEndpoint();
				docView->moveRelativeZero(center);
				setStatus(SetPos);
			}
		}
		break;

		case SetPos:
		{
			GuiCoordinateEvent ce(snapPoint(e));
			coordinateEvent(&ce);
		}
		break;

		default:
			break;
		}
	}
	else if (Qt::RightButton == e->button())
	{
		deletePreview();
		init(getStatus() - 1);
	}
}

/// @brief 坐标事件处理
void ActionDimAngular::coordinateEvent(GuiCoordinateEvent* e)
{
	if (!e)
	{
		return;
	}

	switch (getStatus())
	{
	case SetPos:
	{
		edata->ptOnArc = e->getCoordinate();
		trigger();
		finishOrthogonal();
		reset();
		setStatus(SetLine1);
	}
	break;

	default:
		break;
	}
}

/// @brief 命令事件处理
void ActionDimAngular::commandEvent(GuiCommandEvent* e)
{
	QString c(e->getCommand().toLower());

	if (checkCommand(QStringLiteral("help"), c))
	{
		GUIDIALOGFACTORY->commandMessage(
			msgAvailableCommands() + getAvailableCommands().join(", "));
		return;
	}

	// 设置新的文本标签
	if (SetText == getStatus())
	{
		setText(c);
		GUIDIALOGFACTORY->requestOptions(this, true, true);
		docView->enableCoordinateInput();
		setStatus(lastStatus);
		return;
	}

	// 命令: text
	if (checkCommand(QStringLiteral("text"), c))
	{
		lastStatus = static_cast<Status>(getStatus());
		docView->disableCoordinateInput();
		setStatus(SetText);
	}
}

/// @brief 获取可用命令列表
QStringList ActionDimAngular::getAvailableCommands()
{
	QStringList cmd;

	switch (getStatus())
	{
	case SetLine1:
	case SetLine2:
	case SetPos:
		cmd += command(QStringLiteral("text"));
		break;

	default:
		break;
	}

	return cmd;
}

/// @brief 显示选项面板
void ActionDimAngular::showOptions()
{
	ActionInterface::showOptions();
	GUIDIALOGFACTORY->requestOptions(this, true);
}

/// @brief 隐藏选项面板
void ActionDimAngular::hideOptions()
{
	ActionInterface::hideOptions();
	GUIDIALOGFACTORY->requestOptions(this, false);
}

/// @brief 更新鼠标按钮提示
void ActionDimAngular::updateMouseButtonHints()
{
	switch (getStatus())
	{
	case SetLine1:
		GUIDIALOGFACTORY->updateMouseWidget(
			tr("Select first line"), tr("Cancel"));
		break;

	case SetLine2:
		GUIDIALOGFACTORY->updateMouseWidget(
			tr("Select second line"), tr("Cancel"));
		break;

	case SetPos:
		GUIDIALOGFACTORY->updateMouseWidget(
			tr("Specify dimension arc line location"), tr("Cancel"));
		break;

	case SetText:
		GUIDIALOGFACTORY->updateMouseWidget(
			tr("Enter dimension text:"), "");
		break;

	default:
		GUIDIALOGFACTORY->updateMouseWidget();
		break;
	}
}
