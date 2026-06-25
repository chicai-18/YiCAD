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

/// @file UIDlgLineType.h
/// @brief 线型管理对话框

#ifndef UIDLGLINETYPE_H
#define UIDLGLINETYPE_H

#include <QDialog>
#include <QWidget>
#include <QDebug>
#include <QPushButton>
#include <QFontDialog>
#include <QFont>
#include <QFile>
#include <QMessageBox>

#include "UIDlgLoadLineType.h"
#include "ui_UIDlgLineType.h"
#include "DmLineTypeTable.h"

QT_BEGIN_NAMESPACE
namespace Ui { class UIDlgLineType; }
QT_END_NAMESPACE

class UIDlgLineType : public QDialog
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] parent 父窗口指针
    /// @param [in] modal 是否为模态对话框
    /// @param [in] fl 窗口标志
    UIDlgLineType(QWidget* parent = nullptr, bool modal = false, Qt::WindowFlags fl = Qt::WindowFlags());

    ~UIDlgLineType();

    void init();
    void UpdateTableWidget();

    /// @brief 获取当前选中的表格项索引
    /// @return 当前行号，若未选中则返回-1
    int getCurTableWidgetItem();

    /// @brief 设置线型表
    /// @param [in] lineTypeTable 线型表指针
    /// @param [in] document 文档指针
    void setLineTypeTable(DmLineTypeTable* lineTypeTable, DmDocument* document);

    /// @brief 获取线型表
    /// @return 线型表指针
    DmLineTypeTable* getLineTypeTable();

    /// @brief 获取指定索引的线型
    /// @param [in] i 索引
    /// @return 线型指针，若未找到则返回nullptr
    DmLineType* lineTypeAt(int i);

private slots:
    void on_LoadLine_clicked();
    void on_DeleteLine_clicked();
    void on_SetCurLine_clicked();
    void on_HideLine_clicked();
    void on_Ok_clicked();
    void on_Cancel_clicked();
    void slotAddLineType(DmLineType* data);
    void on_LineTypeData_clicked();

private:
    std::unique_ptr<Ui::UIDlgLineType> ui;

    DmLineTypeTable* m_pLineTypeTable = nullptr; ///< 线型列表
    DmLineType* m_pLineType = nullptr;           ///< 线型
    DmDocument* m_pDocument = nullptr;           ///< 文档指针

    bool m_hideBox = true;                       ///< 判断是否隐藏Box
};

#endif // UIDLGLINETYPE_H
