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


/// @file ActionDrawMText.cpp
/// @brief 多行文字绘制与编辑动作类实现文件

#include "ActionDrawMText.h"

#include <QMouseEvent>
#include <QMessageBox>

#include "GuiDialogFactory.h"
#include "GuiCoordinateEvent.h"
#include "Preview.h"
#include "MTextEditWidget.h"
#include "ApplicationWindow.h"
#include "TextConsts.h"
#include "DmDocument.h"
#include "DmFont.h"
#include "DmFontList.h"
#include "DmPolyline.h"
#include "UIMTextOptions.h"
#include "DmSettings.h"

/// @brief 预览边界框的RGB颜色分量值（白色）
static const int BOUNDING_BOX_PREVIEW_RGB = 255;

/// @brief 构造函数
/// @param doc 文档指针
/// @param docView 文档视图指针
/// @param isModify 是否为修改模式
ActionDrawMText::ActionDrawMText(DmDocument* doc, GuiDocumentView* docView, bool isModify)
	: PreviewActionInterface("Draw MText", doc, docView)
	, m_bIsModify(isModify)
	, pPoints(new Points())
	, m_pEditWidget(nullptr)
	, m_pOptionWidget(nullptr)
	, m_pOptionBack(nullptr)
	, m_pEditingText(nullptr)
	, m_pOriginText(nullptr)
{
	actionType = DM::ActionDrawMText;
}

/// @brief 初始化动作
/// @param status 初始状态
void ActionDrawMText::init(int status)
{
	//修改
	if (m_bIsModify)
	{
		initDisplayDialogs();
	}
	//新建
	else
	{
		ActionInterface::init(status);
		switch (status)
		{
		case DrawingBoundingBox:
			reset();
			break;
		case Editing:
			break;
		default:
			break;
		}
	}
}

/// @brief 重置动作状态
void ActionDrawMText::reset()
{
	pPoints->pos = DmVector(false);
	pPoints->secPos = DmVector(false);
	pPoints->clickPt = DmVector(false);
}

/// @brief 触发动作执行（完成文字创建或修改）
void ActionDrawMText::trigger()
{
	PreviewActionInterface::trigger();
	//if (nullptr == document || nullptr != m_pEditingText)	//一般不可能发生
	//{
	//	return;
	//}

	//如果文字不正，编辑时把它放正了，现在需要还原
	double angle = m_pEditingText->getDataConstPtr()->getAngle();
	if (angle != 0.0)
	{
		DmVector pos = m_pEditingText->getDataConstPtr()->getPosition();
		m_pEditingText->moveEntities(-pos);
		m_pEditingText->rotateEntities(DmVector(0.0, 0.0), angle);
		m_pEditingText->moveEntities(pos);
	}

	//文字不为空
	if (!m_pEditingText->isEmptyText())
	{
		if (!m_bIsModify)
		{
			Transaction t(tr("Create MText").toStdString(), pDocument);
			t.start();
			// 设置文字的内容
			m_pEditingText->updateContent();
			m_pEditingText->update();
			pDocument->getEntityTable()->add(m_pEditingText);
			t.commit();
		}
		else
		{
			Transaction t(tr("Modify MText").toStdString(), pDocument);
			t.start();
			m_pEditingText->updateContent();
			pDocument->getEntityTable()->startModify(m_pOriginText);
			m_pOriginText->setData(m_pEditingText->getData());
			m_pOriginText->setVisible(true);
			m_pOriginText->setSelected(false);
			m_pOriginText->update();
			t.commit();
			delete m_pEditingText;
			m_pEditingText = nullptr;
		}
	}
	//文字为空
	else
	{
		// 修改时，将原文字删除
		if (m_bIsModify)
		{
			Transaction t(tr("Delete MText").toStdString(), pDocument);
			t.start();
			pDocument->getEntityTable()->remove(m_pOriginText);
			t.commit();
		}
		else
		{
			// 新建时文字为空，不做处理
		}

		//删除编辑中的文字
		if (nullptr != m_pEditingText)
		{
			delete m_pEditingText;
			m_pEditingText = nullptr;
		}
	}

	m_trans->commit();
}

