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

/// @file UISpecialCharBox.cpp
/// @brief 特殊字符（直径符号、角度符号、正负号等）选择下拉框控件

#include "UISpecialCharBox.h"

UISpecialCharBox::UISpecialCharBox(QWidget* parent /*= nullptr*/)
    : QComboBox(parent)
{
    init();
}

void UISpecialCharBox::slotTextActivated(const QString& text)
{
    QStringList strs = text.split(QRegExp("\\s"));
    QString last = strs.back();
    QString res{ "" };
    if (last.startsWith("%%"))
    {
        QChar c = last.at(2).toLower();
        if (c == "c")
        {
            //res = "ø";	//无效

            res = QChar(0x00F8);

            ////另一种有效的转换方式：
            //std::wstring wstr(L"ø");
            //res = QString::fromStdWString(wstr);
        }
        else if (c == "p")
        {
            res = QChar(0x00B1);
        }
        else if (c == "d")
        {
            res = QChar(0x00B0);
        }
    }
    else
    {
        int plusIdx = last.indexOf("+");
        QString numStr = last.mid(plusIdx + 1);
        bool ok = false;
        int unicode = numStr.toInt(&ok, 16);
        res = QChar(unicode);
    }
    if (!res.isEmpty())
    {
        emit charActivated(res);
    }
}

void UISpecialCharBox::init()
{
    QStringList items{
        tr("Degrees %%d"),
        tr("Plus/Minus %%p"),
        tr("Diameter %%c"),
        tr("Almost Equal \\U+2248"),
        tr("Angle \\U+2220"),
        tr("Boundary Line \\U+E100"),
        tr("Center Line \\U+2104"),
        tr("Delta \\U+0394"),
        tr("Electrical Phase \\U+0278"),
        tr("Flow Line \\U+E101"),
        tr("Identity \\U+2261"),
        tr("Monument Line \\U+E102"),
        tr("Not Equal \\U+2260"),
        tr("Ohm \\U+2126"),
        tr("Omega \\U+03A9"),
        tr("Property Line \\U+214A"),
        tr("Subscript2 \\U+2082"),
        tr("Squared \\U+00B2"),
        tr("Cubed \\U+00B3"),
    };
    addItems(items);
    connect(this, SIGNAL(textActivated(QString)), this, SLOT(slotTextActivated(QString)));
    setMaxVisibleItems(items.size());
}
