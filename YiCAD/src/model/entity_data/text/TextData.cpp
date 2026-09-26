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

/// @file TextData.cpp
/// @brief 单行文字数据类实现

#include "TextData.h"

TextData::TextData()
	: EntityData()
	, m_strTextString("")
	, m_pTextStyle(nullptr)
	, m_dAngle(0.0)
	, m_ptPosition(DmVector(0.0, 0, 0))
	, m_eTextHorzMode(ETextHorzMode::kTextLeft)
	, m_eTextVertMode(ETextVertMode::kTextTop)
	, m_dTextHeight(2.5)
	, m_updateMode(EUpdateMode::Update)
{
	setEntityType(EEntityType::eText);
}

TextData::TextData(const DmVector& insertionPoint, double height, ETextVertMode valign, ETextHorzMode halign,
	const QString& text, DmTextStyle* style, double angle, EUpdateMode updateMode /*= EUpdateMode::Update*/)
	: m_ptPosition(insertionPoint)
	, m_dTextHeight(height)
	, m_eTextVertMode(valign)
	, m_eTextHorzMode(halign)
	, m_strTextString(text)
	, m_pTextStyle(style)
	, m_dAngle(angle)
	, m_updateMode(updateMode)
{
	setEntityType(EEntityType::eText);
}

QString TextData::getTextString() const
{
	return m_strTextString;
}

void TextData::setTextString(const QString& strTextString)
{
	m_strTextString = strTextString;
}

DmTextStyle* TextData::getTextStyle() const
{
	return m_pTextStyle;
}

void TextData::setTextStyle(DmTextStyle* pStyle)
{
	m_pTextStyle = pStyle;
}

double TextData::getAngle() const
{
	return m_dAngle;
}

void TextData::setAngle(const double& dAngle)
{
	m_dAngle = dAngle;
}

DmVector TextData::getPosition() const
{
	return m_ptPosition;
}

void TextData::setPosition(const DmVector& pt)
{
	m_ptPosition = pt;
}

DmVector TextData::getAlignment() const
{
	return m_ptAlignment;
}

void TextData::setAlignment(const DmVector& pt)
{
	m_ptAlignment = pt;
}

ETextHorzMode TextData::getTextHorzMode() const
{
	return m_eTextHorzMode;
}

void TextData::setTextHorzMode(ETextHorzMode eTextHorzMode)
{
	m_eTextHorzMode = eTextHorzMode;
}

ETextVertMode TextData::getTextVertMode() const
{
	return m_eTextVertMode;
}

void TextData::setTextVertMode(ETextVertMode eTextVertMode)
{
	m_eTextVertMode = eTextVertMode;
}

double TextData::getTextHeight() const
{
	return m_dTextHeight;
}

void TextData::setTextHeight(const double& dHeight)
{
	m_dTextHeight = dHeight;
}

EUpdateMode TextData::getUpdateMode() const
{
	return m_updateMode;
}

void TextData::setUpdateMode(EUpdateMode updateMode)
{
	m_updateMode = updateMode;
}

bool TextData::getUpsideDown() const
{
	return m_bUpsideDown;
}

void TextData::setUpsideDown(const bool& upsideDown)
{
	m_bUpsideDown = upsideDown;
}

bool TextData::getReverseDirection() const
{
	return m_bReverseDirection;
}

void TextData::setReverseDirection(const bool& reverseDirection)
{
	m_bReverseDirection = reverseDirection;
}

double TextData::getWidthFactor() const
{
	return m_dWidthFactor;
}

void TextData::setWidthFactor(const double& widthFactor)
{
	m_dWidthFactor = widthFactor;
}

double TextData::getSlashAngle() const
{
	return m_dSlashAngle;
}

void TextData::setSlashAngle(const double& slashAngle)
{
	m_dSlashAngle = slashAngle;
}

ETextMode TextData::getTextMode() const
{
	return HVToAlignment(m_eTextHorzMode, m_eTextVertMode);
}

