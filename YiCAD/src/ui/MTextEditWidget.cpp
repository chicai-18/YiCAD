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

/// @file MTextEditWidget.cpp
/// @brief 多行文字编辑器控件，支持文字插入、删除、格式刷匹配、选择高亮、剪贴板操作和Undo/Redo

#include "MTextEditWidget.h"
#include "TextConsts.h"
#include <QWheelEvent>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QPixmap>
#include <QSvgRenderer>
#include <QPainter>
#include <QRectF>

#include "DmColor.h"
#include "DmLine.h"
#include "DmText.h"
#include "DmSolid.h"
#include "DmChar.h"
#include "DmMTextParagraph.h"
#include "DmMTextLine.h"
#include "DmDocument.h"
#include "DmFont.h"
#include "DmFontList.h"
#include "DmCharTemplate.h"
#include "ActionDrawMText.h"
#include "ApplicationWindow.h"
#include "MTextEditCmd.h"


MTextEditWidget::MTextEditWidget(DmMText* mtext, GuiDocumentView* docView, ActionDrawMText* action, QWidget* parent, Qt::WindowFlags f)
	:GuiPreviewWidget(parent, f)
	, m_pDocumentView(docView)
	, m_pMText(mtext)
	, m_pAction(action)
	, m_pCursor(nullptr)
	, m_pContext(nullptr)
	, m_matchContext(nullptr)
	, m_pSelectCover(nullptr)
	, m_cursorPreChar(nullptr)
	, m_cursorPostChar(nullptr)
    //, m_cursorIdx(-1)
	, m_selectBeginPreChar(nullptr)
	, m_selectBeginPostChar(nullptr)
	, m_selectEndPreChar(nullptr)
	, m_selectEndPostChar(nullptr)
{
	if (action)
	{
		m_pContext = m_pAction->getContext();
		connect(m_pContext, SIGNAL(uiStyleChanged()), this, SLOT(slotTextStyleChanged()));
		connect(m_pContext, SIGNAL(uiFontFamilyChanged()), this, SLOT(slotFormatChanged()));
		connect(m_pContext, SIGNAL(uiColorChanged()), this, SLOT(slotFormatChanged()));
		connect(m_pContext, SIGNAL(uiCharHeightChanged()), this, SLOT(slotFormatChanged()));
		connect(m_pContext, SIGNAL(uiBoldChanged()), this, SLOT(slotFormatChanged()));
		connect(m_pContext, SIGNAL(uiItalicChanged()), this, SLOT(slotFormatChanged()));
		connect(m_pContext, SIGNAL(uiStrikethroughChanged()), this, SLOT(slotFormatChanged()));
		connect(m_pContext, SIGNAL(uiUnderlineChanged()), this, SLOT(slotFormatChanged()));
		connect(m_pContext, SIGNAL(uiOverlineChanged()), this, SLOT(slotFormatChanged()));
		connect(m_pContext, SIGNAL(uiJustificationChanged()), this, SLOT(slotJustificationChanged()));
		connect(m_pContext, SIGNAL(uiParaAlignmentChanged()), this, SLOT(slotParaAlignmentChanged()));
		connect(m_pContext, SIGNAL(uiObliqueChanged()), this, SLOT(slotFormatChanged()));
		connect(m_pContext, SIGNAL(uiWidthFactorChanged()), this, SLOT(slotFormatChanged()));
		connect(m_pContext, SIGNAL(uiSymbolActivated(QString)), this, SLOT(slotAddSymbol(QString)));
		connect(m_pContext, SIGNAL(uiMatching()), this, SLOT(slotMatch()));
        connect(m_pContext, SIGNAL(uiUndo()), this, SLOT(slotUndo()));
        connect(m_pContext, SIGNAL(uiRedo()), this, SLOT(slotRedo()));
	}
    connect(&m_cmdManager, SIGNAL(cmdChanged()), this, SLOT(slotCmdChanged()));
	//设置FocusPolicy，Attribute才能让编辑窗体获得输入事件
	setFocusPolicy(Qt::StrongFocus);	
	setAttribute(Qt::WA_InputMethodEnabled, true);
	//m_pEditWidget->close()不会立刻析构，需特殊处理
	setAttribute(Qt::WA_DeleteOnClose, true);
	setMouseTracking(true);//监视鼠标移动事件

	background = DmColor(100, 100, 100, 100);
	//background = DmColor(0, 0, 0, 0);
	container = new DmEntityContainer(nullptr, false);
    //m_pMText->setParent(container);
	container->addEntity(m_pMText);
	m_timer = std::make_unique<QTimer>();
	m_timer->start(500);
	QObject::connect(m_timer.get(), &QTimer::timeout, [this]() { this->slotTimerTrigger(); });
    m_matchCurxor.reset(createSvgCursor(":/ribbon/cursor_style/text_match.svg",QSize(32, 32), QPoint(8, 8)));
	m_curStatus = Status::Normal;
	connect(docView, SIGNAL(viewChanged()), this, SLOT(slotViewChanged()));
}

MTextEditWidget::~MTextEditWidget()
{
    if (m_timer.get())
    {
        m_timer->stop();
    }
    if (m_pCursor)
    {
        delete m_pCursor;
        m_pCursor = nullptr;
    }
    if (m_pSelectCover)
    {
        delete m_pSelectCover;
        m_pSelectCover = nullptr;
    }
    delete container;
}

