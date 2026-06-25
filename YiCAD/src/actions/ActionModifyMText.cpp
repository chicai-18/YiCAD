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


/// @file ActionModifyMText.cpp
/// @brief 编辑多行文字属性的交互动作类实现

#include "ActionModifyMText.h"

#include <QMouseEvent>

#include "ApplicationWindow.h"
#include "DmDocument.h"
#include "DmMText.h"
#include "GuiDocumentView.h"
#include "GuiEventHandler.h"
#include "MDIWindow.h"
#include "Transaction.h"
#include "UIMTextModifyOptions.h"

namespace
{
    /// @brief 选项面板的几何X位置
    constexpr int OPTION_PANEL_X = 1;

    /// @brief 选项面板的几何Y位置
    constexpr int OPTION_PANEL_Y = 186;

    /// @brief 选项面板宽度
    constexpr int OPTION_PANEL_WIDTH = 500;

    /// @brief 选项面板高度
    constexpr int OPTION_PANEL_HEIGHT = 70;
}

/// @brief 构造函数
/// @param [in] doc 文档指针
/// @param [in] docView 文档视图指针
ActionModifyMText::ActionModifyMText(DmDocument* doc, GuiDocumentView* docView)
    : ActionInterface("Modify MText", doc, docView)
    , m_pMText(nullptr)
    , m_option(nullptr)
    , m_optionBack(nullptr)
{
    actionType = DM::ActionModifyMText;
}

/// @brief 判断是否可被其他Action中断
/// @return 始终返回false，此动作不可中断
bool ActionModifyMText::canBeInterrupt()
{
    return false;
}

/// @brief 初始化动作，创建非模态选项面板
/// @param [in] status 初始状态
void ActionModifyMText::init(int status)
{
    ActionInterface::init(status);
    m_optionBack = new QWidget(ApplicationWindow::getAppWindow());
    m_optionBack->setObjectName("mTextModifyOptionBackWidget");
    m_option = new UIMTextModifyOptions(m_optionBack);
    m_option->setAction(this);
    m_optionBack->setGeometry(OPTION_PANEL_X, OPTION_PANEL_Y, OPTION_PANEL_WIDTH, OPTION_PANEL_HEIGHT);
    m_optionBack->show();
}

/// @brief 鼠标移动事件处理
/// @param [in] e 鼠标事件指针
void ActionModifyMText::mouseMoveEvent(QMouseEvent* e)
{
    // 这样才能绘制鼠标，否则显示像卡顿的效果
    deleteSnapper();
}

/// @brief 鼠标释放事件处理
/// @param [in] e 鼠标事件指针
void ActionModifyMText::mouseReleaseEvent(QMouseEvent* e)
{
    if (m_pMText)
    {
        m_pMText->setSelected(false);
        // 不能发送选择信号，因为本Action不可中断，this会被释放，后面的finish()调用就会异常
    }
    finish();
    docView->emitSelectedChanged();
}

/// @brief 鼠标双击事件处理
/// @param [in] e 鼠标事件指针
void ActionModifyMText::mouseDoubleClickEvent(QMouseEvent* e)
{
    if (m_pMText)
    {
        m_pMText->setSelected(false);
    }
    finish();
    // 触发双击编辑
    auto defAction = ApplicationWindow::getAppWindow()->getMDIWindow()->getEventHandler()->getDefaultAction();
    defAction->mouseDoubleClickEvent(e);
}

/// @brief 如果多行文字内容为空，更新其内容
/// @param [in] theText 多行文字实体指针
void ActionModifyMText::updateContentIfEmpty(DmMText* theText)
{
    // 更新实体
    if (theText->getContent().isEmpty())
    {
        theText->updateContent();
    }
}

/// @brief 获取当前文档指针
/// @return 文档指针
DmDocument* ActionModifyMText::getDocument()
{
    return pDocument;
}

/// @brief 设置当前编辑的多行文字
/// @param [in] editingText 多行文字实体指针
void ActionModifyMText::setText(DmMText* editingText)
{
    m_pMText = editingText;
}

