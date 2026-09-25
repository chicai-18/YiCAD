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

/// @file UILineTypeBox.cpp
/// @brief 线型选择下拉框控件，支持预定义线型和自定义线型加载

#include "UILineTypeBox.h"

#include "Debug.h"

#include "UIDialogRunner.h"
#include "UIDlgLineType.h"
#include "ApplicationWindow.h"
#include "DmDocument.h"

class Document;
class MDIWindow;

namespace
{
    constexpr int kLineTypePreviewWidth = 10;
}

UILineTypeBox::UILineTypeBox(QWidget* parent)
    : QComboBox(parent)
    , m_currentLineType(nullptr)
    , m_isChangingByCode(false)
{
    m_isShowByLayer = false;

    // 默认用当前文档的线型表。没有主窗口时（单测里构造含画笔控件的对话框）为空，这时
    // 下拉框只有"自定义"一项，选中也什么都不做
    ApplicationWindow* appWindow = ApplicationWindow::getAppWindow();
    m_document = appWindow ? static_cast<DmDocument*>(appWindow->getDocument()) : nullptr;
    m_LineTypeTable = m_document ? m_document->getLineTypeTable() : nullptr;
}

UILineTypeBox::~UILineTypeBox()
{
}

void UILineTypeBox::init(bool m_isShowByLayer)
{
    this->m_isShowByLayer = m_isShowByLayer;

    updateLineTypeTable();
    connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(slotLineTypeChanged(int)));
    setCurrentIndex(0);
    setMinimumWidth(180);
}

void UILineTypeBox::updateLineTypeTable()
{
    m_isChangingByCode = true;
    this->clear();
    if (!m_LineTypeTable)
    {
        addItem(tr("Custom"));
        m_isChangingByCode = false;
        return;
    }
    QStringList list;

    for (auto& linetype : *m_LineTypeTable)
    {
        list.append(linetype->getLineTypeOutWard().mid(0, kLineTypePreviewWidth) + linetype->getLineTypeName());
        setToolTip(linetype->getLineTypeOutWard() + linetype->getLineTypeName());
    }
    addItems(list);
    addItem(tr("Custom"));
    setLineType(m_LineTypeTable->getActive());
    m_isChangingByCode = false;
}

void UILineTypeBox::updateLineTypeTable(DmDocument* doc)
{
    QStringList list;
    m_document = doc;
    m_LineTypeTable = doc->getLineTypeTable();
    updateLineTypeTable();
}

DmLineType* UILineTypeBox::getLineType()
{
    return m_currentLineType;
}

void UILineTypeBox::setLineType(DmLineType* t)
{
    int index = indexOf(t);
    if (-1 == index)
    {
        return;
    }
    m_isChangingByCode = true;
    setCurrentIndex(index);
    m_currentLineType = t;
    m_isChangingByCode = false;
}

void UILineTypeBox::setLineType(DmLineType* t, DmDocument* d)
{
    m_document = d;
    m_LineTypeTable = m_document->getLineTypeTable();
    setLineType(t);
}

int UILineTypeBox::indexOf(DmLineType* t)
{
    int index = -1;
    if (!m_LineTypeTable || !t)
    {
        return index;
    }
    int i = 0;
    for (auto lineType : *m_LineTypeTable)
    {
        if (lineType->getLineTypeName() == t->getLineTypeName())
        {
            index = i;
            break;
        }
        i++;
    }
    return index;
}

DmLineType* UILineTypeBox::lineTypeAt(int idx)
{
    if (!m_LineTypeTable)
    {
        return nullptr;
    }
    int i = 0;
    for (auto lineType : *m_LineTypeTable)
    {
        if (idx == i)
        {
            return lineType;
        }
        i++;
    }
    return nullptr;
}

void UILineTypeBox::slotLineTypeChanged(int index)
{
    if (m_isChangingByCode || !m_LineTypeTable)
    {
        return;
    }

    // 自定义。加载线型
    if (m_LineTypeTable->count() == index)
    {
        {
            // 挂在线型框所在的窗口上：主窗口，或打开它的对话框（如标注样式）
            UIDlgLineType dlg(window(), true);
            dlg.setLineTypeTable(m_LineTypeTable, m_document);
            UIDialogRunner::exec(dlg);
        }

        m_isChangingByCode = true;
        updateLineTypeTable();
        m_currentLineType = m_LineTypeTable->getActive();
        int idx = indexOf(m_currentLineType);
        setCurrentIndex(idx);
        m_isChangingByCode = false;
        setToolTip(m_currentLineType->getLineTypeOutWard() + m_currentLineType->getLineTypeName());
    }
    // 选择已有线型
    else
    {
        m_currentLineType = lineTypeAt(index);
        m_isChangingByCode = true;
        setCurrentIndex(index);
        m_isChangingByCode = false;
        setToolTip(m_currentLineType->getLineTypeOutWard() + m_currentLineType->getLineTypeName());
    }

    if (m_isChangingByCode)
    {
        return;
    }
    emit lineTypeChanged(m_currentLineType);
}
