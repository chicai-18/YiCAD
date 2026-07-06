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
#include <QMessageBox>
#include "UIActionHandler.h"
#include "UISnapWidget.h"
#include "GuiDialogFactory.h"
#include "GuiCommandEvent.h"
#include "Commands.h"

#include <utility>

#include "ActionBlocksCreate.h"
#include "ActionBlocksDelete.h"
#include "ActionBlocksEdit.h"
#include "ActionBlocksImport.h"
#include "ActionBlocksSave.h"
#include "ActionBlocksSaveAs.h"
#include "ActionModifyExplode.h"
#include "ActionModifyReverse.h"
#include "ActionBlockInsertPrepare.h"
#include "ActionBlocksInsert.h"
#include "ActionDefineAttributes.h"
#include "ActionDimAligned.h"
#include "ActionDimAngular.h"
#include "ActionDimDiametric.h"
#include "ActionDimLeader.h"
#include "ActionDimLinear.h"
#include "ActionDimRadial.h"
#include "ActionDimBaseline.h"
#include "ActionDimStyle.h"
#include "ActionDrawArc.h"
#include "ActionDrawArc3p.h"
#include "ActionDrawArcTangential.h"
#include "ActionDrawCircle.h"
#include "ActionDrawCircle2p.h"
#include "ActionDrawCircle3p.h"
#include "ActionDrawCircleTan2.h"
#include "ActionDrawCircleTan3.h"
#include "ActionDrawCloudLineRectangle.h"
#include "ActionDrawCloudLinePolygon.h"
#include "ActionDrawCloudLineFree.h"
#include "ActionDrawEllipseAxis.h"
#include "ActionDrawEllipseInscribe.h"
#include "ActionDrawHatch.h"
#include "ActionDrawImage.h"
#include "ActionDrawLine.h"
#include "ActionDrawLineBisector.h"
#include "ActionDrawLineFree.h"
#include "ActionDrawLinePolygon.h"
#include "ActionDrawLinePolygon3.h"
#include "ActionDrawLineRectangle.h"
#include "ActionDrawLineOrthTan.h"
#include "ActionDrawLineTangent1.h"
#include "ActionDrawLineTangent2.h"
#include "ActionDrawMText.h"
#include "ActionDrawPoint.h"
#include "ActionDrawSpline.h"
#include "ActionDrawSplinePoints.h"
#include "ActionDrawText.h"
#include "ActionTextStyle.h"
#include "ActionDrawRay.h"
#include "ActionDrawXline.h"
#include "ActionEditCopy.h"
#include "ActionEditPaste.h"
#include "ActionEditUndo.h"

#include "ActionFileExportImage.h"
#include "ActionFileNew.h"
#include "ActionFileOpen.h"
#include "ActionFileSave.h"
#include "ActionFileSaveAs.h"
#include "ActionInfoAngle.h"
#include "ActionInfoArea.h"
#include "ActionInfoDist.h"
#include "ActionInfoTotalLength.h"
#include "ActionInfoSelected.h"
#include "ActionLayersFreeze.h"
#include "ActionLayersLock.h"
#include "ActionLayersPrint.h"
#include "ActionLayersColor.h"
#include "ActionLayersActivate.h"
#include "ActionLayersDelete.h"
#include "ActionLayersAdd.h"
#include "ActionLayersFreezeAll.h"
#include "ActionLayersLockAll.h"
#include "ActionLayersRename.h"
#include "ActionModifyBevel.h"
#include "ActionModifyCut.h"
#include "ActionModifyDelete.h"
#include "ActionModifyEntity.h"
#include "ActionModifyMirror.h"
#include "ActionModifyCopy.h"
#include "ActionModifyMove.h"
#include "ActionModifyRotate.h"
#include "ActionModifyRound.h"
#include "ActionModifySingleOffset.h"
#include "ActionModifyScale.h"
#include "ActionModifyTrim.h"
#include "ActionOptionsGeneral.h"
#include "ActionOptionsDrawing.h"
#include "ActionSelect.h"
#include "ActionSelectSingle.h"
#include "ActionSetSnapMode.h"
#include "ActionSetSnapRestriction.h"
#include "ActionZoomIn.h"
#include "ActionZoomPan.h"

#include "ActionDrawPolyline.h"
#include "ActionPolylineAdd.h"
#include "ActionPolylineAppend.h"
#include "ActionPolylineDel.h"
#include "Selection.h"

#include "ActionSelectedChanged.h"

