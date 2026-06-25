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

/// @file UIMTextOptions.cpp
/// @brief 多行文字选项控件实现

#include <QMessageBox>
#include <QDoubleValidator>

#include "UIMTextOptions.h"
#include "ActionInterface.h"
#include "ActionDrawMText.h"
#include "DmDocument.h"
#include "DmFontList.h"
#include "DmFont.h"
#include "Math2d.h"

UIMTextOptions::UIMTextOptions(QWidget* parent, Qt::WindowFlags fl)
	: QWidget(parent, fl)
	, action(nullptr)
	, context(nullptr)
	, m_isDlgShow(false)
	, m_isChangingStyle(false)
	, m_isChangingSpinBoxValue(false)
	, m_isChangingComboBoxValue(false)
{
	setupUi(this);
}

void UIMTextOptions::slotEditLineEditingFinished()
{
	if (sender() == leHeight)
	{
		double height = leHeight->text().toDouble();
		context->setCurCharHeight(height);
		if (!context->IsUpdatingToOption())
		{
			context->emitUiCharHeightChanged();
		}
	}
	//action->update(ActionDrawMText::DataUpdateMode::UpdateToDMSelect);
	action->focusEditWidget();
}

void UIMTextOptions::languageChange()
{
	retranslateUi(this);
}

void UIMTextOptions::setAction(ActionInterface* a, bool update)
{
	if (a && (a->getEntityType() == DM::ActionDrawMText))
	{
		action = static_cast<ActionDrawMText*>(a);
		context = action->getContext();
		connect(context, SIGNAL(dmToOption()), this, SLOT(updateUIFromData()));
		connect(context, SIGNAL(undoToOption(bool, bool)), this, SLOT(slotUpdateUndoUI(bool, bool)));
		init();

		//bool reversed;
		//if (update) {
		//	reversed = action->isReversed();
		//}
		//else {
		//	DMSETTINGS->beginGroup("/Draw");
		//	reversed = DMSETTINGS->readNumEntry("/ArcReversed", 0);
		//	DMSETTINGS->endGroup();
		//	action->setReversed(reversed);
		//}
		//ui->rbNeg->setChecked(reversed);
	}
	else
	{
		action = nullptr;
	}
}

void UIMTextOptions::init()
{
	// 字体
	initFonts();
	connect(cbFont, SIGNAL(fontChanged(QString)), this, SLOT(slotCboFontChanged(QString)));

	// 颜色
	cbColor->init(true);
	connect(cbColor, SIGNAL(colorChanged(const DmColor&)), this, SLOT(slotColorChanged(const DmColor&)));

	//高度
	QDoubleValidator* heightValidator = new QDoubleValidator();
	heightValidator->setBottom(1e-10);
	leHeight->setValidator(heightValidator);
	connect(leHeight, SIGNAL(editingFinished()), this, SLOT(slotEditLineEditingFinished()));

	//按钮
	connect(btnBold, SIGNAL(toggled(bool)), this, SLOT(slotBtnToggled(bool)));
	connect(btnItalic, SIGNAL(toggled(bool)), this, SLOT(slotBtnToggled(bool)));
	connect(btnStrikethrough, SIGNAL(toggled(bool)), this, SLOT(slotBtnToggled(bool)));
	connect(btnUnderLine, SIGNAL(toggled(bool)), this, SLOT(slotBtnToggled(bool)));
	connect(btnOverline, SIGNAL(toggled(bool)), this, SLOT(slotBtnToggled(bool)));
	connect(btnMatchFormat, SIGNAL(toggled(bool)), this, SLOT(slotBtnToggled(bool)));
	connect(btnUndo, SIGNAL(clicked(bool)), this, SLOT(slotBtnToggled(bool)));//toggled()无效
	connect(btnRedo, SIGNAL(clicked(bool)), this, SLOT(slotBtnToggled(bool)));

	//对正
	QStringList justifyStrs{ tr("TopLeft"), tr("TopCenter"), tr("TopRight")
		, tr("MiddleLeft"), tr("MiddleCenter"), tr("MiddleRight")
		, tr("BottomLeft"), tr("BottomCenter"), tr("BottomRight") };
	//QStringList justifyStrs{ tr("Top"), tr("Middle"), tr("Bottom")};
	cboJustify->addItems(justifyStrs);
	connect(cboJustify, SIGNAL(currentTextChanged(const QString&)), this, SLOT(slotJustifyChanged(const QString&)));

	// 对齐
	QStringList alignmentStrs{ tr("Default"), tr("Left"), tr("Mid"), tr("Right"), tr("Justify"), tr("Distribute")};
	cboParaAlign->addItems(alignmentStrs);
	connect(cboParaAlign, SIGNAL(currentTextChanged(const QString&)), this, SLOT(slotParaAlignmentChanged(const QString&)));

	// 大小写转换
	//connect(btnToLower, SIGNAL(pressed()), this, SLOT(slotCaseChange()));
	//connect(btnToUpper, SIGNAL(pressed()), this, SLOT(slotCaseChange()));

	// 符号
	connect(cboSymbol, SIGNAL(charActivated(QString)), this, SLOT(slotCboSymbolActivated(QString)));

	// 倾斜角度及宽度系数
	dsbOblique->setMinimum(0.0);	//仅对spinbox的按钮有效，手动输入无效
	dsbOblique->setMaximum(90.0);
	connect(dsbOblique, SIGNAL(valueChanged(double)), this, SLOT(slotSpinBoxValueChanged(double)));
	dsbWidthFactor->setMinimum(1e-10);
	connect(dsbWidthFactor, SIGNAL(valueChanged(double)), this, SLOT(slotSpinBoxValueChanged(double)));

	// 文字样式初始化（放在后面是因为要设置默认字体等）
	connect(cbStyle, SIGNAL(styleChanged()), this, SLOT(slotStyleChanged()));
	DmTextStyleTable* textStyleTable = action->getDocument()->getTextStyleTable();
	cbStyle->init(textStyleTable);
	cbStyle->setChangeQueryFunc(queryChangeItem);
}

