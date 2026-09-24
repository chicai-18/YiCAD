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

/// @file MTextEditContext.cpp
/// @brief MTextEditContext 的实现（从原 ActionDrawMText.cpp 搬来）

#include "MTextEditContext.h"

#include "DmDocument.h"
#include "DmFont.h"
#include "DmFontList.h"
#include "DmTextStyle.h"
#include "DmTextStyleTable.h"
#include "TextConsts.h"

/// @brief 默认构造函数
MTextEditContext::MTextEditContext()
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
MTextEditContext::MTextEditContext(const MTextEditContext& context)
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
void MTextEditContext::init(DmDocument* pDocument)
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
DmTextStyle* MTextEditContext::getCurTextStyle() const
{
	return m_pCurTextStyle;
}

/// @brief 设置当前文字样式
/// @param style 文字样式指针
void MTextEditContext::setCurTextStyle(DmTextStyle* style)
{
	m_pCurTextStyle = style;
}

/// @brief 获取当前文字高度
/// @return 文字高度
double MTextEditContext::getCurCharHeight() const
{
	return m_dCurCharHeight;
}

/// @brief 设置当前文字高度
/// @param charHeight 文字高度
void MTextEditContext::setCurCharHeight(const double& charHeight)
{
	m_dCurCharHeight = charHeight;
}

/// @brief 获取当前字体族名称
/// @return 字体族名称
QString MTextEditContext::getCurFontFamilyName() const
{
	return m_strCurFontFamilyName;
}

/// @brief 设置当前字体族名称
/// @param name 字体族名称
void MTextEditContext::setCurFontFamilyName(const QString& name)
{
	m_strCurFontFamilyName = name;
}

/// @brief 获取是否粗体
/// @return 是否粗体
bool MTextEditContext::getIsBold() const
{
	return m_isBold;
}

/// @brief 设置是否粗体
/// @param isBold 是否粗体
void MTextEditContext::setIsBold(const bool& isBold)
{
	m_isBold = isBold;
}

/// @brief 获取是否斜体
/// @return 是否斜体
bool MTextEditContext::getIsItalic() const
{
	return m_isItalic;
}

/// @brief 设置是否斜体
/// @param isItalic 是否斜体
void MTextEditContext::setIsItalic(const bool& isItalic)
{
	m_isItalic = isItalic;
}

/// @brief 获取是否有上划线
/// @return 是否有上划线
bool MTextEditContext::getHasOverline() const
{
	return m_hasOverline;
}

/// @brief 设置是否有上划线
/// @param has 是否有上划线
void MTextEditContext::setHasOverline(const bool& has)
{
	m_hasOverline = has;
}

/// @brief 获取是否有下划线
/// @return 是否有下划线
bool MTextEditContext::getHasUnderline() const
{
	return m_hasUnderline;
}

/// @brief 设置是否有下划线
/// @param has 是否有下划线
void MTextEditContext::setHasUnderline(const bool& has)
{
	m_hasUnderline = has;
}

/// @brief 获取是否有删除线
/// @return 是否有删除线
bool MTextEditContext::getHasStrikethrough() const
{
	return m_hasStrikethrough;
}

/// @brief 设置是否有删除线
/// @param has 是否有删除线
void MTextEditContext::setHasStrikethrough(const bool has)
{
	m_hasStrikethrough = has;
}

/// @brief 获取是否正在使用格式刷
/// @return 是否匹配
bool MTextEditContext::getIsMatching() const
{
	return m_bIsMatching;
}

/// @brief 设置是否使用格式刷
/// @param match 是否匹配
void MTextEditContext::setIsMatching(const bool& match)
{
	m_bIsMatching = match;
}

/// @brief 获取当前颜色
/// @return 颜色对象
DmColor MTextEditContext::getCurColor() const
{
	return m_curColor;
}

/// @brief 设置当前颜色
/// @param color 颜色对象
void MTextEditContext::setCurColor(const DmColor& color)
{
	m_curColor = color;
}

/// @brief 获取段落对齐方式
/// @return 段落对齐枚举
DmMTextParagraph::Alignment MTextEditContext::getParaAlignment() const
{
	return m_eParaAlignment;
}

/// @brief 设置段落对齐方式
/// @param alignment 段落对齐枚举
void MTextEditContext::setParaAlignment(DmMTextParagraph::Alignment alignment)
{
	m_eParaAlignment = alignment;
}

/// @brief 获取对正方式
/// @return 对正方式枚举
EMTextMode MTextEditContext::getJustification() const
{
	return m_eJustification;
}

