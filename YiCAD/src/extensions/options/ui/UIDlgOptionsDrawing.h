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

/// @file UIDlgOptionsDrawing.h
/// @brief 绘图选项设置对话框

#ifndef UIDLGOPTIONSDRAWING_H
#define UIDLGOPTIONSDRAWING_H

#include <memory>
#include <QGraphicsScene>
#include "ui_UIDlgOptionsDrawing.h"

class DmDocument;
class DmVector;

class UIDlgOptionsDrawing : public QDialog, public Ui::UIDlgOptionsDrawing
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] parent 父窗口指针
    /// @param [in] modal 是否为模态对话框
    /// @param [in] fl 窗口标志
    UIDlgOptionsDrawing(QWidget* parent = nullptr, bool modal = false, Qt::WindowFlags fl = Qt::WindowFlags());

    ~UIDlgOptionsDrawing();

public slots:
    /// @brief 设置关联的文档对象
    /// @param [in] g 文档指针
    virtual void setDocument(DmDocument* g);

    /// @brief 验证并保存设置
    virtual void validate();

    virtual void updateLengthPrecision();
    virtual void updateAnglePrecision();
    virtual void updatePreview();
    virtual void updateUnitLabels();

protected slots:
    virtual void languageChange();

private slots:
    void on_tabWidget_currentChanged(int index);

private:
    /// @brief 更新长度精度下拉框
    /// @param [in] u 格式下拉框
    /// @param [in,out] l 精度下拉框
    void updateCBLengthPrecision(QComboBox* u, QComboBox* l);

    /// @brief 更新角度精度下拉框
    /// @param [in] u 格式下拉框
    /// @param [in,out] p 精度下拉框
    void updateCBAnglePrecision(QComboBox* u, QComboBox* p);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void init();

private:
    QStringList listPrec1;            ///< 精度选项列表
    DmDocument* document = nullptr;   ///< 关联的文档对象
};

#endif // UIDLGOPTIONSDRAWING_H
