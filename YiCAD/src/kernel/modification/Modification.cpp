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

/// @file Modification.cpp
/// @brief 实体修改操作实现，提供复制、移动、偏移、裁剪、倒角、倒圆、打断等功能

#include "Modification.h"

#include <cmath>
#include <QSet>
#include <functional>
#include <utility>

#include "DmArc.h"
#include "DmCircle.h"
#include "DmEllipse.h"
#include "DmLine.h"
#include "DmSpline.h"
#include "GuiDocumentView.h"
#include "DmClipboard.h"
#include "DmDocument.h"
#include "Information.h"
#include "DmBlockReference.h"
#include "DmBlock.h"
#include "DmPolyline.h"
#include "DmMText.h"
#include "DmMTextParagraph.h"
#include "DmMTextLine.h"
#include "DmChar.h"
#include "DmCharTemplate.h"
#include "DmCharTemplateList.h"
#include "DmFont.h"
#include "DmFontList.h"
#include "DmText.h"
#include "DmLayer.h"
#include "Math2d.h"
#include "GeometryMethods.h"
#include "Debug.h"
#include "GuiDialogFactory.h"
#include "DmEntityContainer.h"
#include "Transaction.h"

// 打断或裁剪时待删除实体颜色（灰色）
static const DmColor REMOVED_ENTITY_COLOR(70, 70, 70);

PasteData::PasteData(DmVector _insertionPoint, double _factor, double _angle, bool _asInsert, const QString& _blockName)
	: insertionPoint(_insertionPoint)
	, factor(_factor)
	, angle(_angle)
	, asInsert(_asInsert)
	, blockName(_blockName)
{
}

Modification::Modification(GuiDocumentView* docView)
{
	this->docView = docView;
	if (docView)
	{
		document = docView->getDocument();
	}
}

void Modification::remove()
{
	if (!document)
	{
		return;
	}

	Transaction t(QObject::tr("Delete Entities").toStdString(), document);
	t.start();
	auto table = document->getEntityTable();
	for (auto it = table->begin(); it != table->end(); ++it)
	{
		if ((*it)->isSelected())
		{
			table->remove((*it)->getId());
		}
	}
	t.commit();
}

/// @brief Copies all selected entities from the given container to the clipboard.
///	Layers and blocks that are needed are also copied if the container is or is part of an DmDocument.
/// @param ref Reference point. The entities will be moved by -ref.
/// @param cut true: cut instead of copying, false: copy
void Modification::copy(const DmVector& ref, const bool cut)
{
	DMCLIPBOARD->clear();
	if (document)
	{
		DMCLIPBOARD->getDocument()->setUnit(document->getUnit());
	}
	else
	{
		DMCLIPBOARD->getDocument()->setUnit(DM::None);
	}

	auto entTable = document->getEntityTable();

	if (cut)
	{
		Transaction t(QObject::tr("Cut").toStdString(), document);
		t.start();
		for (auto e : *entTable)
		{
			if (e && e->isSelected())
			{
				copyEntity(e, ref);
				entTable->remove(e->getId());
			}
		}
		t.commit();
	}
	else
	{
		for (auto e : *entTable)
		{
			if (e && e->isSelected())
			{
				copyEntity(e, ref);
			}
		}
	}
}

/// @brief Copies the given entity from the given container to the clipboard.
///	Layers and blocks that are needed are also copied if the container is or is part of an DmDocument.
/// @param e The entity.
/// @param ref Reference point. The entities will be moved by -ref.
void Modification::copyEntity(DmEntity* e, const DmVector& ref)
{
	if (!e || !e->isSelected())
	{
		return;
	}

	// add entity to clipboard:
	DmEntity* c = e->clone();
	c->move(-ref);

	DMCLIPBOARD->addEntity(c);
	copyLayers(e);
	copyBlocks(e);

	// set layer to the layer clone:
	c->setLayer(e->getLayer()->getName());

	e->setSelected(false);
	if (docView)
	{
		docView->specifyDocumentModified();
		docView->redraw();
	}
}

// Copies all layers of the given entity to the clipboard.
void Modification::copyLayers(DmEntity* e)
{
	if (!e)
	{
		return;
	}

	// add layer(s) of the entity insert can also be into any layer
	DmLayer* l = e->getLayer();
	if (!l)
	{
		return;
	}

	if (!DMCLIPBOARD->hasLayer(l->getName()))
	{
		DMCLIPBOARD->addLayer(l->clone());
	}

	// special handling of inserts:
	if (e->getEntityType() == DM::EntityBlockReference)
	{
		DmBlock* b = ((DmBlockReference*)e)->getBlockForInsert();
		if (!b)
		{
			return;
		}
		for (auto e2 : b->getEntityTable())
		{
			copyLayers(e2);
		}
	}
}

// Copies all blocks of the given entity to the clipboard.
void Modification::copyBlocks(DmEntity* e)
{
	if (!e)
	{
		return;
	}

	// add block of the entity only if it's an insert
	if (e->getEntityType() != DM::EntityBlockReference)
	{
		return;
	}

	DmBlock* b = ((DmBlockReference*)e)->getBlockForInsert();
	if (!b)
	{
		return;
	}
	// add block of an insert
	QString bn = b->getName();
	if (!DMCLIPBOARD->hasBlock(bn))
	{
		DMCLIPBOARD->addBlock((DmBlock*)b->clone());
	}
	// find insert into insert
	for (auto e2 : b->getEntityTable())
	{
		// call copyBlocks only if entity are insert
		if (e2->getEntityType() == DM::EntityBlockReference)
		{
			copyBlocks(e2);
		}
	}
}

