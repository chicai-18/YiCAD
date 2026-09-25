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

/// @file UIDlgHatch.h
/// @brief 填充图案属性编辑对话框

#ifndef UIDLGHATCH_H
#define UIDLGHATCH_H

#include "ui_UIDlgHatch.h"

class DmHatch;

class UIDlgHatch : public QDialog, public Ui::UIDlgHatch
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] parent 父窗口指针
    /// @param [in] modal 是否为模态对话框
    /// @param [in] fl 窗口标志
    UIDlgHatch(QWidget* parent = nullptr, bool modal = false, Qt::WindowFlags fl = Qt::WindowFlags());

    ~UIDlgHatch();

    /// @brief 保存设置（如果需要）
    void saveSettingIfNeed();

public slots:
    virtual void showEvent(QShowEvent* e);

    /// @brief 设置要编辑的填充实体
    /// @param [in] h 填充实体引用
    /// @param [in] isNew 是否新建
    virtual void setHatch(DmHatch& h, bool isNew);

    /// @brief 更新填充实体以匹配用户修改
    virtual void updateHatch();

    /// @brief 设置填充模式
    /// @param [in] p 模式名称
    virtual void setPattern(const QString& p);

    virtual void resizeEvent(QResizeEvent*);
    virtual void updatePreview();
    virtual void reject();
    virtual void accept();

protected slots:
    virtual void languageChange();

private:
    void init();

private:
    DmEntityContainer* m_pPreview = nullptr; ///< 预览容器
    bool m_isNew = false;                    ///< 是否新建
    DmPattern* m_pPattern = nullptr;         ///< 填充模式
    DmHatch* m_pHatch = nullptr;             ///< 待编辑的填充实体指针
    bool m_saveSettings = true;              ///< 是否保存设置
};

#endif // UIDLGHATCH_H
