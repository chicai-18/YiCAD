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

/// @file UIDlgEllipse.h
/// @brief 椭圆/椭圆弧属性编辑对话框

#ifndef UIDLGELLIPSE_H
#define UIDLGELLIPSE_H

class DmEllipse;

#include "ui_UIDlgEllipse.h"

class UIDlgEllipse : public QDialog, public Ui::UIDlgEllipse
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] parent 父窗口指针
    /// @param [in] modal 是否为模态对话框
    /// @param [in] fl 窗口标志
    UIDlgEllipse(QWidget* parent = nullptr, bool modal = false, Qt::WindowFlags fl = Qt::WindowFlags());

    ~UIDlgEllipse();

public slots:
    /// @brief 设置要编辑的椭圆实体
    /// @param [in] e 椭圆引用
    virtual void setEllipse(DmEllipse& e);

    /// @brief 更新椭圆实体以匹配用户修改
    virtual void updateEllipse();

protected slots:
    virtual void languageChange();

private:
    DmEllipse* m_pEllipse = nullptr; ///< 待编辑的椭圆实体指针
};

#endif // UIDLGELLIPSE_H
