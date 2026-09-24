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

/// @file MTextEditContext.h
/// @brief 多行文字编辑上下文 MTextEditContext

#ifndef MTEXTEDITCONTEXT_H
#define MTEXTEDITCONTEXT_H

#include <QObject>
#include <QString>

#include "DmColor.h"
#include "DmMTextParagraph.h"
#include "MTextData.h"

class DmDocument;
class DmFont;
class DmTextStyle;

/// @brief 多行文字编辑的上下文：当前的样式、字体、字高、颜色等编辑状态，在编辑框与选项条之间
///        同步（原 ActionDrawMTextContext，定义在 ActionDrawMText.h）
class MTextEditContext : public QObject
{
	Q_OBJECT
public:
	/// @brief 默认构造函数
	MTextEditContext();

	/// @brief 拷贝构造函数
	/// @param context 要拷贝的上下文对象
	MTextEditContext(const MTextEditContext& context);

	/// @brief 初始化上下文
	/// @param pDocument 文档指针
	void init(DmDocument* pDocument);

	// 获取/设置当前信息

	/// @brief 获取当前文字样式
	/// @return 文字样式指针
	DmTextStyle* getCurTextStyle() const;

	/// @brief 设置当前文字样式
	/// @param style 文字样式指针
	void setCurTextStyle(DmTextStyle* style);

	/// @brief 获取当前文字高度
	/// @return 文字高度
	double getCurCharHeight() const;

	/// @brief 设置当前文字高度
	/// @param charHeight 文字高度
	void setCurCharHeight(const double& charHeight);

	/// @brief 获取当前字体族名称
	/// @return 字体族名称
	QString getCurFontFamilyName() const;

	/// @brief 设置当前字体族名称
	/// @param name 字体族名称
	void setCurFontFamilyName(const QString& name);

	/// @brief 获取是否粗体
	/// @return 是否粗体
	bool getIsBold() const;

	/// @brief 设置是否粗体
	/// @param isBold 是否粗体
	void setIsBold(const bool& isBold);

	/// @brief 获取是否斜体
	/// @return 是否斜体
	bool getIsItalic() const;

	/// @brief 设置是否斜体
	/// @param isItalic 是否斜体
	void setIsItalic(const bool& isItalic);

	/// @brief 获取是否有上划线
	/// @return 是否有上划线
	bool getHasOverline() const;

	/// @brief 设置是否有上划线
	/// @param has 是否有上划线
	void setHasOverline(const bool& has);

	/// @brief 获取是否有下划线
	/// @return 是否有下划线
	bool getHasUnderline() const;

	/// @brief 设置是否有下划线
	/// @param has 是否有下划线
	void setHasUnderline(const bool& has);

	/// @brief 获取是否有删除线
	/// @return 是否有删除线
	bool getHasStrikethrough() const;

	/// @brief 设置是否有删除线
	/// @param has 是否有删除线
	void setHasStrikethrough(const bool has);

	/// @brief 获取是否正在使用格式刷
	/// @return 是否匹配
	bool getIsMatching() const;

	/// @brief 设置是否使用格式刷
	/// @param match 是否匹配
	void setIsMatching(const bool& match);

	/// @brief 获取当前颜色
	/// @return 颜色对象
	DmColor getCurColor() const;

	/// @brief 设置当前颜色
	/// @param color 颜色对象
	void setCurColor(const DmColor& color);

	/// @brief 获取段落对齐方式
	/// @return 段落对齐枚举
	DmMTextParagraph::Alignment getParaAlignment() const;

	/// @brief 设置段落对齐方式
	/// @param alignment 段落对齐枚举
	void setParaAlignment(DmMTextParagraph::Alignment alignment);

	/// @brief 获取对正方式
	/// @return 对正方式枚举
	EMTextMode getJustification() const;

	/// @brief 设置对正方式
	/// @param justification 对正方式枚举
	void setJustification(EMTextMode justification);

	/// @brief 获取倾斜角度
	/// @return 倾斜角度（弧度）
	double getOblique() const;

	/// @brief 设置倾斜角度
	/// @param oblique 倾斜角度（弧度）
	void setOblique(const double& oblique);

	/// @brief 获取宽度因子
	/// @return 宽度因子
	double getWidthFactor() const;

	/// @brief 设置宽度因子
	/// @param widthFactor 宽度因子
	void setWidthFactor(const double& widthFactor);

	// 发送选项UI修改信号

	/// @brief 发出文字样式修改信号
	void emitUiStyleChanged();

	/// @brief 发出字体族修改信号
	void emitUiFontFamilyChanged();

	/// @brief 发出颜色修改信号
	void emitUiColorChanged();

	/// @brief 发出文字高度修改信号
	void emitUiCharHeightChanged();

