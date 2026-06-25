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

/// @file UIDlgTextStyle.h
/// @brief 文字样式管理对话框

#ifndef UIDLGTEXTSTYLE_H
#define UIDLGTEXTSTYLE_H

#include "ui_UIDlgTextStyle.h"
#include "DmTextStyle.h"
#include <memory>

class DmDocument;
namespace Ui {
    class UIDlgTextStyle;
}

class DmTextStyleTable;
class UIDlgTextStyle : public QDialog
{
    Q_OBJECT

public:
    /// @brief 构造函数
    /// @param [in] parent 父窗口指针
    /// @param [in] modal 是否为模态对话框
    /// @param [in] fl 窗口标志
    UIDlgTextStyle(QWidget* parent = nullptr, bool modal = false, Qt::WindowFlags fl = Qt::WindowFlags());

    ~UIDlgTextStyle();

public:
    /// @brief 设置文字样式列表
    /// @param [in] textStyleTable 文字样式表指针
    /// @param [in] document 文档指针
    void setStyleList(DmTextStyleTable* textStyleTable, DmDocument* document);

    void updateToOriginStyleList();

public slots:
    virtual void slotLswTextStylesActiveChanged(QString style);
    virtual void slotSelectedStyleChanged();
    void slotSelectedStyleChanging(bool& canChange);
    void slotBtnNewClick();
    void slotBtnActivateClick();
    void slotBtnRenameClick();
    void slotBtnDeleteClick();

    virtual void slotCboFontChanged(QString);
    virtual void slotCboBigFontChanged(QString);
    virtual void slotChkUseBigFontActived(int);
    virtual void slotChkReverseChanged(int);
    virtual void slotChkUpsidedownChanged(int);
    virtual void slotChkVerticalChanged(int);
    virtual void slotTxtDefaultChanged(QString);
    virtual void slotTxtWidthFactorChanged(QString);
    virtual void slotTxtSlashAngleChanged(QString);

protected:
    virtual void showEvent(QShowEvent* e) override;
    virtual void resizeEvent(QResizeEvent* event) override;

protected slots:
    virtual void languageChange();

public slots:
    void close();
    void apply();

private:
    /// @brief 添加字体到大字体列表
    void initBigFontWidget();

    void init();
    void initTempData();
    void askSaveCurrrentStyle();
    void saveCurrent();

    /// @brief 添加字体到字体列表
    void initFirstFontBox();

    /// @brief 更新当前选择文字样式到UI
    void updateUI();

    /// @brief 查找系统字体，将可用的字体样式添加到字体样式combobox
    /// @param [in] sysFontFamily 系统字体系列名称
    void addCboSysFontStyleItems(const QString& sysFontFamily);

    /// @brief 由信息更新字体样式combobox
    /// @param [in] data 文字样式数据
    void updateCboSysFontStyle(const DmTextStyleData& data);

    void updatePreview();

private:
    std::unique_ptr<Ui::UIDlgTextStyle> ui;

    DmTextStyleTable* m_pTextStyleTable = nullptr;                    ///< 文字样式表
    std::shared_ptr<DmTextStyle> m_pTempTextStyle;                    ///< 临时的文字样式，m_data是它的数据，用它创建预览
    std::shared_ptr<DmTextStyleData> m_data;                          ///< UI选项改变时，更新到此
    DmDocument* m_pDocument = nullptr;                                ///< 关联的文档
    DmEntityContainer* m_pPreview = nullptr;                          ///< 预览容器
    bool m_isChanged = false;                                         ///< 当前选择的文字样式是否已修改
    bool m_bIsUpdatingToUI = true;                                    ///< 当前是否更新到UI，为true时一些槽函数直接返回
};

#endif // UIDLGTEXTSTYLE_H
