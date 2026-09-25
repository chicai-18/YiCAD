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

/// @file UIDlgLine.h
/// @brief 直线属性编辑对话框

#ifndef UIDLGLINE_H
#define UIDLGLINE_H

class DmLine;

#include "ui_UIDlgLine.h"

class UIDlgLine : public QDialog, public Ui::UIDlgLine
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] parent 父窗口指针
    /// @param [in] modal 是否为模态对话框
    /// @param [in] fl 窗口标志
    UIDlgLine(QWidget* parent = nullptr, bool modal = false, Qt::WindowFlags fl = Qt::WindowFlags());

    ~UIDlgLine();

public slots:
    /// @brief 设置要编辑的直线实体
    /// @param [in] l 直线实体引用
    virtual void setLine(DmLine& l);

    /// @brief 更新直线实体以匹配用户修改
    virtual void updateLine();

protected slots:
    virtual void languageChange();

private:
    DmLine* m_pLine = nullptr; ///< 待编辑的直线实体指针
};

#endif // UIDLGLINE_H