/// @brief 设置对正方式
/// @param justification 对正方式枚举
void MTextEditContext::setJustification(EMTextMode justification)
{
	m_eJustification = justification;
}

/// @brief 获取倾斜角度
/// @return 倾斜角度（弧度）
double MTextEditContext::getOblique() const
{
	return m_dOblique;
}

/// @brief 设置倾斜角度
/// @param oblique 倾斜角度（弧度）
void MTextEditContext::setOblique(const double& oblique)
{
	m_dOblique = oblique;
}

/// @brief 获取宽度因子
/// @return 宽度因子
double MTextEditContext::getWidthFactor() const
{
	return m_dWidthFactor;
}

/// @brief 设置宽度因子
/// @param widthFactor 宽度因子
void MTextEditContext::setWidthFactor(const double& widthFactor)
{
	m_dWidthFactor = widthFactor;
}

/// @brief 发出文字样式修改信号
void MTextEditContext::emitUiStyleChanged()
{
	emit uiStyleChanged();
}

/// @brief 发出字体族修改信号
void MTextEditContext::emitUiFontFamilyChanged()
{
	emit uiFontFamilyChanged();
}

/// @brief 发出颜色修改信号
void MTextEditContext::emitUiColorChanged()
{
	emit uiColorChanged();
}

/// @brief 发出文字高度修改信号
void MTextEditContext::emitUiCharHeightChanged()
{
	emit uiCharHeightChanged();
}

/// @brief 发出粗体修改信号
void MTextEditContext::emitUiBoldChanged()
{
	emit uiBoldChanged();
}

/// @brief 发出斜体修改信号
void MTextEditContext::emitUiItalicChanged()
{
	emit uiItalicChanged();
}

/// @brief 发出删除线修改信号
void MTextEditContext::emitUiStrikethroughChanged()
{
	emit uiStrikethroughChanged();
}

/// @brief 发出下划线修改信号
void MTextEditContext::emitUiUnderlineChanged()
{
	emit uiUnderlineChanged();
}

/// @brief 发出上划线修改信号
void MTextEditContext::emitUiOverlineChanged()
{
	emit uiOverlineChanged();
}

/// @brief 发出对正方式修改信号
void MTextEditContext::emitUiJustificationChanged()
{
	emit uiJustificationChanged();
}

/// @brief 发出段落对齐修改信号
void MTextEditContext::emitUiParaAlignmentChanged()
{
	emit uiParaAlignmentChanged();
}

/// @brief 发出倾斜角度修改信号
void MTextEditContext::emitUiObliqueChanged()
{
	emit uiObliqueChanged();
}

/// @brief 发出宽度因子修改信号
void MTextEditContext::emitUiWidthFactorChanged()
{
	emit uiWidthFactorChanged();
}

/// @brief 发出转小写信号
void MTextEditContext::emitUiToLower()
{
	emit uiToLower();
}

/// @brief 发出转大写信号
void MTextEditContext::emitUiToUpper()
{
	emit uiToUpper();
}

/// @brief 发出插入符号信号
/// @param symbol 符号字符串
void MTextEditContext::emitUiSymbolActivated(const QString& symbol)
{
	emit uiSymbolActivated(symbol);
}

/// @brief 发出格式刷匹配信号
void MTextEditContext::emitUiMatching()
{
	emit uiMatching();
}

/// @brief 发出撤销信号
void MTextEditContext::emitUiUndo()
{
	emit uiUndo();
}

/// @brief 发出重做信号
void MTextEditContext::emitUiRedo()
{
	emit uiRedo();
}

/// @brief 发出数据模型到选项的更新信号
void MTextEditContext::emitDmToOption()
{
	m_bIsUpdatingToOption = true;
	emit dmToOption();
	m_bIsUpdatingToOption = false;
}

/// @brief 发出ESC按下信号
/// @param save 是否保存
void MTextEditContext::emitEscPressed(bool save)
{
	emit escPressed(save);
}

/// @brief 检查是否正在更新到选项
/// @return 是否正在更新
bool MTextEditContext::IsUpdatingToOption() const
{
	return m_bIsUpdatingToOption;
}

/// @brief 发出撤销/重做状态更新到选项的信号
/// @param undoable 是否可撤销
/// @param redoable 是否可重做
void MTextEditContext::emitUndoToOption(bool undoable, bool redoable)
{
	m_bIsUpdatingToOption = true;
	emit undoToOption(undoable, redoable);
	m_bIsUpdatingToOption = false;
}