/// @brief 检查动作是否可被中断
/// @return 始终返回false，此动作不可被中断
bool ActionDrawMText::canBeInterrupt()
{
	return false;
}

/// @brief 结束动作
/// @param updateTB 是否更新工具栏
void ActionDrawMText::finish(bool updateTB)
{
	// 此Action不能挂起
	if (getStatus() == Editing)
	{
		//提示是否保存
		auto btn = QMessageBox::critical(nullptr, tr("Tips"), tr("Save the changes?"),
			QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No,
			QMessageBox::StandardButton::Yes);
		bool save = btn == QMessageBox::StandardButton::Yes;
		//结束命令
		slotEscPressed(save);
	}
	else
	{
		PreviewActionInterface::finish(false);
	}
}

/// @brief 鼠标移动事件处理
/// @param e 鼠标事件指针
void ActionDrawMText::mouseMoveEvent(QMouseEvent* e)
{
	if (getStatus() == DrawingBoundingBox)
	{
		DmVector mouse = snapPoint(e);
		if (pPoints->pos.valid)
		{
			pPoints->secPos = mouse;
		}
		if (pPoints->pos.valid && pPoints->secPos.valid)
		{
			drawBoundingBox();
		}
	}
	else
	{
		// 编辑模式下不处理鼠标移动
	}
}

/// @brief 鼠标释放事件处理
/// @param e 鼠标事件指针
void ActionDrawMText::mouseReleaseEvent(QMouseEvent* e)
{
	if (getStatus() == DrawingBoundingBox)
	{
		if (e->button() == Qt::LeftButton)
		{
			GuiCoordinateEvent ce(snapPoint(e));
			coordinateEvent(&ce);
		}
		else if (e->button() == Qt::RightButton)
		{
			deletePreview();
			PreviewActionInterface::finish(false);
		}
	}
	else
	{
		// 编辑模式下不处理鼠标释放
	}
}

/// @brief 鼠标按下事件处理
/// @param e 鼠标事件指针
void ActionDrawMText::mousePressEvent(QMouseEvent* e)
{
	if (getStatus() == Editing)
	{
		trigger();
		PreviewActionInterface::finish(false);
		freeUI();
	}
}

/// @brief 坐标事件处理
/// @param e 坐标事件指针
void ActionDrawMText::coordinateEvent(GuiCoordinateEvent* e)
{
	if (e == nullptr)
	{
		return;
	}
	DmVector mouse = e->getCoordinate();
	if (getStatus() == DrawingBoundingBox)
	{
		if (!pPoints->pos.valid)
		{
			pPoints->pos = mouse;
		}
		else
		{
			// 计算左上，右下角点
			DmVector LT(0.0, 0.0);
			DmVector RB(0.0, 0.0);
			if (pPoints->pos.x < mouse.x)
			{
				LT.x = pPoints->pos.x;
				RB.x = mouse.x;
			}
			else
			{
				LT.x = mouse.x;
				RB.x = pPoints->pos.x;
			}
			if (pPoints->pos.y < mouse.y)
			{
				LT.y = mouse.y;
				RB.y = pPoints->pos.y;
			}
			else
			{
				LT.y = pPoints->pos.y;
				RB.y = mouse.y;
			}
			pPoints->pos = LT;
			pPoints->secPos = RB;

			//deleteSnapper();//没用
			//显示选项及编辑框
			initDisplayDialogs();
		}
	}
}

/// @brief 命令事件处理
/// @param e 命令事件指针
void ActionDrawMText::commandEvent(GuiCommandEvent* e)
{
	// 当前不支持命令事件
}

/// @brief 更新鼠标按钮提示信息
void ActionDrawMText::updateMouseButtonHints()
{
	switch (getStatus())
	{
	case DrawingBoundingBox:
	{
		if (!pPoints->pos.valid)
		{
			GUIDIALOGFACTORY->updateMouseWidget(
				tr("Specify first point of edit box"), tr("Cancel"));
		}
		else
		{
			GUIDIALOGFACTORY->updateMouseWidget(
				tr("Specify second point of edit box"), tr("Cancel"));
		}
	}
		break;
	default:
		break;
	}
}