//TODO : 重构undo时注释
/// Pastes all entities from the clipboard into the container.
///	Layers and blocks that are needed are also copied if the container is or is part of an DmDocument.
/// @param data Paste data.
/// @param source The source from where to paste. nullptr means the source is the clipboard.
//void Modification::paste(const PasteData& data, DmDocument* source)
//{
//	if (!document)
//	{
//		return;
//	}
//
//	double factor = (DM_TOLERANCE < fabs(data.factor)) ? data.factor : 1.0;
//	DmVector vfactor = DmVector(factor, factor);
//	if (!source)
//	{
//		source = DMCLIPBOARD->getDocument();
//		DM::Unit sourceUnit = source->getUnit();
//		DM::Unit targetUnit = document->getUnit();
//		factor = DmUnits::convert(1.0, sourceUnit, targetUnit);
//		vfactor = DmVector(factor, factor);
//	}
//
//	DmVector ip = data.insertionPoint;
//	DmLayer* l = document->getActiveLayer();
//
//	if (!pasteLayers(source))
//	{
//		return;
//	}
//
//	if (!l)
//	{
//		return;
//	}
//	document->activateLayer(l);
//
//	// hash for renaming duplicated blocks
//	QHash<QString, QString> blocksDict;
//
//	// create block to paste entities as a whole
//	QString name_old = "paste-block";
//	if (data.blockName != nullptr)
//	{
//		name_old = data.blockName;
//	}
//	QString name_new = name_old;
//	if (document->findBlock(name_old))
//	{
//		name_new = document->getBlockTable()->newName(name_old);
//	}
//	blocksDict[name_old] = name_new;
//
//	// create block
//	DmBlockData db = DmBlockData(name_new, DmVector(0.0, 0.0), false);
//	DmBlock* b = new DmBlock(document, db);
//	document->addBlock(b);
//
//	// create insert object for the paste block
//	DmBlockReferenceData di = DmBlockReferenceData(b->getName(), ip, vfactor, data.angle, 1, 1, DmVector(0.0, 0.0));
//	DmBlockReference* i = new DmBlockReference(document->getEntityContainer(), di);
//	i->setLayerToActive();
//	i->setPenToActive();
//	i->setParent(document->getEntityContainer());
//	document->addEntity(i);
//
//	for (auto e : *source->getEntityContainer())
//	{
//		if (!e)
//		{
//			continue;
//		}
//
//		if (e->getEntityType() == DM::EntityBlockReference)
//		{
//			if (!pasteContainer(e, b->getEntityContainer(), blocksDict, DmVector(0.0, 0.0)))
//			{
//				return;
//			}
//			e->setSelected(false);
//		}
//		else
//		{
//			if (!pasteEntity(e, b->getEntityContainer()))
//			{
//				return;
//			}
//			e->setSelected(false);
//		}
//	}
//	i->update();
//	i->setSelected(false);
//
//	// TODO undo: undo 重构标记
//	//DmUndoSection undo(document, handleUndo);
//
//	if (!data.asInsert)
//	{
//		container->setSelected(false);
//		i->setSelected(true);
//		//explode(false);
//		document->removeEntity(i);
//		b->getEntityContainer()->clear();
//		document->removeBlock(b);
//	}
//	else
//	{
//		// TODO undo: undo 重构标记
//		//undo.addUndoable(i);
//	}
//}

//// Create layers in destination document corresponding to entity to be copied
//bool Modification::pasteLayers(DmDocument* source)
//{
//	if (!source)
//	{
//		return false;
//	}
//
//	DmLayerTable* layerTable = source->getLayerTable();
//	for (DmLayer* l : *layerTable)
//	{
//		if (!l)
//		{
//			continue;
//		}
//
//		QString ln = l->getName();
//		if (!document->findLayer(ln))
//		{
//			document->addLayer(l->clone());
//		}
//	}
//	return true;
//}

// TODO: Modification 废弃，块相关代码已注释
// Create inserts and blocks in destination document corresponding to entity to be copied
bool Modification::pasteContainer(DmEntity* entity, DmEntityContainer* container, QHash<QString, QString> blocksDict, DmVector insertionPoint)
{
	//if (!entity || entity->getEntityType() != DM::EntityBlockReference)
	//{
	//	return false;
	//}

	//DmBlockReference* i = (DmBlockReference*)entity;
	//// get block for this insert object
	//DmBlock* ib = i->getBlockForInsert();
	//if (!ib)
	//{
	//	return false;
	//}
	//// get name for this insert object
	//QString name_old = ib->getName();
	//QString name_new = name_old;
	//if (name_old != i->getName())
	//{
	//	return false;
	//}
	//// rename if needed
	//if (document->findBlock(name_old))
	//{
	//	name_new = document->getBlockTable()->newName(name_old);
	//}
	//blocksDict[name_old] = name_new;
	//// make new block in the destination
	//DmBlockData db = DmBlockData(name_new, DmVector(0.0, 0.0), false);
	//DmBlock* bc = new DmBlock(document, db);
	//document->addBlock(bc);
	//// create insert for the new block
	//DmBlockReferenceData di = DmBlockReferenceData(name_new, insertionPoint, DmVector(1.0, 1.0), i->getAngle(), 1, 1, DmVector(0.0, 0.0));
	//DmBlockReference* ic = new DmBlockReference(container, di);
	//ic->setParent(container);
	//container->addEntity(ic);

	//// set the same layer in clone as in source
	//QString ln = entity->getLayer()->getName();
	//DmLayer* l = document->getLayerTable()->find(ln);
	//if (!l)
	//{
	//	return false;
	//}
	//ic->setLayer(l);
	//ic->setPen(entity->getPen(false));

	//// get relative insertion point
	//DmVector ip = DmVector(0.0, 0.0);
	//if (container->getId() != document->getEntityContainer()->getId())
	//{
	//	ip = bc->getBasePoint();
	//}

	//// copy content of block/insert to destination
	//for (auto* e : *i)
	//{
	//	if (!e)
	//	{
	//		continue;
	//	}

	//	if (e->getEntityType() == DM::EntityBlockReference)
	//	{
	//		if (!pasteContainer(e, (DmEntityContainer*)bc, blocksDict, ip))
	//		{
	//			return false;
	//		}
	//	}
	//	else
	//	{
	//		if (!pasteEntity(e, (DmEntityContainer*)bc))
	//		{
	//			return false;
	//		}
	//	}
	//}

	//ic->update();
	//ic->setSelected(false);

	//return true;
	return false;
}

