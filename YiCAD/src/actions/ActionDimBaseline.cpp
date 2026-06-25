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


/// @file ActionDimBaseline.cpp
/// @brief 基线标注操作类实现文件

#include "ActionDimBaseline.h"

#include <QMouseEvent>

#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "GuiCoordinateEvent.h"
#include "GeometryMethods.h"
#include "Preview.h"
#include "DmDimension.h"
#include "DmDimAligned.h"
#include "DmDimLinear.h"
#include "DmDimAngular.h"
#include "Tools.h"
#include "Transaction.h"

/// @brief 构造函数
ActionDimBaseline::ActionDimBaseline(DmDocument* doc, GuiDocumentView* docView)
	: ActionDimension("Draw baseline dimension", doc, docView)
{
	actionType = DM::ActionDimBaseline;
	reset();
}

/// @brief 重置操作状态
void ActionDimBaseline::reset()
{
	selectedDim = nullptr;
	newEnt = nullptr;
	addNum = 1;
}

/// @brief 触发创建基线标注
void ActionDimBaseline::trigger()
{
	PreviewActionInterface::trigger();

	if (!newEnt)
	{
		return;
	}

	Transaction t(tr("Add dimension baseline").toStdString(), pDocument);
	t.start();
	pDocument->getEntityTable()->add(newEnt);
	t.commit();

	DmVector rz = docView->getRelativeZero();
	addNum++;
}

/// @brief 鼠标移动事件处理
void ActionDimBaseline::mouseMoveEvent(QMouseEvent* e)
{
	switch (getStatus())
	{
	case SelectDim:
	{
		deleteSnapper();
	}
	break;

	case SetPos:
	{
		deletePreview();
		DmVector pos = snapPoint(e);

		DmDimAligned* dimAligned = dynamic_cast<DmDimAligned*>(selectedDim);

		if (dimAligned)
		{
			DmDimLinear* dim = createDimForAligned(pos);
			dim->setParent(preview->getEntityContainer());
			preview->addEntity(dim);
		}

		DmDimLinear* dimLinear = dynamic_cast<DmDimLinear*>(selectedDim);

		if (dimLinear)
		{
			DmDimLinear* dim = createDimForLinear(pos);
			dim->setParent(preview->getEntityContainer());
			preview->addEntity(dim);
		}

		drawPreview();
	}
	break;

	default:
		break;
	}
}

