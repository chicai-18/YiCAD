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

/// @file UITextStyleBox.h
/// @brief 文字样式下拉列表框

#ifndef UITEXTSTYLEBOX_H
#define UITEXTSTYLEBOX_H

#include <QComboBox>

class DmTextStyle;
class DmTextStyleTable;

typedef bool (*ChangeQueryFunc)() ;
/// @brief 文字样式下拉列表框。支持用户选择确认提示
class UITextStyleBox : public QComboBox
{
Q_OBJECT
public:
    explicit UITextStyleBox(QWidget* parent = nullptr);
    virtual ~UITextStyleBox() = default;

    DmTextStyle* getStyle();
    void setStyle(const QString& style);
    void init(DmTextStyleTable* textStyleTable);
    /// @brief 选择提交时的确认回调
    void setChangeQueryFunc(ChangeQueryFunc callBack);
protected:
    /// @brief 判断是由用户在UI触发
    void mousePressEvent(QMouseEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
private slots:
    void slotStyleChanged(const QString& text);
signals:
    void styleChanged();

private:
    bool m_userChoose = false; ///< 是否为用户在UI上触发
    int m_lastIndex = 0;	///< 选择改变之前的当前索引
    ChangeQueryFunc m_changeQueryFunc = nullptr;	///< 外部注册的回调函数，用来在选项改变前做提示，根据返回值来决定是否做改变
    DmTextStyle*		m_pStyle = nullptr;
    DmTextStyleTable*	m_pTextStyleTable = nullptr;
};
#endif //UITEXTSTYLEBOX_H