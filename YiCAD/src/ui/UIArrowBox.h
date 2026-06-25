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

/// @file UIArrowBox.h
/// @brief 标注箭头形状选择下拉框控件

#ifndef UIARROWBOX_H
#define UIARROWBOX_H

#include <QComboBox>
#include "Datamodel.h"

class UIArrowBox : public QComboBox
{
    Q_OBJECT
public:
    UIArrowBox(QWidget* parent = nullptr);
    virtual ~UIArrowBox() = default;

    DM::ArrowType getArrowType() const;
    void setArrowType(DM::ArrowType arrowType);
private:
    void init();
private slots:
    void slotArrowChanged(int index);
signals:
    void arrowChanged(DM::ArrowType);

private:
    DM::ArrowType m_curArrowType;   // TODO: 需确认默认值并初始化
};
#endif // UIARROWBOX_H