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

/// @file UITextStyleBox.cpp
/// @brief 文字样式选择下拉框控件，管理和切换CAD文档中的文字样式

#include "UITextStyleBox.h"
#include "DmTextStyle.h"
#include "DmTextStyleTable.h"
#include <QKeyEvent>
#include <QMessageBox>


UITextStyleBox::UITextStyleBox(QWidget* parent)
    : QComboBox(parent)
    , m_lastIndex(-1)
    , m_changeQueryFunc(nullptr)
    , m_pStyle(nullptr)
    , m_pTextStyleTable(nullptr)
{

}

DmTextStyle* UITextStyleBox::getStyle()
{
    return m_pStyle;
}

void UITextStyleBox::setStyle(const QString& style)
{
    setCurrentText(style);
    auto oldStyle = m_pStyle;
    m_pStyle = m_pTextStyleTable->find(style);
    if (oldStyle != m_pStyle)
    {
        emit styleChanged();
    }
}

void UITextStyleBox::init(DmTextStyleTable* textStyleTable)
{
    m_pTextStyleTable = textStyleTable;
    clear();
    QStringList list;
    DmTextStyle* pActive = m_pTextStyleTable->getActive();
    int i = 0;
    int activeIdx = 0;
    for (auto it = m_pTextStyleTable->begin(); it != m_pTextStyleTable->end(); ++it)
    {
        list.append((*it)->getName());
        if (*it == pActive)
        {
            activeIdx = i;
        }
        i++;
    }
    addItems(list);
    connect(this, SIGNAL(currentTextChanged(const QString&)), this, SLOT(slotStyleChanged(const QString&)));
    if (pActive)
    {
        setCurrentIndex(activeIdx);
        m_lastIndex = activeIdx;
        setStyle(pActive->getName());
    }
}

void UITextStyleBox::setChangeQueryFunc(ChangeQueryFunc callBack)
{
    m_changeQueryFunc = callBack;
}

void UITextStyleBox::mousePressEvent(QMouseEvent* e)
{
    m_userChoose = true;
    QComboBox::mousePressEvent(e);
}

void UITextStyleBox::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Up || e->key() == Qt::Key_Down)
    {
        m_userChoose = true;
    }
    QComboBox::keyPressEvent(e);
}

void UITextStyleBox::slotStyleChanged(const QString& text)
{
    if (m_userChoose)
    {
        // 如果注册了回调函数，调用它
        bool checkVal = true;
        if (m_changeQueryFunc != nullptr)
        {
            checkVal = (*m_changeQueryFunc)();
        }

        // 取消选择变化
        if (!checkVal)
        {
            m_userChoose = false;
            if (m_lastIndex != -1 && count() != 0)
            {
                setCurrentIndex(m_lastIndex);
            }
            return;
        }
    }

    // 确认选择变化
    m_lastIndex = currentIndex();
    m_pStyle = m_pTextStyleTable->find(text);
    emit styleChanged();
}