// Paste entity in supplied container
bool Modification::pasteEntity(DmEntity* entity, DmEntityContainer* container)
{
	if (!entity)
	{
		return false;
	}

	// create entity copy to paste
	DmEntity* e = entity->clone();

	// set the same layer in clone as in source
	QString ln = entity->getLayer()->getName();
	DmLayer* l = document->getLayerTable()->find(ln);
	if (!l)
	{
		return false;
	}
	e->setLayer(l);
	e->setPen(entity->getPen(false));

	e->setParent(container);
	container->addEntity(e);
	e->setSelected(false);

	return true;
}

void Modification::move(const DmVector& offset)
{
	Transaction t(QObject::tr("Move").toStdString(), document);
	t.start();
	auto entTable = document->getEntityTable();
	for (auto e : *entTable)
	{
		if (e->isSelected())
		{
			e->setSelected(false);
			entTable->startModify(e);
			e->move(offset);
			// TODO: Modification 废弃，块相关代码已注释
			//if (e->getEntityType() == DM::EntityBlockReference)
			//{
			//    ((DmBlockReference*)e)->update();
			//}
		}
	}
	t.commit();
}

bool Modification::offset(const OffsetData& data)
{
	auto entTable = document->getEntityTable();
	for (int num = 1; num <= data.number || (data.number == 0 && num <= 1); num++)
	{
		for (auto e : *entTable)
		{
			if (e)
			{
				e->setHighlighted(false);

				if (!e->offset(data.coord, num * data.distance))
				{
					if (e->getEntityType() == DM::EntityPolyline)
					{
						return false;
					}
					continue;
				}
				if (data.useCurrentLayer)
				{
					e->setLayerToActive();
				}
				if (data.useCurrentAttributes)
				{
					e->setPenToActive();
				}
				// TODO: Modification 废弃，块相关代码已注释
				//if (e->getEntityType() == DM::EntityBlockReference)
				//{
				//	static_cast<DmBlockReference*>(e)->update();
				//}
				e->setSelected(true);
				if (document)
				{
					document->specifyModifiedEntity(e);
				}
			}
		}
	}

	// TODO undo: undo 重构标记
	//DmUndoSection undo(document, handleUndo); // bundle remove/add entities in one undoCycle
	deselectOriginals(data.number == 0);

	return true;
}

bool Modification::trim(std::vector<DmEntity*>& ents, DmEntity* entBeenCut, const DmVector& mousePt)
{
	//查找被剪切实体是否在ents
	std::vector<DmEntity*> entsCopy = ents;
	std::vector<DmEntity*> remainEnts;
	DmEntity* delEnt = nullptr;
	bool isInEnts = false;	//被剪实体在剪切集中
	auto it = std::find(entsCopy.begin(), entsCopy.end(), const_cast<DmEntity*>(entBeenCut));
	if (it != entsCopy.end())
	{
		isInEnts = true;
		entsCopy.erase(it);
	}

	//试着剪切
	bool res = tryTrim(entsCopy, entBeenCut, mousePt, remainEnts, delEnt);
	if (res)
	{
		//删除的实体（用于显示要被删除的那段实体）要释放
		if (delEnt)
		{
			delete delEnt;
		}
		auto it2 = std::find(ents.begin(), ents.end(), entBeenCut);
		if (it2 != ents.end())
		{
			ents.erase(it2);
		}
		Transaction t(QObject::tr("Trim").toStdString(), document);
		t.start();
		auto table = document->getEntityTable();
		// 圆特殊处理
		if (entBeenCut->getEntityType() == DM::EntityCircle)
		{
			table->remove(entBeenCut);
			for (auto e : remainEnts)
			{
				table->add(e);
			}
		}
		// 不是圆
		else
		{
			//修剪后剩下有实体，第一个用原来的id
			if (remainEnts.size() > 0)
			{
				// 先取消高亮，undo才不会变成变成高亮
				entBeenCut->setHighlighted(false);
				table->startModify(entBeenCut);
				updateEntityData(entBeenCut, remainEnts.front());
				for (auto it3 = remainEnts.begin() + 1; it3 != remainEnts.end(); ++it3)
				{
					table->add(*it3);
				}
				delete remainEnts.front();
			}
			else
			{
				table->remove(entBeenCut);
			}
		}
		t.commit();

		if (isInEnts)
		{
			ents = entsCopy;
		}
		return true;
	}
	else
	{
		return false;
	}
}

