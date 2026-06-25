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


/// @file ActionDrawMText.h
/// @brief 多行文字绘制与编辑动作类头文件

#ifndef ACTIONDRAWMTEXT_H
#define ACTIONDRAWMTEXT_H

#include "PreviewActionInterface.h"
#include "MTextData.h"
#include "DmMTextParagraph.h"
#include "Transaction.h"

class MTextEditWidget;
class UIMTextOptions;
class QDialog;
class DmFont;
class DmDocument;
class DmMText;
class ActionDrawMTextContext;

/// @brief 绘制多行文字，双击编辑多行文字Action
class ActionDrawMText : public PreviewActionInterface
{
	Q_OBJECT
public:
	/// @brief 动作状态枚举
	enum Status
	{
		DrawingBoundingBox, ///< 绘制边界框
		Editing             ///< 编辑中
	};

public:
	/// @brief 构造函数
	/// @param doc 文档指针
	/// @param docView 文档视图指针
	/// @param isModify 是否为修改模式
	ActionDrawMText(DmDocument* doc, GuiDocumentView* docView, bool isModify);

	/// @brief 析构函数
	~ActionDrawMText() = default;

	/// @brief 初始化动作
	/// @param status 初始状态，默认为0
	void init(int status = 0) override;

	/// @brief 重置动作状态
	void reset();

	/// @brief 触发动作执行（完成文字创建或修改）
	void trigger() override;

	/// @brief 检查动作是否可被中断
	/// @return 始终返回false，此动作不可被中断
	bool canBeInterrupt() override;

	/// @brief 结束动作
	/// @param updateTB 是否更新工具栏
	void finish(bool updateTB = true) override;

	/// @brief 鼠标移动事件处理
	/// @param e 鼠标事件指针
	void mouseMoveEvent(QMouseEvent* e) override;

	/// @brief 鼠标释放事件处理
	/// @param e 鼠标事件指针
	void mouseReleaseEvent(QMouseEvent* e) override;

	/// @brief 鼠标按下事件处理
	/// @param e 鼠标事件指针
	void mousePressEvent(QMouseEvent* e) override;

	/// @brief 坐标事件处理
	/// @param e 坐标事件指针
	void coordinateEvent(GuiCoordinateEvent* e) override;

	/// @brief 命令事件处理
	/// @param e 命令事件指针
	void commandEvent(GuiCommandEvent* e) override;

	/// @brief 更新鼠标按钮提示信息
	void updateMouseButtonHints() override;

	/// @brief 更新鼠标光标样式
	void updateMouseCursor() override;

	/// @brief 聚焦编辑控件
	void focusEditWidget();

	/// @brief 获取关联的文档
	/// @return 文档指针
	DmDocument* getDocument();

	/// @brief 初始化并显示选项及编辑框
	void initDisplayDialogs();

	/// @brief 设置修改模式的数据
	/// @param pOriginText 原始文字实体指针
	/// @param clickPt 点击位置
	void setModifyData(DmMText* pOriginText, const DmVector& clickPt);

	/// @brief 取消当前操作
	void cancel();

	/// @brief 获取上下文对象
	/// @return 上下文对象指针
	ActionDrawMTextContext* getContext();

private slots:
	/// @brief ESC键按下槽函数
	/// @param save 是否保存
	void slotEscPressed(bool save);

private:
	/// @brief 绘制边界框
	void drawBoundingBox();

	/// @brief 释放UI资源
	void freeUI();

protected:
	struct Points;
	std::unique_ptr<Points> pPoints;                     ///< 点集（位置信息）
	MTextEditWidget* m_pEditWidget = nullptr;            ///< 文字编辑控件指针
	QWidget* m_pOptionBack = nullptr;                    ///< 选项背景控件指针
	UIMTextOptions* m_pOptionWidget = nullptr;           ///< 选项控件指针
	DmMText* m_pOriginText = nullptr;                    ///< 原始文字，仅对"修改文字"命令下有效
	DmMText* m_pEditingText = nullptr;                   ///< 正在编辑的文字指针
	bool m_bIsModify;                                    ///< 是否为"双击修改文字"命令
	std::shared_ptr<TransactionGroup> m_trans;           ///< 事务组
	std::unique_ptr<ActionDrawMTextContext> m_context;   ///< 当前上下文信息
};

/// @brief 动作绘制多行文字的点集结构体
struct ActionDrawMText::Points
{
	DmVector pos{ false };       ///< 新建时第一个角点
	DmVector secPos{ false };    ///< 新建时第二个角点

	DmVector clickPt{ false };   ///< 双击编辑时点击的点
};

/// @brief 多行文字绘制上下文类，管理当前编辑状态和样式信息
class ActionDrawMTextContext : public QObject
{
	Q_OBJECT
public:
	/// @brief 默认构造函数
	ActionDrawMTextContext();

	/// @brief 拷贝构造函数
	/// @param context 要拷贝的上下文对象
	ActionDrawMTextContext(const ActionDrawMTextContext& context);

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

#endif // !ACTIONDRAWMTEXT_H
