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


/// @file ActionBlocksImport.cpp
/// @brief 从外部文件导入块定义到当前文档的动作类实现文件

#include "ActionBlocksImport.h"

#include <QApplication>
#include <QMessageBox>

#include "ApplicationWindow.h"
#include "DmBlock.h"
#include "DmBlockTable.h"
#include "DmDimension.h"
#include "DmDimensionStyle.h"
#include "DmDimensionStyleTable.h"
#include "DmDocument.h"
#include "DmEntityContainer.h"
#include "DmLayer.h"
#include "DmLayerTable.h"
#include "DmLineType.h"
#include "DmLineTypeTable.h"
#include "DmMText.h"
#include "DmText.h"
#include "DmTextStyle.h"
#include "DmTextStyleTable.h"
#include "GuiDialogFactory.h"
#include "MDIWindow.h"
#include "Transaction.h"
#include "UIFileDialog.h"


/// @brief 构造函数
/// @param doc 文档指针
/// @param docView 文档视图指针
ActionBlocksImport::ActionBlocksImport(DmDocument* doc,
	GuiDocumentView* docView)
	: ActionInterface("Block Import", doc, docView)
{
}

/// @brief 初始化动作
/// @param status 状态参数，默认为0
void ActionBlocksImport::init(int status)
{
	ActionInterface::init(status);
	trigger();
}

/// @brief 触发动作执行
void ActionBlocksImport::trigger()
{
	ApplicationWindow* appWindow = ApplicationWindow::getAppWindow();
	if (!appWindow)
	{
		finish(false);
		return;
	}

	// 1. 打开文件对话框
	UIFileDialog dlg(appWindow->getMDIWindow(), Qt::WindowFlags(),
		UIFileDialog::BlockFile);
	QString const& fn = dlg.getOpenFile();
	if (fn.isEmpty())
	{
		finish(false);
		return;
	}

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	// 2. 创建临时文档并加载文件（复用已有 filter 系统）
	DmDocument tempDoc;
	bool ok = tempDoc.open(fn);
	if (!ok)
	{
		QApplication::restoreOverrideCursor();
		QMessageBox::warning(appWindow, tr("Import Block"),
			tr("Failed to open file: %1").arg(fn));
		finish(false);
		return;
	}

	DmBlockTable* srcTable = tempDoc.getBlockTable();
	DmBlockTable* dstTable = pDocument->getBlockTable();

	if (srcTable->count() == 0)
	{
		QApplication::restoreOverrideCursor();
		QMessageBox::information(appWindow, tr("Import Block"),
			tr("No blocks found in file."));
		finish(false);
		return;
	}

	// 3. Transaction 包裹
	Transaction t("Import Block", pDocument);
	t.start();

	// 3a. 导入线型（在图层之前，因为图层的画笔可能引用线型）
	DmLineTypeTable* srcLineTypeTable = tempDoc.getLineTypeTable();
	DmLineTypeTable* dstLineTypeTable = pDocument->getLineTypeTable();
	for (auto it = srcLineTypeTable->begin(); it != srcLineTypeTable->end(); ++it)
	{
		DmLineType* src = *it;
		if (!dstLineTypeTable->find(src->getLineTypeName()))
		{
			dstLineTypeTable->add(new DmLineType(src));
		}
	}

	// 3b. 导入图层：名字已存在则不覆盖
	DmLayerTable* srcLayerTable = tempDoc.getLayerTable();
	DmLayerTable* dstLayerTable = pDocument->getLayerTable();
	for (auto it = srcLayerTable->begin(); it != srcLayerTable->end(); ++it)
	{
		DmLayer* src = *it;
		if (!dstLayerTable->find(src->getName()))
		{
			DmLayer* clone = src->clone();
			dstLayerTable->add(clone);
		}
	}

	// 3c. 导入文字样式（在标注样式之前，因为标注样式引用文字样式）
	DmTextStyleTable* srcTextStyleTable = tempDoc.getTextStyleTable();
	DmTextStyleTable* dstTextStyleTable = pDocument->getTextStyleTable();
	for (auto it = srcTextStyleTable->begin(); it != srcTextStyleTable->end(); ++it)
	{
		DmTextStyle* src = *it;
		if (!dstTextStyleTable->find(src->getName()))
		{
			DmTextStyle* clone = src->clone();
			dstTextStyleTable->add(clone);
		}
	}

	// 3d. 导入标注样式，重定向内部的文字样式指针
	DmDimensionStyleTable* srcDimStyleTable = tempDoc.getDimStyleTable();
	DmDimensionStyleTable* dstDimStyleTable = pDocument->getDimStyleTable();
	for (auto it = srcDimStyleTable->begin(); it != srcDimStyleTable->end(); ++it)
	{
		DmDimensionStyle* src = *it;
		if (!dstDimStyleTable->find(src->getName()))
		{
			DmDimensionStyle* clone = new DmDimensionStyle(*src);
			DmTextStyle* srcTextStyle = clone->getDataRef().textStyle();
			if (srcTextStyle)
			{
				DmTextStyle* dstTextStyle = dstTextStyleTable->find(srcTextStyle->getName());
				if (dstTextStyle)
				{
					clone->getDataRef().setTextStyle(dstTextStyle);
				}
			}
			dstDimStyleTable->add(clone);
		}
	}

	// 3e. 导入块定义，重定向实体的图层/样式引用
	for (auto it = srcTable->begin(); it != srcTable->end(); ++it)
	{
		DmBlock* src = *it;
		QString name = src->getName();

		// 重名处理：不导入
		if (dstTable->find(name))
		{
			continue;
		}

		// 创建新块定义
		DmBlockData newData;
		newData.name = name;
		newData.basePoint = src->getBasePoint();
		newData.frozen = src->isFrozen();
		newData.pathName = fn;
		DmBlock* newBlock = new DmBlock(pDocument, newData);

		// 克隆块内所有实体
		auto& srcContainer = src->getEntityTable();
		auto& dstContainer = newBlock->getEntityTable();
		for (auto entity : srcContainer)
		{
			DmEntity* clone = entity->clone();

			// 保存原始图层和画笔（setDocument 会覆盖为活动图层/画笔）
			QString savedLayerName;
			DmLayer* srcLayer = clone->getLayer(false);
			if (srcLayer)
			{
				savedLayerName = srcLayer->getName();
			}
			DmPen savedPen = clone->getPen(false);

			clone->setDocument(pDocument);

			// 恢复图层
			if (!savedLayerName.isEmpty())
			{
				DmLayer* dstLayer = dstLayerTable->find(savedLayerName);
				if (dstLayer)
				{
					clone->setLayer(dstLayer);
				}
			}

			// 恢复画笔
			clone->setPen(savedPen);

			// 递归重定向文字样式、标注样式等指针
			repointEntityStyles(clone);

			dstContainer.add_direct(clone);
		}

		// 通过命令系统添加（支持 undo/redo）
		dstTable->add(newBlock);
	}

	t.commit();

	QApplication::restoreOverrideCursor();

	pDocument->regenerate();
	finish(false);
}