bool Modification::tryTrim(const std::vector<DmEntity*>& boundryEnts, DmEntity* cutEntity, const DmVector& mousePt,
	 std::vector<DmEntity*>& remainResults, DmEntity*& deleteEnt)
{
	remainResults.clear();
	deleteEnt = nullptr;
	DmPen pen = cutEntity->getPen();
	DmLayer* layer = cutEntity->getLayer();

	//计算交点
	DmVectorSolutions sol;
	for (auto& ent : boundryEnts)
	{
		if (ent->isContainer() && cutEntity->isContainer())
		{
			DmEntityContainer* container = static_cast<DmEntityContainer*>(ent);
			const DmEntityContainer* toBeCutContainer = static_cast<const DmEntityContainer*>(cutEntity);
			for (auto& subEnt : *container)
			{
				getIntersectionOfContainer(toBeCutContainer, subEnt, sol);
			}
		}
		else if (ent->isContainer())
		{
			DmEntityContainer* container = static_cast<DmEntityContainer*>(ent);
			getIntersectionOfContainer(container, cutEntity, sol);
		}
		else if (cutEntity->isContainer())
		{
			const DmEntityContainer* container = static_cast<const DmEntityContainer*>(cutEntity);
			getIntersectionOfContainer(container, ent, sol);
		}
		else
		{
			DmVectorSolutions subSol = Information::getIntersection(ent, cutEntity, true);
			if (subSol.size() > 0)
			{
				sol.push_back(subSol);
			}
		}
	}

	//去除重复
	sol.distinct();
	if (sol.size() == 0)
	{
		return false;
	}

	//计算投影点
	double maxDist = DM_MAXDOUBLE;
	DmVector mouseOnEnt = cutEntity->getNearestPointOnEntity(mousePt, true, &maxDist, nullptr);

	//用交点切割实体
	std::vector<DmVector> intersectPts = sol.getVector();
	std::vector<DmEntity*> splitEntities;
	DmEntity* cut1 = nullptr;
	DmEntity* cut2 = nullptr;
	DmEntity* cut3 = nullptr;
	// 如果多于2个交点，先用2个点裁剪（因为圆用一个点裁剪没用），再多次用1个点裁剪。
	if (intersectPts.size() >= 2)
	{
		cutEntity2P(intersectPts.at(0), intersectPts.at(1), cutEntity, cut1, cut2, cut3);
		if (cut1)
		{
			splitEntities.emplace_back(cut1);
		}
		if (cut2)
		{
			splitEntities.emplace_back(cut2);
		}
		if (cut3)
		{
			splitEntities.emplace_back(cut3);
		}

		// todo : 此处的复杂度会随交点个数呈级数增长
		for (auto it = intersectPts.begin() + 2; it != intersectPts.end(); ++it)
		{
			for (auto splitIt = splitEntities.begin(); splitIt != splitEntities.end(); ++splitIt)
			{
				if (cutEntity1P(*it, *splitIt, cut1, cut2))
				{
					delete *splitIt;
					*splitIt = cut1;
					splitEntities.insert(splitIt + 1, cut2);
					break;
				}
			}
		}
	}
	// 仅一个交点
	else
	{
		if (cutEntity1P(intersectPts.at(0), cutEntity, cut1, cut2))
		{
			splitEntities.emplace_back(cut1);
			splitEntities.emplace_back(cut2);
		}
	}
	if (splitEntities.size() == 0)
	{
		return false;
	}

	// 判断鼠标在哪个子实体上，判断待删除实体
	std::vector<DmEntity*>::iterator deleteEntIt = splitEntities.begin();
	for (auto it = splitEntities.begin(); it != splitEntities.end(); ++it)
	{
		if ((*it)->isPointOnEntity(mouseOnEnt))
		{
			deleteEnt = *it;
			deleteEntIt = it;
			break;
		}
	}
	if (deleteEnt == nullptr)
	{
		return false;
	}
	// 取出待删除子实体，获得端点，再重新打断
	DmVector delBegin = deleteEnt->getStartpoint();
	DmVector delEnd = deleteEnt->getEndpoint();
	// 释放第一次打断的所有子实体
	for (auto e : splitEntities)
	{
		delete e;
	}
	deleteEnt = nullptr;
	splitEntities.clear();
	// 重新打断
	bool res = cutEntity2P(delBegin, delEnd, cutEntity, cut1, cut2, cut3);
	if (!res)
	{
		return false;
	}
	if (cut1)
	{
		splitEntities.emplace_back(cut1);
	}
	if (cut2)
	{
		splitEntities.emplace_back(cut2);
	}
	if (cut3)
	{
		splitEntities.emplace_back(cut3);
	}
	for (auto e : splitEntities)
	{
		if (e->isPointOnEntity(mouseOnEnt) && deleteEnt == nullptr)
		{
			deleteEnt = e;
		}
		else
		{
			remainResults.emplace_back(e);
		}
	}

	//设置pen
	if (deleteEnt != nullptr)
	{
		deleteEnt->setPen(DmPen(REMOVED_ENTITY_COLOR, pen.getWidth(), pen.getLineType()));
	}
	if (remainResults.size() != 0)
	{
		for (auto& ent : remainResults)
		{
			ent->setPen(pen);
			ent->setLayer(layer);
		}
	}
	return true;
}

