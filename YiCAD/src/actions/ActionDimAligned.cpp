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


/// @file ActionDimAligned.cpp
/// @brief 对齐标注操作类实现文件

#include "ActionDimAligned.h"

#include <cmath>

#include <QAction>
#include <QMouseEvent>

#include "Debug.h"
#include "DmConstructionLine.h"
#include "DmDimAligned.h"
#include "DmLine.h"
#include "GuiCommandEvent.h"
#include "GuiCoordinateEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Preview.h"
#include "DmDocument.h"
#include "Transaction.h"

/// @brief 构造函数
ActionDimAligned::ActionDimAligned(DmDocument* doc, GuiDocumentView* docView)
	: ActionDimension("Draw aligned dimensions", doc, docView)
{
	actionType = DM::ActionDimAligned;
	reset();
}

/// @brief 析构函数
ActionDimAligned::~ActionDimAligned() = default;

/// @brief 重置操作状态
void ActionDimAligned::reset()
{
	ActionDimension::reset();

	edata.reset(new DmDimAlignedData(DmVector(false), DmVector(false)));
	lastStatus = SetExtPoint1;
	GUIDIALOGFACTORY->requestOptions(this, true, true);
}

/// @brief 触发创建对齐标注
void ActionDimAligned::trigger()
{
	PreviewActionInterface::trigger();

	preparePreview();
	docView->moveRelativeZero(data->definitionPoint);

	DmDimAligned* dim = new DmDimAligned(nullptr, *data, *edata);
	dim->setDocument(pDocument);
	dim->update();

	Transaction t(tr("Add dimension aligned").toStdString(), pDocument);
	t.start();
	pDocument->getEntityTable()->add(dim);
	t.commit();

	DmVector rz = docView->getRelativeZero();
	docView->moveRelativeZero(rz);
}

/// @brief 准备预览图形，计算标注线位置
void ActionDimAligned::preparePreview()
{
	constexpr double PREVIEW_LINE_LENGTH = 100.0;

	DmVector dirV = DmVector::polar(
		PREVIEW_LINE_LENGTH,
		edata->extensionPoint1.angleTo(edata->extensionPoint2) + M_PI_2);
	DmConstructionLine cl(
		nullptr,
		DmConstructionLineData(edata->extensionPoint2, edata->extensionPoint2 + dirV));

	data->definitionPoint = cl.getNearestPointOnEntity(data->definitionPoint);
}

/// @brief 鼠标移动事件处理
void ActionDimAligned::mouseMoveEvent(QMouseEvent* e)
{
	DmVector mouse = snapPoint(e);

	switch (getStatus())
	{
	case SetExtPoint1:
		break;

	case SetExtPoint2:
	{
		if (edata->extensionPoint1.valid)
		{
			deletePreview();

			auto l = new DmLine{
				preview->getEntityContainer(),
				edata->extensionPoint1, mouse};
			l->setDocument(pDocument);
			preview->addEntity(l);
			drawPreview();
		}
	}
	break;

	case SetDefPoint:
	{
		if (edata->extensionPoint1.valid && edata->extensionPoint2.valid)
		{
			deletePreview();
			data->definitionPoint = mouse;

			preparePreview();

			DmDimAligned* dim =
				new DmDimAligned(preview->getEntityContainer(), *data, *edata);
			dim->setDocument(pDocument);
			preview->addEntity(dim);
			dim->update();
			drawPreview();
		}
	}
	break;

	default:
		break;
	}
}

/// @brief 鼠标释放事件处理
void ActionDimAligned::mouseReleaseEvent(QMouseEvent* e)
{
	if (e->button() == Qt::LeftButton)
	{
		GuiCoordinateEvent ce(snapPoint(e));
		coordinateEvent(&ce);
	}
	else if (e->button() == Qt::RightButton)
	{
		deletePreview();
		init(getStatus() - 1);
	}
}

/// @brief 坐标事件处理
void ActionDimAligned::coordinateEvent(GuiCoordinateEvent* e)
{
	if (!e)
	{
		return;
	}

	DmVector pos = e->getCoordinate();

	switch (getStatus())
	{
	case SetExtPoint1:
		edata->extensionPoint1 = pos;
		docView->moveRelativeZero(pos);
		setStatus(SetExtPoint2);
		break;

	case SetExtPoint2:
		edata->extensionPoint2 = pos;
		docView->moveRelativeZero(pos);
		setStatus(SetDefPoint);
		break;

	case SetDefPoint:
		data->definitionPoint = pos;
		trigger();
		finishOrthogonal();
		reset();
		setStatus(SetExtPoint1);
		break;

	default:
		break;
	}
}

/// @brief 命令事件处理
void ActionDimAligned::commandEvent(GuiCommandEvent* e)
{
	QString c = e->getCommand().toLower();

	if (checkCommand("help", c))
	{
		GUIDIALOGFACTORY->commandMessage(
			msgAvailableCommands() + getAvailableCommands().join(", "));
		return;
	}

	switch (getStatus())
	{
	case SetText:
	{
		// TODO: 待修正
		// setText(c);
		GUIDIALOGFACTORY->requestOptions(this, true, true);
		setStatus(lastStatus);
		docView->enableCoordinateInput();
	}
	break;

	default:
	{
		if (checkCommand("text", c))
		{
			lastStatus = static_cast<Status>(getStatus());
			docView->disableCoordinateInput();
			setStatus(SetText);
		}
	}
	break;
	}
}

/// @brief 获取可用命令列表
QStringList ActionDimAligned::getAvailableCommands()
{
	QStringList cmd;

	switch (getStatus())
	{
	case SetExtPoint1:
	case SetExtPoint2:
	case SetDefPoint:
		cmd += command("text");
		break;

	default:
		break;
	}

	return cmd;
}

/// @brief 更新鼠标按钮提示
void ActionDimAligned::updateMouseButtonHints()
{
	switch (getStatus())
	{
	case SetExtPoint1:
		GUIDIALOGFACTORY->updateMouseWidget(
			tr("Specify first extension line origin"), tr("Cancel"));
		break;

	case SetExtPoint2:
		GUIDIALOGFACTORY->updateMouseWidget(
			tr("Specify second extension line origin"), tr("Back"));
		break;

	case SetDefPoint:
		GUIDIALOGFACTORY->updateMouseWidget(
			tr("Specify dimension line location"), tr("Back"));
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

/// @brief 隐藏选项面板
void ActionDimAligned::hideOptions()
{
	GUIDIALOGFACTORY->requestOptions(this, false);
	ActionInterface::hideOptions();
}

/// @brief 显示选项面板
void ActionDimAligned::showOptions()
{
	ActionInterface::showOptions();
	GUIDIALOGFACTORY->requestOptions(this, true);
}
