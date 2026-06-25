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

/// @file UIBlockSaveAs.h
/// @brief 块另存为对话框

#ifndef UIBLOCKSAVEAS_H
#define UIBLOCKSAVEAS_H

#include "ui_UIBlockSaveAs.h"
#include <memory>

#include <QComboBox>

class DmBlockTable;
class UIActionHandler;

/// @class ModelComboBox
/// @brief 块选择下拉框，支持文本变化信号
class ModelComboBox : public QComboBox
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] parent 父窗口指针
    ModelComboBox(QWidget* parent = nullptr);

    /// @brief 析构函数
    ~ModelComboBox() = default;

public slots:
    /// @brief 文本变化槽
    /// @param [in] name 当前文本
    void slotsCurrentTextChanged(const QString& name);

signals:
    /// @brief 文本变化信号
    void currentTextChanged();
};

/// @class UIBlockSaveAs
/// @brief 块另存为对话框
class UIBlockSaveAs : public QDialog, public Ui::UIBlockSaveAs
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] pActionHandler Action 处理器指针
    /// @param [in] parent 父窗口指针
    /// @param [in] modal 是否模态
    /// @param [in] fl 窗口标志
    UIBlockSaveAs(UIActionHandler* pActionHandler, QWidget* parent = nullptr, bool modal = false, Qt::WindowFlags fl = Qt::WindowFlags());

    /// @brief 析构函数
    ~UIBlockSaveAs();

    /// @brief 设置块表
    /// @param [in] blockList 块表指针
    void setBlockList(DmBlockTable* blockList);

    /// @brief 下拉框文本变化槽
    void slotComBoxTextChanged();

private:
    /// @brief 更新预览
    void updatePreview();

    /// @brief 刷新块列表
    void updateBlockList();

    /// @brief 执行另存为操作
    void saveAs();

private:
    ModelComboBox*      m_pBlockComboBox;   ///< 块选择下拉框
    DmBlockTable*       m_pBlockList;       ///< 块表指针
    DmEntityContainer*  m_pPreview;         ///< 预览容器
    UIActionHandler*    m_pActionHandler;   ///< Action 处理器指针
};

#endif // UIBLOCKSAVEAS_H
