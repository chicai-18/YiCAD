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

/// @file UIPatternBox.cpp
/// @brief 填充图案选择下拉框控件

#include "UIPatternBox.h"

#include <QPixmap>
#include <QStringList>

#include "DmPattern.h"
#include "DmPatternList.h"
#include "Debug.h"

// Default Constructor. You must call init manually if you choose to use this constructor.
UIPatternBox::UIPatternBox(QWidget* parent)
    : QComboBox(parent)
{
}

UIPatternBox::~UIPatternBox() = default;

// Initialisation (called manually and only once).
void UIPatternBox::init()
{
    QStringList patterns;

    for (auto const& pa : *DMPATTERNLIST)
    {
        patterns.append(pa.first);
    }

    patterns.sort();
    insertItems(0, patterns);

    connect(this, SIGNAL(activated(int)), this, SLOT(slotPatternChanged(int)));

    setCurrentIndex(0);
    slotPatternChanged(currentIndex());
}

// Sets the currently selected width item to the given width.
void UIPatternBox::setPattern(const QString& pName)
{
    setCurrentIndex(findText(pName));
    slotPatternChanged(currentIndex());
}

DmPattern* UIPatternBox::getPattern()
{
    if (nullptr == m_pCurrentPattern)
    {
        m_pCurrentPattern = DMPATTERNLIST->requestPattern(currentText());
    }
    return m_pCurrentPattern;
}

// Called when the pattern has changed. This method sets the current pattern to the value chosen.
void UIPatternBox::slotPatternChanged(int index)
{
    m_pCurrentPattern = DMPATTERNLIST->requestPattern(currentText());
    emit patternChanged();
}
