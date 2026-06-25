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

/// @file UITextAlignmentBox.cpp
/// @brief 文本对齐方式下拉框控件，支持水平对齐与垂直对齐的多种组合

#include "UITextAlignmentBox.h"


UITextAlignmentBox::UITextAlignmentBox(QWidget* parent /*= nullptr*/)
	: QComboBox(parent)
{
	//对齐方式
	QStringList items{
	tr("Left"),					//左对齐
	tr("Center"),				//居中
	tr("Right"),				//右对齐
	tr("Aligned"),				//对齐
	tr("Middle"),				//中间
	tr("Fit"),						//布满
	tr("Top left"),				//左上
	tr("Top center"),		//中上
	tr("Top right"),			//右上
	tr("Middle left"),		//左中
	tr("Middle center"),	//正中
	tr("Middle right"),		//右中
	tr("Bottom left"),		//左下
	tr("Bottom center"),	//中下
	tr("Bottom right"),	//右下
	};
	addItems(items);
}

void UITextAlignmentBox::setAlignment(ETextHorzMode hAlign, ETextVertMode vAlign)
{
	ETextMode align = TextData::HVToAlignment(hAlign, vAlign);
	setAlignment(align);
}

void UITextAlignmentBox::getAlignment(ETextHorzMode& hAlign, ETextVertMode& vAlign) const
{
	ETextMode align = getAlignment();
	TextData::AlignmentToHV(align, hAlign, vAlign);
}

void UITextAlignmentBox::setAlignment(ETextMode align)
{
	switch (align)
	{
	case ETextMode::kTextLeft:
		setCurrentText(tr("Left"));	//左对齐
		break;
	case ETextMode::kTextCenter:
		setCurrentText(tr("Center"));	//居中
		break;
	case ETextMode::kTextRight:
		setCurrentText(tr("Right"));	//右对齐
		break;
	case ETextMode::kTextAligned:
		setCurrentText(tr("Aligned"));	//对齐
		break;
	case ETextMode::kTextMiddle:
		setCurrentText(tr("Middle"));	//中间
		break;
	case ETextMode::kTextFit:
		setCurrentText(tr("Fit"));	//布满
		break;
	case ETextMode::kTextTopLeft:
		setCurrentText(tr("Top left"));	//左上
		break;
	case ETextMode::kTextTopCenter:
		setCurrentText(tr("Top center"));	//中上
		break;
	case ETextMode::kTextTopRight:
		setCurrentText(tr("Top right"));	//右上
		break;
	case ETextMode::kTextMiddleLeft:
		setCurrentText(tr("Middle left"));	//左中
		break;
	case ETextMode::kTextMiddleCenter:
		setCurrentText(tr("Middle center"));	//正中
		break;
	case ETextMode::kTextMiddleRight:
		setCurrentText(tr("Middle right"));	//右中
		break;
	case ETextMode::kTextBottomLeft:
		setCurrentText(tr("Bottom left"));	//左下
		break;
	case ETextMode::kTextBottomCenter:
		setCurrentText(tr("Bottom center"));	//中下
		break;
	case ETextMode::kTextBottomRight:
		setCurrentText(tr("Bottom right"));	//右下
		break;
	default:
		break;
	}
}

ETextMode UITextAlignmentBox::getAlignment() const
{
	ETextMode align = ETextMode::kTextLeft;
	if (currentText() == tr("Left"))
	{
		align = ETextMode::kTextLeft;
	}
	else if (currentText() == tr("Center"))
	{
		align = ETextMode::kTextCenter;
	}
	else if (currentText() == tr("Right"))
	{
		align = ETextMode::kTextRight;
	}
	else if (currentText() == tr("Aligned"))
	{
		align = ETextMode::kTextAligned;
	}
	else if (currentText() == tr("Middle"))
	{
		align = ETextMode::kTextMiddle;
	}
	else if (currentText() == tr("Fit"))
	{
		align = ETextMode::kTextFit;
	}
	else if (currentText() == tr("Top left"))
	{
		align = ETextMode::kTextTopLeft;
	}
	else if (currentText() == tr("Top center"))
	{
		align = ETextMode::kTextTopCenter;
	}
	else if (currentText() == tr("Top right"))
	{
		align = ETextMode::kTextTopRight;
	}
	else if (currentText() == tr("Middle left"))
	{
		align = ETextMode::kTextMiddleLeft;
	}
	else if (currentText() == tr("Middle center"))
	{
		align = ETextMode::kTextMiddleCenter;
	}
	else if (currentText() == tr("Middle right"))
	{
		align = ETextMode::kTextMiddleRight;
	}
	else if (currentText() == tr("Bottom left"))
	{
		align = ETextMode::kTextBottomLeft;
	}
	else if (currentText() == tr("Bottom center"))
	{
		align = ETextMode::kTextBottomCenter;
	}
	else if (currentText() == tr("Bottom right"))
	{
		align = ETextMode::kTextBottomRight;
	}
	return align;
}