QCursor* MTextEditWidget::createSvgCursor(const QString& svgPath, const QSize& size, const QPoint& hotSpot)
{
    // 创建SVG渲染器
    QSvgRenderer renderer(svgPath);
    if (!renderer.isValid()) {
        qWarning("Invalid SVG file");
        return new QCursor(Qt::ArrowCursor);
    }

    // 创建透明背景图像
    QImage image(size, QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    // 绘制SVG到图像
    QPainter painter(&image);
    renderer.render(&painter,  image.rect());
    painter.end();

    // 转换为QPixmap并创建光标
    QPixmap pixmap = QPixmap::fromImage(image);
    return new QCursor(pixmap, hotSpot.x(), hotSpot.y());
}

void MTextEditWidget::setCornersForNew(const DmVector& leftTop, const DmVector& rightBottom)
{
	m_leftTop = leftTop;
	m_rightBottom = rightBottom;
	updateGeometry();
	m_pMText->init(leftTop, rightBottom.x - leftTop.x, leftTop.y - rightBottom.y, m_pContext->getCurCharHeight());
	setEditCursorPos(0, 0, true);
}

void MTextEditWidget::setCornersForModify(const DmVector& leftTop, const DmVector& rightBottom, const DmVector& clickPt)
{
	m_leftTop = leftTop;
	m_rightBottom = rightBottom;
	updateGeometry();
	if (clickPt.valid)
	{
		int x = (int)toGuiX(clickPt.x);
		int y = (int)toGuiY(clickPt.y);
		setEditCursorPos(x, y, true);
	}
}

void MTextEditWidget::updateGeometry()
{
    m_factor = DmVector(1.0, 1.0) / m_pDocumentView->getFactor();
    double width = m_pDocumentView->toGuiDX(m_rightBottom.x - m_leftTop.x);
    double height = m_pDocumentView->toGuiDY(m_leftTop.y - m_rightBottom.y);
    DmVector pos = m_pDocumentView->toGui(m_leftTop);
    setGeometry((int)pos.x, (int)(pos.y), (int)width, (int)height);
}

void MTextEditWidget::increaseHeightIfNotEnough()
{
	auto data = m_pMText->getDataConstPtr();
	double defineHeight = data->getDefineHeight();
	double minHeight = m_pMText->calculateHeight();;
	if (defineHeight < minHeight)	//如果定义高度为比较小，计算真实高度来代替
	{
		defineHeight = minHeight;
        double guiHeight = m_pDocumentView->toGuiDY(defineHeight);
        auto resizeCmd = new MTextEdit_ResizeCmd(this, width(), (int)guiHeight);
        m_cmdManager.addAndExecuteCmd(resizeCmd);

        double originHeight = m_pMText->getHeight();
        auto textHeightCmd = new EntitySetParaCmd2<DmMText, double, const double>(&DmMText::setHeight, m_pMText, defineHeight, originHeight);
        m_cmdManager.addAndExecuteCmd(textHeightCmd);
	}
}

void MTextEditWidget::updateCorners()
{
	QRect rect = geometry();
	double worldWidth = m_pDocumentView->toGraphDX(rect.width());
	double worldHeight = m_pDocumentView->toGraphDY(rect.height());
	m_rightBottom = m_leftTop + DmVector(worldWidth, -worldHeight);
	m_pMText->setWidth(m_rightBottom.x - m_leftTop.x);
	m_pMText->setHeight(m_leftTop.y - m_rightBottom.y);
	m_pMText->updateTextPosition();
	setEditCursorPos(false);
}

bool MTextEditWidget::isDrawCursor() const
{
	return m_pCursor != nullptr;
}

bool MTextEditWidget::isMatching() const
{
	return m_matchContext.get() != nullptr;
}

DmChar* MTextEditWidget::createChar(QString& charStr)
{
	double height = m_pContext->getCurCharHeight();
	auto style = m_pContext->getCurTextStyle();
	auto color = m_pContext->getCurColor();
	QString fontName = m_pContext->getCurFontFamilyName();
	bool isBold = m_pContext->getIsBold();
	bool isItalic = m_pContext->getIsItalic();
	double widthFactor = m_pContext->getWidthFactor();
	double slashAngle = m_pContext->getOblique();
	bool hasUnderline = m_pContext->getHasUnderline();
	bool hasStrikethrough = m_pContext->getHasStrikethrough();
	bool hasOverline = m_pContext->getHasOverline();
	DmChar* c = DmMText::createChar(charStr, style, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
	return c;
}

DmChar* MTextEditWidget::createChar(QString& charStr, const DmChar* preChar, const DmChar* postChar)
{
    DmChar* theChar = const_cast<DmChar*>(preChar);
    if (nullptr == theChar) {
        theChar = const_cast<DmChar*>(postChar);
    }
    if (nullptr == theChar) {
        return createChar(charStr);
    }
    else{
        double height = theChar->getHeight();
        auto style = m_pContext->getCurTextStyle();
        auto color = theChar->getPen().getColor();
        QString fontName = m_pContext->getCurFontFamilyName();
        bool isBold = theChar->isBold();
        bool isItalic = theChar->isItalic();
        double widthFactor = theChar->getWidthFactor();
        double slashAngle = theChar->getSlashAngle();
        bool hasUnderline = theChar->hasUnderline();
        bool hasStrikethrough = theChar->hasStrikethrough();
        bool hasOverline = theChar->hasOverline();
        DmChar* c = DmMText::createChar(charStr, style, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
        return c;
    }
}

void MTextEditWidget::setSelectBeginEndToNull(bool needUndo)
{
    if (needUndo) {
        MTextEdit_SetSelectBeginEndToNull_Cmd* cmd= new MTextEdit_SetSelectBeginEndToNull_Cmd(m_pMText, this);
        m_cmdManager.addAndExecuteCmd(cmd);
    }
    else{
        m_selectBeginPreChar = nullptr;
        m_selectBeginPostChar = nullptr;
        m_selectEndPreChar = nullptr;
        m_selectEndPostChar = nullptr;
    }
}

void MTextEditWidget::insertTextAtCursor(const QString& str)
{
    m_cmdManager.start((QString("insert string: ") + str).toStdString());
	if (hasSelectedChar())
	{
		deleteSelected();
	}
	else
	{
		locateChar();
	}

	//插入文字
	QString tempStr = str;
	std::vector<DmChar*> insertChars;
	for (auto tempC : tempStr)
	{
		QString charStr(tempC);
		DmChar* c = createChar(charStr);
		if (nullptr == c)
		{
			continue;
		}
		insertChars.emplace_back(c);
	}

    DmMTextParagraph* para = m_pMText->getParagraph(m_cursorPreChar, m_cursorPostChar);
    int idx;
    para->locateCharIndex(m_cursorPreChar, m_cursorPostChar, idx);
    MTextEdit_AddCharsCmd* addCharsCmd = new MTextEdit_AddCharsCmd(m_pMText, para, insertChars, idx);
    m_cmdManager.addAndExecuteCmd(addCharsCmd);
    m_pMText->updateTextPosition();

    //设置新光标坐标
	if (insertChars.size() != 0)
    {
        DmChar* lastC = insertChars.back();
        double nominalHeight = lastC->getNominalHeight();
//        auto newPos = lastC->getPosition() + DmVector(lastC->getWidth() + nominalHeight * CHARGAPFACTOR, 0.0);
//        auto cursorCmd = new EntityChangeCmd<MTextEditWidget,DmVector,&MTextEditWidget::m_currentCursorPos>(this, newPos, m_currentCursorPos);
//        m_cmdManager.addAndExecuteCmd(cursorCmd);
        m_currentCursorPos = lastC->getPosition() + DmVector(lastC->getWidth() + nominalHeight * CHARGAPFACTOR, 0.0);
        locateChar(true);
        setEditCursorPos(false);
    }

    setSelectBeginEndToNull(false);
    setSelectBeginEndToNull(true);
	//绘制光标
	drawEditCursorImmediately(true);

	increaseHeightIfNotEnough();
    m_cmdManager.done();
}

void MTextEditWidget::matchSelectedChars(const ActionDrawMTextContext* context)
{
	if (nullptr == context)
	{
		return;
	}
	if (!hasSelectedChar())
	{
		return;
	}

	//获得选择的文字
	DmChar* startChar = nullptr;
	DmChar* endChar = nullptr;
	m_pMText->getStartEndCharByPos(startChar, endChar, m_selectBeginPreChar, m_selectBeginPostChar, m_selectEndPreChar, m_selectEndPostChar);
	if (startChar == nullptr || endChar == nullptr)	//啥也没选中
	{
		return;
	}

	// 获得要匹配得格式
	double height = context->getCurCharHeight();
	auto style = context->getCurTextStyle();
	auto color = context->getCurColor();
	QString fontName = context->getCurFontFamilyName();
	bool isBold = context->getIsBold();
	bool isItalic = context->getIsItalic();
	double widthFactor = context->getWidthFactor();
	double slashAngle = context->getOblique();
	bool hasUnderline = context->getHasUnderline();
	bool hasStrikethrough = context->getHasStrikethrough();
	bool hasOverline = context->getHasOverline();

    DmChar* oldCursorPreChar = m_cursorPreChar;
    DmChar* oldCursorPostChar = m_cursorPostChar;
    DmChar* oldSelectBeginPreChar = m_selectBeginPreChar;
    DmChar* oldSelectBeginPostChar = m_selectBeginPostChar;
    DmChar* oldSelectEndPreChar = m_selectEndPreChar;
    DmChar* oldSelectEndPostChar = m_selectEndPostChar;

    //替换操作
	const std::initializer_list<DmChar**> refChars{ &m_cursorPreChar,  &m_cursorPostChar , &m_selectBeginPreChar , &m_selectBeginPostChar , &m_selectEndPreChar , &m_selectEndPostChar };
	matchChars(startChar, endChar, refChars, style, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);

    auto precharChangeCmd = new EntityChangeCmd<MTextEditWidget,DmChar*,&MTextEditWidget::m_cursorPreChar>(this, m_cursorPreChar, oldCursorPreChar);
    m_cmdManager.addAndExecuteCmd(precharChangeCmd);
    auto postcharChangeCmd = new EntityChangeCmd<MTextEditWidget,DmChar*,&MTextEditWidget::m_cursorPostChar>(this, m_cursorPostChar, oldCursorPostChar);
    m_cmdManager.addAndExecuteCmd(postcharChangeCmd);
    auto selectBeginPreCharCmd = new EntityChangeCmd<MTextEditWidget,DmChar*,&MTextEditWidget::m_selectBeginPreChar>(this, m_selectBeginPreChar, oldSelectBeginPreChar);
    m_cmdManager.addAndExecuteCmd(selectBeginPreCharCmd);
    auto selectBeginPostCharCmd = new EntityChangeCmd<MTextEditWidget,DmChar*,&MTextEditWidget::m_selectBeginPostChar>(this, m_selectBeginPostChar, oldSelectBeginPostChar);
    m_cmdManager.addAndExecuteCmd(selectBeginPostCharCmd);
    auto selectEndPreCharCmd = new EntityChangeCmd<MTextEditWidget,DmChar*,&MTextEditWidget::m_selectEndPreChar>(this, m_selectEndPreChar, oldSelectEndPreChar);
    m_cmdManager.addAndExecuteCmd(selectEndPreCharCmd);
    auto selectEndPostCharCmd = new EntityChangeCmd<MTextEditWidget,DmChar*,&MTextEditWidget::m_selectEndPostChar>(this, m_selectEndPostChar, oldSelectEndPostChar);
    m_cmdManager.addAndExecuteCmd(selectEndPostCharCmd);
}

bool MTextEditWidget::charShouldBeReplacedInMatching(DmChar* c, const double& widthFactor, const double& slashAngle, const QString& fontName, const bool& isBold, const bool& isItalic) const
{
	if (c == nullptr)
	{
		return false;
	}
	DmCharTemplateList* templList = c->getCharTemplate()->getOwner();
	if (templList == nullptr)
	{
		return false;
	}
	DmFont* originFont = templList->getFont();
	if (originFont == nullptr)
	{
		return false;
	}

	//需匹配的字体与原字体不一样，需要替换
	bool bold = isBold;
	bool italic = isItalic;
	DmFont* f = DMFONTLIST->requestFontCloset(fontName, bold, italic);
	if (f != originFont)
	{
		return true;
	}

	//宽度系数，倾斜角度不一样，需要替换
	if (widthFactor != c->getWidthFactor())
	{
		return true;
	}
	if (slashAngle != c->getSlashAngle())
	{
		return true;
	}

	return false;
}

void MTextEditWidget::matchChar(std::unordered_map<DmChar*, DmChar*>& origin_replaced_map, DmChar* originChar, const DmTextStyle* style, const DmColor& color, const double& height, const double& widthFactor, const double& slashAngle, const QString& fontName, const bool& bold, const bool& italic, const bool& hasUnderline, const bool& hasStrikethrough, const bool& hasOverline)
{
	// 是否需要重新创建文字
	bool shouldReplace = charShouldBeReplacedInMatching(originChar, widthFactor, slashAngle, fontName, bold, italic);
	if (shouldReplace)
	{
		DmChar* c = DmMText::createChar(originChar->getName(), style, color, height, widthFactor, slashAngle, fontName, bold, italic, hasUnderline, hasStrikethrough, hasOverline);
        MTextEdit_ReplaceCharCmd* replaceCmd = new MTextEdit_ReplaceCharCmd(m_pMText, originChar, c);
        m_cmdManager.addAndExecuteCmd(replaceCmd);
		origin_replaced_map.insert(std::make_pair(originChar, c));
	}
	// 仅仅修改上下划线等格式，不重新创建文字
	else
	{
		double originHeight = originChar->getNominalHeight();
		if (originHeight != height)
		{
			double scale = height / originHeight;
            MTextEdit_ModifyCharCmd* scaleCmd = new MTextEdit_ModifyCharCmd(originChar, scale);
            m_cmdManager.addAndExecuteCmd(scaleCmd);
		}
		DmColor originColor = originChar->getPen(false).getColor();
		if (originColor != color)
		{
            MTextEdit_ModifyCharCmd* colorCmd = new MTextEdit_ModifyCharCmd(originChar, color);
            m_cmdManager.addAndExecuteCmd(colorCmd);
		}
		if (originChar->hasUnderline() != hasUnderline)
		{
			if (hasUnderline)
			{
                MTextEdit_ModifyCharCmd* addUnderlineCmd = new MTextEdit_ModifyCharCmd(originChar, MTextEdit_ModifyCharCmd::SubCmdType::AddUnderline);
                m_cmdManager.addAndExecuteCmd(addUnderlineCmd);
			}
			else
			{
                MTextEdit_ModifyCharCmd* removeUnderlineCmd = new MTextEdit_ModifyCharCmd(originChar, MTextEdit_ModifyCharCmd::SubCmdType::RemoveUnderline);
                m_cmdManager.addAndExecuteCmd(removeUnderlineCmd);
			}
		}
		if (originChar->hasStrikethrough() != hasStrikethrough)
		{
			if (hasStrikethrough)
			{
                MTextEdit_ModifyCharCmd* addStrikethrough = new MTextEdit_ModifyCharCmd(originChar, MTextEdit_ModifyCharCmd::SubCmdType::AddStrikethrough);
                m_cmdManager.addAndExecuteCmd(addStrikethrough);
			}
			else
			{
                MTextEdit_ModifyCharCmd* removeStrikethrough = new MTextEdit_ModifyCharCmd(originChar, MTextEdit_ModifyCharCmd::SubCmdType::RemoveStrikethrough);
                m_cmdManager.addAndExecuteCmd(removeStrikethrough);
			}
		}
		if (originChar->hasOverline() != hasOverline)
		{
			if (hasOverline)
			{
                MTextEdit_ModifyCharCmd* addOverline = new MTextEdit_ModifyCharCmd(originChar, MTextEdit_ModifyCharCmd::SubCmdType::AddOverline);
                m_cmdManager.addAndExecuteCmd(addOverline);
			}
			else
			{
                MTextEdit_ModifyCharCmd* removeOverline = new MTextEdit_ModifyCharCmd(originChar, MTextEdit_ModifyCharCmd::SubCmdType::RemoveOverline);
                m_cmdManager.addAndExecuteCmd(removeOverline);
			}
		}
	}
}

void MTextEditWidget::matchChars(DmChar* startChar, DmChar* endChar, const std::initializer_list<DmChar**>& refChars, const DmTextStyle* style, const DmColor& color, const double& height, const double& widthFactor, const double& slashAngle, const QString& fontName, const bool& isBold, const bool& isItalic, const bool& hasUnderline, const bool& hasStrikethrough, const bool& hasOverline)
{
	std::unordered_map<DmChar*, DmChar*> origin_replaced_map;
	//仅选择了一个字符
	if (startChar == endChar)
	{
		matchChar(origin_replaced_map, startChar, style, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
	}
	//选择了多个字符
	else
	{
		matchChar(origin_replaced_map, startChar, style, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
        DmChar* c = nullptr; //用于迭代
        auto it = origin_replaced_map.find(startChar); //字体改变的文字会直接替换，取替换之后的
        if (it != origin_replaced_map.end()) {
            c = it->second;
        }
        else{
            c = const_cast<DmChar*>(startChar);
        }
		c = DmMText::getPostChar(c, true, true);
		while (c && c != endChar)
		{
			matchChar(origin_replaced_map, c, style, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
            auto it2 = origin_replaced_map.find(c); //字体改变的文字会直接替换，取替换之后的
            if (it2 != origin_replaced_map.end()) {
                c = it2->second;
            }
            c = DmMText::getPostChar(c, true, true);
		}
		if (c && c == endChar)
		{
			matchChar(origin_replaced_map, c, style, color, height, widthFactor, slashAngle, fontName, isBold, isItalic, hasUnderline, hasStrikethrough, hasOverline);
		}
	}

	//替换引用
	for (auto refChar : refChars)
	{
		if (*refChar != nullptr)
		{
			auto it = origin_replaced_map.find(*refChar);
			if (origin_replaced_map.end() != it)
			{
				*refChar = it->second;
			}
		}
	}
}

void MTextEditWidget::getSelectedParas(std::vector<DmMTextParagraph*>& paras)
{
	//获得选择的文字
	DmChar* startChar = nullptr;
	DmChar* endChar = nullptr;
	m_pMText->getStartEndCharByPos(startChar, endChar, m_selectBeginPreChar, m_selectBeginPostChar, m_selectEndPreChar, m_selectEndPostChar);
	if (startChar == nullptr || endChar == nullptr)	//啥也没选中
	{
		return;
	}

	DmMTextParagraph* startPara = static_cast<DmMTextParagraph*>(startChar->getParent()->getParent());
	DmMTextParagraph* endPara = static_cast<DmMTextParagraph*>(endChar->getParent()->getParent());
	if (startPara == endPara)
	{
		paras.emplace_back(startPara);
	}
	else
	{
		DmMTextParagraph* para = startPara;
		while (para != endPara && para != nullptr)
		{
			paras.emplace_back(para);
			para = m_pMText->getPostParagraph(para);
		}
		if (para != nullptr)
		{
			paras.emplace_back(para);
		}
	}
}

void MTextEditWidget::locateChar(bool needUndo /*= false*/)
{
    if (needUndo) {
        DmChar* tempPreChar = nullptr, *tempPostChar = nullptr;
        int tempIndex;

        DmVector cursorCenter = getCursorCenter();
        m_pMText->locateChar(cursorCenter, true, tempPreChar, tempPostChar);
        m_pMText->locateCharIndex(tempPreChar, tempPostChar, tempIndex);

        auto precharChangeCmd = new EntityChangeCmd<MTextEditWidget,DmChar*,&MTextEditWidget::m_cursorPreChar>(this, tempPreChar, m_cursorPreChar);
        m_cmdManager.addAndExecuteCmd(precharChangeCmd);
        auto postcharChangeCmd = new EntityChangeCmd<MTextEditWidget,DmChar*,&MTextEditWidget::m_cursorPostChar>(this, tempPostChar, m_cursorPostChar);
        m_cmdManager.addAndExecuteCmd(postcharChangeCmd);
//        auto cursorIndexChangeCmd = new EntityChangeCmd<MTextEditWidget,int,&MTextEditWidget::m_cursorIdx>(this, tempIndex, m_cursorIdx);
//        m_cmdManager.addAndExecuteCmd(cursorIndexChangeCmd);
    }
    else{
        DmVector cursorCenter = getCursorCenter();
        m_pMText->locateChar(cursorCenter, true, m_cursorPreChar, m_cursorPostChar);
//        m_pMText->locateCharIndex(m_cursorPreChar, m_cursorPostChar, m_cursorIdx);
    }
}

DmVector MTextEditWidget::getCursorCenter() const
{
	double height = m_pContext->getCurCharHeight();
	return DmVector(m_currentCursorPos.x, m_currentCursorPos.y + height / 2.0);
}

double MTextEditWidget::toGuiX(double x) const
{
	return (x - m_leftTop.x) * m_factor.x;
}

double MTextEditWidget::toGuiY(double y) const
{
	return (m_leftTop.y - y) * m_factor.y;
}

double MTextEditWidget::toGraphX(double x) const
{
	return x / m_factor.x + m_leftTop.x;
}

double MTextEditWidget::toGraphY(double y) const
{
	return m_leftTop.y - y / m_factor.y ;
}

void MTextEditWidget::setCursorByPosition(Position pos)
{
	if (pos == Position::Bottom)
	{
		setCursor(Qt::SizeVerCursor);
	}
	else if (pos == Position::Right)
	{
		setCursor(Qt::SizeHorCursor);
	}
	else if (pos == Position::ButtomRight)
	{
		setCursor(Qt::SizeFDiagCursor);
	}
	else
	{
		if (isMatching())
		{
			setCursor(*m_matchCurxor);
		}
		else
		{
			setCursor(Qt::IBeamCursor);
		}
	}
}

void MTextEditWidget::resizing(double x, double y)
{
	QRect oldGeometry = geometry();
	if (m_lastPos == Position::Bottom)
	{
		if (y > 0)
		{
			setGeometry(oldGeometry.x(), oldGeometry.y(), oldGeometry.width(), y);
			updateCorners();
		}
	}
	else if (m_lastPos == Position::Right)
	{
		if (x > 0)
		{
			setGeometry(oldGeometry.x(), oldGeometry.y(), x, oldGeometry.height());
			updateCorners();
		}
	}
	else if (m_lastPos == Position::ButtomRight)
	{
		if (y > 0 && x > 0)
		{
			setGeometry(oldGeometry.x(), oldGeometry.y(), x, y);
			updateCorners();
		}
	}
}

void MTextEditWidget::selecting(double x, double y)
{
	//定位并绘制选择遮罩
	double graphX = toGraphX((double)x);
	double graphY = toGraphY((double)y);
	DmVector pos(graphX, graphY);
	DmChar* preChar = nullptr;
	DmChar* postChar = nullptr;
	m_pMText->locateChar(pos, true, preChar, postChar, true);
	m_selectEndPreChar = preChar;
	m_selectEndPostChar = postChar;

	drawSelectCoverImmediately();
	if (!hasSelectedChar())
	{
		return;
	}

	//获得选择的文字
	DmChar* startChar = nullptr;
	DmChar* endChar = nullptr;
	m_pMText->getStartEndCharByPos(startChar, endChar, m_selectBeginPreChar, m_selectBeginPostChar, m_selectEndPreChar, m_selectEndPostChar);
	if (startChar == nullptr || endChar == nullptr)	//啥也没选中
	{
		return;
	}

	//更新选择的第一个字符的格式到选项
	updateCharFormatToContext(startChar);
	// 更新数据到选项界面
	m_pContext->emitDmToOption();
}

void MTextEditWidget::drawEditCursorImmediately(bool draw)
{
	if (draw)
	{
		m_timer->stop();
		if (m_pCursor)
		{
			//调用2次，第一次删除原来光标，第二次绘制新光标
			slotTimerTrigger();
			slotTimerTrigger();
		}
		else
		{
			//绘制光标
			slotTimerTrigger();
		}
		//removeSelectCover();
		m_timer->start();
	}
	else
	{
		m_timer->stop();
		if (m_pCursor)
		{
			//删除原来的光标
			slotTimerTrigger();
		}
	}
}

void MTextEditWidget::drawSelectCoverImmediately()
{
    if (m_pSelectCover)	//每次要移除原来的遮罩
    {
        container->removeEntity(m_pSelectCover);
        delete m_pSelectCover;
        m_pSelectCover = nullptr;
    }
    m_pSelectCover = m_pMText->getSelectedCover(m_selectBeginPreChar, m_selectBeginPostChar, m_selectEndPreChar, m_selectEndPostChar);
    if (nullptr == m_pSelectCover)
    {
        return;
    }

    m_pSelectCover->setParent(container);
    container->addEntity(m_pSelectCover);
    DmPen containerPen;
    containerPen.setColor(DmColor(121, 144, 169, 100));
    m_pSelectCover->setPen(containerPen);
    //DmPen solidPen;
    //DmColor solidColor;
    //solidColor.setFlag(DM::FlagByBlock);
    //solidPen.setColor(solidColor);
    removeEditCursor(true);

	specifyModified();
	update();
}

void MTextEditWidget::drawCursorOrCoverBySelectStateImmediately()
{
	if (hasSelectedChar())
	{
		//更新选择高亮
		drawSelectCoverImmediately();
	}
	else
	{
		setEditCursorPos(false);
	}
}

void MTextEditWidget::removeEditCursor(bool notShowAnymore)
{
	if (notShowAnymore)
	{
		m_timer->stop();
	}
	//删除原来的光标
	if (m_pCursor)
	{
		container->removeEntity(m_pCursor);
        delete m_pCursor;
		m_pCursor = nullptr;
	}
}

void MTextEditWidget::resumeEditCursor()
{
	m_timer->start();
}

void MTextEditWidget::removeSelectCover()
{
	if (m_pSelectCover)
	{
		container->removeEntity(m_pSelectCover);
        delete m_pSelectCover;
		m_pSelectCover = nullptr;

		//重置选择的实体
		m_selectBeginPreChar = nullptr;
		m_selectBeginPostChar = nullptr;
		m_selectEndPreChar = nullptr;
		m_selectEndPostChar = nullptr;
	}
}

void MTextEditWidget::deleteSelected()
{
	if (!hasSelectedChar())
	{
		return;
	}

    //给位置排序（因为有可能是从后往前选）
    const DmChar* preCharBeginNew = nullptr;
    const DmChar* postCharBeginNew = nullptr;
    const DmChar* preCharEndNew = nullptr;
    const DmChar* postCharEndNew = nullptr;
    m_pMText->getOrderedPos(m_selectBeginPreChar, m_selectBeginPostChar, m_selectEndPreChar, m_selectEndPostChar, preCharBeginNew, postCharBeginNew, preCharEndNew, postCharEndNew);
    int charCount = m_pMText->getSelectCharCount(preCharBeginNew, postCharBeginNew, preCharEndNew, postCharEndNew);
    int startIdx;
    m_pMText->locateCharIndex(preCharBeginNew, postCharBeginNew, startIdx);

//    MTextEdit_SetSelectBeginEndToNull_Cmd* setSelectBeginEndToNullCmd= new MTextEdit_SetSelectBeginEndToNull_Cmd(m_pMText, this);
//    m_cmdManager.addAndExecuteCmd(setSelectBeginEndToNullCmd);
    setSelectBeginEndToNull(true);

    MTextEdit_RemoveParasAndCharsCmd* cmd = new MTextEdit_RemoveParasAndCharsCmd(m_pMText, startIdx, charCount);
    m_cmdManager.addAndExecuteCmd(cmd);

    auto tempPreChar = const_cast<DmChar*>(preCharBeginNew);
    auto tempPostChar = const_cast<DmChar*>(postCharEndNew);
    int tempIndex;
    m_pMText->locateCharIndex(m_cursorPreChar, m_cursorPostChar, tempIndex);
    auto precharChangeCmd = new EntityChangeCmd<MTextEditWidget,DmChar*,&MTextEditWidget::m_cursorPreChar>(this, tempPreChar, m_cursorPreChar);
    m_cmdManager.addAndExecuteCmd(precharChangeCmd);
    auto postcharChangeCmd = new EntityChangeCmd<MTextEditWidget,DmChar*,&MTextEditWidget::m_cursorPostChar>(this, tempPostChar, m_cursorPostChar);
    m_cmdManager.addAndExecuteCmd(postcharChangeCmd);
//    auto cursorIndexChangeCmd = new EntityChangeCmd<MTextEditWidget,int,&MTextEditWidget::m_cursorIdx>(this, tempIndex, m_cursorIdx);
//    m_cmdManager.addAndExecuteCmd(cursorIndexChangeCmd);

	//删除选择遮罩
	removeSelectCover();
}

bool MTextEditWidget::hasSelectedChar() const
{
	return nullptr != m_pSelectCover;
}

void MTextEditWidget::saveSelectedCharsToClipboard()
{
	// 获得选中实体，生成内容
	DmChar* startChar = nullptr;
	DmChar* endChar = nullptr;
	m_pMText->getStartEndCharByPos(startChar, endChar, m_selectBeginPreChar, m_selectBeginPostChar, m_selectEndPreChar, m_selectEndPostChar);
	if (startChar == nullptr || endChar == nullptr)	//啥也没选中
	{
		return;
	}
	QString content = m_pMText->getContentByRange(startChar, endChar);

	//将内容放入剪切板
	QClipboard* clip = QApplication::clipboard();
	QMimeData* mimeData = new QMimeData();
	mimeData->setData(CLIPBOARD_MTEXT_KEY, content.toUtf8());
	clip->setMimeData(mimeData);
	//TODO : 纯文本也保存一下
}

void MTextEditWidget::wheelEvent(QWheelEvent* e)
{
	// 只有鼠标在本窗口，才会激活该方法
	// 显式调用e->ignore();可以让父窗体GuiDocumentView调用wheelEvent
	// 默认也会调用父窗体的wheelEvent，除非调用e->accept();
	e->ignore();
}

//void MTextEditWidget::enterEvent(QEvent* e)
//{
//	setCursor(Qt::IBeamCursor);
//}
//
//void MTextEditWidget::leaveEvent(QEvent* e)
//{
//	//setCursor(Qt::ArrowCursor);
//}
//
//void MTextEditWidget::focusOutEvent(QFocusEvent* event)
//{
//	//切换任务栏也会激活此函数
//}

void MTextEditWidget::mouseReleaseEvent(QMouseEvent* e)
{
	if (getStatus() == Status::Resizing)
	{
		setStatus(Status::Normal);
	}
	else if (getStatus() == Status::Selecting)
	{
		//如果是在格式刷状态，更改文字格式并刷新对文字的引用
		if (isMatching() && hasSelectedChar())
		{
            m_cmdManager.start("Match text format");
			matchSelectedChars(m_matchContext.get());
			m_pMText->updateTextPosition();
            m_cmdManager.done();
		}

		drawCursorOrCoverBySelectStateImmediately();
		setStatus(Status::Normal);
	}
}

void MTextEditWidget::mousePressEvent(QMouseEvent* e)
{
	//当鼠标在本窗体外点击时，不响应（非模态）
	Position pos = getPosition(e->x(), e->y());
	if (pos == Position::Other)
	{
		//设置光标位置
		setEditCursorPos(e->x(), e->y());
        removeSelectCover();
		setSelectBegin();
		setStatus(Status::Selecting);
	}
	else
	{
		//在边框处按下，准备resize
		setStatus(Status::Resizing);
		m_lastPos = pos;
	}
}

void MTextEditWidget::mouseMoveEvent(QMouseEvent* e)
{
	Position pos = getPosition(e->x(), e->y());
	QRect oldGeometry = geometry();
	if (getStatus() == Status::Normal)
	{
		//设置鼠标光标
		setCursorByPosition(pos);
	}
	else if (getStatus() == Status::Resizing)
	{
		//resizing
		resizing(e->x(), e->y());
	}
	else if (getStatus() == Status::Selecting)
	{
		//选择
		selecting(e->x(), e->y());
	}
}

void MTextEditWidget::keyPressEvent(QKeyEvent* e)
{
	QString key = e->text();
	if (e->matches(QKeySequence::Cut))
	{
		keyPressEvent_Cut();
	}
	else if (e->matches(QKeySequence::Paste))
	{
		keyPressEvent_Paste();
	}
	else if (e->matches(QKeySequence::Copy))
	{
		keyPressEvent_Copy();
	}
	else if (e->matches(QKeySequence::Redo))
	{
		//重做
        keyPressEvent_Redo();
	}
	else if (e->matches(QKeySequence::Undo))
	{
		//撤销
        keyPressEvent_Undo();
	}
	else if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter)	//中间的Enter键是Key_Return，键盘右边数字键的Enter是Key_Enter
	{
		//回车
		keyPressEvent_Enter();
	}
	else if (e->key() == Qt::Key_Left || e->key() == Qt::Key_Right || e->key() == Qt::Key_Down || e->key() == Qt::Key_Up)
	{
		//方向键，移动光标
		switch (e->key())
		{
		default:
		case Qt::Key_Left:
			keyPressEvent_Left();
			break;
		case Qt::Key_Right:
			keyPressEvent_Right();
			break;
		case Qt::Key_Down:
			keyPressEvent_Down();
			break;
		case Qt::Key_Up:
			keyPressEvent_Up();
			break;
		}
	}
	else if (e->key() == Qt::Key_Delete)
	{
		//Delete
		keyPressEvent_Delete();
	}
	else if (e->key() == Qt::Key_Backspace)
	{
		//退格
		keyPressEvent_Backspace();
	}
	else if (e->key() == Qt::Key_Escape)
	{
		//ESC
		keyPressEvent_Escape();
	}
	else
	{
		if (!e->text().isEmpty())
		{
			//其他可显示文字
			insertTextAtCursor(e->text());
		}
	}
}

void MTextEditWidget::inputMethodEvent(QInputMethodEvent* e)
{
	QString commitStr = e->commitString();
	//QString preStr = e->preeditString();
	//auto attrs = e->attributes();
	if (!commitStr.isEmpty())
	{
		insertTextAtCursor(commitStr);
	}
}

void MTextEditWidget::closeEvent(QCloseEvent* event)
{
	//外部调用close()时， this不会立马释放，要停止计时器响应
	if (m_timer.get() != nullptr)
	{
		m_timer->stop();	
	}
	QWidget::closeEvent(event);
}

void MTextEditWidget::keyPressEvent_Undo()
{
    bool canUndo = false;
    bool canRedo = false;
    m_cmdManager.getCmdData(canUndo, canRedo);
    if (!canUndo)
    {
        return;
    }
    m_cmdManager.undo();
    m_pMText->updateTextPosition();

    //undo有时候有resize操作，导致重绘，这与后面的drawEditCursorImmediately绘制操作可能会合并，导致后面的显示无效，
    // 所以调用QCoreApplication::processEvents()。
    // 在deepseek中问“有时候设置窗体大小后，会自动重绘，但是如果紧接着改变绘制实体，再update()，重绘没有效果，这是什么原因”
    QCoreApplication::processEvents();
    setEditCursorPos(true);
    drawEditCursorImmediately(true);
    drawSelectCoverImmediately();
}

void MTextEditWidget::keyPressEvent_Redo()
{
    bool canUndo = false;
    bool canRedo = false;
    m_cmdManager.getCmdData(canUndo, canRedo);
    if (!canRedo)
    {
        return;
    }
    m_cmdManager.redo();
    m_pMText->updateTextPosition();

    QCoreApplication::processEvents();
    setEditCursorPos(true);
    drawEditCursorImmediately(true);
    drawSelectCoverImmediately();
}

void MTextEditWidget::keyPressEvent_Copy()
{
	if (!hasSelectedChar())
	{
		return;
	}
	saveSelectedCharsToClipboard();
}

void MTextEditWidget::keyPressEvent_Cut()
{
	if (!hasSelectedChar())
	{
		return;
	}
	saveSelectedCharsToClipboard();
    m_cmdManager.start("cut");
	deleteSelected();
    m_cmdManager.done();

    m_pMText->updateTextPosition();
    setEditCursorPos();
}

void MTextEditWidget::keyPressEvent_Paste()
{
    //从剪切板读取信息
    QClipboard* clip = QApplication::clipboard();
    const QMimeData* data = clip->mimeData();
    QString content;
    if (data->hasFormat(CLIPBOARD_MTEXT_KEY))
    {
        QByteArray byteArray = data->data(CLIPBOARD_MTEXT_KEY);
        content = QString::fromUtf8(byteArray);
    }
        //如果没有读到自定义信息，尝试读取纯文本
    else
    {
        content = clip->text();
    }
    if (content.isEmpty())
    {
        return;
    }

    //粘贴
    m_cmdManager.start("paste");
	deleteSelected();
	setEditCursorPos();

	//在指定位置插入内容
    MTextEdit_ContentPasteCmd* cmd = new MTextEdit_ContentPasteCmd(m_pMText, m_cursorPreChar, m_cursorPostChar, content, this);
    m_cmdManager.addAndExecuteCmd(cmd);
    auto newPreChar = cmd->getNewPreChar();
    auto newPostChar = cmd->getNewPostChar();
    if (newPreChar != m_cursorPreChar) {
        auto precharChangeCmd = new EntityChangeCmd<MTextEditWidget,DmChar*,&MTextEditWidget::m_cursorPreChar>(this, newPreChar, m_cursorPreChar);
        m_cmdManager.addAndExecuteCmd(precharChangeCmd);
    }
    if (newPostChar != m_cursorPostChar) {
        auto postcharChangeCmd = new EntityChangeCmd<MTextEditWidget,DmChar*,&MTextEditWidget::m_cursorPostChar>(this, newPostChar, m_cursorPostChar);
        m_cmdManager.addAndExecuteCmd(postcharChangeCmd);
    }

    m_pMText->updateTextPosition();
	setEditCursorPos();
	increaseHeightIfNotEnough();
    m_cmdManager.done();
}

//QVariant MTextEditWidget::inputMethodQuery(Qt::InputMethodQuery query) const
//{
//	return QVariant();
//}

void MTextEditWidget::keyPressEvent_Enter()
{
    m_cmdManager.start("enter");
    deleteSelected();

    MTextEdit_InsertLineFeedCmd* cmd = new MTextEdit_InsertLineFeedCmd(m_pMText, m_cursorPreChar, m_cursorPostChar, this);
    m_cmdManager.addAndExecuteCmd(cmd);
    DmMTextParagraph* afterPara = cmd->getAfterPara();

    auto firstLine = afterPara->lineAt(0);
    auto precharChangeCmd = new EntityChangeCmd<MTextEditWidget,DmChar*,&MTextEditWidget::m_cursorPreChar>(this, nullptr, m_cursorPreChar);
    m_cmdManager.addAndExecuteCmd(precharChangeCmd);
    auto postcharChangeCmd = new EntityChangeCmd<MTextEditWidget,DmChar*,&MTextEditWidget::m_cursorPostChar>(this, firstLine->charAt(0), m_cursorPostChar);
    m_cmdManager.addAndExecuteCmd(postcharChangeCmd);

    m_pMText->updateTextPosition();

    setSelectBeginEndToNull(false);
    setSelectBeginEndToNull(true);
    setEditCursorPos();
	increaseHeightIfNotEnough();
    m_cmdManager.done();
}

void MTextEditWidget::keyPressEvent_Left()
{
	removeSelectCover();
	DmChar* newPreChar = nullptr;
	DmChar* newPostChar = nullptr;
	m_pMText->getPreviousChar(m_cursorPreChar, m_cursorPostChar, newPreChar, newPostChar);
	m_cursorPreChar = newPreChar;
	m_cursorPostChar = newPostChar;
//    m_pMText->locateCharIndex(m_cursorPreChar, m_cursorPostChar, m_cursorIdx);
	setEditCursorPos();
}

void MTextEditWidget::keyPressEvent_Right()
{
	removeSelectCover();
	DmChar* newPreChar = nullptr;
	DmChar* newPostChar = nullptr;
	m_pMText->getPostChar(m_cursorPreChar, m_cursorPostChar, newPreChar, newPostChar);
	m_cursorPreChar = newPreChar;
	m_cursorPostChar = newPostChar;
//    m_pMText->locateCharIndex(m_cursorPreChar, m_cursorPostChar, m_cursorIdx);
	setEditCursorPos();
}

void MTextEditWidget::keyPressEvent_Down()
{
	removeSelectCover();
	DmChar* newPreChar = nullptr;
	DmChar* newPostChar = nullptr;
	m_pMText->getDownChar(m_cursorPreChar, m_cursorPostChar, newPreChar, newPostChar);
	m_cursorPreChar = newPreChar;
	m_cursorPostChar = newPostChar;
//    m_pMText->locateCharIndex(m_cursorPreChar, m_cursorPostChar, m_cursorIdx);
	setEditCursorPos();
}

void MTextEditWidget::keyPressEvent_Up()
{
	removeSelectCover();
	DmChar* newPreChar = nullptr;
	DmChar* newPostChar = nullptr;
	m_pMText->getUpChar(m_cursorPreChar, m_cursorPostChar, newPreChar, newPostChar);
	m_cursorPreChar = newPreChar;
	m_cursorPostChar = newPostChar;
//    m_pMText->locateCharIndex(m_cursorPreChar, m_cursorPostChar, m_cursorIdx);
	setEditCursorPos();
}

void MTextEditWidget::keyPressEvent_Delete()
{
	if (hasSelectedChar())
	{
        m_cmdManager.start("\"delete\" delete selected");
        deleteSelected();
        m_cmdManager.done();
	}
	else
	{
		DmChar* newPreChar = nullptr;
		DmChar* newPostChar = nullptr;
		m_pMText->del(m_cursorPreChar, m_cursorPostChar, newPreChar, newPostChar);
		m_cursorPreChar = newPreChar;
		m_cursorPostChar = newPostChar;
//        m_pMText->locateCharIndex(m_cursorPreChar, m_cursorPostChar, m_cursorIdx);
	}
	m_pMText->updateTextPosition();
	setEditCursorPos();
}

void MTextEditWidget::keyPressEvent_Backspace()
{
	if (hasSelectedChar())
	{
        m_cmdManager.start("\"backspace\" delete selected");
		deleteSelected();
        m_cmdManager.done();
	}
	else
	{
		DmChar* newPreChar = nullptr;
		DmChar* newPostChar = nullptr;
        bool postPreCharChanged = false;
        DmChar* preChar = m_cursorPreChar;
        DmChar* postChar = m_cursorPostChar;
		//m_pMText->backspace(m_cursorPreChar, m_cursorPostChar, newPreChar, newPostChar);

        // 最后一个空行
        if (nullptr == preChar && nullptr == postChar)
        {
            if (m_pMText->size() == 1)	//整个文字就只有空行
            {
                return;
            }
            m_cmdManager.start("backspace");
            DmMTextParagraph* lastPara = static_cast<DmMTextParagraph*>(m_pMText->last());
            DmMTextParagraph* preLastPara = m_pMText->getPreParagraph(lastPara);
            DmMTextLine* preLastLine = static_cast<DmMTextLine*>(preLastPara->last());
            newPostChar = nullptr;
            if (preLastPara->getCharsCount(true) == 1)
            {
                //前一个段落只有换行，删除该段落
                newPostChar = nullptr;
                newPreChar = nullptr;
                MTextEdit_RemoveParaCmd* removeParaCmd = new MTextEdit_RemoveParaCmd(m_pMText, m_pMText->size() - 2);
                m_cmdManager.addAndExecuteCmd(removeParaCmd);
            }
            else{
                // 前一个段落有换行及其他文字
                newPreChar = preLastLine->charAt(preLastLine->size() - 2);//（换行符不会独立成一行）
                MTextEdit_RemoveParaCmd* removeParaCmd = new MTextEdit_RemoveParaCmd(m_pMText, m_pMText->size() - 1);
                m_cmdManager.addAndExecuteCmd(removeParaCmd);
                MTextEdit_RemoveNewLineCharCmd* removeNewLineCharCmd = new MTextEdit_RemoveNewLineCharCmd(m_pMText, m_pMText->last());
                m_cmdManager.addAndExecuteCmd(removeNewLineCharCmd);
                postPreCharChanged = true;
            }
            if (postPreCharChanged) {
                // 更新m_cursorPreChar，m_cursorPostChar
                auto precharChangeCmd = new EntityChangeCmd<MTextEditWidget,DmChar*,&MTextEditWidget::m_cursorPreChar>(this, newPreChar, m_cursorPreChar);
                m_cmdManager.addAndExecuteCmd(precharChangeCmd);
                auto postcharChangeCmd = new EntityChangeCmd<MTextEditWidget,DmChar*,&MTextEditWidget::m_cursorPostChar>(this, newPostChar, m_cursorPostChar);
                m_cmdManager.addAndExecuteCmd(postcharChangeCmd);
            }
            m_cmdManager.done();
        }
        // 一行行首
        else if (nullptr == preChar)
        {
            DmMTextParagraph* curPara = static_cast<DmMTextParagraph*>(postChar->getParent()->getParent());
            DmMTextLine* curLine = static_cast<DmMTextLine*>(postChar->getParent());
            bool isFirstLine = curPara->findLine(curLine) == 0;
            bool isFirstPara = m_pMText->findPara(curPara) == 0;
            if (isFirstLine && isFirstPara) //第一个段落的首行
                return;
            m_cmdManager.start("backspace");
            //段落的首行
            if (isFirstLine)
            {
                //不会是第一个段落（否则前面已返回）
                DmMTextParagraph* prePara = m_pMText->getPreParagraph(curPara);
                //前一个段落只有换行，删除该段落
                if (prePara->getCharsCount(true) == 1)
                {
                    newPostChar = postChar;
                    newPreChar = preChar;
                    MTextEdit_RemoveParaCmd* removeParaCmd = new MTextEdit_RemoveParaCmd(m_pMText, m_pMText->size() - 2);
                    m_cmdManager.addAndExecuteCmd(removeParaCmd);
                }
                //前一个段落不只有换行，删除本段落，内容合并到前一段落
                else
                {
                    DmMTextLine* preLastLine = prePara->last();
                    MTextEdit_RemoveNewLineCharCmd* removeNewLineCharCmd = new MTextEdit_RemoveNewLineCharCmd(m_pMText, prePara);
                    m_cmdManager.addAndExecuteCmd(removeNewLineCharCmd);
                    if (preLastLine->size() == 0)//前一个段落只有换行
                    {
                        newPreChar = nullptr;
                    }
                    else
                    {
                        newPreChar = preLastLine->last();
                    }
                    std::vector<DmChar*> temp;
                    curPara->getCharsByRange(0, curPara->getCharsCount(true), temp);
                    MTextEdit_AddCharsCmd* addCharsCmd = new MTextEdit_AddCharsCmd(m_pMText, prePara, temp, prePara->getCharsCount(true));
                    m_cmdManager.addAndExecuteCmd(addCharsCmd);
                    MTextEdit_RemoveParaCmd* removeParaCmd = new MTextEdit_RemoveParaCmd(m_pMText, m_pMText->findPara(curPara));
                    m_cmdManager.addAndExecuteCmd(removeParaCmd);
                    newPostChar = postChar;
                    postPreCharChanged = true;
                }
            }
            //非段落的首行
            else
            {
                DmMTextLine* preLine = curPara->getPreLine(curLine);
                int removeIdx = curPara->indexOf(preLine->last());
                MTextEdit_RemoveCharsCmd* removeCharsCmd = new MTextEdit_RemoveCharsCmd(m_pMText, curPara, removeIdx, 1);
                m_cmdManager.addAndExecuteCmd(removeCharsCmd);

                int newPreCharIdx = removeIdx - 1;
                if (newPreCharIdx >= 0) {
                    newPreChar = curPara->getChar(newPreCharIdx);
                }
                else{
                    newPreChar = nullptr;
                }
                newPostChar = curPara->getChar(removeIdx);
                postPreCharChanged = true;
            }

            if (postPreCharChanged) {
                auto precharChangeCmd = new EntityChangeCmd<MTextEditWidget,DmChar*,&MTextEditWidget::m_cursorPreChar>(this, newPreChar, m_cursorPreChar);
                m_cmdManager.addAndExecuteCmd(precharChangeCmd);
                auto postcharChangeCmd = new EntityChangeCmd<MTextEditWidget,DmChar*,&MTextEditWidget::m_cursorPostChar>(this, newPostChar, m_cursorPostChar);
                m_cmdManager.addAndExecuteCmd(postcharChangeCmd);
            }
            m_cmdManager.done();
        }
        // 非行首
        else{
            m_cmdManager.start("backspace");
            // 一般位置（preChar不为空（也不可能是换行））
            DmMTextParagraph* curPara = static_cast<DmMTextParagraph*>(preChar->getParent()->getParent());
            DmMTextLine* curLine = static_cast<DmMTextLine*>(preChar->getParent());
            int curLineIdx = curPara->findLine(curLine);
            int preCharIdx = curLine->findChar(preChar);
            if (preCharIdx == 0)	//当前行的第一个字符
            {
                if (curLineIdx == 0)	//当前是第一个行
                {
                    newPreChar = nullptr;
                }
                else
                {
                    DmMTextLine* lastLine = curPara->lineAt(curLineIdx - 1);
                    newPreChar = static_cast<DmChar*>(lastLine->last());
                }
            }
            else
            {
                newPreChar = curLine->charAt(preCharIdx - 1);
            }
            int cIndex = curPara->indexOf(preChar);
            MTextEdit_RemoveCharsCmd* removeCharsCmd = new MTextEdit_RemoveCharsCmd(m_pMText, curPara, cIndex, 1);
            m_cmdManager.addAndExecuteCmd(removeCharsCmd);
            newPostChar = postChar;

            auto precharChangeCmd = new EntityChangeCmd<MTextEditWidget,DmChar*,&MTextEditWidget::m_cursorPreChar>(this, newPreChar, m_cursorPreChar);
            m_cmdManager.addAndExecuteCmd(precharChangeCmd);
            auto postcharChangeCmd = new EntityChangeCmd<MTextEditWidget,DmChar*,&MTextEditWidget::m_cursorPostChar>(this, newPostChar, m_cursorPostChar);
            m_cmdManager.addAndExecuteCmd(postcharChangeCmd);
            m_cmdManager.done();
        }
	}
	m_pMText->updateTextPosition();
	setEditCursorPos();
}

void MTextEditWidget::keyPressEvent_Escape()
{
	// 如果在格式刷状态，按ESC退出
	if (isMatching())
	{
		m_matchContext.reset(nullptr);
		m_pContext->setIsMatching(false);
		m_pContext->emitDmToOption();
		setEditCursorPos();
		setStatus(Status::Normal);
		//让光标恢复正常状态
		Position pos = getPosition(0, 0);
		setCursorByPosition(pos);
		return;
	}
	
	//提示是否保存
	auto btn = QMessageBox::critical(this, tr("Tips"), tr("Save the changes?"),
		QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No | QMessageBox::StandardButton::Cancel, QMessageBox::StandardButton::Yes);
	if (btn == QMessageBox::StandardButton::Cancel)
	{
		return;
	}
	bool save = btn == QMessageBox::StandardButton::Yes;
	m_pContext->emitEscPressed(save);
}

void MTextEditWidget::resizeGL(int w, int h)
{
	if (m_pPainter)
	{
		double centerx = m_leftTop.x + ((double)w) * 0.5 / m_factor.x;
		double centery = m_leftTop.y - ((double)h) * 0.5 / m_factor.y;
		m_pPainter->new_device_size(w, h);
		m_pPainter->setScale(1.0 / m_factor.x);
		m_pPainter->setViewPosition(centerx, centery);
	}
}

bool MTextEditWidget::focusNextPrevChild(bool next)
{
	return false;
}

MTextEditWidget::Position MTextEditWidget::getPosition(int x, int y)
{
	int border = 10;
	QRect rect = geometry();
	if (abs(x - rect.width()) < border && abs(y - rect.height()) < border)
	{
		return Position::ButtomRight;
	}
	else if (abs(x - rect.width()) < border)
	{
		return Position::Right;
	}
	else if (abs(y - rect.height()) < border)
	{
		return Position::Bottom;
	}
	else
	{
		return Position::Other;
	}
}

void MTextEditWidget::setStatus(Status s)
{
	m_curStatus = s;
}

MTextEditWidget::Status MTextEditWidget::getStatus() const
{
	return m_curStatus;
}

void MTextEditWidget::setEditCursorPos(const int x, const int y, const bool init /*= false*/)
{
	double graphX = toGraphX((double)x);
	double graphY = toGraphY((double)y);
	DmVector pos(graphX, graphY);
	DmChar* tempPreChar = nullptr;
	DmChar* tempPostChar = nullptr;
	m_pMText->locateChar(pos, true, tempPreChar, tempPostChar);
	
	//初始化时,更新选项
	if (init)
	{
		m_cursorPreChar = tempPreChar;
		m_cursorPostChar = tempPostChar;
//        m_pMText->locateCharIndex(m_cursorPreChar, m_cursorPostChar, m_cursorIdx);
		setEditCursorPos(true);
	}
	// 不是初始化时, 如果光标点位的字符不变，不更新选项
	else
	{
		if (tempPreChar == m_cursorPreChar && tempPostChar == m_cursorPostChar)
		{
			setEditCursorPos(false);
		}
		else
		{
			setEditCursorPos(true);
		}
		m_cursorPreChar = tempPreChar;
		m_cursorPostChar = tempPostChar;
//        m_pMText->locateCharIndex(m_cursorPreChar, m_cursorPostChar, m_cursorIdx);
	}
	
}

void MTextEditWidget::setEditCursorPos(const bool& updateToOption /*= true*/)
{
	// 新启段落没有文字，定位失败，该段落起始位置
	if (m_cursorPreChar == nullptr && m_cursorPostChar == nullptr)
	{
		DmMTextParagraph* lastPara = m_pMText->paraAt(m_pMText->size() - 1);
		DmVector startPos = lastPara->getStartPosition();
		double height = m_pContext->getCurCharHeight();
		m_currentCursorPos = startPos + DmVector(0.0, -1.0) * height /*+ DmVector(1.0, 0.0) * 0.1 * height*/;
	}
	else
	{
		//更新光标处文字信息到选项
		DmChar* theChar = nullptr;
		double nominalHeight = 0.0;
		if (m_cursorPreChar != nullptr)
		{
			theChar = m_cursorPreChar;
			nominalHeight = theChar->getNominalHeight();
			m_currentCursorPos = theChar->getPosition() +
				DmVector(theChar->getWidth() + nominalHeight * CHARGAPFACTOR, 0.0);
		}
		else if (m_cursorPostChar != nullptr)
		{
			theChar = m_cursorPostChar;
			nominalHeight = theChar->getNominalHeight();
			m_currentCursorPos = theChar->getPosition() - DmVector(nominalHeight * CHARGAPFACTOR, 0.0);
		}
		if (updateToOption)
		{
			updateCharFormatToContext(theChar);
		}
	}

	if (updateToOption)
	{
		// 更新数据到选项界面
		m_pContext->emitDmToOption();
	}

	//立即刷新光标
	drawEditCursorImmediately(true);
}

void MTextEditWidget::updateCharFormatToContext(const DmChar* theChar)
{
	if (nullptr == theChar)
	{
		return;
	}
	double nominalHeight = theChar->getNominalHeight();
	DmMTextParagraph* para = static_cast<DmMTextParagraph*>(theChar->getParent()->getParent());
	DmCharTemplate* charTempl = theChar->getCharTemplate();
	bool isBold = false;
	bool isItalic = false;
	DmCharTemplateList* templList = charTempl->getOwner();
	DmFont* f = templList->getFont();
	QString fontFamilyName = DMFONTLIST->getFontFamilyName(f, isBold, isItalic);
	m_pContext->setCurFontFamilyName(fontFamilyName);
	m_pContext->setIsBold(theChar->isBold());
	m_pContext->setIsItalic(theChar->isItalic());
	m_pContext->setHasOverline(theChar->getOverline() != nullptr);
	m_pContext->setHasUnderline(theChar->getUnderline() != nullptr);
	m_pContext->setHasStrikethrough(theChar->getStrikethrough() != nullptr);
	m_pContext->setCurCharHeight(nominalHeight);
	m_pContext->setCurColor(theChar->getPen(false).getColor());
	m_pContext->setOblique(theChar->getSlashAngle());
	m_pContext->setWidthFactor(theChar->getWidthFactor());
	m_pContext->setParaAlignment(para->getAlignment());
    m_pContext->setJustification(m_pMText->getJustification());
}

void MTextEditWidget::setSelectBegin()
{
	m_selectBeginPreChar = m_cursorPreChar;
	m_selectBeginPostChar = m_cursorPostChar;
}

void MTextEditWidget::setSelectEnd()
{
	m_selectEndPreChar = m_cursorPreChar;
	m_selectEndPostChar = m_cursorPostChar;
}

void MTextEditWidget::slotTimerTrigger()
{
	//删除原来的光标
	if (m_pCursor)
	{
		removeEditCursor(false);
	}
	else
	{
		//if (getStatus() == Status::Selecting)
		//{
		//	//选择状态不绘制光标
		//	return;
		//}
		//添加光标
		DmPen containerPen;
		containerPen.setColor(DmColor(255, 255, 255, 255));
		DmPen linePen;
		DmColor lineColor;
		lineColor.setFlag(DM::FlagByBlock);
		linePen.setColor(lineColor);
		//solidPen.setWidth(DM::LineWidth::Width08);	//无效
		m_pCursor = new DmEntityContainer(container);
		m_pCursor->setPen(containerPen);
		container->addEntity(m_pCursor);
		double height = m_pContext->getCurCharHeight();
		DmVector xOffset(height * 0.1, 0.0);
		DmVector yOffsetUp(0.0, height);
		DmVector yOffsetDown(0.0, -height / 3.0);

		DmVector cursorPos = m_currentCursorPos;
		if (std::abs(m_leftTop.x - cursorPos.x) < 1e-5)	//如果在编辑框左侧边界，将光标右移一个像素，避免不显示
		{
			cursorPos.x = m_leftTop.x + 1.0 / m_factor.x;
		}
		else if (std::abs(m_rightBottom.x - cursorPos.x) < 1e-5)	//如果在编辑框右侧边界
		{
			cursorPos.x = m_rightBottom.x - 1.0 / m_factor.x;
		}

		DmLine* l1 = new DmLine(m_pCursor, cursorPos + yOffsetDown, cursorPos + yOffsetUp);
		l1->setPen(linePen);
		m_pCursor->addEntity(l1);
		//DmLine* l2 = new DmLine(m_pCursor, m_currentCursorPos - xOffset, m_currentCursorPos + xOffset);
		//l2->setPen(solidPen);
		//m_pCursor->addEntity(l2);
		//DmLine* l3 = new DmLine(m_pCursor, m_currentCursorPos + yOffsetUp - xOffset, m_currentCursorPos + yOffsetUp + xOffset);
		//l3->setPen(solidPen);
		//m_pCursor->addEntity(l3);
	}
	specifyModified();
	update();
}

void MTextEditWidget::slotFormatChanged()
{
    if (!hasSelectedChar())
    {
        return;
    }
    m_cmdManager.start("Change text format");
	matchSelectedChars(m_pContext);
	m_pMText->updateTextPosition();
	drawCursorOrCoverBySelectStateImmediately();
    m_cmdManager.done();
}

void MTextEditWidget::slotViewChanged()
{
	updateGeometry();
}

void MTextEditWidget::slotTextStyleChanged()
{
    int idx;
    m_pMText->locateCharIndex(m_cursorPreChar, m_cursorPostChar, idx);
    DmChar* oldPreChar = m_cursorPreChar;
    DmChar* oldPostChar = m_cursorPostChar;

    m_cmdManager.start("Change text style");
    // 修改文字样式将替换原来的所有段落及字符
    auto cmd = new MTextEdit_ChangeStyleCmd(m_pMText, m_pContext->getCurTextStyle());
    m_cmdManager.addAndExecuteCmd(cmd);
    // 保存光标位置
    m_pMText->locateByIndex(idx, m_cursorPreChar, m_cursorPostChar);
    auto precharChangeCmd = new EntityChangeCmd<MTextEditWidget,DmChar*,&MTextEditWidget::m_cursorPreChar>(this, m_cursorPreChar, oldPreChar);
    m_cmdManager.addAndExecuteCmd(precharChangeCmd);
    auto postcharChangeCmd = new EntityChangeCmd<MTextEditWidget,DmChar*,&MTextEditWidget::m_cursorPostChar>(this, m_cursorPostChar, oldPostChar);
    m_cmdManager.addAndExecuteCmd(postcharChangeCmd);

    m_pMText->updateTextPosition();
    //设置新光标坐标
    setEditCursorPos(true);
    setSelectBeginEndToNull(false);
    setSelectBeginEndToNull(true);
    //绘制光标
    drawEditCursorImmediately(true);

    increaseHeightIfNotEnough();
    m_cmdManager.done();
}

void MTextEditWidget::slotJustificationChanged()
{
    m_cmdManager.start("Change text justification");
    MTextEdit_ModifyJustificationCmd* cmd = new MTextEdit_ModifyJustificationCmd(m_pMText, m_pContext->getJustification());
    m_cmdManager.addAndExecuteCmd(cmd);

	m_pMText->updateTextPosition();
	if (hasSelectedChar())
	{
		//更新选择高亮
		drawSelectCoverImmediately();
	}
	else
	{
		setEditCursorPos();
	}
    m_cmdManager.done();
}

void MTextEditWidget::slotParaAlignmentChanged()
{
	// 设置选择段落或当前段落的对齐方式
	std::vector<DmMTextParagraph*> paras;
	bool hasSelect = hasSelectedChar();
	if (hasSelect)
	{
		getSelectedParas(paras);
	}
	else
	{
		DmMTextParagraph* para = m_pMText->getParagraph(m_cursorPreChar, m_cursorPostChar);
		if (para)
		{
			paras.emplace_back(para);
		}
	}
	if (paras.size() == 0)
	{
		return;
	}

    m_cmdManager.start("Change paragraphs alignment");
    MTextEdit_ParasAlignCmd*  cmd = new MTextEdit_ParasAlignCmd(m_pMText, paras, m_pContext->getParaAlignment());
    m_cmdManager.addAndExecuteCmd(cmd);
	m_pMText->updateTextPosition();
	if (hasSelect)
	{
		//更新选择高亮
		drawSelectCoverImmediately();
	}
	else
	{
		setEditCursorPos();
	}
    m_cmdManager.done();
}

void MTextEditWidget::slotAddSymbol(const QString& symbol)
{
	insertTextAtCursor(symbol);
}

void MTextEditWidget::slotMatch()
{
	//进入格式刷状态
	if (m_pContext->getIsMatching())
	{
		if (nullptr == m_matchContext.get())
		{
			m_matchContext.reset(new ActionDrawMTextContext(*m_pContext));
		}
	}
	//取消格式刷状态
	else
	{
		m_matchContext.reset(nullptr);
	}
}

void MTextEditWidget::slotUndo()
{
    keyPressEvent_Undo();
}

void MTextEditWidget::slotRedo()
{
    keyPressEvent_Redo();
}

void MTextEditWidget::slotCmdChanged()
{
    bool hasUndo=false;
    bool hasRedo=false;
    m_cmdManager.getCmdData(hasUndo, hasRedo);
    m_pContext->emitUndoToOption(hasUndo, hasRedo);
}