/// @brief 更新鼠标光标样式
void ActionDrawMText::updateMouseCursor()
{
	if (getStatus() == Editing)
	{
		docView->setCursor(Qt::ArrowCursor);//光标设置为Qt的箭头
		docView->setIsDrawCursor(false);//屏蔽十字光标（十字光标是渲染绘制的）
	}
}

/// @brief 聚焦编辑控件
void ActionDrawMText::focusEditWidget()
{
	if (m_pEditWidget == nullptr)
	{
		return;
	}
	m_pEditWidget->setFocus();
}

/// @brief 获取关联的文档
/// @return 文档指针
DmDocument* ActionDrawMText::getDocument()
{
	return pDocument;
}

/// @brief 初始化并显示选项及编辑框
void ActionDrawMText::initDisplayDialogs()
{
	// 隐藏原来的文字
	if (m_bIsModify)
	{
		m_trans = std::make_shared<TransactionGroup>(
			tr("Modify MText").toStdString(), pDocument);
		m_trans->start();
		Transaction t(tr("Hind origin MText").toStdString(), pDocument);
		t.start();
		pDocument->getEntityTable()->startModify(m_pOriginText);
		m_pOriginText->setVisible(false);
		t.commit();
	}
	else
	{
		m_trans = std::make_shared<TransactionGroup>(
			tr("Create MText").toStdString(), pDocument);
		m_trans->start();
	}
	//上下文初始化
	m_context = std::make_unique<ActionDrawMTextContext>();
	m_context->init(getDocument());
	connect(m_context.get(), SIGNAL(escPressed(bool)), this, SLOT(slotEscPressed(bool)));
	// 选项
	m_pOptionBack = new QWidget(ApplicationWindow::getAppWindow());
	m_pOptionBack->setObjectName("mTextOptionBackWidget");
	m_pOptionWidget = new UIMTextOptions(m_pOptionBack);
	m_pOptionWidget->setAction(this, true);
	m_pOptionBack->setGeometry(1, 186, 655, 90);
	m_pOptionBack->show();

	// 编辑框
	//修改
	if (m_bIsModify)
	{
		DmVector pos = m_pEditingText->getPosition();
		pPoints->pos = pos;
		auto data = m_pEditingText->getDataConstPtr();
		auto mtextData = const_cast<MTextData*>(data);
		double defineHeight = mtextData->getDefineHeight();
		double minHeight = m_pEditingText->calculateHeight();
		if (defineHeight < minHeight)	//如果定义高度为比较小，计算真实高度来代替
		{
			defineHeight = minHeight;
			mtextData->setDefineHeight(defineHeight);
		}
		double defineWidth = mtextData->getDefineWidth();
		if (defineWidth == 0.0)
		{
			defineWidth = 100.0;
			mtextData->setDefineWidth(defineWidth);
		}
		pPoints->secPos = pos + DmVector(defineWidth, -defineHeight);
		m_pEditWidget = new MTextEditWidget(m_pEditingText, docView, this, docView);
		m_pEditWidget->setCornersForModify(pPoints->pos, pPoints->secPos, pPoints->clickPt);
	}
	//新建
	else
	{
		DmTextStyle* activeStyle = pDocument->getTextStyleTable()->getActive();
		double defaultHeight = activeStyle->getValidDefaultHeight();
		double defineWidth = pPoints->secPos.x - pPoints->pos.x;
		double defineHeight = pPoints->pos.y - pPoints->secPos.y;
		MTextData data(pPoints->pos, defaultHeight,
			EMTextVertMode::kTextTop, EMTextHorzMode::kTextLeft,
			LINE_HEIGHT_PER_CHAR_HEIGHT * defaultHeight,
			defineWidth, "", activeStyle, 0.0);
		m_pEditingText = new DmMText(nullptr, data);
		m_pEditingText->setDocument(pDocument);
		m_pEditWidget = new MTextEditWidget(m_pEditingText, docView, this, docView);
		m_pEditWidget->setCornersForNew(pPoints->pos, pPoints->secPos);
	}

	m_pEditWidget->setFocus();
	m_pEditWidget->show();
	docView->setCursor(Qt::ArrowCursor);//光标设置为Qt的箭头
	docView->setIsDrawCursor(false);//屏蔽十字光标（十字光标是渲染绘制的）
	setStatus(Editing);
}

