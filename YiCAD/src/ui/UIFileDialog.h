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

/// @file UIFileDialog.h
/// @brief 文件 打开/保存 对话框

#ifndef UIFILEDIALOG_H
#define UIFILEDIALOG_H

#include <QFileDialog>

/// @class UIFileDialog
/// @brief 文件 打开/保存 对话框
class UIFileDialog : public QFileDialog
{
public:
    enum FileType
    {
        DrawingFile = 0,
        BlockFile = 1
    };

    UIFileDialog(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowType::Widget, FileType type = DrawingFile);
    virtual ~UIFileDialog();

    /// @brief 获取打开文件路径
    QString getOpenFile();
    /// @brief 获取保存文件路径
    /// @param [out] formatType 保存格式类型名
    QString getSaveFile(QString& formatType);

private:
    QString         m_strName;      ///< 窗体名
    static const QString  m_strDefaultFilter; ///< Types
};

#endif
