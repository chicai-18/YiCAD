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

/// @file UIDlgPoint.h
/// @brief 点实体属性编辑对话框

#ifndef UIDLGPOINT_H
#define UIDLGPOINT_H

class DmPoint;

#include "ui_UIDlgPoint.h"

class UIDlgPoint : public QDialog, public Ui::UIDlgPoint
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] parent 父窗口指针
    /// @param [in] modal 是否为模态对话框
    /// @param [in] fl 窗口标志
    UIDlgPoint(QWidget* parent = nullptr, bool modal = false, Qt::WindowFlags fl = Qt::WindowFlags());

    ~UIDlgPoint();

public slots:
    /// @brief 设置要编辑的点实体
    /// @param [in] p 点实体引用
    virtual void setPoint(DmPoint& p);

    /// @brief 更新点实体以匹配用户修改
    virtual void updatePoint();

protected slots:
    virtual void languageChange();

private:
    DmPen pen;                   ///< 画笔对象
    DmPoint* m_pPoint = nullptr; ///< 待编辑的点实体指针
};

#endif // UIDLGPOINT_H