void UIMTextOptions::updateUIFromData()
{
	if (!m_isDlgShow)
	{
		return;
	}
	auto style = context->getCurTextStyle();
	cbStyle->setStyle(style->getName());
	cbFont->setCurrentText(context->getCurFontFamilyName());
	cbColor->setColor(context->getCurColor());
	double charHeight = context->getCurCharHeight();
	QString charHeightStr = QString::number(charHeight);
	leHeight->setText(charHeightStr);
	btnBold->setChecked(context->getIsBold());
	btnItalic->setChecked(context->getIsItalic());
	btnStrikethrough->setChecked(context->getHasStrikethrough());
	btnUnderLine->setChecked(context->getHasUnderline());
	btnOverline->setChecked(context->getHasOverline());
	btnMatchFormat->setChecked(context->getIsMatching());

	m_isChangingComboBoxValue = true;
	QString alignStr = alignmentToTranslateString(context->getParaAlignment());
	cboParaAlign->setCurrentText(alignStr);
	QString justifyStr = justifyToTranslateString(context->getJustification());
	cboJustify->setCurrentText(justifyStr);
	m_isChangingComboBoxValue = false;

	m_isChangingSpinBoxValue = true;
	double widthFactor = context->getWidthFactor();
	dsbWidthFactor->setValue(widthFactor);
	double oblique = context->getOblique();
	dsbOblique->setValue(Math2d::rad2deg(oblique));
	m_isChangingSpinBoxValue = false;
}

void UIMTextOptions::slotUpdateUndoUI(bool undoable, bool redoable)
{
	btnUndo->setEnabled(undoable);
	btnRedo->setEnabled(redoable);
}

bool UIMTextOptions::queryChangeItem()
{
	QMessageBox::StandardButton res = (QMessageBox::StandardButton)QMessageBox::question(
		nullptr, tr("Tips"), tr("Change text style will effect the entire multiline text, whether to confirm the modification?"),
		QMessageBox::StandardButton::Ok, QMessageBox::StandardButton::Cancel);
	return (res == QMessageBox::StandardButton::Ok);
}

void UIMTextOptions::showEvent(QShowEvent* ev)
{
	QWidget::showEvent(ev);
	m_isDlgShow = true;
}

