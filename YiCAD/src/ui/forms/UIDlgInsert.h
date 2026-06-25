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

/// @file UIDlgInsert.h
/// @brief 块参照插入/编辑对话框

#ifndef UIDLGINSERT_H
#define UIDLGINSERT_H

class DmBlockReference;

#include "ui_UIDlgInsert.h"

class UIDlgInsert : public QDialog, public Ui::UIDlgInsert
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] parent 父窗口指针
    /// @param [in] modal 是否为模态对话框
    /// @param [in] fl 窗口标志
    UIDlgInsert(QWidget* parent = nullptr, bool modal = false, Qt::WindowFlags fl = Qt::WindowFlags());

    ~UIDlgInsert();

public slots:
    /// @brief 设置要编辑的块参照
    /// @param [in] i 块参照引用
    virtual void setInsert(DmBlockReference& i);

    /// @brief 更新块参照以匹配用户修改
    virtual void updateInsert();

protected slots:
    virtual void languageChange();

private:
    DmBlockReference* m_pInsert = nullptr; ///< 待编辑的块参照指针
};

#endif // UIDLGINSERT_H
