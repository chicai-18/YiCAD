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

#include "GuiDialogFactory.h"
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

    m_document = static_cast<DmDocument*>(ApplicationWindow::getAppWindow()->getDocument());
    m_LineTypeTable = m_document->getLineTypeTable();
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
    if (m_isChangingByCode)
    {
        return;
    }

    // 自定义。加载线型
    if (m_LineTypeTable->count() == index)
    {
        GUIDIALOGFACTORY->requestLineTypeDialog(m_LineTypeTable, m_document);

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