void UIMTextOptions::leaveEvent(QEvent* event)
{
	if (nullptr == action)
	{
		return;
	}
	action->focusEditWidget();
}

void UIMTextOptions::slotStyleChanged()
{
	auto style = cbStyle->getStyle();
	context->setCurTextStyle(style);
	auto dataPtr = style->getDataConstPtr();
	m_isChangingStyle = true;
	if (dataPtr->isSystemFont)
	{
		cbFont->setCurrentText(dataPtr->sysFontFamily);
		btnBold->setEnabled(true);
		btnBold->setChecked(dataPtr->isSysFontBold);
		context->setIsBold(dataPtr->isSysFontBold);
		btnItalic->setEnabled(true);
		btnItalic->setChecked(dataPtr->isSysFontItalic);
		context->setIsItalic(dataPtr->isSysFontItalic);
	}
	else
	{
		cbFont->setCurrentText(dataPtr->pAsciiFont->getFileName());
		btnBold->setEnabled(false);
		btnItalic->setEnabled(false);
		context->setIsBold(false);
		context->setIsItalic(false);
	}
	m_isChangingStyle = false;
	if (!m_isDlgShow)
	{
		return;
	}

	// 到在这里代表UI显示后手动触发
	if (!context->IsUpdatingToOption())
	{
		context->emitUiStyleChanged();
	}
	//action->update(ActionDrawMText::DataUpdateMode::UpdateToDMAll);
	action->focusEditWidget();
}

void UIMTextOptions::slotCboFontChanged(const QString& font)
{
	context->setCurFontFamilyName(font);
	if (!m_isDlgShow)
	{
		return;
	}
	if (m_isChangingStyle)
	{
		return;
	}
	if (!context->IsUpdatingToOption())
	{
		context->emitUiFontFamilyChanged();
	}
	//action->update(ActionDrawMText::DataUpdateMode::UpdateToDMSelect);
	action->focusEditWidget();
}

void UIMTextOptions::slotColorChanged(const DmColor& color)
{
	if (!m_isDlgShow)
	{
		return;
	}
	context->setCurColor(color);
	if (!context->IsUpdatingToOption())
	{
		context->emitUiColorChanged();
	}
	//action->update(ActionDrawMText::DataUpdateMode::UpdateToDMSelect);
	action->focusEditWidget();
}

void UIMTextOptions::slotCboSymbolActivated(const QString& symbol)
{
	if (!m_isDlgShow)
	{
		return;
	}
	context->emitUiSymbolActivated(symbol);
}

void UIMTextOptions::slotBtnToggled(bool checked)
{
	if (!m_isDlgShow)
	{
		return;
	}
	if (sender() == btnBold)
	{
		context->setIsBold(checked);
		if (m_isChangingStyle)
		{
			return;
		}
		if (!context->IsUpdatingToOption())
		{
			context->emitUiBoldChanged();
		}
	}
	else if (sender() == btnItalic)
	{
		context->setIsItalic(checked);
		if (m_isChangingStyle)
		{
			return;
		}
		if (!context->IsUpdatingToOption())
		{
			context->emitUiItalicChanged();
		}
	}
	else if (sender() == btnStrikethrough)
	{
		context->setHasStrikethrough(checked);
		if (!context->IsUpdatingToOption())
		{
			context->emitUiStrikethroughChanged();
		}
	}
	else if (sender() == btnUnderLine)
	{
		context->setHasUnderline(checked);
		if (!context->IsUpdatingToOption())
		{
			context->emitUiUnderlineChanged();
		}
	}
	else if (sender() == btnOverline)
	{
		context->setHasOverline(checked);
		if (!context->IsUpdatingToOption())
		{
			context->emitUiOverlineChanged();
		}
	}
	else if (sender() == btnMatchFormat)
	{
		context->setIsMatching(checked);
		if (!context->IsUpdatingToOption())
		{
			context->emitUiMatching();
		}
	}
	else if (sender() == btnUndo)
	{
		if (!context->IsUpdatingToOption())
		{
			context->emitUiUndo();
		}
	}
	else if (sender() == btnRedo)
	{
		if (!context->IsUpdatingToOption())
		{
			context->emitUiRedo();
		}
	}
	else
	{
		// 未识别的发送者，不做处理
	}
	action->focusEditWidget();
}