///	Deselects all selected entities and removes them if remove is true;
///	@param remove true: Remove entities.
void Modification::deselectOriginals(bool remove)
{
	// TODO undo: undo 重构标记
	//DmUndoSection undo(document, handleUndo);

	auto entTable = document->getEntityTable();
	for (auto e : *entTable)
	{
		if (e)
		{
			bool selected = false;
			if (e->isSelected())
			{
				selected = true;
			}

			if (selected)
			{
				e->setSelected(false);
				if (remove)
				{
					// TODO undo: undo 重构标记
					//e->changeUndoState();
					//undo.addUndoable(e);
				}
			}
		}
	}
}

bool Modification::cut(const DmVector& cutCoord, DmEntity* ent)
{
	if (!ent)
	{
		return false;
	}
	if (!cutCoord.valid)
	{
		return false;
	}
	// 打断
	DmEntity* cut1 = nullptr;
	DmEntity* cut2 = nullptr;
	bool res = cutEntity1P(cutCoord, ent, cut1, cut2);
	if (!res)
	{
		return false;
	}

	Transaction t(QObject::tr("cut entity").toStdString(), document);
	t.start();
	document->getEntityTable()->startModify(ent);
	updateEntityData(ent, cut1);
	if (cut1)
	{
		delete cut1;
		cut1 = nullptr;
	}
	if (cut2)
	{
		document->getEntityTable()->add(cut2);
	}
	t.commit();
	return true;
}

bool Modification::tryCut2P(const DmVector& firstCoord, const DmVector& secondCoord, DmEntity* ent, std::vector<DmEntity*>& remainResults, DmEntity*& deleteEnt)
{
	// TODO :可以再传入一个点，用于判断闭合的样条该取哪一段
	if (!ent)
	{
		return false;
	}
	// 两点太近
	if (secondCoord.distanceTo(firstCoord) < DM_TOLERANCE)
	{
		return false;
	}
	DmEntity* cut1 = nullptr;
	DmEntity* cut2 = nullptr;
	DmEntity* cut3 = nullptr;
	// 两点打断实体，找出剩余的实体及删除的实体
	if (cutEntity2P(firstCoord, secondCoord, ent, cut1, cut2, cut3))
	{
		// 圆或闭合椭圆特殊处理
		bool isSpecialEnt = ent->getEntityType() == DM::EntityCircle ||
				(ent->getEntityType() == DM::EntityEllipse && ((DmEllipse*)ent)->isClosed()) ||
				(ent->getEntityType() == DM::EntitySpline && ((DmSpline*)ent)->isClosed());
		if (isSpecialEnt)
		{
			deleteEnt = cut1;
			deleteEnt->setPen(DmPen(REMOVED_ENTITY_COLOR, ent->getPen(false).getWidth(), ent->getPen(false).getLineType()));
			remainResults.emplace_back(cut2);
		}
		else
		{
			std::array<DmEntity*, 3> cuts{ cut1, cut2, cut3 };
			for (int i = 0; i < 3; i++)
			{
				DmEntity* cutEnt = cuts.at(i);
				if (cutEnt == nullptr)
				{
					continue;
				}
				// 2端点匹配，才是删除的段
				bool isMatchEndPt = cutEnt->isPointOnEntity(firstCoord) && cutEnt->isPointOnEntity(secondCoord);
				if (isMatchEndPt)
				{
					deleteEnt = cutEnt;
					deleteEnt->setPen(DmPen(REMOVED_ENTITY_COLOR, ent->getPen(false).getWidth(), ent->getPen(false).getLineType()));
				}
				else
				{
					remainResults.emplace_back(cutEnt);
				}
			}
		}
		return true;
	}
	return false;
}

bool Modification::cut2P(const DmVector& firstCoord, const DmVector& secondCoord, DmEntity* ent)
{
	std::vector<DmEntity*> remainEnts;
	DmEntity* deleteEnt = nullptr;
	bool res = tryCut2P(firstCoord, secondCoord, ent, remainEnts, deleteEnt);
	if (!res)
	{
		return false;
	}

	// 修改原实体信息为留下的第一个实体，留下的其他实体保存，删除实体释放
	DmEntity* remainEnt1 = remainEnts.at(0);
	Transaction t(QObject::tr("cut entity 2P").toStdString(), document);
	t.start();
	document->getEntityTable()->startModify(ent);
	updateEntityData(ent, remainEnt1);
	for (auto it = remainEnts.begin() + 1; it != remainEnts.end(); ++it)
	{
		document->getEntityTable()->add(*it);
	}
	if (deleteEnt)
	{
		delete deleteEnt;
		deleteEnt = nullptr;
	}
	t.commit();
	return true;
}

bool Modification::isCutableEntity(DmEntity* ent)
{
	if (ent->isLocked() || !ent->isVisible())
	{
		return false;
	}
	switch (ent->getEntityType())
	{
	case DM::EntityCircle:
	case DM::EntityArc:
	case DM::EntityLine:
	case DM::EntityPolyline:
	case DM::EntityEllipse:
	case DM::EntitySpline:
		return true;
	default:
		return false;
	}
}

