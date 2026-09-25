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

/// @file UIBlockDelete.h
/// @brief 块删除对话框

#ifndef UIBLOCKDELETE_H
#define UIBLOCKDELETE_H

#include "ui_UIBlockDelete.h"

class DmBlockTable;
class DmDocument;
class DmEntityContainer;

class UIBlockDelete : public QDialog, public Ui::UIBlockDelete
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] parent 父窗口指针
    /// @param [in] fl 窗口标志
    UIBlockDelete(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());

    /// @brief 析构函数
    ~UIBlockDelete();

    /// @brief 设置块表
    /// @param [in] blockTable 块表指针
    void setBlockTable(DmBlockTable* blockTable);

    /// @brief 设置文档
    /// @param [in] doc 文档指针
    void setDocument(DmDocument* doc);

private slots:
    /// @brief 块选择变化槽
    /// @param [in] curBlock 当前选中的块名
    void slotBlockSelectionChanged(const QString& curBlock);

    /// @brief 删除按钮点击槽
    void slotDeleteClicked();

private:
    /// @brief 刷新块列表
    void updateBlockList();

    /// @brief 检查块是否有引用
    /// @param [in] blockName 块名
    /// @return 存在引用返回 true，否则返回 false
    bool hasReferences(const QString& blockName) const;

private:
    DmBlockTable*       m_pBlockTable;          ///< 块表指针
    DmDocument*         m_pDocument;            ///< 文档指针
    DmEntityContainer*  m_pPreviewContainer;    ///< 预览用空容器，由本类负责释放
};

#endif // UIBLOCKDELETE_H
