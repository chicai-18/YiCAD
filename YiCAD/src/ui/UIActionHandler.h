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

/// @file UIActionHandler.h
/// @brief 操作处理器，负责触发菜单、按钮等CAD操作命令

#ifndef UIACTIONHANDLER_H
#define UIACTIONHANDLER_H

#include "ActionInterface.h"
#include "MDIWindow.h"
#include "QMdiArea"
#include "UITabDrawWidget.h"
#include "UISnapWidget.h"

class UISnapWidget;
class DmLayer;

/// @class UIActionHandler
/// @brief 这个类可以触发操作（菜单、按钮等）
class UIActionHandler : public QObject
{
    Q_OBJECT

public:
    UIActionHandler(QObject* parent);
    virtual ~UIActionHandler() = default;

    ActionInterface* getCurrentAction();
    ActionInterface* setCurrentAction(DM::ActionType id);

    void setSnapToolBar(UISnapWidget* toolbar);
    void setMDIWindow(MDIWindow* m);
    void setMdiArea(QMdiArea* m);
    void setUITabDrawWidget(UITabDrawWidget* tabDrawWidget);

    /// @brief Kills all running selection actions. Called when a selection action is launched to reduce confusion.
    void killSelectActions();
    /// @brief killAllActions kill all actions
    void killAllActions();

    bool keycode(const QString& code);
    // special handling of actions issued from command line, currently used for snap actions
    // return true if handled
    bool commandLineActions(DM::ActionType id);
    bool command(const QString& cmd);
    QStringList getAvailableCommands();
    SnapMode getSnaps();
    DM::SnapRestriction getSnapRestriction();
    void set_view(GuiDocumentView* pDocumentView);
    void set_document(DmDocument* document);
    void redrawAll();
    void updateGrids();

public slots:
    void slotFileNew();
    void slotFileOpen();
    void slotFileSave();
    void slotFileSaveAs();

    void slotFileExportImage();

    void slotZoomIn();
    void slotZoomOut();
    void slotZoomPan();

    void slotEditKillAllActions();
    void slotEditUndo();
    void slotEditRedo();
    void slotEditCut();
    void slotEditCopy();
    void slotEditPaste();

    void slotDrawPoint();
    void slotDrawLine();
    void slotDrawLineFree();
    void slotDrawLineRectangle();
    void slotDrawLineBisector();
    void slotDrawLineTangent1();
    void slotDrawLineTangent2();
    void slotDrawLineOrthTan();
    void slotDrawLinePolygon();
    void slotDrawLinePolygon3();
    void slotDrawCircle();
    void slotDrawCircle2P();
    void slotDrawCircle3P();
    void slotDrawCircleTan2();
    void slotDrawCircleTan3();
    void slotDrawArc();
    void slotDrawArc3P();
    void slotDrawArcTangential();
    void slotDrawEllipseAxis();
    void slotDrawEllipseInscribe();
    void slotDrawRay();
    void slotDrawSpline();
    void slotDrawSplinePoints();
    void slotDrawMText();
    void slotDrawText();
    void slotDrawHatch();
    void slotDrawImage();
    void slotDrawXline();
    void slotDrawPolyline();
    void slotPolylineAdd();
    void slotPolylineAppend();
    void slotPolylineDel();
    void slotCloudLineRectangle();
    void slotCloudLinePolygon();
    void slotCloudLineFree();

    void slotDimAligned();
    void slotDimLinear();
    void slotDimRadial();
    void slotDimDiametric();
    void slotDimAngular();
    void slotDimLeader();
    void slotDimBaseline();
    void slotDimStyle();
    void slotTextStyle();

    void slotModifyDelete();
    void slotModifyDeleteNoSelect();     // 已在Ribbon菜单入口屏蔽此功能
    void slotModifyCopy();
    void slotModifyMove();
    void slotModifyScale();
    void slotModifyRotate();
    void slotModifyMirror();
    void slotModifyEntity();
    void slotModifyTrim();
    void slotModifyExtend();
    void slotModifyCut();
    void slotModifyCut_2P();
    void slotModifyBevel();
    void slotModifyRound();
    void slotModifySingleOffset();
    void slotModifyExplode();

    void slotSetSnaps(SnapMode const& s);
    void slotSnapFree();
    void slotSnapGrid();
    void slotSnapEndpoint();
    void slotSnapOnEntity();
    void slotSnapCenter();
    void slotSnapMiddle();
    void slotSnapIntersection();

    void slotRestrictNothing();
    void slotRestrictOrthogonal();
    void slotRestrictHorizontal();
    void slotRestrictVertical();

    void disableSnaps();
    void disableRestrictions();

    void slotInfoDist();
    void slotInfoAngle();
    void slotInfoTotalLength();
    void slotInfoArea();
    void slotIndoSelected();

    void slotLayersFreeze();
    void slotLayersLock();
    void slotLayersPrint();
    void slotLayersColor();
    void slotLayersActivate();
    void slotLayersDelete();
    void slotLayersDefreezeAll();
    void slotLayersFreezeAll();
    void slotLayersUnlockAll();
    void slotLayersLockAll();
    void slotLayersRename();
    void slotLayersAdd();

    void slotBlocksSave();
    void slotBlocksSaveAs();
    void slotBlocksInsertPrepare();
    void slotBlocksInsert();
    void slotBlocksCreate();
    void slotBlocksDelete();
    void slotBlocksEdit();
    void slotBlocksImport();
    void slotDefineAttributes();
    void slotOptionsGeneral();
    void slotOptionsDrawing();

    void slotViewGrid();
    void slotCopyToLayer();

    void slotSecectedChanged();

    /// @brief 响应撤销/重做后的块编辑状态变化
    void slotCmdStateChanged();

private:
    UISnapWidget*       m_pSnapToolbar = nullptr;
    GuiDocumentView*    m_pView = nullptr;
    DmDocument*         m_pDocument = nullptr;
    MDIWindow*          m_pMdiWin = nullptr;
    QMdiArea*           m_pDrawingArea = nullptr;
    UITabDrawWidget*    m_pTabDrawWidget = nullptr;
};

#endif
