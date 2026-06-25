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

/// @file UICurrentActivePen.h
/// @brief 当前文档激活的画笔UI控件

#ifndef UICURRENTACTIVEPEN_H
#define UICURRENTACTIVEPEN_H

#include <QWidget>
#include "Datamodel.h"
#include "DmLineType.h"

class UILineTypeBox;
class UIColorBox;
class UIWidthBox;
class DmDocument;
class DmPen;
class DmColor;

/// @brief 当前文档激活的画笔UI（显示当前文档激活的画笔颜色、线型、线宽）
class UICurrentActivePen : public QWidget
{
    Q_OBJECT

public:
    UICurrentActivePen(QWidget* parent);
    ~UICurrentActivePen();

public:
    void setPen(DmDocument* doc);

    DmLineType* getLineType();

    DM::LineWidth getLineWidth();
    DmColor getColor();
    void update(DmDocument* doc);

private slots:
    void slotSelectChanged();

private:
    UILineTypeBox*  m_pCurrentLineType = nullptr;   ///< 当前文档活动的线型
    UIColorBox*     m_pCurrentColor = nullptr;       ///< 当前文档活动的颜色
    UIWidthBox*     m_pCurrentWidth = nullptr;       ///< 当前文档活动的线宽
    DmDocument*     m_document = nullptr;            ///< doc
};

#endif