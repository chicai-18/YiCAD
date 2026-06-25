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

/// @file UIDlgImage.h
/// @brief 图片属性编辑对话框

#ifndef UIDLGIMAGE_H
#define UIDLGIMAGE_H

class DmImage;

#include "ui_UIDlgImage.h"

class UIDlgImage : public QDialog, public Ui::UIDlgImage
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] parent 父窗口指针
    /// @param [in] modal 是否为模态对话框
    /// @param [in] fl 窗口标志
    UIDlgImage(QWidget* parent = nullptr, bool modal = false, Qt::WindowFlags fl = Qt::WindowFlags());

    ~UIDlgImage();

public slots:
    /// @brief 设置要编辑的图片实体
    /// @param [in] e 图片实体引用
    virtual void setImage(DmImage& e);

    virtual void changeWidth();
    virtual void changeHeight();
    virtual void changeScale();
    virtual void changeDPI();

    /// @brief 更新图片实体以匹配用户修改
    virtual void updateImage();

protected slots:
    virtual void languageChange();

private:
    DmImage* m_pImage = nullptr;   ///< 待编辑的图片实体指针
    double m_dScale = 1.0;         ///< 当前缩放比例
    QDoubleValidator* val = nullptr; ///< 输入验证器
};

#endif // UIDLGIMAGE_H
