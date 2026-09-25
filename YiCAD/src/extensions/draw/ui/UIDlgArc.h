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

/// @file UIDlgArc.h
/// @brief 圆弧属性对话框

#ifndef UIDLGARC_H
#define UIDLGARC_H

class DmArc;

#include "ui_UIDlgArc.h"

class UIDlgArc : public QDialog, public Ui::UIDlgArc
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] parent 父窗口指针
    /// @param [in] modal 是否模态
    /// @param [in] fl 窗口标志
    UIDlgArc(QWidget* parent = nullptr, bool modal = false, Qt::WindowFlags fl = Qt::WindowFlags());

    /// @brief 析构函数
    ~UIDlgArc();

public slots:
    /// @brief 设置圆弧对象
    /// @param [in,out] a 圆弧对象引用
    virtual void setArc(DmArc& a);

    /// @brief 更新圆弧属性
    virtual void updateArc();

protected slots:
    /// @brief 语言切换槽
    virtual void languageChange();

private:
    DmArc* arc;   ///< 圆弧对象指针
};

#endif // UIDLGARC_H
