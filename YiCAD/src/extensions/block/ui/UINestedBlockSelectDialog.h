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

/// @file UINestedBlockSelectDialog.h
/// @brief 嵌套块选择对话框，用于选择要编辑的嵌套块层级

#ifndef UINESTEDBLOCKSELECT_DIALOG_H
#define UINESTEDBLOCKSELECT_DIALOG_H

#include <memory>

#include <QDialog>
#include <QStringList>

class QListWidget;
class QDialogButtonBox;
class DmDocument;
class GsModel;
class GuiPreviewWidget;

/// @brief 嵌套块选择对话框
/// 左侧显示块层级列表，右侧显示选中块的预览
class UINestedBlockSelectDialog : public QDialog
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param doc 文档指针
    /// @param graphics 文档的图形模型，预览直接画其中的块几何；可以为空
    /// @param blockNames 嵌套块名称列表（包含顶层块自身）
    /// @param parent 父窗口
    UINestedBlockSelectDialog(DmDocument* doc, std::shared_ptr<GsModel> graphics, const QStringList& blockNames,
                              QWidget* parent = nullptr);
    ~UINestedBlockSelectDialog() override;

    /// @brief 获取用户选择的块名称
    QString selectedBlockName() const;

private slots:
    void onSelectionChanged(int row);

private:
    DmDocument* m_document;
    QListWidget* m_listWidget;
    GuiPreviewWidget* m_previewWidget;
    QDialogButtonBox* m_buttonBox;
    std::shared_ptr<GsModel> m_graphics;
    QString m_selectedName;
    QStringList m_blockNames;
};

#endif // UINESTEDBLOCKSELECT_DIALOG_H
