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

/// @file UIDlgDimensionStyleMgr.h
/// @brief 标注样式管理器对话框

#ifndef UIDLGDIMENSIONSTYLEMGR_H
#define UIDLGDIMENSIONSTYLEMGR_H

#include "ui_UIDlgDimensionStyleMgr.h"
#include <memory>
#include "DmDimensionStyleTable.h"

class UIDlgDimensionStyleMgr : public QDialog, public Ui::UIDlgDimensionStyleMgr
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] parent 父窗口指针
    /// @param [in] modal 是否模态
    /// @param [in] fl 窗口标志
    UIDlgDimensionStyleMgr(QWidget* parent = nullptr, bool modal = false, Qt::WindowFlags fl = Qt::WindowFlags());

    /// @brief 析构函数
    ~UIDlgDimensionStyleMgr();

    /// @brief 初始化管理器
    /// @param [in] dimStyleTable 标注样式表指针
    /// @param [in] document 文档指针
    void init(DmDimensionStyleTable* dimStyleTable, DmDocument* document);

protected:
    /// @brief 显示事件
    /// @param [in] e 显示事件
    virtual void showEvent(QShowEvent* e) override;

    /// @brief 大小变化事件
    /// @param [in] event 大小变化事件
    virtual void resizeEvent(QResizeEvent* event) override;

private slots:
    /// @brief 当前样式变化槽
    /// @param [in] style 样式名称
    void slotActiveDimStyleChanged(QString style);

    /// @brief 选中样式变化槽
    void slotSelectedStyleChanged();

    /// @brief 样式变化槽
    void slotStyleChanged();

private:
    /// @brief 更新预览
    void updatePreview();

private:
    DmDimensionStyleTable*  m_pDimStyleTable;   ///< 标注样式表指针
    DmDocument*             m_pDocument;        ///< 文档指针
    DmEntityContainer*      m_pPreview;         ///< 预览容器
};

#endif // !UIDLGDIMENSIONSTYLEMGR_H