/// @brief 设置修改模式的数据
/// @param pOriginText 原始文字实体指针
/// @param clickPt 点击位置
void ActionDrawMText::setModifyData(DmMText* pOriginText, const DmVector& clickPt)
{
	assert(pOriginText != nullptr);
	pPoints->clickPt = clickPt;
	m_pOriginText = pOriginText;
	m_pEditingText = static_cast<DmMText*>(pOriginText->clone());

	//如果文字不正，把它放正后编辑
	double angle = m_pEditingText->getDataConstPtr()->getAngle();
	if (angle != 0.0)
	{
		DmVector pos = m_pEditingText->getDataConstPtr()->getPosition();
		m_pEditingText->moveEntities(-pos);
		m_pEditingText->rotateEntities(DmVector(0.0, 0.0), -angle);
		m_pEditingText->moveEntities(pos);
	}
}

/// @brief 取消当前操作
void ActionDrawMText::cancel()
{
	if (m_pOriginText)
	{
		m_trans->rollback();
//		m_pOriginText->setVisible(true);
//		docView->redraw();
//		m_pOriginText->setSelected(false);
	}
	if (m_pEditingText)
	{
		delete m_pEditingText;
		m_pEditingText = nullptr;
	}
}

/// @brief 获取上下文对象
/// @return 上下文对象指针
ActionDrawMTextContext* ActionDrawMText::getContext()
{
	return m_context.get();
}

/// @brief ESC键按下槽函数
/// @param save 是否保存
void ActionDrawMText::slotEscPressed(bool save)
{
	if (save)
	{
		if (getStatus() == Editing)
		{
			trigger();
			freeUI();
		}
		PreviewActionInterface::finish(false);
	}
	else
	{
		cancel();
		PreviewActionInterface::finish(false);
		freeUI();
	}
}

/// @brief 绘制边界框预览
void ActionDrawMText::drawBoundingBox()
{
	if (!pPoints->pos.valid || !pPoints->secPos.valid)
	{
		return;
	}
	deletePreview();
	DmColor color(BOUNDING_BOX_PREVIEW_RGB,
		BOUNDING_BOX_PREVIEW_RGB, BOUNDING_BOX_PREVIEW_RGB);
	DmPen pen(color, DM::Width00, DmLineTypeTable::Continuous);
	DmVector p1(pPoints->pos);
	DmVector p2(pPoints->secPos.x, pPoints->pos.y);
	DmVector p3(pPoints->secPos);
	DmVector p4(pPoints->pos.x, pPoints->secPos.y);
	DmPolyline* poly = new DmPolyline(nullptr, PolylineData());
	poly->setPen(pen);
	poly->appendVertex(p1);
	poly->appendVertex(p2);
	poly->appendVertex(p3);
	poly->appendVertex(p4);
	poly->setClosed(true);
	poly->update();
	preview->addEntity(poly);
	drawPreview();
}

/// @brief 释放UI资源
void ActionDrawMText::freeUI()
{
	//释放UI
	if (m_pEditWidget)
	{
		m_pEditWidget->close();
//        m_pEditWidget->hide();    //这种释放方式不行
//        delete m_pEditWidget;
		m_pEditWidget = nullptr;
	}
	if (m_pOptionBack)
	{
		m_pOptionBack->close();
		m_pOptionBack = nullptr;
		m_pOptionWidget = nullptr;
	}
	docView->setIsDrawCursor(true);
	docView->setMouseCursor(DM::ArrowCursor);
}