void TextData::setTextMode(const ETextMode& align)
{
	ETextHorzMode hAlign;
	ETextVertMode vAlign;
	AlignmentToHV(align, hAlign, vAlign);
	setTextHorzMode(hAlign);
	setTextVertMode(vAlign);
}

ETextMode TextData::HVToAlignment(ETextHorzMode hAlign, ETextVertMode vAlign)
{
	/***
	* 单行文字对正效果及参数：
	* m_eTextHorzMode = kTextLeft (0) m_eTextVertMode = kTextBase (0)         ----左对齐  （对齐点坐标总为0）
	* m_eTextHorzMode = kTextCenter (1)   m_eTextVertMode = kTextBase (0)     ----居中    （对齐点x在文字中间，y在基线处）
	* m_eTextHorzMode = kTextRight (2)    m_eTextVertMode = kTextBase (0)     ----右对齐  （对齐点坐标为右基线位置，世界坐标）
	* m_eTextHorzMode = kTextAlign (3)    m_eTextVertMode = kTextBase (0)     ----对齐    （同右对齐，只是2个点可以拖拽来控制文字大小）
	* m_eTextHorzMode = kTextMid (4)      m_eTextVertMode = kTextBase (0)     ----中间    （对齐点为包围框的中心）
	* m_eTextHorzMode = kTextFit (5)      m_eTextVertMode = kTextBase (0)     ----布满    （同右对齐，只是2个点可以拖拽来控制文字拉伸，文字高度不变）
	* m_eTextHorzMode = kTextLeft (0)     m_eTextVertMode = kTextTop (3)      ----左上    （对齐点在“A”的左上，y与“A”顶部对齐）
	* m_eTextHorzMode = kTextCenter (1)   m_eTextVertMode = kTextTop (3)      ----中上    
	* m_eTextHorzMode = kTextRight (2)    m_eTextVertMode = kTextTop (3)      ----右上    
	* m_eTextHorzMode = kTextLeft (0)     m_eTextVertMode = kTextVertMid (2)  ----左中    （对齐点y在基线以上无留白部分的中心）
	* m_eTextHorzMode = kTextCenter (1)   m_eTextVertMode = kTextVertMid (2)  ----正中    
	* m_eTextHorzMode = kTextRight (2)    m_eTextVertMode = kTextVertMid (2)  ----右中
	* m_eTextHorzMode = kTextLeft (0)     m_eTextVertMode = kTextBottom (1)   ----左下    （对齐点文字块底部位置）
	* m_eTextHorzMode = kTextCenter (1)   m_eTextVertMode = kTextBottom (1)   ----中下
	* m_eTextHorzMode = kTextRight (2)    m_eTextVertMode = kTextBottom (1)   ----右下
	**/
	ETextMode align = ETextMode::kTextLeft;
	if (vAlign == ETextVertMode::kTextBase)
	{
		if (hAlign == ETextHorzMode::kTextLeft)
		{
			align = ETextMode::kTextLeft;	//左对齐
		}
		else if (hAlign == ETextHorzMode::kTextCenter)
		{
			align = ETextMode::kTextCenter;		//居中
		}
		else if (hAlign == ETextHorzMode::kTextRight)
		{
			align = ETextMode::kTextRight;		//右对齐
		}
		else if (hAlign == ETextHorzMode::kTextAlign)
		{
			align = ETextMode::kTextAligned;		//对齐
		}
		else if (hAlign == ETextHorzMode::kTextMid)
		{
			align = ETextMode::kTextMiddle;		//中间
		}
		else if (hAlign == ETextHorzMode::kTextFit)
		{
			align = ETextMode::kTextFit;		//布满
		}
	}
	else if (vAlign == ETextVertMode::kTextTop)
	{
		if (hAlign == ETextHorzMode::kTextLeft)
		{
			align = ETextMode::kTextTopLeft;		//左上
		}
		else if (hAlign == ETextHorzMode::kTextCenter)
		{
			align = ETextMode::kTextTopCenter;		//中上
		}
		else if (hAlign == ETextHorzMode::kTextRight)
		{
			align = ETextMode::kTextTopRight;		//右上
		}
	}
	else if (vAlign == ETextVertMode::kTextVertMid)
	{
		if (hAlign == ETextHorzMode::kTextLeft)
		{
			align = ETextMode::kTextMiddleLeft;		//左中
		}
		else if (hAlign == ETextHorzMode::kTextCenter)
		{
			align = ETextMode::kTextMiddleCenter;		//正中
		}
		else if (hAlign == ETextHorzMode::kTextRight)
		{
			align = ETextMode::kTextMiddleRight;		//右中
		}
	}
	else if (vAlign == ETextVertMode::kTextBottom)
	{
		if (hAlign == ETextHorzMode::kTextLeft)
		{
			align = ETextMode::kTextBottomLeft;		//左下
		}
		else if (hAlign == ETextHorzMode::kTextCenter)
		{
			align = ETextMode::kTextBottomCenter;		//中下
		}
		else if (hAlign == ETextHorzMode::kTextRight)
		{
			align = ETextMode::kTextBottomRight;		//右下
		}
	}
	return align;
}