	/// @brief 发出粗体修改信号
	void emitUiBoldChanged();

	/// @brief 发出斜体修改信号
	void emitUiItalicChanged();

	/// @brief 发出删除线修改信号
	void emitUiStrikethroughChanged();

	/// @brief 发出下划线修改信号
	void emitUiUnderlineChanged();

	/// @brief 发出上划线修改信号
	void emitUiOverlineChanged();

	/// @brief 发出对正方式修改信号
	void emitUiJustificationChanged();

	/// @brief 发出段落对齐修改信号
	void emitUiParaAlignmentChanged();

	/// @brief 发出倾斜角度修改信号
	void emitUiObliqueChanged();

	/// @brief 发出宽度因子修改信号
	void emitUiWidthFactorChanged();

	/// @brief 发出转小写信号
	void emitUiToLower();

	/// @brief 发出转大写信号
	void emitUiToUpper();

	/// @brief 发出插入符号信号
	/// @param symbol 符号字符串
	void emitUiSymbolActivated(const QString& symbol);

	/// @brief 发出格式刷匹配信号
	void emitUiMatching();

	/// @brief 发出撤销信号
	void emitUiUndo();

	/// @brief 发出重做信号
	void emitUiRedo();

	// 发送编辑框信号

	/// @brief 发出数据模型到选项的更新信号
	void emitDmToOption();

	/// @brief 发出ESC按下信号
	/// @param save 是否保存
	void emitEscPressed(bool save);

	/// @brief 检查是否正在更新到选项
	/// @return 是否正在更新
	bool IsUpdatingToOption() const;

	/// @brief 发出撤销/重做状态更新到选项的信号
	/// @param undoable 是否可撤销
	/// @param redoable 是否可重做
	void emitUndoToOption(bool undoable, bool redoable);

signals:
	/// @brief 文字样式更改信号
	void uiStyleChanged();

	/// @brief 字体族更改信号
	void uiFontFamilyChanged();

	/// @brief 颜色更改信号
	void uiColorChanged();

	/// @brief 文字高度更改信号
	void uiCharHeightChanged();

	/// @brief 粗体更改信号
	void uiBoldChanged();

	/// @brief 斜体更改信号
	void uiItalicChanged();

	/// @brief 删除线更改信号
	void uiStrikethroughChanged();

	/// @brief 下划线更改信号
	void uiUnderlineChanged();

	/// @brief 上划线更改信号
	void uiOverlineChanged();

	/// @brief 对正方式更改信号
	void uiJustificationChanged();

	/// @brief 段落对齐更改信号
	void uiParaAlignmentChanged();

	/// @brief 倾斜角度更改信号
	void uiObliqueChanged();

	/// @brief 宽度因子更改信号
	void uiWidthFactorChanged();

	/// @brief 转小写信号
	void uiToLower();

	/// @brief 转大写信号
	void uiToUpper();

	/// @brief 插入符号信号
	/// @param symbol 符号字符串
	void uiSymbolActivated(const QString& symbol);

	/// @brief 格式刷匹配信号
	void uiMatching();

	/// @brief 撤销信号
	void uiUndo();

	/// @brief 重做信号
	void uiRedo();

	/// @brief 请求更新到选项的信号
	void dmToOption();

	/// @brief 编辑中ESC按下消息
	void escPressed(bool save);

	/// @brief 请求更新undo，redo状态到选项
	/// @param undoable 是否可撤销
	/// @param redoable 是否可重做
	void undoToOption(bool undoable, bool redoable);

private:
	bool m_bIsUpdatingToOption;           ///< 是否准备更新到选项（用来避免编辑框信息更新到选项后，选项修改又更新到选中文字的问题）

	DmTextStyle* m_pCurTextStyle;          ///< 当前文字样式指针
	QString m_strCurFontFamilyName;        ///< 字体（族）名
	DmFont* m_curFont;                     ///< 当前字体指针
	DmColor m_curColor;                    ///< 当前颜色
	double m_dCurCharHeight;               ///< 当前文字高度
	bool m_isBold;                         ///< 是否粗体
	bool m_isItalic;                       ///< 是否斜体
	bool m_hasStrikethrough;               ///< 是否有删除线
	bool m_hasUnderline;                   ///< 是否有下划线
	bool m_hasOverline;                    ///< 是否有上划线
	bool m_bIsMatching;                    ///< 是否正在使用格式刷
	EMTextMode m_eJustification;           ///< 对正方式
	DmMTextParagraph::Alignment m_eParaAlignment; ///< 段落对齐方式
	double m_dOblique;                     ///< 倾斜角度（弧度角）
	double m_dWidthFactor;                 ///< 宽度因子
};

#endif  // MTEXTEDITCONTEXT_H