/// @brief 默认构造函数
ActionDrawMTextContext::ActionDrawMTextContext()
	: m_pCurTextStyle(nullptr)
	, m_dCurCharHeight(2.5)
	, m_strCurFontFamilyName(DEFAULT_FONT_FAMILY_NAME)
	, m_curFont(nullptr)
	, m_isBold(false)
	, m_isItalic(false)
	, m_hasOverline(false)
	, m_hasUnderline(false)
	, m_hasStrikethrough(false)
	, m_bIsMatching(false)
	, m_curColor(DM::FlagByLayer)
	, m_eParaAlignment(DmMTextParagraph::Alignment::Default)
	, m_eJustification(EMTextMode::kTextTopLeft)
	, m_dWidthFactor(1.0)
	, m_dOblique(0.0)
	, m_bIsUpdatingToOption(false)
{
}

/// @brief 拷贝构造函数
/// @param context 要拷贝的上下文对象
ActionDrawMTextContext::ActionDrawMTextContext(const ActionDrawMTextContext& context)
{
	m_pCurTextStyle = context.m_pCurTextStyle;
	m_dCurCharHeight = context.m_dCurCharHeight;
	m_strCurFontFamilyName = context.m_strCurFontFamilyName;
	m_curFont = context.m_curFont;
	m_isBold = context.m_isBold;
	m_isItalic = context.m_isItalic;
	m_hasOverline = context.m_hasOverline;
	m_hasUnderline = context.m_hasUnderline;
	m_hasStrikethrough = context.m_hasStrikethrough;
	m_bIsMatching = context.m_bIsMatching;
	m_curColor = context.m_curColor;
	m_eParaAlignment = context.m_eParaAlignment;
	m_eJustification = context.m_eJustification;
	m_dWidthFactor = context.m_dWidthFactor;
	m_dOblique = context.m_dOblique;
	m_bIsUpdatingToOption = false;
}

/// @brief 初始化上下文
/// @param pDocument 文档指针
void ActionDrawMTextContext::init(DmDocument* pDocument)
{
	auto style = pDocument->getTextStyleTable()->getActive();
	auto styleData = style->getDataConstPtr();
	setCurTextStyle(style);
	setCurCharHeight(styleData->defaultHeight);
	bool isSysFont = true;
	if (styleData->isSystemFont && styleData->invalidSysFontFamily.isEmpty())
	{
		setCurFontFamilyName(styleData->sysFontFamily);
		setIsBold(styleData->isSysFontBold);
		setIsItalic(styleData->isSysFontItalic);
		m_curFont = styleData->pSysFont;
		isSysFont = true;
	}
	else if (!styleData->isSystemFont && styleData->invalidAsciiFont.isEmpty())
	{
		setCurFontFamilyName(styleData->pAsciiFont->getFileName());
		setIsBold(false);
		setIsItalic(false);
		m_curFont = styleData->pAsciiFont;
		isSysFont = false;
	}
	// 文字样式存在无效字体，采用默认的
	else
	{
		auto font = DMFONTLIST->requestSysFont(DEFAULT_FONT_FAMILY_NAME, false, false);
		setCurFontFamilyName(DEFAULT_FONT_FAMILY_NAME);
		setIsBold(false);
		setIsItalic(false);
		m_curFont = font;
		isSysFont = true;
	}
	setCurColor(DmColor(DM::FlagByLayer));
	setHasOverline(false);
	setHasStrikethrough(false);
	setHasUnderline(false);
	setOblique(styleData->slashAngle);
	setWidthFactor(styleData->widhFactor);
}

/// @brief 获取当前文字样式
/// @return 文字样式指针
DmTextStyle* ActionDrawMTextContext::getCurTextStyle() const
{
	return m_pCurTextStyle;
}

/// @brief 设置当前文字样式
/// @param style 文字样式指针
void ActionDrawMTextContext::setCurTextStyle(DmTextStyle* style)
{
	m_pCurTextStyle = style;
}

/// @brief 获取当前文字高度
/// @return 文字高度
double ActionDrawMTextContext::getCurCharHeight() const
{
	return m_dCurCharHeight;
}

/// @brief 设置当前文字高度
/// @param charHeight 文字高度
void ActionDrawMTextContext::setCurCharHeight(const double& charHeight)
{
	m_dCurCharHeight = charHeight;
}

