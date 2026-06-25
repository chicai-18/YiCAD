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


/// @file ActionDimDiametric.cpp
/// @brief 直径标注操作类实现文件

#include "ActionDimDiametric.h"

#include <cmath>

#include <QAction>
#include <QMouseEvent>

#include "Debug.h"
#include "DmArc.h"
#include "DmCircle.h"
#include "DmDimDiametric.h"
#include "DmLine.h"
#include "GuiCommandEvent.h"
#include "GuiCoordinateEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Math2d.h"
#include "Preview.h"
#include "Transaction.h"

/// @brief 构造函数
ActionDimDiametric::ActionDimDiametric(DmDocument* doc, GuiDocumentView* docView)
	: ActionDimension("Draw Diametric Dimensions", doc, docView)
	, pos(new DmVector())
{
	actionType = DM::ActionDimDiametric;
	reset();
}

/// @brief 析构函数
ActionDimDiametric::~ActionDimDiametric() = default;

/// @brief 重置操作状态
void ActionDimDiametric::reset()
{
	ActionDimension::reset();

	edata.reset(new DmDimDiametricData(DmVector{false}, 0.0));
	entity = nullptr;
	*pos = {};
}

/// @brief 触发创建直径标注
void ActionDimDiametric::trigger()
{
	PreviewActionInterface::trigger();

	preparePreview();

	if (entity)
	{
		DmDimDiametric* newEntity = new DmDimDiametric(nullptr, *data, *edata);
		newEntity->setDocument(pDocument);
		newEntity->update();

		Transaction t(tr("Add dimension diametric").toStdString(), pDocument);
		t.start();
		pDocument->getEntityTable()->add(newEntity);
		t.commit();

		DmVector rz = docView->getRelativeZero();
		docView->moveRelativeZero(rz);
		Snapper::finish();
	}
}

/// @brief 准备预览图形，根据选择的圆/圆弧计算标注位置
void ActionDimDiametric::preparePreview()
{
	if (entity)
	{
		double radius{0.};
		DmVector center{false};

		if (entity->getEntityType() == DM::EntityArc)
		{
			DmArc* p = static_cast<DmArc*>(entity);
			radius = p->getRadius();
			center = p->getCenter();
		}
		else if (entity->getEntityType() == DM::EntityCircle)
		{
			DmCircle* p = static_cast<DmCircle*>(entity);
			radius = p->getRadius();
			center = p->getCenter();
		}

		double angle = center.angleTo(*pos);

		data->definitionPoint.setPolar(radius, angle + M_PI);
		data->definitionPoint += center;

		edata->endPoint.setPolar(radius, angle);
		edata->endPoint += center;

		// 圆心到鼠标距离减去半径 = 引线长度
		double dist = center.distanceTo(*pos);
		edata->leader = dist - radius;
	}
}

/// @brief 鼠标移动事件处理
void ActionDimDiametric::mouseMoveEvent(QMouseEvent* e)
{
	switch (getStatus())
	{
	case SetPos:
	{
		if (entity)
		{
			*pos = snapPoint(e);

			preparePreview();

			DmDimDiametric* d =
				new DmDimDiametric(preview->getEntityContainer(), *data, *edata);
			d->setDocument(pDocument);
			d->update();

			deletePreview();
			preview->addEntity(d);
			drawPreview();
		}
	}
	break;

	default:
		break;
	}
}

/// @brief 鼠标释放事件处理
void ActionDimDiametric::mouseReleaseEvent(QMouseEvent* e)
{
	if (e->button() == Qt::LeftButton)
	{
		switch (getStatus())
		{
		case SetEntity:
		{
			DmEntity* en = catchEntity(e, DM::ResolveAll);

			if (en)
			{
				if (en->getEntityType() == DM::EntityArc
					|| en->getEntityType() == DM::EntityCircle)
				{
					entity = en;

					DmVector center;

					if (entity->getEntityType() == DM::EntityArc)
					{
						center = static_cast<DmArc*>(entity)->getCenter();
					}
					else
					{
						center = static_cast<DmCircle*>(entity)->getCenter();
					}

					docView->moveRelativeZero(center);
					setStatus(SetPos);
				}
				else
				{
					GUIDIALOGFACTORY->commandMessage(
						tr("Not a circle or arc entity"));
				}
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
	else if (e->button() == Qt::RightButton)
	{
		deletePreview();
		init(getStatus() - 1);
	}
}

/// @brief 坐标事件处理
void ActionDimDiametric::coordinateEvent(GuiCoordinateEvent* e)
{
	if (!e)
	{
		return;
	}

	switch (getStatus())
	{
	case SetPos:
	{
		*pos = e->getCoordinate();
		trigger();
		finishOrthogonal();
		reset();
		setStatus(SetEntity);
	}
	break;

	default:
		break;
	}
}

/// @brief 命令事件处理
void ActionDimDiametric::commandEvent(GuiCommandEvent* e)
{
	QString c = e->getCommand().toLower();

	if (checkCommand("help", c))
	{
		GUIDIALOGFACTORY->commandMessage(
			msgAvailableCommands() + getAvailableCommands().join(", "));
		return;
	}

	// 设置新的文本标签
	if (getStatus() == SetText)
	{
		setText(c);
		docView->enableCoordinateInput();
		setStatus(lastStatus);
		return;
	}

	// 命令: text
	if (checkCommand("text", c))
	{
		lastStatus = static_cast<Status>(getStatus());
		docView->disableCoordinateInput();
		setStatus(SetText);
	}

	// 设置角度
	if (getStatus() == SetPos)
	{
		bool ok = false;
		double a = Math2d::eval(c, &ok);

		if (ok)
		{
			pos->setPolar(1.0, Math2d::deg2rad(a));
			*pos += data->definitionPoint;
			trigger();
			reset();
			setStatus(SetEntity);
		}
		else
		{
			GUIDIALOGFACTORY->commandMessage(tr("Not a valid expression"));
		}

		return;
	}
}

/// @brief 获取可用命令列表
QStringList ActionDimDiametric::getAvailableCommands()
{
	QStringList cmd;

	switch (getStatus())
	{
	case SetEntity:
	case SetPos:
		cmd += command("text");
		break;

	default:
		break;
	}

	return cmd;
}

/// @brief 更新鼠标按钮提示
void ActionDimDiametric::updateMouseButtonHints()
{
	switch (getStatus())
	{
	case SetEntity:
		GUIDIALOGFACTORY->updateMouseWidget(
			tr("Select arc or circle entity"), tr("Cancel"));
		break;

	case SetPos:
		GUIDIALOGFACTORY->updateMouseWidget(
			tr("Specify dimension line location"), tr("Cancel"));
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

/// @brief 显示选项面板
void ActionDimDiametric::showOptions()
{
	ActionInterface::showOptions();
}

/// @brief 隐藏选项面板
void ActionDimDiametric::hideOptions()
{
	ActionInterface::hideOptions();
}