bool Modification::cutEntity1P(const DmVector& cutCoord, DmEntity* cutEntity, DmEntity*& cut1, DmEntity*& cut2)
{
	if (!isCutableEntity(cutEntity))
	{
		return false;
	}

	// 打断点在端点处
	if (cutCoord.distanceTo(cutEntity->getStartpoint()) < DM_TOLERANCE || cutCoord.distanceTo(cutEntity->getEndpoint()) < DM_TOLERANCE)
	{
		return false;
	}

	// 保证打断点在实体上
	if (!cutEntity->isPointOnEntity(cutCoord))
	{
		return false;
	}

	// 打断操作
	cut1 = nullptr;
	cut2 = nullptr;
	double a = 0.0;

	switch (cutEntity->getEntityType())
	{
	case DM::EntityLine:
	{
		DmLine* line = static_cast<DmLine*>(cutEntity);
		cut1 = new DmLine(cutEntity->getParent(), LineData(line->getStartpoint(), cutCoord));
		cut2 = new DmLine(cutEntity->getParent(), LineData(cutCoord, line->getEndpoint()));
	}
	break;
	case DM::EntityArc:
	{
		DmArc* arc = static_cast<DmArc*>(cutEntity);
		bool isClockwise = arc->isClockwise();
		double startAngle = arc->getStartAngleNormal();
		double endAngle = arc->getEndAngleNormal();
		a = arc->getCenter().angleTo(cutCoord);
		cut1 = new DmArc(cutEntity->getParent(), ArcData(arc->getCenter(), DmVector(0.0, 0.0, 1.0), arc->getRadius(), startAngle, a));
		cut2 = new DmArc(cutEntity->getParent(), ArcData(arc->getCenter(), DmVector(0.0, 0.0, 1.0), arc->getRadius(), a, endAngle));
		if (isClockwise)
		{
			((DmArc*)cut1)->setClockwise(isClockwise);
			((DmArc*)cut2)->setClockwise(isClockwise);
		}
	}
	break;
	case DM::EntityEllipse:
	{
		DmEllipse* ellipse = static_cast<DmEllipse*>(cutEntity);
		if (ellipse->isClosed())	//只有椭圆弧支持打断
		{
			return false;
		}
		bool isClockwise = ellipse->isClockwise();
		double angle = ellipse->getCenter().angleTo(cutCoord);
		a = ellipse->getParam(angle);
		double startParam = ellipse->getStartParam();
		double endParam = ellipse->getEndParam();
		cut1 = new DmEllipse(cutEntity->getParent(), EllipseData(ellipse->getCenter(), ellipse->getMajorP(), ellipse->getNormal(), ellipse->getRatio(), false, startParam, a));
		cut2 = new DmEllipse(cutEntity->getParent(), EllipseData(ellipse->getCenter(), ellipse->getMajorP(), ellipse->getNormal(), ellipse->getRatio(), false, a, endParam));
	}
	break;
	case DM::EntityPolyline:
	{
		DmPolyline* poly = static_cast<DmPolyline*>(cutEntity);
		cutForPolyline(cutCoord, poly, cut1, cut2);
	}
	break;
	case DM::EntitySpline:
	{
		DmSpline* spline = static_cast<DmSpline*>(cutEntity);
		DmSpline::cutForSpline(cutCoord, spline, cut1, cut2);
	}
	break;
	default:
		break;
	}

	if (cut1 == nullptr && cut2 == nullptr)
	{
		return false;
	}
	if (cut1)
	{
		cut1->setPen(cutEntity->getPen(false));
		cut1->setLayer(cutEntity->getLayer(false));
	}
	if (cut2)
	{
		cut2->setPen(cutEntity->getPen(false));
		cut2->setLayer(cutEntity->getLayer(false));
	}
	return true;
}

bool Modification::cutEntity2P(const DmVector& firstCoord, const DmVector& secondCoord, DmEntity* ent, DmEntity*& cut1, DmEntity*& cut2, DmEntity*& cut3)
{
	if (!firstCoord.valid || !secondCoord.valid)
	{
		return false;
	}
	if (firstCoord.distanceTo(secondCoord) < DM_TOLERANCE)
	{
		return false;
	}
	if (!isCutableEntity(ent))
	{
		return false;
	}
	// 保证打断点在实体上
	if (!ent->isPointOnEntity(firstCoord) || !ent->isPointOnEntity(secondCoord))
	{
		return false;
	}

	// 打断操作
	cut1 = nullptr;
	cut2 = nullptr;
	cut3 = nullptr;
	std::array<DmEntity*, 4> tempCut;
	tempCut.fill(nullptr);

	// 圆与闭合椭圆需要一次处理
	bool hasCut = false;
	switch (ent->getEntityType())
	{
	case DM::EntityCircle:
	{
		hasCut = true;
		DmCircle* circle = static_cast<DmCircle*>(ent);
		double a1 = circle->getCenter().angleTo(firstCoord);
		double a2 = circle->getCenter().angleTo(secondCoord);
		cut1 = new DmArc(ent->getParent(), ArcData(circle->getCenter(), DmVector(0.0, 0.0, 1.0), circle->getRadius(), a1, a2));
		cut2 = new DmArc(ent->getParent(), ArcData(circle->getCenter(), DmVector(0.0, 0.0, 1.0), circle->getRadius(), a2, a1));
	}
	break;
	case DM::EntityEllipse:
	{
		DmEllipse* ellipse = static_cast<DmEllipse*>(ent);
		if (ellipse->isClosed())
		{
			hasCut = true;
			double a1 = ellipse->getCenter().angleTo(firstCoord);
			double a2 = ellipse->getCenter().angleTo(secondCoord);
			double param1 = ellipse->getParam(a1);
			double param2 = ellipse->getParam(a2);
			cut1 = new DmEllipse(ent->getParent(), EllipseData(ellipse->getCenter(), ellipse->getMajorP(), ellipse->getNormal(), ellipse->getRatio(), false, param1, param2));
			cut2 = new DmEllipse(ent->getParent(), EllipseData(ellipse->getCenter(), ellipse->getMajorP(), ellipse->getNormal(), ellipse->getRatio(), false, param2, param1));
		}
	}
	break;
	case DM::EntitySpline:
	{
		DmSpline* spline = static_cast<DmSpline*>(ent);
		if (spline->isClosed())
		{
			hasCut = true;
			DmSpline::cutForSpline2P_Closed(spline, firstCoord, secondCoord, cut1, cut2);
		}
	}
	break;
	default:
		break;
	}
	if (hasCut)
	{
		if (cut1)
		{
			cut1->setPen(ent->getPen(false));
			cut1->setLayer(ent->getLayer(false));
		}
		if (cut2)
		{
			cut2->setPen(ent->getPen(false));
			cut2->setLayer(ent->getLayer(false));
		}
		return true;
	}

	// 其他情况通过2次单点打断
	if (cutEntity1P(firstCoord, ent, tempCut[0], tempCut[1]))
	{
		if (cutEntity1P(secondCoord, tempCut[0], tempCut[2], tempCut[3]))
		{
			cut1 = tempCut[1];
			cut2 = tempCut[2];
			cut3 = tempCut[3];
		}
		else if (cutEntity1P(secondCoord, tempCut[1], tempCut[2], tempCut[3]))
		{
			cut1 = tempCut[0];
			cut2 = tempCut[2];
			cut3 = tempCut[3];
		}
		else
		{
			cut1 = tempCut[0];
			cut2 = tempCut[1];
		}
		return true;
	}
	else if (cutEntity1P(secondCoord, ent, tempCut[0], tempCut[1]))
	{
		cut1 = tempCut[0];
		cut2 = tempCut[1];
		return true;
	}
	return false;
}