/// @brief 获取当前字体族名称
/// @return 字体族名称
QString ActionDrawMTextContext::getCurFontFamilyName() const
{
	return m_strCurFontFamilyName;
}

/// @brief 设置当前字体族名称
/// @param name 字体族名称
void ActionDrawMTextContext::setCurFontFamilyName(const QString& name)
{
	m_strCurFontFamilyName = name;
}

/// @brief 获取是否粗体
/// @return 是否粗体
bool ActionDrawMTextContext::getIsBold() const
{
	return m_isBold;
}

/// @brief 设置是否粗体
/// @param isBold 是否粗体
void ActionDrawMTextContext::setIsBold(const bool& isBold)
{
	m_isBold = isBold;
}

/// @brief 获取是否斜体
/// @return 是否斜体
bool ActionDrawMTextContext::getIsItalic() const
{
	return m_isItalic;
}

/// @brief 设置是否斜体
/// @param isItalic 是否斜体
void ActionDrawMTextContext::setIsItalic(const bool& isItalic)
{
	m_isItalic = isItalic;
}

/// @brief 获取是否有上划线
/// @return 是否有上划线
bool ActionDrawMTextContext::getHasOverline() const
{
	return m_hasOverline;
}

/// @brief 设置是否有上划线
/// @param has 是否有上划线
void ActionDrawMTextContext::setHasOverline(const bool& has)
{
	m_hasOverline = has;
}

/// @brief 获取是否有下划线
/// @return 是否有下划线
bool ActionDrawMTextContext::getHasUnderline() const
{
	return m_hasUnderline;
}

/// @brief 设置是否有下划线
/// @param has 是否有下划线
void ActionDrawMTextContext::setHasUnderline(const bool& has)
{
	m_hasUnderline = has;
}

/// @brief 获取是否有删除线
/// @return 是否有删除线
bool ActionDrawMTextContext::getHasStrikethrough() const
{
	return m_hasStrikethrough;
}

/// @brief 设置是否有删除线
/// @param has 是否有删除线
void ActionDrawMTextContext::setHasStrikethrough(const bool has)
{
	m_hasStrikethrough = has;
}

/// @brief 获取是否正在使用格式刷
/// @return 是否匹配
bool ActionDrawMTextContext::getIsMatching() const
{
	return m_bIsMatching;
}

/// @brief 设置是否使用格式刷
/// @param match 是否匹配
void ActionDrawMTextContext::setIsMatching(const bool& match)
{
	m_bIsMatching = match;
}

/// @brief 获取当前颜色
/// @return 颜色对象
DmColor ActionDrawMTextContext::getCurColor() const
{
	return m_curColor;
}

/// @brief 设置当前颜色
/// @param color 颜色对象
void ActionDrawMTextContext::setCurColor(const DmColor& color)
{
	m_curColor = color;
}

/// @brief 获取段落对齐方式
/// @return 段落对齐枚举
DmMTextParagraph::Alignment ActionDrawMTextContext::getParaAlignment() const
{
	return m_eParaAlignment;
}

/// @brief 设置段落对齐方式
/// @param alignment 段落对齐枚举
void ActionDrawMTextContext::setParaAlignment(DmMTextParagraph::Alignment alignment)
{
	m_eParaAlignment = alignment;
}

/// @brief 获取对正方式
/// @return 对正方式枚举
EMTextMode ActionDrawMTextContext::getJustification() const
{
	return m_eJustification;
}

/// @brief 设置对正方式
/// @param justification 对正方式枚举
void ActionDrawMTextContext::setJustification(EMTextMode justification)
{
	m_eJustification = justification;
}

/// @brief 获取倾斜角度
/// @return 倾斜角度（弧度）
double ActionDrawMTextContext::getOblique() const
{
	return m_dOblique;
}

/// @brief 设置倾斜角度
/// @param oblique 倾斜角度（弧度）
void ActionDrawMTextContext::setOblique(const double& oblique)
{
	m_dOblique = oblique;
}

/// @brief 获取宽度因子
/// @return 宽度因子
double ActionDrawMTextContext::getWidthFactor() const
{
	return m_dWidthFactor;
}