void UIMTextOptions::slotJustifyChanged(const QString& text)
{
	if (!m_isDlgShow)
	{
		return;
	}
	if (m_isChangingComboBoxValue)
	{
		return;
	}
	ActionDrawMTextContext* context = action->getContext();
	EMTextMode curMode = translateStringToJustify(text);
	context->setJustification(curMode);
	context->emitUiJustificationChanged();
	//action->update(ActionDrawMText::DataUpdateMode::UpdateToDMAll);
	action->focusEditWidget();
}

void UIMTextOptions::slotParaAlignmentChanged(const QString& text)
{
	if (!m_isDlgShow)
	{
		return;
	}
	if (m_isChangingComboBoxValue)
	{
		return;
	}
	ActionDrawMTextContext* context = action->getContext();
	DmMTextParagraph::Alignment align = translateStringToAlignment(text);
	context->setParaAlignment(align);
	context->emitUiParaAlignmentChanged();
	//action->update(ActionDrawMText::DataUpdateMode::UpdateToDMSelect);
	action->focusEditWidget();
}

void UIMTextOptions::slotCaseChange()
{
	//if (sender() == btnToLower)
	//{
	//	context->emitUiToLower();
	//}
	//else if (sender() == btnToUpper)
	//{
	//	context->emitUiToUpper();
	//}
}

void UIMTextOptions::slotSpinBoxValueChanged(double d)
{
	if (!m_isDlgShow)
	{
		return;
	}
	if (m_isChangingSpinBoxValue)
	{
		return;
	}
	ActionDrawMTextContext* context = action->getContext();
	if (sender() == dsbOblique)
	{
		if (d >= 0.0 && d < 90.0)
		{
			context->setOblique(Math2d::deg2rad(d));
			context->emitUiObliqueChanged();
		}
	}
	else if (sender() == dsbWidthFactor)
	{
		if (d > 0.0)
		{
			context->setWidthFactor(d);
			context->emitUiWidthFactorChanged();
		}
	}
}

void UIMTextOptions::initFonts()
{
	cbFont->clearFonts();
	std::vector<std::pair<FontIconType, QString>> shxFonts;
	std::vector<std::pair<FontIconType, QString>> sysFonts;
	std::vector<std::pair<FontIconType, QString>> allFonts;
	//初始化字体列表。读取所有字体文件的头
	DMFONTLIST->readAllFontFiles();
	//先添加shx字体
	for (auto const& font : *DMFONTLIST)
	{
		if (font->getFontType() == FontType::ShxBigFont)
		{
			continue;
		}
		if (font->getFontType() == FontType::ShxASCII || font->getFontType() == FontType::ShxUnifont)
		{
			shxFonts.emplace_back(std::make_pair(FontIconType::Shx, font->getFileName()));
		}
	}
	//再添加系统字体族
	for (auto kv : DMFONTLIST->getSysFontsMapConstRef())
	{
		QString family = kv.first;
		sysFonts.emplace_back(std::make_pair(FontIconType::SystemFont, family));
	}

	allFonts.reserve(shxFonts.size() + sysFonts.size());
	allFonts.insert(allFonts.end(), shxFonts.begin(), shxFonts.end());
	allFonts.insert(allFonts.end(), sysFonts.begin(), sysFonts.end());
	cbFont->addFonts(allFonts);
}