void TextData::AlignmentToHV(ETextMode align, ETextHorzMode& hAlign, ETextVertMode& vAlign)
{
	hAlign = ETextHorzMode::kTextLeft;
	vAlign = ETextVertMode::kTextBase;
	switch (align)
	{
	case ETextMode::kTextLeft:
		hAlign = ETextHorzMode::kTextLeft;
		vAlign = ETextVertMode::kTextBase;
		break;
	case ETextMode::kTextCenter:
		hAlign = ETextHorzMode::kTextCenter;
		vAlign = ETextVertMode::kTextBase;
		break;
	case ETextMode::kTextRight:
		hAlign = ETextHorzMode::kTextRight;
		vAlign = ETextVertMode::kTextBase;
		break;
	case ETextMode::kTextAligned:
		hAlign = ETextHorzMode::kTextAlign;
		vAlign = ETextVertMode::kTextBase;
		break;
	case ETextMode::kTextMiddle:
		hAlign = ETextHorzMode::kTextMid;
		vAlign = ETextVertMode::kTextBase;
		break;
	case ETextMode::kTextFit:
		hAlign = ETextHorzMode::kTextFit;
		vAlign = ETextVertMode::kTextBase;
		break;
	case ETextMode::kTextTopLeft:
		hAlign = ETextHorzMode::kTextLeft;
		vAlign = ETextVertMode::kTextTop;
		break;
	case ETextMode::kTextTopCenter:
		hAlign = ETextHorzMode::kTextCenter;
		vAlign = ETextVertMode::kTextTop;
		break;
	case ETextMode::kTextTopRight:
		hAlign = ETextHorzMode::kTextRight;
		vAlign = ETextVertMode::kTextTop;
		break;
	case ETextMode::kTextMiddleLeft:
		hAlign = ETextHorzMode::kTextLeft;
		vAlign = ETextVertMode::kTextVertMid;
		break;
	case ETextMode::kTextMiddleCenter:
		hAlign = ETextHorzMode::kTextCenter;
		vAlign = ETextVertMode::kTextVertMid;
		break;
	case ETextMode::kTextMiddleRight:
		hAlign = ETextHorzMode::kTextRight;
		vAlign = ETextVertMode::kTextVertMid;
		break;
	case ETextMode::kTextBottomLeft:
		hAlign = ETextHorzMode::kTextLeft;
		vAlign = ETextVertMode::kTextBottom;
		break;
	case ETextMode::kTextBottomCenter:
		hAlign = ETextHorzMode::kTextCenter;
		vAlign = ETextVertMode::kTextBottom;
		break;
	case ETextMode::kTextBottomRight:
		hAlign = ETextHorzMode::kTextRight;
		vAlign = ETextVertMode::kTextBottom;
		break;
	default:
		break;
	}
}