/// @brief 设置宽度因子
/// @param widthFactor 宽度因子
void ActionDrawMTextContext::setWidthFactor(const double& widthFactor)
{
	m_dWidthFactor = widthFactor;
}

/// @brief 发出文字样式修改信号
void ActionDrawMTextContext::emitUiStyleChanged()
{
	emit uiStyleChanged();
}

/// @brief 发出字体族修改信号
void ActionDrawMTextContext::emitUiFontFamilyChanged()
{
	emit uiFontFamilyChanged();
}

/// @brief 发出颜色修改信号
void ActionDrawMTextContext::emitUiColorChanged()
{
	emit uiColorChanged();
}

/// @brief 发出文字高度修改信号
void ActionDrawMTextContext::emitUiCharHeightChanged()
{
	emit uiCharHeightChanged();
}

/// @brief 发出粗体修改信号
void ActionDrawMTextContext::emitUiBoldChanged()
{
	emit uiBoldChanged();
}

/// @brief 发出斜体修改信号
void ActionDrawMTextContext::emitUiItalicChanged()
{
	emit uiItalicChanged();
}

/// @brief 发出删除线修改信号
void ActionDrawMTextContext::emitUiStrikethroughChanged()
{
	emit uiStrikethroughChanged();
}

/// @brief 发出下划线修改信号
void ActionDrawMTextContext::emitUiUnderlineChanged()
{
	emit uiUnderlineChanged();
}

/// @brief 发出上划线修改信号
void ActionDrawMTextContext::emitUiOverlineChanged()
{
	emit uiOverlineChanged();
}

/// @brief 发出对正方式修改信号
void ActionDrawMTextContext::emitUiJustificationChanged()
{
	emit uiJustificationChanged();
}

/// @brief 发出段落对齐修改信号
void ActionDrawMTextContext::emitUiParaAlignmentChanged()
{
	emit uiParaAlignmentChanged();
}

/// @brief 发出倾斜角度修改信号
void ActionDrawMTextContext::emitUiObliqueChanged()
{
	emit uiObliqueChanged();
}

/// @brief 发出宽度因子修改信号
void ActionDrawMTextContext::emitUiWidthFactorChanged()
{
	emit uiWidthFactorChanged();
}

/// @brief 发出转小写信号
void ActionDrawMTextContext::emitUiToLower()
{
	emit uiToLower();
}

/// @brief 发出转大写信号
void ActionDrawMTextContext::emitUiToUpper()
{
	emit uiToUpper();
}

/// @brief 发出插入符号信号
/// @param symbol 符号字符串
void ActionDrawMTextContext::emitUiSymbolActivated(const QString& symbol)
{
	emit uiSymbolActivated(symbol);
}

/// @brief 发出格式刷匹配信号
void ActionDrawMTextContext::emitUiMatching()
{
	emit uiMatching();
}

/// @brief 发出撤销信号
void ActionDrawMTextContext::emitUiUndo()
{
	emit uiUndo();
}

/// @brief 发出重做信号
void ActionDrawMTextContext::emitUiRedo()
{
	emit uiRedo();
}

/// @brief 发出数据模型到选项的更新信号
void ActionDrawMTextContext::emitDmToOption()
{
	m_bIsUpdatingToOption = true;
	emit dmToOption();
	m_bIsUpdatingToOption = false;
}

/// @brief 发出ESC按下信号
/// @param save 是否保存
void ActionDrawMTextContext::emitEscPressed(bool save)
{
	emit escPressed(save);
}

/// @brief 检查是否正在更新到选项
/// @return 是否正在更新
bool ActionDrawMTextContext::IsUpdatingToOption() const
{
	return m_bIsUpdatingToOption;
}

/// @brief 发出撤销/重做状态更新到选项的信号
/// @param undoable 是否可撤销
/// @param redoable 是否可重做
void ActionDrawMTextContext::emitUndoToOption(bool undoable, bool redoable)
{
	m_bIsUpdatingToOption = true;
	emit undoToOption(undoable, redoable);
	m_bIsUpdatingToOption = false;
}