QString UIMTextOptions::justifyToTranslateString(EMTextMode mode)
{
	//QString res = tr("Top");
	//if (mode == EMTextMode::Top)
	//{
	//	res = tr("Top");
	//}
	//else if (mode == EMTextMode::Middle)
	//{
	//	res = tr("Middle");
	//}
	//else if (mode == EMTextMode::Bottom)
	//{
	//	res = tr("Bottom");
	//}
	//return res;

	QString res = tr("TopLeft");
	if (mode == EMTextMode::kTextTopLeft)
	{
		res = tr("TopLeft");
	}
	else if (mode == EMTextMode::kTextTopCenter)
	{
		res = tr("TopCenter");
	}
	else if (mode == EMTextMode::kTextTopRight)
	{
		res = tr("TopRight");
	}
	else if (mode == EMTextMode::kTextMiddleLeft)
	{
		res = tr("MiddleLeft");
	}
	else if (mode == EMTextMode::kTextMiddleCenter)
	{
		res = tr("MiddleCenter");
	}
	else if (mode == EMTextMode::kTextMiddleRight)
	{
		res = tr("MiddleRight");
	}
	else if (mode == EMTextMode::kTextBottomLeft)
	{
		res = tr("BottomLeft");
	}
	else if (mode == EMTextMode::kTextBottomCenter)
	{
		res = tr("BottomCenter");
	}
	else if (mode == EMTextMode::kTextBottomRight)
	{
		res = tr("BottomRight");
	}
	else
	{
		// 保持默认
	}
	return res;
}

EMTextMode UIMTextOptions::translateStringToJustify(const QString& text)
{
	//EMTextMode mode = EMTextMode::Top;
	//if (text == tr("Top"))
	//{
	//	mode = EMTextMode::Top;
	//}
	//else if (text == tr("Middle"))
	//{
	//	mode = EMTextMode::Middle;
	//}
	//else if (text == tr("Bottom"))
	//{
	//	mode = EMTextMode::Bottom;
	//}
	//return mode;

	EMTextMode mode = EMTextMode::kTextTopLeft;
	if (text == tr("TopLeft"))
	{
		mode = EMTextMode::kTextTopLeft;
	}
	else if (text == tr("TopCenter"))
	{
		mode = EMTextMode::kTextTopCenter;
	}
	else if (text == tr("TopRight"))
	{
		mode = EMTextMode::kTextTopRight;
	}
	else if (text == tr("MiddleLeft"))
	{
		mode = EMTextMode::kTextMiddleLeft;
	}
	else if (text == tr("MiddleCenter"))
	{
		mode = EMTextMode::kTextMiddleCenter;
	}
	else if (text == tr("MiddleRight"))
	{
		mode = EMTextMode::kTextMiddleRight;
	}
	else if (text == tr("BottomLeft"))
	{
		mode = EMTextMode::kTextBottomLeft;
	}
	else if (text == tr("BottomCenter"))
	{
		mode = EMTextMode::kTextBottomCenter;
	}
	else if (text == tr("BottomRight"))
	{
		mode = EMTextMode::kTextBottomRight;
	}
	else
	{
		// 保持默认值
	}
	return mode;
}

QString UIMTextOptions::alignmentToTranslateString(DmMTextParagraph::Alignment align)
{
	QString res = tr("Default");
	if (align == DmMTextParagraph::Alignment::Default)
	{
		res = tr("Default");
	}
	else if (align == DmMTextParagraph::Alignment::Left)
	{
		res = tr("Left");
	}
	else if (align == DmMTextParagraph::Alignment::Mid)
	{
		res = tr("Mid");
	}
	else if (align == DmMTextParagraph::Alignment::Right)
	{
		res = tr("Right");
	}
	else if (align == DmMTextParagraph::Alignment::Justify)
	{
		res = tr("Justify");
	}
	else if (align == DmMTextParagraph::Alignment::Distribute)
	{
		res = tr("Distribute");
	}
	else
	{
		// 保持默认
	}
	return res;
}

DmMTextParagraph::Alignment UIMTextOptions::translateStringToAlignment(const QString& text)
{
	DmMTextParagraph::Alignment align = DmMTextParagraph::Alignment::Default;
	if (text == tr("Default"))
	{
		align = DmMTextParagraph::Alignment::Default;
	}
	else if (text == tr("Left"))
	{
		align = DmMTextParagraph::Alignment::Left;
	}
	else if (text == tr("Mid"))
	{
		align = DmMTextParagraph::Alignment::Mid;
	}
	else if (text == tr("Right"))
	{
		align = DmMTextParagraph::Alignment::Right;
	}
	else if (text == tr("Justify"))
	{
		align = DmMTextParagraph::Alignment::Justify;
	}
	else if (text == tr("Distribute"))
	{
		align = DmMTextParagraph::Alignment::Distribute;
	}
	else
	{
		// 保持默认值
	}
	return align;
}