bool Modification::cutForPolyline(const DmVector& cutCoord, DmPolyline* poly, DmEntity*& cut1, DmEntity*& cut2)
{
	// 求与打断点最近的顶点
	DmVector closetVertex(false);
	int closetIdx = -1;
	double minDistSquare = DM_MAXDOUBLE;
	int vertexCount = poly->getVertexCount();
	for (int i = 0; i < vertexCount; i++)
	{
		DmVector v = poly->getVertexAt(i);
		double square = v.squaredTo(cutCoord);
		if (square < minDistSquare)
		{
			closetIdx = i;
			minDistSquare = square;
		}
	}

	std::vector<DmVector> vertices = poly->getDataConstRef().getVertexs();
	std::vector<double> bulges = poly->getDataConstRef().getBulges();
	std::vector<double> weights = poly->getDataConstRef().getLineWeights();
	// 打断点在顶点上
	if (minDistSquare < DM_TOLERANCE2)
	{
		// 已判断不在2个端点上
		std::vector<DmVector> vertices1(vertices.begin(), vertices.begin() + closetIdx + 1);
		std::vector<DmVector> vertices2(vertices.begin() + closetIdx, vertices.end());
		std::vector<double> bulges1(bulges.begin(), bulges.begin() + closetIdx);
		std::vector<double> bulges2(bulges.begin() + closetIdx, bulges.end());
		std::vector<double> weights1(weights.begin(), weights.begin() + bulges1.size() * 2);
		std::vector<double> weights2(weights.begin() + bulges1.size() * 2, weights.end());
		if (poly->isClosed())
		{
			vertices2.emplace_back(vertices.front());
		}
		PolylineData data1(vertices1, bulges1, weights1, false);
		PolylineData data2(vertices2, bulges2, weights2, false);
		cut1 = new DmPolyline(poly->getParent(), data1);
		cut2 = new DmPolyline(poly->getParent(), data2);
		cut1->update();
		cut2->update();
		return true;
	}
	// 打断点不在顶点上
	else
	{
		// 查找最近的段
		int segCount = poly->getSegmentCount();
		double bulge = 0.0;
		double radius = 0.0;
		double startAngle = 0.0;
		double endAngle = 0.0;
		double startWeight = 0.0;
		double endWeight = 0.0;
		DmVector pt1;
		DmVector pt2;
		DmVector center;
		DmVector normal;
		std::unique_ptr<DmEntity> segment;
		int cutSegmentIdx = -1;
		for (int i = 0; i < segCount; i++)
		{
			poly->getSegmentInfoAt(i, bulge, pt1, pt2, &startWeight, &endWeight);
			if (bulge == 0.0)
			{
				DmLine line(pt1, pt2);
				bool isOnEntity = line.isPointOnEntity(cutCoord, DM_TOLERANCE);
				if (isOnEntity)
				{
					segment.reset(line.clone());
					cutSegmentIdx = i;
					break;
				}
			}
			else
			{
				GeometryMethods::getArcInfo(pt1, pt2, bulge, center, radius, startAngle, endAngle, normal);
				DmArc arc(nullptr, ArcData(center, normal, radius, startAngle, endAngle));
				bool isOnEntity = arc.isPointOnEntity(cutCoord, DM_TOLERANCE);
				if (isOnEntity)
				{
					segment.reset(arc.clone());
					cutSegmentIdx = i;
					break;
				}
			}
		}

		// 未找到最近的段
		if (segment.get() == nullptr)
		{
			return false;
		}
		// 计算由打断点产生的新圆弧（直线）的凸度，线宽。
		double cutBulge1 = 0.0;
		double cutBulge2 = 0.0;
		double cutWeight = 0.0;
		if (segment->getEntityType() == DM::EntityArc)
		{
			DmArc* arc = static_cast<DmArc*>(segment.get());
			double cutAngle = arc->getCenter().angleTo(cutCoord);	//打断点的方位角
			if (arc->isClockwise())
			{
				cutAngle = Math2d::correctAngle(M_PI - cutAngle);	//转为与arc法向一致的角度
			}
			double angleToStart = Math2d::correctAngle(cutAngle - arc->getStartAngle());
			double angleToEnd = Math2d::correctAngle(arc->getEndAngle() - cutAngle);
			cutBulge1 = std::tan(angleToStart / 4.0);
			cutBulge2 = std::tan(angleToEnd / 4.0);
			if (arc->isClockwise())
			{
				cutBulge1 = -cutBulge1;
				cutBulge2 = -cutBulge2;
			}
			double factor = angleToStart / arc->getAngleLength();
			cutWeight = startWeight + factor * (endWeight - startWeight);
		}
		else
		{
			DmLine* line = static_cast<DmLine*>(segment.get());
			cutBulge1 = 0.0;
			cutBulge2 = 0.0;
			double distToStart = line->getStartpoint().distanceTo(cutCoord);
			double factor = distToStart / line->getLength();
			cutWeight = startWeight + factor * (endWeight - startWeight);
		}

		// 创建子实体
		std::vector<DmVector> vertexes1(vertices.begin(), vertices.begin() + cutSegmentIdx + 1);
		vertexes1.emplace_back(cutCoord);
		std::vector<double> bulges1(bulges.begin(), bulges.begin() + cutSegmentIdx);
		bulges1.emplace_back(cutBulge1);
		std::vector<double> weight1(weights.begin(), weights.begin() + cutSegmentIdx * 2);
		double tempStartWeight = 0.0;
		double tempEndWeight = 0.0;
		poly->getDataConstRef().getLineWeightsAt(cutSegmentIdx, tempStartWeight, tempEndWeight);
		weight1.emplace_back(tempStartWeight);
		weight1.emplace_back(cutWeight);
		cut1 = new DmPolyline(poly->getParent(), PolylineData(vertexes1, bulges1, weight1, false));
		cut1->update();

		std::vector<DmVector> vertexes2(vertices.begin() + cutSegmentIdx + 1, vertices.end());
		vertexes2.insert(vertexes2.begin(), cutCoord);
		if (poly->isClosed())
		{
			vertexes2.emplace_back(vertices.front());
		}
		std::vector<double> bulges2(bulges.begin() + cutSegmentIdx + 1, bulges.end());
		bulges2.insert(bulges2.begin(), cutBulge2);
		std::vector<double> weight2(weights.begin() + (cutSegmentIdx + 1) * 2, weights.end());
		weight2.insert(weight2.begin(), { cutWeight, tempEndWeight });
		cut2 = new DmPolyline(poly->getParent(), PolylineData(vertexes2, bulges2, weight2, false));
		cut2->update();
		return true;
	}
}