/// @brief 递归重定向实体及其子实体的图层、文字样式、标注样式到当前文档
/// @param entity 待处理的实体
void ActionBlocksImport::repointEntityStyles(DmEntity* entity)
{
	if (!entity) return;

	// 重定向图层（子实体仍持有临时文档的图层指针）
	DmLayer* currentLayer = entity->getLayer(false);
	if (currentLayer)
	{
		DmLayer* newLayer = pDocument->getLayerTable()->find(currentLayer->getName());
		if (newLayer && newLayer != currentLayer)
		{
			entity->setLayer(newLayer);
		}
	}

	// 重定向画笔的线型指针
	DmPen currentPen = entity->getPen(false);
	DmLineType* currentLineType = currentPen.getLineType();
	if (currentLineType
		&& currentLineType != DmLineTypeTable::ByLayer
		&& currentLineType != DmLineTypeTable::ByBlock)
	{
		DmLineType* newLineType = pDocument->getLineTypeTable()->find(currentLineType->getLineTypeName());
		if (newLineType && newLineType != currentLineType)
		{
			currentPen.setLineType(newLineType);
			entity->setPen(currentPen);
		}
	}

	// 重定向 DmText 的文字样式
	if (auto* text = dynamic_cast<DmText*>(entity))
	{
		DmTextStyle* srcStyle = text->getStyle();
		if (srcStyle)
		{
			DmTextStyle* dstStyle = pDocument->getTextStyleTable()->find(srcStyle->getName());
			if (dstStyle && dstStyle != srcStyle)
			{
				text->setStyle(dstStyle);
			}
		}
	}

	// 重定向 DmMText 的文字样式
	if (auto* mtext = dynamic_cast<DmMText*>(entity))
	{
		DmTextStyle* srcStyle = mtext->getStyle();
		if (srcStyle)
		{
			DmTextStyle* dstStyle = pDocument->getTextStyleTable()->find(srcStyle->getName());
			if (dstStyle && dstStyle != srcStyle)
			{
				mtext->setStyle(dstStyle);
			}
		}
	}

	// 重定向 DmDimension 的标注样式
	if (auto* dim = dynamic_cast<DmDimension*>(entity))
	{
		DmDimensionStyle* srcDimStyle = dim->getStyle();
		if (srcDimStyle)
		{
			DmDimensionStyle* dstDimStyle = pDocument->getDimStyleTable()->find(srcDimStyle->getName());
			if (dstDimStyle && dstDimStyle != srcDimStyle)
			{
				dim->getDataRef().pDimStyle = dstDimStyle;
			}
		}
	}

	// 递归处理子实体
	for (auto* sub : entity->getSubEntities())
	{
		repointEntityStyles(sub);
	}
}