/// @brief 结束当前动作
/// @param [in] updateTB 是否更新工具栏
void ActionModifyMText::finish(bool updateTB)
{
    ActionInterface::finish(updateTB);

    // 结束时删除窗体
    if (m_optionBack)
    {
        m_optionBack->close();
        m_optionBack = nullptr;
        m_option = nullptr;
    }
    m_pMText = nullptr;
    // 考虑到用户可能按Delete删除实体，（被中断的情况下）编辑的文字还是保持选择状态
    // if (m_pMText)
    // {
    //     m_pMText->setSelected(false);
    // }
}

/// @brief 设置文字高度
/// @param [in] height 文字高度
void ActionModifyMText::setHeight(const double height)
{
    if (!m_pMText || m_pMText->getCharHeight() == height)
    {
        return;
    }

    Transaction t(tr("Modify MText").toStdString(), pDocument);
    t.start();
    pDocument->getEntityTable()->startModify(m_pMText);
    m_pMText->setCharHeight(height);
    updateContentIfEmpty(m_pMText);
    t.commit();
}

/// @brief 获取当前文字高度
/// @return 文字高度
double ActionModifyMText::getHeight() const
{
    return m_pMText->getDataConstPtr()->getCharHeight();
}

/// @brief 设置文字样式
/// @param [in] textStyle 文字样式指针
void ActionModifyMText::setStyle(DmTextStyle* textStyle)
{
    if (!m_pMText || m_pMText->getTextStyle() == textStyle)
    {
        return;
    }

    Transaction t(tr("Modify MText").toStdString(), pDocument);
    t.start();
    pDocument->getEntityTable()->startModify(m_pMText);
    m_pMText->setTextStyle(textStyle);
    updateContentIfEmpty(m_pMText);
    t.commit();
}

/// @brief 获取当前文字样式
/// @return 文字样式指针
DmTextStyle* ActionModifyMText::getStyle() const
{
    return m_pMText->getDataConstPtr()->getTextStyle();
}

/// @brief 设置行间距因子
/// @param [in] factor 行间距因子
void ActionModifyMText::setLineSpaceFatctor(const double factor)
{
    if (!m_pMText || m_pMText->getLineSpacingFactor() == factor)
    {
        return;
    }

    Transaction t(tr("Modify MText").toStdString(), pDocument);
    t.start();
    pDocument->getEntityTable()->startModify(m_pMText);
    m_pMText->setLineSpacingFactor(factor);
    updateContentIfEmpty(m_pMText);
    t.commit();
}

/// @brief 获取当前行间距因子
/// @return 行间距因子
double ActionModifyMText::getLineSpaceFatctor() const
{
    return m_pMText->getDataConstPtr()->getLineSpacingFactor();
}

/// @brief 设置旋转角度
/// @param [in] angle 旋转角度
void ActionModifyMText::setAngle(const double angle)
{
    if (!m_pMText || m_pMText->getAngle() == angle)
    {
        return;
    }
    Transaction t(tr("Modify MText").toStdString(), pDocument);
    t.start();
    pDocument->getEntityTable()->startModify(m_pMText);
    m_pMText->setAngle(angle);
    updateContentIfEmpty(m_pMText);
    t.commit();
}

/// @brief 获取当前旋转角度
/// @return 旋转角度
double ActionModifyMText::getAngle() const
{
    return m_pMText->getDataConstPtr()->getAngle();
}

/// @brief 设置行间距
/// @param [in] lineSpace 行间距
void ActionModifyMText::setLineSpace(const double lineSpace)
{
    if (!m_pMText || m_pMText->getLineSpace() == lineSpace)
    {
        return;
    }

    Transaction t(tr("Modify MText").toStdString(), pDocument);
    t.start();
    pDocument->getEntityTable()->startModify(m_pMText);
    m_pMText->setLineSpace(lineSpace);
    updateContentIfEmpty(m_pMText);
    t.commit();
}

/// @brief 获取当前行间距
/// @return 行间距
double ActionModifyMText::getLineSpace() const
{
    return m_pMText->getDataConstPtr()->getLineSpace();
}