#include "Debug.h"
#include "DmLayer.h"
#include "DmSettings.h"
#include "MDIWindow.h"
#include "QMdiArea"
#include "GuiDocumentView.h"
#include "GuiEventHandler.h"
#include "UICurrentActivePen.h"
#include "ActionModifyCut2P.h"
#include "ActionModifyExtend.h"
#include "ActionCopyToLayer.h"

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
	ActionInterface* a = NULL;
	switch (id)
	{
	case DM::ActionFileNew:
		a = new ActionFileNew(m_pDocument, m_pView);
		break;
	case DM::ActionFileOpen:
		a = new ActionFileOpen(m_pDocument, m_pView);
		break;
	case DM::ActionFileSave:
		a = new ActionFileSave(m_pDocument, m_pView);
		break;
	case DM::ActionFileSaveAs:
		a = new ActionFileSaveAs(m_pDocument, m_pView);
		break;

	case DM::ActionFileExportImage:
		a = new ActionFileExportImage(m_pDocument, m_pView);
		break;

		// Editing actions:
		//
	case DM::ActionEditKillAllActions:
		if (m_pView)
		{
			// DO we need to call some form of a 'clean' function?
			m_pView->killAllActions();

			Selection s(m_pDocument, m_pView);
			s.selectAll(false);
			GUIDIALOGFACTORY->updateSelectionWidget(m_pDocument->getEntityTable()->countSelect());
		}
		break;
	case DM::ActionEditUndo:
		a = new ActionEditUndo(true, m_pDocument, m_pView);
		break;
	case DM::ActionEditRedo:
		a = new ActionEditUndo(false, m_pDocument, m_pView);
		break;
	case DM::ActionEditCut:
		if (!m_pDocument->getEntityTable()->hasSelect())
		{
			a = new ActionSelect(this, m_pDocument, m_pView, DM::ActionEditCutNoSelect);
			break;
		}
		// fall-through
	case DM::ActionEditCutNoSelect:
		a = new ActionEditCopy(false, m_pDocument, m_pView);
		break;
	case DM::ActionEditCopy:
		if (!m_pDocument->getEntityTable()->hasSelect())
		{
			a = new ActionSelect(this, m_pDocument, m_pView, DM::ActionEditCopyNoSelect);
			break;
		}
		// fall-through
	case DM::ActionEditCopyNoSelect:
		a = new ActionEditCopy(true, m_pDocument, m_pView);
		break;
	case DM::ActionEditPaste:
		a = new ActionEditPaste(m_pDocument, m_pView);
		break;

		// Selecting actions:
		//
	case DM::ActionSelectSingle:
		if (getCurrentAction()->getEntityType() != DM::ActionSelectSingle)
		{
			a = new ActionSelectSingle(m_pDocument, m_pView, getCurrentAction());
		}
		else
		{
			a = NULL;
		}
		break;

		// Zooming actions:
		//
	case DM::ActionZoomIn:
		a = new ActionZoomIn(m_pDocument, m_pView, DM::In, DM::Both);
		break;
	case DM::ActionZoomOut:
		a = new ActionZoomIn(m_pDocument, m_pView, DM::Out, DM::Both);
		break;
	case DM::ActionZoomPan:
		a = new ActionZoomPan(m_pDocument, m_pView);
		break;

		// Drawing actions:
		//
	case DM::ActionDrawPoint:
		a = new ActionDrawPoint(m_pDocument, m_pView);
		break;
	case DM::ActionDrawLine:
		a = new ActionDrawLine(m_pDocument, m_pView);
		break;
	case DM::ActionDrawLineFree:
		a = new ActionDrawLineFree(m_pDocument, m_pView);
		break;
	case DM::ActionDrawLineRectangle:
		a = new ActionDrawLineRectangle(m_pDocument, m_pView);
		break;
	case DM::ActionDrawLineBisector:
		a = new ActionDrawLineBisector(m_pDocument, m_pView);
		break;
	case DM::ActionDrawLineOrthTan:
		a = new ActionDrawLineOrthTan(m_pDocument, m_pView);
		break;
	case DM::ActionDrawLineTangent1:
		a = new ActionDrawLineTangent1(m_pDocument, m_pView);
		break;
	case DM::ActionDrawLineTangent2:
		a = new ActionDrawLineTangent2(m_pDocument, m_pView);
		break;
	case DM::ActionDrawPolyline:
		a = new ActionDrawPolyline(m_pDocument, m_pView);
		break;
	case DM::ActionPolylineAdd:
		a = new ActionPolylineAdd(m_pDocument, m_pView);
		break;
	case DM::ActionPolylineAppend:
		a = new ActionPolylineAppend(m_pDocument, m_pView);
		break;
	case DM::ActionPolylineDel:
		a = new ActionPolylineDel(m_pDocument, m_pView);
		break;
	case DM::ActionCloudLineRectangle:
		a = new ActionDrawCloudLineRectangle(m_pDocument, m_pView);
		break;
	case DM::ActionCloudLinePolygon:
		a = new ActionDrawCloudLinePolygon(m_pDocument, m_pView);
		break;
	case DM::ActionCloudLineFree:
		a = new ActionDrawCloudLineFree(m_pDocument, m_pView);
		break;
	case DM::ActionDrawLinePolygonCenCor:
		a = new ActionDrawLinePolygonCenCor(m_pDocument, m_pView);
		break;
	case DM::ActionDrawLinePolygonCenTan:
		a = new ActionDrawLinePolygonCenTan(m_pDocument, m_pView);
		break;
	case DM::ActionDrawCircle:
		a = new ActionDrawCircle(m_pDocument, m_pView);
		break;
	case DM::ActionDrawCircle2P:
		a = new ActionDrawCircle2P(m_pDocument, m_pView);
		break;
	case DM::ActionDrawCircle3P:
		a = new ActionDrawCircle3P(m_pDocument, m_pView);
		break;
	case DM::ActionDrawCircleTan2:
		a = new ActionDrawCircleTan2(m_pDocument, m_pView);
		break;
	case DM::ActionDrawCircleTan3:
		a = new ActionDrawCircleTan3(m_pDocument, m_pView);
		break;
	case DM::ActionDrawArc:
		a = new ActionDrawArc(m_pDocument, m_pView);
		break;
	case DM::ActionDrawArc3P:
		a = new ActionDrawArc3P(m_pDocument, m_pView);
		break;
	case DM::ActionDrawArcTangential:
		a = new ActionDrawArcTangential(m_pDocument, m_pView);
		break;
	case DM::ActionDrawEllipseAxis:
		a = new ActionDrawEllipseAxis(m_pDocument, m_pView, false);
		a->setActionType(id);
		break;
	case DM::ActionDrawEllipseArcAxis:
		a = new ActionDrawEllipseAxis(m_pDocument, m_pView, true);
		a->setActionType(id);
		break;
	case DM::ActionDrawEllipseInscribe:
		a = new ActionDrawEllipseInscribe(m_pDocument, m_pView);
		break;
	case DM::ActionDrawRay:
		a = new ActionDrawRay(m_pDocument, m_pView);
		break;
	case DM::ActionDrawSpline:
		a = new ActionDrawSpline(m_pDocument, m_pView);
		break;
	case DM::ActionDrawSplinePoints:
		a = new ActionDrawSplinePoints(m_pDocument, m_pView);
		break;
	case DM::ActionDrawMText:
		a = new ActionDrawMText(m_pDocument, m_pView, false);
		break;
	case DM::ActionDrawText:
		a = new ActionDrawText(m_pDocument, m_pView);
		break;
	case DM::ActionTextStyle:
		a = new ActionTextStyle(m_pDocument, m_pView);
		break;
	case DM::ActionDrawHatch:
	case DM::ActionDrawHatchNoSelect:
		a = new ActionDrawHatch(m_pDocument, m_pView, false);
		break;
	case DM::ActionDrawImage:
		a = new ActionDrawImage(m_pDocument, m_pView);
		break;
	case DM::ActionDrawXline:
		a = new ActionDrawXline(m_pDocument, m_pView);
		break;
		// Dimensioning actions:
		//
	case DM::ActionDimAligned:
		a = new ActionDimAligned(m_pDocument, m_pView);
		break;
	case DM::ActionDimLinear:
		a = new ActionDimLinear(m_pDocument, m_pView);
		break;
	case DM::ActionDimRadial:
		a = new ActionDimRadial(m_pDocument, m_pView);
		break;
	case DM::ActionDimDiametric:
		a = new ActionDimDiametric(m_pDocument, m_pView);
		break;
	case DM::ActionDimAngular:
		a = new ActionDimAngular(m_pDocument, m_pView);
		break;
	case DM::ActionDimLeader:
		a = new ActionDimLeader(m_pDocument, m_pView);
		break;
	case DM::ActionDimBaseline:
		a = new ActionDimBaseline(m_pDocument, m_pView);
		break;
	case DM::ActionDimStyle:
		a = new ActionDimStyle(m_pDocument, m_pView);
		break;

		// Modifying actions:
	case DM::ActionModifyDelete:
		a = new ActionSelect(this, m_pDocument, m_pView, DM::ActionModifyDeleteNoSelect);
		break;
	case DM::ActionModifyDeleteNoSelect:
		a = new ActionModifyDelete(m_pDocument, m_pView);
		break;
    case DM::ActionModifyCopy:
        if (!m_pDocument->getEntityTable()->hasSelect())
        {
            a = new ActionSelect(this, m_pDocument, m_pView, DM::ActionModifyCopyNoSelect);
            break;
        }
        // fall-through
    case DM::ActionModifyCopyNoSelect:
        a = new ActionModifyCopy(m_pDocument, m_pView);
        break;
	case DM::ActionModifyMove:
		if (!m_pDocument->getEntityTable()->hasSelect())
		{
			a = new ActionSelect(this, m_pDocument, m_pView, DM::ActionModifyMoveNoSelect);
			break;
		}
		// fall-through
	case DM::ActionModifyMoveNoSelect:
		a = new ActionModifyMove(m_pDocument, m_pView);
		break;
	case DM::ActionModifyRotate:
		if (!m_pDocument->getEntityTable()->hasSelect())
		{
			a = new ActionSelect(this, m_pDocument, m_pView, DM::ActionModifyRotateNoSelect);
			break;
		}
		// fall-through
	case DM::ActionModifyRotateNoSelect:
		a = new ActionModifyRotate(m_pDocument, m_pView);
		break;
	case DM::ActionModifyScale:
		if (!m_pDocument->getEntityTable()->hasSelect())
		{
			a = new ActionSelect(this, m_pDocument, m_pView, DM::ActionModifyScaleNoSelect);
			break;
		}
		// fall-through
	case DM::ActionModifyScaleNoSelect:
		a = new ActionModifyScale(m_pDocument, m_pView);
		break;
	case DM::ActionModifyMirror:
		if (!m_pDocument->getEntityTable()->hasSelect())
		{
			a = new ActionSelect(this, m_pDocument, m_pView, DM::ActionModifyMirrorNoSelect);
			break;
		}
		// fall-through
	case DM::ActionModifyMirrorNoSelect:
		a = new ActionModifyMirror(m_pDocument, m_pView);
		break;
	case DM::ActionModifyEntity:
		a = new ActionModifyEntity(m_pDocument, m_pView);
		break;
	case DM::ActionModifyTrim:
		a = new ActionModifyTrim(m_pDocument, m_pView);
		break;
	case DM::ActionModifyExtend:
		a = new ActionModifyExtend(m_pDocument, m_pView);
		break;
	case DM::ActionModifyCut:
		a = new ActionModifyCut(m_pDocument, m_pView);
		break;
	case DM::ActionModifyCut2P:
		a = new ActionModifyCut2P(m_pDocument, m_pView);
		break;
	case DM::ActionModifyBevel:
		a = new ActionModifyBevel(m_pDocument, m_pView);
		break;
	case DM::ActionModifyRound:
		a = new ActionModifyRound(m_pDocument, m_pView);
		break;
	case DM::ActionModifySingleOffset:
		a = new ActionModifySingleOffset(m_pDocument, m_pView);
		break;
		// Snapping actions:
		//
	case DM::ActionSnapFree:
		slotSnapFree();
		break;
	case DM::ActionSnapCenter:
		slotSnapCenter();
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

		// Snap restriction actions:
		//
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

		// Info actions:
		//
	case DM::ActionInfoDist:
		a = new ActionInfoDist(m_pDocument, m_pView);
		break;
	case DM::ActionInfoAngle:
		a = new ActionInfoAngle(m_pDocument, m_pView);
		break;
	case DM::ActionInfoTotalLength:
		if (!m_pDocument->getEntityTable()->hasSelect())
		{
			a = new ActionSelect(this, m_pDocument, m_pView, DM::ActionInfoTotalLengthNoSelect);
			break;
		}
		// fall-through
	case DM::ActionInfoTotalLengthNoSelect:
		a = new ActionInfoTotalLength(m_pDocument, m_pView);
		break;
	case DM::ActionInfoArea:
		a = new ActionInfoArea(m_pDocument, m_pView);
		break;
    case DM::ActionInfoSelected:
        a = new ActionInfoSelected(m_pDocument, m_pView);
            break;
        break;

		// Layer actions:
		//
	case DM::ActionLayersFreeze:
		a = new ActionLayersFreeze(sender(), m_pDocument, m_pView);
		break;
	case DM::ActionLayersLock:
		a = new ActionLayersLock(sender(), m_pDocument, m_pView);
		break;
	case DM::ActionLayersPrint:
		a = new ActionLayersPrint(sender(), m_pDocument, m_pView);
		break;
	case DM::ActionLayersColor:
		a = new ActionLayersColor(sender(), m_pDocument, m_pView);
		break;
	case DM::ActionLayersActivate:
		a = new ActionLayersActivate(sender(), m_pDocument, m_pView);
		break;
	case DM::ActionLayersDelete:
		a = new ActionLayersDelete(sender(), m_pDocument, m_pView);
		break;
	case DM::ActionLayersDefreezeAll:
		a = new ActionLayersFreezeAll(false, m_pDocument, m_pView);
		break;
	case DM::ActionLayersFreezeAll:
		a = new ActionLayersFreezeAll(true, m_pDocument, m_pView);
		break;
	case DM::ActionLayersUnlockAll:
		a = new ActionLayersLockAll(false, m_pDocument, m_pView);
		break;
	case DM::ActionLayersLockAll:
		a = new ActionLayersLockAll(true, m_pDocument, m_pView);
		break;
	case DM::ActionLayersRename:
		a = new ActionLayersRename(m_pDocument, m_pView);
		break;
	case DM::ActionLayersAdd:
		a = new ActionLayersAdd(m_pDocument, m_pView);
		break;
	case DM::ActionBlocksSaveAs:
		a = new ActionBlocksSaveAs(m_pDocument, m_pView);
		break;
	case DM::ActionBlocksSave:
		a = new ActionBlocksSave(m_pDocument, m_pView);
		break;
	case DM::ActionBlockInsertPrepare:
		a = new ActionBlockInsertPrepare(m_pDocument, m_pView);
		break;
	case DM::ActionBlocksInsert:
		a = new ActionBlocksInsert(m_pDocument, m_pView);
		break;
	case DM::ActionBlocksCreate:
		if (!m_pDocument->getEntityTable()->hasSelect())
		{
			a = new ActionSelect(this, m_pDocument, m_pView, DM::ActionBlocksCreateNoSelect);
			break;
		}
		// fall-through
	case DM::ActionBlocksCreateNoSelect:
		a = new ActionBlocksCreate(m_pDocument, m_pView);
		break;
	case DM::ActionBlocksDelete:
		a = new ActionBlocksDelete(m_pDocument, m_pView);
		break;
	case DM::ActionBlocksEdit:
		if (m_pDocument->getEditingBlock() != nullptr)
		{
			QMessageBox::warning(nullptr,
				tr("Block Edit"),
				tr("Cannot edit block references while already editing a block."));
			break;
		}
		if (!m_pDocument->getEntityTable()->hasSelect())
		{
			a = new ActionSelect(this, m_pDocument, m_pView, DM::ActionBlocksEditNoSelect);
			break;
		}
		// fall-through
	case DM::ActionBlocksEditNoSelect:
		{
			if (m_pDocument->getEditingBlock() != nullptr)
			{
				QMessageBox::warning(nullptr,
					tr("Block Edit"),
					tr("Cannot edit block references while already editing a block."));
				break;
			}
			DmBlockReference* selectedRef = nullptr;
			for (auto e : *m_pDocument->getEntityTable())
			{
				if (e && e->isSelected() && e->getEntityType() == DM::EntityBlockReference)
				{
					selectedRef = static_cast<DmBlockReference*>(e);
					break;
				}
			}
			a = new ActionBlocksEdit(m_pDocument, m_pView, selectedRef);
		}
		break;
	case DM::ActionBlocksImport:
		a = new ActionBlocksImport(m_pDocument, m_pView);
		break;
	case DM::ActionDefineAttributes:
		a = new ActionDefineAttributes(m_pDocument, m_pView);
		break;
	case DM::ActionModifyExplode:
		if (!m_pDocument->getEntityTable()->hasSelect())
		{
			a = new ActionSelect(this, m_pDocument, m_pView, DM::ActionModifyExplodeNoSelect);
			break;
		}
		// fall-through
	case DM::ActionModifyExplodeNoSelect:
		a = new ActionModifyExplode(m_pDocument, m_pView);
		break;
    case DM::ActionModifyReverse:
        if (!m_pDocument->getEntityTable()->hasSelect())
        {
            a = new ActionSelect(this, m_pDocument, m_pView, DM::ActionModifyReverseNoSelect);
            break;
        }
        // fall-through
    case DM::ActionModifyReverseNoSelect:
        a = new ActionModifyReverse(m_pDocument, m_pView);
        break;
	case DM::ActionOptionsGeneral:
		a = new ActionOptionsGeneral(m_pDocument, m_pView);
		break;
	case DM::ActionOptionsDrawing:
		a = new ActionOptionsDrawing(m_pDocument, m_pView);
		break;
	case DM::ActionCopyToLayer:
		if (!m_pDocument->getEntityTable()->hasSelect())
		{
			a = new ActionSelect(this, m_pDocument, m_pView, DM::ActionNoSelectCopyToLayer);
			break;
		}
	case DM::ActionNoSelectCopyToLayer:
		a = new ActionCopyToLayer(m_pDocument, m_pView);
		break;
	case DM::ActionSelectedChanged:
		a = new ActionSelectedChanged(m_pDocument, m_pView);
		break;
	default:
		break;
	}

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
	if (type != DM::ActionNone)
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

	return false;
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
		// command for new action:
		DM::ActionType type = COMMANDS->cmdToAction(cmd);
		if (type != DM::ActionNone) 
		{
			//special handling, currently needed for snap actions
			if (!commandLineActions(type)) 
			{
				//not handled yet
				setCurrentAction(type);
			}
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

void UIActionHandler::slotFileNew()
{
	setCurrentAction(DM::ActionFileNew);
}

void UIActionHandler::slotFileOpen() 
{
	setCurrentAction(DM::ActionFileOpen);
}

void UIActionHandler::slotFileSave()
{
	setCurrentAction(DM::ActionFileSave);
}

void UIActionHandler::slotFileSaveAs() 
{
	setCurrentAction(DM::ActionFileSaveAs);
}



void UIActionHandler::slotFileExportImage()
{
	setCurrentAction(DM::ActionFileExportImage);
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

void UIActionHandler::slotEditRedo() 
{
	setCurrentAction(DM::ActionEditRedo);
}

void UIActionHandler::slotEditCut() 
{
	setCurrentAction(DM::ActionEditCut);
}

void UIActionHandler::slotEditCopy() 
{
	setCurrentAction(DM::ActionEditCopy);
}

void UIActionHandler::slotEditPaste() 
{
	setCurrentAction(DM::ActionEditPaste);
}

void UIActionHandler::slotDrawPoint() 
{
	setCurrentAction(DM::ActionDrawPoint);
}

void UIActionHandler::slotDrawLine() 
{
	setCurrentAction(DM::ActionDrawLine);
}

void UIActionHandler::slotDrawLineFree() 
{
	setCurrentAction(DM::ActionDrawLineFree);
}

void UIActionHandler::slotDrawLineRectangle() 
{
	setCurrentAction(DM::ActionDrawLineRectangle);
}

void UIActionHandler::slotDrawLineBisector() 
{
	setCurrentAction(DM::ActionDrawLineBisector);
}

void UIActionHandler::slotDrawLineTangent1() 
{
	setCurrentAction(DM::ActionDrawLineTangent1);
}

void UIActionHandler::slotDrawLineTangent2() 
{
	setCurrentAction(DM::ActionDrawLineTangent2);
}

void UIActionHandler::slotDrawLineOrthTan() 
{
	setCurrentAction(DM::ActionDrawLineOrthTan);
}

void UIActionHandler::slotDrawPolyline() 
{
	setCurrentAction(DM::ActionDrawPolyline);
}

void UIActionHandler::slotPolylineAdd() 
{
	setCurrentAction(DM::ActionPolylineAdd);
}

void UIActionHandler::slotPolylineAppend() 
{
	setCurrentAction(DM::ActionPolylineAppend);
}

void UIActionHandler::slotPolylineDel() 
{
	setCurrentAction(DM::ActionPolylineDel);
}

void UIActionHandler::slotCloudLineRectangle()
{
	setCurrentAction(DM::ActionCloudLineRectangle);
}

void UIActionHandler::slotCloudLinePolygon()
{
	setCurrentAction(DM::ActionCloudLinePolygon);
}

void UIActionHandler::slotCloudLineFree()
{
	setCurrentAction(DM::ActionCloudLineFree);
}

void UIActionHandler::slotDrawLinePolygon() 
{
	setCurrentAction(DM::ActionDrawLinePolygonCenCor);
}

void UIActionHandler::slotDrawLinePolygon3() 
{
	setCurrentAction(DM::ActionDrawLinePolygonCenTan);
}

void UIActionHandler::slotDrawCircle() 
{
	setCurrentAction(DM::ActionDrawCircle);
}

void UIActionHandler::slotDrawCircle2P() 
{
	setCurrentAction(DM::ActionDrawCircle2P);
}

void UIActionHandler::slotDrawCircle3P() 
{
	setCurrentAction(DM::ActionDrawCircle3P);
}

void UIActionHandler::slotDrawCircleTan2() 
{
	setCurrentAction(DM::ActionDrawCircleTan2);
}
void UIActionHandler::slotDrawCircleTan3() 
{
	setCurrentAction(DM::ActionDrawCircleTan3);
}
void UIActionHandler::slotDrawArc() 
{
	setCurrentAction(DM::ActionDrawArc);
}

void UIActionHandler::slotDrawArc3P() 
{
	setCurrentAction(DM::ActionDrawArc3P);
}

void UIActionHandler::slotDrawArcTangential() 
{
	setCurrentAction(DM::ActionDrawArcTangential);
}

void UIActionHandler::slotDrawEllipseAxis() 
{
	setCurrentAction(DM::ActionDrawEllipseAxis);
}

void UIActionHandler::slotDrawEllipseInscribe() 
{
	setCurrentAction(DM::ActionDrawEllipseInscribe);
}

void UIActionHandler::slotDrawRay()
{
	setCurrentAction(DM::ActionDrawRay);
}

void UIActionHandler::slotDrawXline()
{
	setCurrentAction(DM::ActionDrawXline);
}

void UIActionHandler::slotDrawSpline() 
{
	setCurrentAction(DM::ActionDrawSpline);
}

void UIActionHandler::slotDrawSplinePoints() 
{
	setCurrentAction(DM::ActionDrawSplinePoints);
}

void UIActionHandler::slotDrawMText() 
{
	setCurrentAction(DM::ActionDrawMText);
}

void UIActionHandler::slotDrawText() 
{
	setCurrentAction(DM::ActionDrawText);
}

void UIActionHandler::slotDrawHatch() 
{
	setCurrentAction(DM::ActionDrawHatch);
}

void UIActionHandler::slotDrawImage() 
{
	setCurrentAction(DM::ActionDrawImage);
}

void UIActionHandler::slotDimAligned() 
{
	setCurrentAction(DM::ActionDimAligned);
}

void UIActionHandler::slotDimLinear() 
{
	setCurrentAction(DM::ActionDimLinear);
}

void UIActionHandler::slotDimRadial() 
{
	setCurrentAction(DM::ActionDimRadial);
}

void UIActionHandler::slotDimDiametric() 
{
	setCurrentAction(DM::ActionDimDiametric);
}

void UIActionHandler::slotDimAngular() 
{
	setCurrentAction(DM::ActionDimAngular);
}

void UIActionHandler::slotDimLeader() 
{
	setCurrentAction(DM::ActionDimLeader);
}

void UIActionHandler::slotDimBaseline()
{
	setCurrentAction(DM::ActionDimBaseline);
}

void UIActionHandler::slotDimStyle()
{
	setCurrentAction(DM::ActionDimStyle);
}

void UIActionHandler::slotTextStyle()
{
	setCurrentAction(DM::ActionTextStyle);
}

void UIActionHandler::slotModifyDelete() 
{
	setCurrentAction(DM::ActionModifyDelete);
}

void UIActionHandler::slotModifyDeleteNoSelect()
{
	setCurrentAction(DM::ActionModifyDeleteNoSelect);
}

void UIActionHandler::slotModifyCopy()
{
    setCurrentAction(DM::ActionModifyCopy);
}

void UIActionHandler::slotModifyMove() 
{
	setCurrentAction(DM::ActionModifyMove);
}

void UIActionHandler::slotModifyRotate() 
{
	setCurrentAction(DM::ActionModifyRotate);
}

void UIActionHandler::slotModifyScale() 
{
	setCurrentAction(DM::ActionModifyScale);
}

void UIActionHandler::slotModifyBevel() 
{
	setCurrentAction(DM::ActionModifyBevel);
}

void UIActionHandler::slotModifyRound() 
{
	setCurrentAction(DM::ActionModifyRound);
}

void UIActionHandler::slotModifySingleOffset()
{
	setCurrentAction(DM::ActionModifySingleOffset);
}

void UIActionHandler::slotModifyMirror() 
{
	setCurrentAction(DM::ActionModifyMirror);
}

void UIActionHandler::slotModifyEntity() 
{
	setCurrentAction(DM::ActionModifyEntity);
}

void UIActionHandler::slotModifyTrim() 
{
	setCurrentAction(DM::ActionModifyTrim);
}

void UIActionHandler::slotModifyExtend()
{
	setCurrentAction(DM::ActionModifyExtend);
}

void UIActionHandler::slotModifyCut() 
{
	setCurrentAction(DM::ActionModifyCut);
}

void UIActionHandler::slotModifyCut_2P()
{
	setCurrentAction(DM::ActionModifyCut2P);
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

void UIActionHandler::slotInfoDist() 
{
	setCurrentAction(DM::ActionInfoDist);
}

void UIActionHandler::slotInfoAngle() 
{
	setCurrentAction(DM::ActionInfoAngle);
}

void UIActionHandler::slotInfoTotalLength() 
{
	setCurrentAction(DM::ActionInfoTotalLength);
}

void UIActionHandler::slotInfoArea() 
{
	setCurrentAction(DM::ActionInfoArea);
}

void UIActionHandler::slotIndoSelected()
{
    setCurrentAction(DM::ActionInfoSelected);
}

void UIActionHandler::slotLayersFreeze()
{
	setCurrentAction(DM::ActionLayersFreeze);
}

void UIActionHandler::slotLayersLock()
{
	setCurrentAction(DM::ActionLayersLock);
}

void UIActionHandler::slotLayersPrint()
{
	setCurrentAction(DM::ActionLayersPrint);
}

void UIActionHandler::slotLayersColor()
{
	setCurrentAction(DM::ActionLayersColor);
}

void UIActionHandler::slotLayersActivate()
{
	setCurrentAction(DM::ActionLayersActivate);
}

void UIActionHandler::slotLayersDelete()
{
	setCurrentAction(DM::ActionLayersDelete);
}

void UIActionHandler::slotLayersDefreezeAll()
{
	setCurrentAction(DM::ActionLayersDefreezeAll);
}

void UIActionHandler::slotLayersFreezeAll() 
{
	setCurrentAction(DM::ActionLayersFreezeAll);
}

void UIActionHandler::slotLayersUnlockAll() 
{
	setCurrentAction(DM::ActionLayersUnlockAll);
}

void UIActionHandler::slotLayersLockAll() 
{
	setCurrentAction(DM::ActionLayersLockAll);
}

void UIActionHandler::slotLayersRename()
{
	setCurrentAction(DM::ActionLayersRename);
}

void UIActionHandler::slotLayersAdd() 
{
	setCurrentAction(DM::ActionLayersAdd);
}

void UIActionHandler::slotBlocksSave() 
{
	setCurrentAction(DM::ActionBlocksSave);
}

void UIActionHandler::slotBlocksSaveAs()
{
	setCurrentAction(DM::ActionBlocksSaveAs);
}

void UIActionHandler::slotBlocksInsertPrepare()
{
	setCurrentAction(DM::ActionBlockInsertPrepare);
}

void UIActionHandler::slotBlocksInsert() 
{
	setCurrentAction(DM::ActionBlocksInsert);
}

void UIActionHandler::slotBlocksCreate()
{
	setCurrentAction(DM::ActionBlocksCreate);
}

void UIActionHandler::slotBlocksDelete()
{
	setCurrentAction(DM::ActionBlocksDelete);
}

void UIActionHandler::slotBlocksEdit()
{
	setCurrentAction(DM::ActionBlocksEdit);
}

void UIActionHandler::slotBlocksImport()
{
	setCurrentAction(DM::ActionBlocksImport);
}

void UIActionHandler::slotModifyExplode()
{
	setCurrentAction(DM::ActionModifyExplode);
}

void UIActionHandler::slotDefineAttributes()
{
	setCurrentAction(DM::ActionDefineAttributes);
}

void UIActionHandler::slotOptionsGeneral()
{
	setCurrentAction(DM::ActionOptionsGeneral);
}

void UIActionHandler::slotOptionsDrawing() 
{
	setCurrentAction(DM::ActionOptionsDrawing);
}

void UIActionHandler::slotCopyToLayer()
{
	setCurrentAction(DM::ActionCopyToLayer);
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
