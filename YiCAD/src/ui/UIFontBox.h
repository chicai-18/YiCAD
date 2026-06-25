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

/// @file UIFontBox.h
/// @brief 字体下拉列表框

#ifndef UIFONTBOX_H
#define UIFONTBOX_H

#include <QComboBox>
#include <vector>

/// @brief 字体图标
enum class FontIconType
{
    None,       //无图标
    Shx,        //shx字体图标
    SystemFont, //系统字体图标
    NotExist    //Yicad不存在的字体图标
};

/// @brief 字体下拉列表框。仅仅显示字体，不能获得DmFont
class UIFontBox: public QComboBox 
{
    Q_OBJECT

public:
    UIFontBox(QWidget* parent = nullptr);
    virtual ~UIFontBox() = default;

    /// @brief 添加字体。唯一接口，不触发fontChanged()信号
    void addFonts(const std::vector<std::pair<FontIconType, QString>>& fonts);
    std::vector<std::pair<FontIconType, QString>> getFonts() const;
    QString firstFontOfType(FontIconType type) const;
    QString firstValidFont() const;
    QString currentFont() const;
    void clearFonts();
    //void init();

    // 下面这些函数删除，避免外部调用
    void addItem(const QIcon& icon, const QString& text, const QVariant& userData = QVariant()) = delete;
    void addItem(const QString& text, const QVariant& userData = QVariant()) = delete;
    void addItems(const QStringList& texts) = delete;
    void clear() = delete;
private slots:

    /// @brief 用来发射fontChanged()信号
    void slotCurrentTextChanged(QString text);
signals:
    /// @brief 字体发生变化信号
    void fontChanged(QString font);

private:
    bool m_canEmitSignal = false;
    std::vector<std::pair<FontIconType, QString>> m_fonts;
};

#endif

