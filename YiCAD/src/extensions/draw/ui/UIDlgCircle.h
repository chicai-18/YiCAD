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

/// @file UIDlgCircle.h
/// @brief 圆属性对话框

#ifndef UIDLGCIRCLE_H
#define UIDLGCIRCLE_H

class DmCircle;

#include "ui_UIDlgCircle.h"

class UIDlgCircle : public QDialog, public Ui::UIDlgCircle
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] parent 父窗口指针
    /// @param [in] modal 是否模态
    /// @param [in] fl 窗口标志
    UIDlgCircle(QWidget* parent = nullptr, bool modal = false, Qt::WindowFlags fl = Qt::WindowFlags());

    /// @brief 析构函数
    ~UIDlgCircle();

public slots:
    /// @brief 设置圆对象
    /// @param [in,out] c 圆对象引用
    virtual void setCircle(DmCircle& c);

    /// @brief 更新圆属性
    virtual void updateCircle();

protected slots:
    /// @brief 语言切换槽
    virtual void languageChange();

private:
    DmCircle* m_pCircle;   ///< 圆对象指针
};

#endif // UIDLGCIRCLE_H
