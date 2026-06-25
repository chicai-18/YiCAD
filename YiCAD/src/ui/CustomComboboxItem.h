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

/// @file CustomComboboxItem.h
/// @brief 自定义图层下拉框控件

#ifndef CUSTOMCOMBOBOXITEM_H
#define CUSTOMCOMBOBOXITEM_H

#include <QWidget>
#include <QColor>

class QLabel;
class QHBoxLayout;
class QToolButton;
class QPushButton;
class DmLayer;

struct ComboBoxData
{
    QToolButton*    btnOn = nullptr;       ///< 显示、隐藏
    QToolButton*    btnLock = nullptr;     ///< 锁定
    QToolButton*    btnPrint = nullptr;    ///< 打印
    QToolButton*    btnColor = nullptr;    ///< 颜色
    QPushButton*    labelName = nullptr;   ///< 名字
    QToolButton*    btnDelete = nullptr;   ///< 删除

    bool            isOn = false;
    bool            isLock = false;
    bool            isPrint = false;
    QColor          rgb;
    QString         strName;

    void setByData(ComboBoxData* data);
    void hide();
    void show();

    void setIsOn(const bool isOn);
    bool getIsOn();

    void setIsLock(const bool isLock);
    bool getIsLock();

    void setIsPrint(const bool isPrint);
    bool getIsPrint();

    void setColor(const QColor& color);
    QColor getColor();

    void setLayerName(const QString& name);
    QString getLayerName();
};


/// @brief 自定义图层下拉框
class CustomComboboxItem : public QWidget
{
    Q_OBJECT
public:
    /// @brief 下拉列表框的一行
    CustomComboboxItem(QWidget* parent, DmLayer* layer);
    ~CustomComboboxItem();

    ComboBoxData* getData() const;
    void setData(ComboBoxData* data);

    void setIsOn(const bool isOn);
    bool getIsOn();

    void setIsLock(const bool isLock);
    bool getIsLock();

    void setIsPrint(const bool isPrint);
    bool getIsPrint();

    void setColor(const QColor& color);
    QColor getColor();

    void setLayerName(const QString& name);
    QString getLayerName();
private:
    DmLayer* m_layer = nullptr;
    ComboBoxData* m_pItemData = nullptr;
};

#endif // CUSTOMCOMBOBOXITEM_H
