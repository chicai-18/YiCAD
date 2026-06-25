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

/// @file UIWidthBox.h
/// @brief 线宽选择下拉列表框

#ifndef UIWIDTHBOX_H
#define UIWIDTHBOX_H

#include <QComboBox>

#include "Datamodel.h"

/// @brief 线宽选择下拉列表框
class UIWidthBox: public QComboBox 
{
    Q_OBJECT

public:
	UIWidthBox(QWidget* parent = nullptr, const char* name = nullptr);
    virtual ~UIWidthBox() = default;

    DM::LineWidth getWidth() const;
    void setWidth(DM::LineWidth w);

    void init(bool showByLayer);

protected:
    /// @brief 判断是由用户在UI触发
    void mousePressEvent(QMouseEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;

private slots:
    void slotWidthChanged(int index);

signals:
    void widthChanged(DM::LineWidth);

private:
    DM::LineWidth  m_currentWidth = DM::WidthDefault;
    bool            m_isShowByLayer = false;
    bool m_userChoose = false; ///< 是否为用户在UI上触发
};

#endif
