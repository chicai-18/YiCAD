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

/// @file UIDlgLoadLineType.h
/// @brief 从线型文件加载线型的对话框

#ifndef UIDLGLOADLINETYPE_H
#define UIDLGLOADLINETYPE_H

#include "ui_UIDlgLoadLineType.h"

#include <QDialog>
#include <QFileDialog>
#include <QString>
#include <QList>

#include "DmLineType.h"
#include "DmLineTypeTable.h"

class UIDlgLoadLineType : public QDialog, public Ui::UIDlgLoadLineType
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] parent 父窗口指针
    /// @param [in] modal 是否为模态对话框
    /// @param [in] fl 窗口标志
    explicit UIDlgLoadLineType(QWidget* parent = nullptr, bool modal = false, Qt::WindowFlags fl = Qt::WindowFlags());

    ~UIDlgLoadLineType();

    void init();

    /// @brief 获取表格中选中的线型
    /// @return 选中的线型指针，若未选中则返回nullptr
    DmLineType* selectTableWidget();

    /// @brief 设置文档对象
    /// @param [in] doc 文档指针
    void setDocument(DmDocument* doc)
    {
        m_doc = doc;
    }

protected:
    /// @brief 读取line文件，获得线型名称和线型数据
    /// @param [in] path 线型文件路径
    void readFile(const QString& path);

    void clearLineTypes();

private slots:
    void on_file_clicked();
    void on_Ok_clicked();
    void on_Cancel_clicked();

signals:
    void lineTypeSelected(DmLineType* data);

private:
    std::vector<DmLineType*> m_lineTypes; ///< 线型列表
    DmDocument* m_doc = nullptr;          ///< 文档指针
};

#endif // UIDLGLOADLINETYPE_H
