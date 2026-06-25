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

/// @file UIColorBox.h
/// @brief 颜色选择下拉框控件

#ifndef UICOLORBOX_H
#define UICOLORBOX_H

#include <QComboBox>
#include <memory>

class DmColor;

/// @brief A combobox for choosing a color.
class UIColorBox : public QComboBox
{
    Q_OBJECT
public:
    UIColorBox(QWidget* parent = nullptr, const char* name = nullptr);
    virtual ~UIColorBox();

    /// @brief 获得当前颜色
    DmColor getColor() const;

    /// @brief 设置当前颜色为指定颜色（不发送colorChanged信号）
    void setColor(const DmColor& color);
    /// @brief 初始化
    /// @param showByLayer 是否显示随层随块
    void init(bool showByLayer);

    /// @brief 初始化条目
    void initItems();

    /// @brief 获得指定颜色的索引，没有则返回-1
    int indexOfColor(const DmColor& c);

protected:
    /// @brief 添加指定颜色条目
    void addColor(QColor color, QString text);
    void addColor(Qt::GlobalColor color, QString text);
    /// @brief 添加颜色到历史
    void addColorToHistory(const DmColor& c);

private slots:
    void slotColorChanged(int index);

signals:
    void colorChanged(const DmColor& color);

private:
    std::unique_ptr<DmColor>    m_pCurrentColor;        ///< 当前颜色
    std::vector<DmColor>        m_historyColors;        ///< 历史颜色。用户选择过的自定义颜色，添加在列表后面
    int                         m_iColorIndexStart = 0; ///< 第一个非随层随块颜色的索引（”红色”的索引）
    bool                        m_isShowByLayer = false;    ///< 是否显示”随层””随块”
    bool                        m_isChangingByCode = false; ///< 是否由代码设置选项项，用于”自定义颜色”时避免死循环
};

#endif
