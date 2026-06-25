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

/// @file UIDialogFactory.h
/// @brief 对话框工厂，负责创建和显示各类CAD对话框

#ifndef UIDIALOGFACTORY_H
#define UIDIALOGFACTORY_H
#include <list>
#include "GuiDialogFactoryInterface.h"
#include "UIActionHandler.h"

class UIPolylineEquidistantOptions;
class UISnapMiddleOptions;
class UISnapDistOptions;
class UIModifyOffsetOptions;
class QWidget;

class QToolBar;
class UIBottomWindow;
class UIArcTangentialOptions;
class UIPrintPreviewOptions;
class UICommandWidget;		
class DmDocument;
class UILineAngleOptions;
class DmVector;
class DmTextStyleTable;
class UIDlgDimensionStyleMgr;
class UIDlgDimensionStyle;
class UIDlgTextStyle;
class DmAttributeDefinition;
class DmViewport;
class DmViewportTable;
class DmTableStyle;
class DmTable;
class UIDlgTableStyle;
class UIDlgLineType;
class UIDlgTableStyleMgr;

#define UIDIALOGFACTORY (GuiDialogFactory::instance()->getFactoryObject())

// This is the Qt implementation of a widget which can create and show dialogs.
class UIDialogFactory: public GuiDialogFactoryInterface
{
public:
	UIDialogFactory(QWidget* parent, QWidget* ow);
	~UIDialogFactory() override;
public:
	// Links this dialog factory to a coordinate widget.
	void setBottomWidget(UIBottomWindow* bw) override;

	// Links this dialog factory to a command widget.
	void setCommandWidget(UICommandWidget* command) override;

	void setActionHandle(UIActionHandler* handle);
	void requestWarningDialog(const QString& warning) override;

	DmLayer* requestNewLayerDialog(DmLayerTable* layerTable = nullptr) override;
	DmLayer* requestEditLayerDialog(DmLayerTable* layerTable = nullptr) override;

	DmBlockData requestNewBlockDialog(DmBlockTable* blockTable) override;

	/// @brief 对于“属性块”，创建块时提示设置属性定义
	bool requestBlockEditAttributeDialog(const QString& blkName, const std::list<DmAttributeDefinition*>& attrDefs, std::list< DmAttribute*>& attrs) override;
	bool requestDefineAttributesDialog(DmAttributeDefinition* attrDef) override;

	QString requestImageOpenDialog() override;

	// 请求类型处理
	void requestOptions(ActionInterface* action,bool on, bool update = false) override;

protected:
	// Links factory to a widget that can host tool options.
	void setOptionWidget(QWidget* ow);

	//void requestPrintPreviewOptions(ActionInterface* action,bool on, bool update);
	void requestLineOptions(ActionInterface* action, bool on);
	void requestPolylineOptions(ActionInterface* action, bool on, bool update);
	void requestLineBisectorOptions(ActionInterface* action, bool on, bool update);
	void requestLinePolygonOptions(ActionInterface* action, bool on, bool update);
	void requestCloudLineOptions(ActionInterface* action, bool on, bool update);

	void requestArcOptions(ActionInterface* action, bool on, bool update);

	void requestArcTangentialOptions(ActionInterface* action, bool on, bool update);

	void requestCircleTan2Options(ActionInterface* action, bool on, bool update);

	void requestSplineOptions(ActionInterface* action, bool on, bool update);

	void requestTextOptions(ActionInterface* action, bool on, bool update);

	void requestDimLinearOptions(ActionInterface* action, bool on, bool update);

	void requestInsertOptions(ActionInterface* action, bool on, bool update);
	void requestBlockEditOptions(ActionInterface* action, bool on);
	void requestImageOptions(ActionInterface* action, bool on, bool update);

	void requestBevelOptions(ActionInterface* action, bool on, bool update);
	void requestRoundOptions(ActionInterface* action, bool on, bool update);

public:
	void requestSnapDistOptions(double& dist, bool on) override;
	void requestSnapMiddleOptions(int& middlePoints, bool on) override;

public:
	bool requestModifyEntityDialog(DmEntity* entity) override;
	void requestModifySingleOffsetOptions(double& dist, bool on, bool update = false) override;
	bool requestTextDialog(DmText* text) override;
	bool requestTextStyleDialog(DmTextStyleTable* textStyles, DmDocument* document) override;
	void requestDimStyleMgrDialog(DmDimensionStyleTable* dimStyleTable, DmDocument* document) override;
	bool requestDimStyleModifyDialog(DmDimensionStyle* dimStyle, DmDocument* document) override;
	bool requestHatchDialog(DmHatch* hatch) override;
	void requestOptionsGeneralDialog() override;
	void requestOptionsDrawingDialog(DmDocument& document) override;


	/// @brief 线型对话框
	/// @return 确定返回true，取消返回false
	bool requestLineTypeDialog(DmLineTypeTable* lineTypeTable, DmDocument* document) override;

	QString requestFileSaveAsDialog(const QString& caption = QString(), const QString& dir = QString(), const QString& filter = QString(), QString* selectedFilter = 0) override;

	void updateCoordinateWidget(const DmVector& abs, const DmVector& rel, bool updateFormat=false) override;
	/// @brief updateMouseWidget Called when an action has a mouse hint.
	///	@param left mouse hint for left button
	/// @param right mouse hint for right button
	void updateMouseWidget(const QString& left=QString(), const QString& right=QString()) override;
	/// @brief 更新选中的实体数量
	void updateSelectionWidget(int num) override;
	void commandMessage(const QString& message) override;

	static QString extToFormat(const QString& ext);

	void updateArcTangentialOptions(const double& radius, const bool& lockRadius, const double& angle, const bool& lockAngle) override;

protected:
	QWidget*						parent = nullptr;						///< Pointer to the widget which can host dialogs
	QWidget*						optionWidget = nullptr;				///< Pointer to the widget which can host individual tool options
	UICommandWidget*				m_pCommandWidget = nullptr;			///< Pointer to the command line widget
	UIArcTangentialOptions*			arcTangentialOptions = nullptr;		///< Pointer to arcTangential Option widge
	UIPolylineEquidistantOptions*	polylineEquidistantOptions = nullptr;
	UIBottomWindow*                 bottomWidget = nullptr;

private:
	// pointers to snap option widgets
	UISnapMiddleOptions*			m_pSnapMiddleOptions = nullptr;
	UISnapDistOptions*				m_pSnapDistOptions = nullptr;
	UIModifyOffsetOptions*			m_pModifyOffsetOptions = nullptr;
	UIPrintPreviewOptions*			m_pPrintPreviewOptions = nullptr;
	UILineAngleOptions*				m_pLineAngleOptions = nullptr;

	UIDlgDimensionStyleMgr*			m_pDimensionStyleMgr = nullptr;
	UIDlgDimensionStyle*			m_pDimensionStyle = nullptr;
	UIDlgTextStyle*					m_pTextStyle = nullptr;
	UIDlgLineType*					m_pLineType = nullptr;
	UIDlgTableStyle*				m_pTableStyle = nullptr;
	UIDlgTableStyleMgr*				m_pTableStyleMgr = nullptr;
	UIActionHandler*                m_pActionHandler = nullptr;
};

#endif