void Modification::updateEntityData(DmEntity* ent, DmEntity* entDataCopyFrom)
{
	switch (ent->getEntityType())
	{
		case DM::EntityLine:
		{
			DmLine* newEnt1 = static_cast<DmLine*>(entDataCopyFrom);
			DmLine* originEnt = static_cast<DmLine*>(ent);
			originEnt->setData(newEnt1->getData());
			originEnt->update();
		}
			break;
		case DM::EntityArc:
		{
			DmArc* newEnt1 = static_cast<DmArc*>(entDataCopyFrom);
			DmArc* originEnt = static_cast<DmArc*>(ent);
			originEnt->setData(newEnt1->getData());
			originEnt->update();
		}
			break;
		case DM::EntityEllipse:
		{
			DmEllipse* newEnt1 = static_cast<DmEllipse*>(entDataCopyFrom);
			DmEllipse* originEnt = static_cast<DmEllipse*>(ent);
			originEnt->setData(newEnt1->getData());
			originEnt->update();
		}
			break;
		case DM::EntityPolyline:
		{
			DmPolyline* newEnt1 = static_cast<DmPolyline*>(entDataCopyFrom);
			DmPolyline* originEnt = static_cast<DmPolyline*>(ent);
			originEnt->setData(newEnt1->getData());
			originEnt->update();
		}
			break;
		case DM::EntitySpline:
		{
			DmSpline* newEnt1 = static_cast<DmSpline*>(entDataCopyFrom);
			DmSpline* originEnt = static_cast<DmSpline*>(ent);
			originEnt->setData(newEnt1->getData());
			originEnt->update();
		}
			break;
		default:
			break;
	}
}

void Modification::getIntersectionOfContainer(const DmEntityContainer* container, const DmEntity* ent, DmVectorSolutions& sol)
{
	for (auto& subEnt : *container)
	{
		DmVectorSolutions subSol = Information::getIntersection(subEnt, ent, true);
		if (subSol.size() > 0)
		{
			sol.push_back(subSol);
		}
	}
}

bool Modification::moveRef(MoveRefData& data)
{
	Transaction t(QObject::tr("move ref").toStdString(), document);
	t.start();
	auto entTable = document->getEntityTable();
	for (auto e : *entTable)
	{
		if (e && e->isSelected())
		{
			entTable->startModify(e);
			e->moveRef(data.ref, data.offset);
			e->setSelected(true);
		}
	}
	t.commit();
	return true;
}