/// @brief 鼠标释放事件处理
void ActionDimBaseline::mouseReleaseEvent(QMouseEvent* e)
{
	if (e->button() == Qt::LeftButton)
	{
		switch (getStatus())
		{
		case SelectDim:
		{
			DmEntity* catchEnt = catchEntity(e);

			if (catchEnt == nullptr)
			{
				GUIDIALOGFACTORY->commandMessage(tr("No Entity found."));
			}
			else
			{
				DM::EntityType type = catchEnt->getEntityType();

				if (type != DM::EntityDimAligned
					&& type != DM::EntityDimAngular
					&& type != DM::EntityDimLinear)
				{
					GUIDIALOGFACTORY->commandMessage(
						tr("Entity must be a aligned dimension, "
						   "angular dimension or linear dimension."));
				}
				else
				{
					selectedDim = static_cast<DmDimension*>(catchEnt);
					setStatus(SetPos);
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
		finish();
	}
}

/// @brief 坐标事件处理
void ActionDimBaseline::coordinateEvent(GuiCoordinateEvent* e)
{
	if (!e)
	{
		return;
	}

	DmVector pos = e->getCoordinate();

	if (getStatus() == SetPos)
	{
		newEnt = nullptr;

		DmDimAligned* dimAligned = dynamic_cast<DmDimAligned*>(selectedDim);

		if (dimAligned)
		{
			newEnt = createDimForAligned(pos);
		}

		DmDimLinear* dimLinear = dynamic_cast<DmDimLinear*>(selectedDim);

		if (dimLinear)
		{
			newEnt = createDimForLinear(pos);
		}

		trigger();
	}
}

/// @brief 更新鼠标按钮提示
void ActionDimBaseline::updateMouseButtonHints()
{
	switch (getStatus())
	{
	case SelectDim:
		GUIDIALOGFACTORY->updateMouseWidget(
			tr("Specify origin dimension"), tr("Cancel"));
		break;

	case SetPos:
		GUIDIALOGFACTORY->updateMouseWidget(
			tr("Specify the point to define dimension"), tr("Cancel"));
		break;

	default:
		break;
	}
}

/// @brief 为选择的对齐标注创建基线标注
DmDimLinear* ActionDimBaseline::createDimForAligned(DmVector pos)
{
	constexpr double DISTANCE_MULTIPLIER = 2.0;
	constexpr double DIR_LINE_LENGTH = 100.0;

	DmDimAligned* dimAligned = dynamic_cast<DmDimAligned*>(selectedDim);

	if (dimAligned == nullptr)
	{
		return nullptr;
	}

	DmDimensionData data = dimAligned->getData();
	DmDimAlignedData edata = dimAligned->getEData();
	DmVector textCenter = data.textCenter;
	DmVector textFoot = GeometryMethods::getPerpendicularFoot(
		data.definitionPoint,
		data.definitionPoint + (edata.extensionPoint1 - edata.extensionPoint2),
		textCenter);

	double offsetDist = textCenter.distanceTo(textFoot) * DISTANCE_MULTIPLIER;
	offsetDist *= addNum;

	DmVector textDir = (edata.extensionPoint1 - edata.extensionPoint2).normalize();
	DmVector textVDir = DmVector(textDir).rotate(M_PI_2);
	DmVector offsetDir(true);

	if (textVDir.dotP(pos - textFoot) < 0)
	{
		offsetDir = textVDir;
	}
	else
	{
		offsetDir = -textVDir;
	}

	// 设置新标注信息
	DmVector definitionFoot = GeometryMethods::getPerpendicularFoot(
		pos, pos + offsetDir, data.definitionPoint);
	data.definitionPoint = definitionFoot + offsetDir * offsetDist;

	DmDimLinearData edata_new;
	edata_new.extensionPoint1 = edata.extensionPoint1;
	edata_new.extensionPoint2 = pos;
	data.angle = textDir.angle();

	DmDimLinear* dim = new DmDimLinear(nullptr, data, edata_new);
	dim->setDocument(pDocument);
	dim->update();

	return dim;
}

/// @brief 为选择的线性标注创建基线标注
DmDimLinear* ActionDimBaseline::createDimForLinear(DmVector pos)
{
	constexpr double DISTANCE_MULTIPLIER = 2.0;
	constexpr double DIR_LINE_LENGTH = 100.0;

	DmDimLinear* dimLinear = dynamic_cast<DmDimLinear*>(selectedDim);

	if (dimLinear == nullptr)
	{
		return nullptr;
	}

	DmDimensionData data = dimLinear->getData();
	DmDimLinearData edata = dimLinear->getEData();
	DmVector textCenter = data.textCenter;
	DmVector textFoot = GeometryMethods::getPerpendicularFoot(
		data.definitionPoint,
		data.definitionPoint + DmVector(data.angle) * DIR_LINE_LENGTH,
		textCenter);

	double offsetDist = textCenter.distanceTo(textFoot) * DISTANCE_MULTIPLIER;
	offsetDist *= addNum;

	DmVector textDir(data.angle);
	DmVector textVDir = DmVector(textDir).rotate(M_PI_2);
	DmVector offsetDir(true);

	if (textVDir.dotP(pos - textFoot) < 0)
	{
		offsetDir = textVDir;
	}
	else
	{
		offsetDir = -textVDir;
	}

	// 设置新标注信息
	DmVector definitionFoot = GeometryMethods::getPerpendicularFoot(
		pos, pos + offsetDir, data.definitionPoint);
	data.definitionPoint = definitionFoot + offsetDir * offsetDist;

	DmDimLinearData edata_new;
	edata_new.extensionPoint1 = edata.extensionPoint1;
	edata_new.extensionPoint2 = pos;
	data.angle = textDir.angle();

	DmDimLinear* dim = new DmDimLinear(nullptr, data, edata_new);
	dim->setDocument(pDocument);
	dim->update();

	return dim;
}

/// @brief 为选择的角度标注创建基线标注（暂未实现）
DmDimAngular* ActionDimBaseline::createDimForAngular(DmVector pos)
{
	DmDimAngular* dimAngular = dynamic_cast<DmDimAngular*>(selectedDim);

	if (dimAngular == nullptr)
	{
		return nullptr;
	}

	// TODO : 角度标注对应的基线标注是"三点角度标注"，是一种新标注类型，暂不处理

	return nullptr;
}
