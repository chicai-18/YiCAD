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


/// @file ActionBlocksCreate.cpp
/// @brief 从现有实体创建块的动作类实现文件

#include "ActionBlocksCreate.h"

#include <QMouseEvent>
#include <QAction>

#include "DmDocument.h"
#include "DmBlockReference.h"
#include "DmAttribute.h"
#include "GuiCoordinateEvent.h"
#include "GuiDialogFactory.h"
#include "GuiDocumentView.h"
#include "Transaction.h"
#include "EntityTable.h"


/// @brief 构造函数
/// @param doc 文档指针
/// @param docView 文档视图指针
ActionBlocksCreate::ActionBlocksCreate(DmDocument* doc,
	GuiDocumentView* docView)
	: PreviewActionInterface("Blocks Create", doc, docView)
	, m_referencePoint(new DmVector{})
{
	actionType = DM::ActionBlocksCreate;
}

/// @brief 析构函数
ActionBlocksCreate::~ActionBlocksCreate() = default;

/// @brief 初始化动作
/// @param status 状态参数，默认为0
void ActionBlocksCreate::init(int status)
{
	PreviewActionInterface::init(status);
}

/// @brief 检查是否为独占动作
/// @return true表示独占
bool ActionBlocksCreate::isExclusive()
{
	return true;
}

/// @brief 触发动作执行
void ActionBlocksCreate::trigger()
{
	if (!pDocument)
	{
		return;
	}

	DmBlockTable* blockTable = pDocument->getBlockTable();
	if (!blockTable)
	{
		return;
	}

	DmBlockData blockData = GUIDIALOGFACTORY->requestNewBlockDialog(blockTable);

	if (blockData.name.isEmpty())
	{
		docView->redraw();
		setStatus(getStatus() + 1);  // 清除鼠标按钮提示
		updateMouseButtonHints();
		docView->killSelectActions();
		finish(false);
		return;
	}

	Transaction tg("Create Block", pDocument);
	tg.start();

	// 创建块定义
	DmBlock* block = new DmBlock(pDocument, blockData);

	// 遍历实体表，找选中的实体，克隆到块容器，移除原始实体
	for (auto entity : *pDocument->getEntityTable())
	{
		if (entity && entity->isSelected())
		{
			entity->setSelected(false);
			DmEntity* clonedEntity = entity->clone();
			clonedEntity->move(-*m_referencePoint);
			block->getEntityTable().add_direct(clonedEntity);
			pDocument->getEntityTable()->remove(entity);
		}
	}

	// 通过块表添加块（走命令系统）
	blockTable->add(block);

	// 处理属性（如有）
	std::list<DmAttribute*> attrs;
	if (block->hasAttributeDefinitions())
	{
		std::list<DmAttributeDefinition*> attrDefs =
			block->getAttributeDefinitions();
		GUIDIALOGFACTORY->requestBlockEditAttributeDialog(
			block->getName(), attrDefs, attrs);
	}
	for (auto attr : attrs)
	{
		attr->move(*m_referencePoint);
	}

	// 创建块参照并添加到文档
	constexpr double DEFAULT_SCALE = 1.0;
	constexpr double DEFAULT_ROTATION = 0.0;
	constexpr int DEFAULT_COLUMN_COUNT = 1;
	constexpr int DEFAULT_ROW_COUNT = 1;
	DmBlockReferenceData id(blockData.name, *m_referencePoint,
		DmVector(DEFAULT_SCALE, DEFAULT_SCALE), DEFAULT_ROTATION,
		DEFAULT_COLUMN_COUNT, DEFAULT_ROW_COUNT,
		DmVector(0.0, 0.0), nullptr, DM::NoUpdate);
	DmBlockReference* ref = new DmBlockReference(nullptr, id);
	ref->setDocument(pDocument);
	ref->addAttributes(attrs);
	ref->update();
	pDocument->getEntityTable()->add(ref);

	tg.commit();
	pDocument->regenerate();

	docView->redraw();

	setStatus(getStatus() + 1);  // 清除鼠标按钮提示
	updateMouseButtonHints();
	docView->killSelectActions();
	finish(false);
}

/// @brief 鼠标移动事件处理
/// @param e 鼠标事件指针
void ActionBlocksCreate::mouseMoveEvent(QMouseEvent* e)
{
	snapPoint(e);

	switch (getStatus())
	{
	case eSetReferencePoint:
		break;

	default:
		break;
	}
}

/// @brief 鼠标释放事件处理
/// @param e 鼠标事件指针
void ActionBlocksCreate::mouseReleaseEvent(QMouseEvent* e)
{
	if (e->button() == Qt::LeftButton)
	{
		GuiCoordinateEvent ce(snapPoint(e));
		coordinateEvent(&ce);
	}
	else if (e->button() == Qt::RightButton)
	{
		init(getStatus() - 1);
	}
}

/// @brief 坐标事件处理
/// @param e 坐标事件指针
void ActionBlocksCreate::coordinateEvent(GuiCoordinateEvent* e)
{
	if (!e)
	{
		return;
	}

	switch (getStatus())
	{
	case eSetReferencePoint:
		*m_referencePoint = e->getCoordinate();
		trigger();
		break;
	default:
		break;
	}
}

/// @brief 更新鼠标按钮提示
void ActionBlocksCreate::updateMouseButtonHints()
{
	switch (getStatus())
	{
	case eSetReferencePoint:
		GUIDIALOGFACTORY->updateMouseWidget(
			tr("Specify reference point"), tr("Cancel"));
		break;
	default:
		GUIDIALOGFACTORY->updateMouseWidget();
		break;
	}
}

/// @brief 更新鼠标光标
void ActionBlocksCreate::updateMouseCursor()
{
	docView->setMouseCursor(DM::CadCursor);
}

// 文件结束
