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

/// @file UIDlgTextStyle.cpp
/// @brief 文字样式管理对话框

#include "UIDlgTextStyle.h"

#include <QPushButton>
#include <QTextCodec>
#include <QTextStream>
#include <QFileDialog>
#include <QAction>
#include "DmSystem.h"
#include "DmSettings.h"
#include "DmFont.h"
#include "DmDocument.h"
#include "Math2d.h"
#include "DmFontList.h"
#include <QMessageBox>
#include "Transaction.h"

// CheckBox 勾选状态对应的 stateChanged 值
constexpr int CHECKBOX_STATE_CHECKED = 2;

// 角度限制常量
constexpr double MAX_SLASH_ANGLE = 85.0;
constexpr double MIN_SLASH_ANGLE = -85.0;

UIDlgTextStyle::UIDlgTextStyle(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
    , ui(new Ui::UIDlgTextStyle())
    , m_pDocument(nullptr)
    , m_pPreview(nullptr)
    , m_bIsUpdatingToUI(true)
    , m_isChanged(false)
    , m_pTempTextStyle(nullptr)
{
    setModal(modal);
    ui->setupUi(this);
    ui->buttonBox->clear();
    QPushButton* applyBtn = ui->buttonBox->addButton(QDialogButtonBox::Apply);
    QPushButton* closeBtn = ui->buttonBox->addButton(QDialogButtonBox::Close);
    ui->buttonBox->addButton(applyBtn, QDialogButtonBox::ApplyRole);
    ui->buttonBox->addButton(closeBtn, QDialogButtonBox::RejectRole);
    ui->buttonBox->setOrientation(Qt::Horizontal);
    connect(ui->buttonBox, &QDialogButtonBox::clicked, [=](QAbstractButton* button){
        switch (ui->buttonBox->buttonRole(button))
        {
            case QDialogButtonBox::ApplyRole:
                this->apply();
                break;
            case QDialogButtonBox::RejectRole:
                this->close();
                break;
            default:
                break;
        }
    });

    connect(ui->lswTextStyles, SIGNAL(activeStyleChanged(QString)), this, SLOT(slotLswTextStylesActiveChanged(QString)));
    connect(ui->lswTextStyles, SIGNAL(selectedStyleChanged()), this, SLOT(slotSelectedStyleChanged()));
    connect(ui->lswTextStyles, SIGNAL(selectedStyleChanging(bool&)), this, SLOT(slotSelectedStyleChanging(bool&)));
    connect(ui->btnActive, &QAbstractButton::clicked, this, &UIDlgTextStyle::slotBtnActivateClick);
    connect(ui->btnNew, &QAbstractButton::clicked, this, &UIDlgTextStyle::slotBtnNewClick);
    connect(ui->btnRename, &QAbstractButton::clicked, this, &UIDlgTextStyle::slotBtnRenameClick);
    connect(ui->btnDelete, &QAbstractButton::clicked, this, &UIDlgTextStyle::slotBtnDeleteClick);
    connect(ui->chkReverse, SIGNAL(stateChanged(int)), this, SLOT(slotChkReverseChanged(int)));
    connect(ui->chkUpsidedown, SIGNAL(stateChanged(int)), this, SLOT(slotChkUpsidedownChanged(int)));
    connect(ui->chkVertical, SIGNAL(stateChanged(int)), this, SLOT(slotChkVerticalChanged(int)));
    connect(ui->leDefaultHeight, SIGNAL(textChanged(QString)), this, SLOT(slotTxtDefaultChanged(QString)));
    connect(ui->leObliqueAngle, SIGNAL(textChanged(QString)), this, SLOT(slotTxtSlashAngleChanged(QString)));
    connect(ui->leWidthFactor, SIGNAL(textChanged(QString)), this, SLOT(slotTxtWidthFactorChanged(QString)));
    connect(ui->chkUseBigFont, SIGNAL(stateChanged(int)), this, SLOT(slotChkUseBigFontActived(int)));
    connect(ui->cboFont, SIGNAL(fontChanged(QString)), this, SLOT(slotCboFontChanged(QString)));
    connect(ui->cboBigFont, SIGNAL(fontChanged(QString)), this, SLOT(slotCboBigFontChanged(QString)));

    // 初始化字体列表。读取所有字体文件的头
    DMFONTLIST->readAllFontFiles();
}

/// @brief 销毁对象并释放资源
UIDlgTextStyle::~UIDlgTextStyle()
{
    if (nullptr != m_pPreview)
    {
        delete m_pPreview;
        m_pPreview = nullptr;
    }
}

void UIDlgTextStyle::setStyleList(DmTextStyleTable* textStyleTable, DmDocument* document)
{
    assert(textStyleTable != nullptr);
    m_pTextStyleTable = textStyleTable;
    m_pDocument = document;
    init();
}

void UIDlgTextStyle::updateToOriginStyleList()
{
    // TODO: 字体样式更新到文档的逻辑待实现
}

void UIDlgTextStyle::slotLswTextStylesActiveChanged(QString style)
{
    ui->lblCurTextStyle->setText(style);
}

void UIDlgTextStyle::slotSelectedStyleChanged()
{
    initTempData();
    initFirstFontBox();
    updateUI();
}

void UIDlgTextStyle::slotSelectedStyleChanging(bool& canChange)
{
    askSaveCurrrentStyle();
    canChange = true;
}

void UIDlgTextStyle::slotBtnNewClick()
{
    askSaveCurrrentStyle();
    ui->lswTextStyles->getActionNew()->trigger();
}

void UIDlgTextStyle::slotBtnActivateClick()
{
    ui->lswTextStyles->getActionActivate()->trigger();
}

void UIDlgTextStyle::slotBtnRenameClick()
{
    ui->lswTextStyles->getActionRename()->trigger();
}

void UIDlgTextStyle::slotBtnDeleteClick()
{
    ui->lswTextStyles->getActionDelete()->trigger();
}

void UIDlgTextStyle::slotCboFontChanged(QString text)
{
    if (!ui->lswTextStyles->isLoaded())
    {
        return;
    }
    if (m_bIsUpdatingToUI)
    {
        return;
    }
    DmTextStyleData data = *m_data;
    bool selectInvalid = false; // 是否选择了无效字体
    bool isOriginSysFont = data.isSystemFont;

    // 选择了无效字体
    if (!ui->lswTextStyles->selectedStyle()->isValid())
    {
        if (data.isSystemFont)
        {
            if (data.invalidSysFontFamily == text)
            {
                selectInvalid = true;
            }
        }
        else
        {
            if (data.invalidAsciiFont == text)
            {
                selectInvalid = true;
            }
        }
    }

    if (text.endsWith(SHX_POST))
    {
        if (selectInvalid)
        {
            data.pAsciiFont = nullptr;
        }
        else
        {
            DmFont* font = DMFONTLIST->requestFont(text);
            data.pAsciiFont = font;
        }
        data.isSystemFont = false;
        data.pSysFont = nullptr;
        *m_data = data;
        ui->chkUseBigFont->setChecked(false);
        ui->chkUseBigFont->setEnabled(true);
        if (isOriginSysFont)
        {
            ui->cboBigFont->clearFonts();
            ui->cboBigFont->setEnabled(false);
        }
        ui->lbBigFont->setText(tr("Big font:"));
    }
    else
    {
        data.isSystemFont = true;
        data.pAsciiFont = nullptr;
        if (selectInvalid)
        {
            data.pSysFont = nullptr;
            ui->cboBigFont->clearFonts();
            ui->cboBigFont->setEnabled(false);
        }
        else
        {
            data.sysFontFamily = text;
            *m_data = data;
            addCboSysFontStyleItems(data.sysFontFamily);
            QString type = ui->cboBigFont->itemText(0);
            slotCboBigFontChanged(type);
        }
        ui->chkUseBigFont->setChecked(false);
        ui->chkUseBigFont->setEnabled(false);
        ui->lbBigFont->setText(tr("Font style:"));
    }
    m_pTempTextStyle->setData(*m_data);
    m_isChanged = true;
    updatePreview();
}

void UIDlgTextStyle::slotCboBigFontChanged(QString text)
{
    if (text.isEmpty())
    {
        return;
    }
    if (m_bIsUpdatingToUI)
    {
        return;
    }
    bool selectInvalid = false;
    if (m_pTempTextStyle->isValid())
    {
        // 无效的大字体
        if (m_data->invalidBigFont == text)
        {
            m_data->isUseBigfont = true;
            m_data->pBigFont = nullptr;
            selectInvalid = true;
        }
    }
    if (!selectInvalid)
    {
        // 有效的大字体或"字体样式"
        if (text.endsWith(SHX_POST))
        {
            // 大字体
            m_data->isUseBigfont = true;
            m_data->pBigFont = DMFONTLIST->requestFont(text);
        }
        else
        {
            // 系统字体的"字体样式"
            if (text == DMFONTLIST->getSysFontStyleName(false, false))
            {
                m_data->isSysFontBold = false;
                m_data->isSysFontItalic = false;
            }
            else if (text == DMFONTLIST->getSysFontStyleName(true, false))
            {
                m_data->isSysFontBold = true;
                m_data->isSysFontItalic = false;
            }
            else if (text == DMFONTLIST->getSysFontStyleName(false, true))
            {
                m_data->isSysFontBold = false;
                m_data->isSysFontItalic = true;
            }
            else if (text == DMFONTLIST->getSysFontStyleName(true, true))
            {
                m_data->isSysFontBold = true;
                m_data->isSysFontItalic = true;
            }
            m_data->pSysFont = DMFONTLIST->requestSysFont(m_data->sysFontFamily, m_data->isSysFontBold, m_data->isSysFontItalic);
            if (m_data->pSysFont == nullptr)
            {
                QMessageBox::critical(this, QObject::tr("Tips"), tr("Font:%1 not exist!").arg(m_data->sysFontFamily));
                return;
            }
        }
    }
    m_pTempTextStyle->setData(*m_data);
    m_isChanged = true;
    updatePreview();
}

void UIDlgTextStyle::slotChkUseBigFontActived(int idx)
{
    if (m_bIsUpdatingToUI)
    {
        return;
    }
    if (idx == CHECKBOX_STATE_CHECKED)
    {
        ui->cboBigFont->setEnabled(true);
        initBigFontWidget();
        QString bigFont = ui->cboBigFont->firstValidFont();
        if (!bigFont.isEmpty())
        {
            ui->cboBigFont->setCurrentText(bigFont);
            // 系统默认会选第一个，所以不一定触发TextChanged事件，手动调用一下
            slotCboBigFontChanged(bigFont);
        }
    }
    else
    {
        ui->cboBigFont->clearFonts();
        ui->cboBigFont->setEnabled(false);
        m_data->isUseBigfont = false;
        m_data->pBigFont = nullptr;
    }
    m_pTempTextStyle->setData(*m_data);
    m_isChanged = true;
    updatePreview();
}

void UIDlgTextStyle::slotChkReverseChanged(int idx)
{
    if (idx == CHECKBOX_STATE_CHECKED)
    {
        m_data->isReverseDirection = true;
    }
    else
    {
        m_data->isReverseDirection = false;
    }
    m_pTempTextStyle->setData(*m_data);
    if (!m_bIsUpdatingToUI)
    {
        m_isChanged = true;
    }
    updatePreview();
}

void UIDlgTextStyle::slotChkUpsidedownChanged(int idx)
{
    if (idx == CHECKBOX_STATE_CHECKED)
    {
        m_data->isUpsideDown = true;
    }
    else
    {
        m_data->isUpsideDown = false;
    }
    m_pTempTextStyle->setData(*m_data);
    if (!m_bIsUpdatingToUI)
    {
        m_isChanged = true;
    }
    updatePreview();
}

void UIDlgTextStyle::slotChkVerticalChanged(int idx)
{
    if (idx == CHECKBOX_STATE_CHECKED)
    {
        m_data->isVertical = true;
    }
    else
    {
        m_data->isVertical = false;
    }
    m_pTempTextStyle->setData(*m_data);
    if (!m_bIsUpdatingToUI)
    {
        m_isChanged = true;
    }
    updatePreview();
}

void UIDlgTextStyle::slotTxtDefaultChanged(QString text)
{
    double d = text.toDouble();
    m_data->defaultHeight = d;
    m_pTempTextStyle->setData(*m_data);
    if (!m_bIsUpdatingToUI)
    {
        m_isChanged = true;
    }
}

void UIDlgTextStyle::slotTxtWidthFactorChanged(QString text)
{
    double d = text.toDouble();
    m_data->widhFactor = d;
    m_pTempTextStyle->setData(*m_data);
    if (!m_bIsUpdatingToUI)
    {
        m_isChanged = true;
    }
    updatePreview();
}

void UIDlgTextStyle::slotTxtSlashAngleChanged(QString text)
{
    double d = text.toDouble();
    if (round(d) > MAX_SLASH_ANGLE || round(d) < MIN_SLASH_ANGLE)
    {
        QMessageBox::information(this, tr("Tips"), tr("Invalid input!"), QMessageBox::Close);
        ui->leObliqueAngle->setText("0");
        return;
    }
    double d_r = Math2d::round(d);
    m_data->slashAngle = Math2d::deg2rad(d_r);
    m_pTempTextStyle->setData(*m_data);
    if (!m_bIsUpdatingToUI)
    {
        m_isChanged = true;
    }
    updatePreview();
}

void UIDlgTextStyle::showEvent(QShowEvent* e)
{
    QDialog::showEvent(e);
    ui->preview->zoomAuto();
    updatePreview();
}

void UIDlgTextStyle::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
    ui->preview->zoomAuto();
    updatePreview();
}

/// @brief 使用当前语言设置子控件的字符串
void UIDlgTextStyle::languageChange()
{
    ui->retranslateUi(this);
}

void UIDlgTextStyle::close()
{
    QDialog::close();
}

void UIDlgTextStyle::apply()
{
    if (m_isChanged)
    {
        saveCurrent();
        m_isChanged = false;
    }
}

void UIDlgTextStyle::initBigFontWidget()
{
    std::vector<std::pair<FontIconType, QString>> bigFonts;
    ui->cboBigFont->clearFonts();
    for (auto const& font : *DMFONTLIST)
    {
        if (font->getFontType() == FontType::ShxBigFont)
        {
            bigFonts.emplace_back(std::make_pair(FontIconType::Shx, font->getFileName()));
        }
    }
    if (!m_data->invalidBigFont.isEmpty())
    {
        bigFonts.emplace_back(std::make_pair(FontIconType::NotExist, m_data->invalidBigFont));
        std::sort(bigFonts.begin(), bigFonts.end(), [](const std::pair<FontIconType, QString>& item1, const std::pair<FontIconType, QString>& item2)
        {
            return item1.second < item2.second;
        });
    }
    ui->cboBigFont->addFonts(bigFonts);
}

void UIDlgTextStyle::init()
{
    ui->lswTextStyles->setStyleList(m_pTextStyleTable, m_pDocument);
    initTempData();
    updateUI();

    // 初始化预览
    m_pPreview = new DmEntityContainer();
    ui->preview->setContainer(m_pPreview);
}

void UIDlgTextStyle::initFirstFontBox()
{
    ui->cboFont->clearFonts();
    std::vector<std::pair<FontIconType, QString>> shxFonts;
    std::vector<std::pair<FontIconType, QString>> sysFonts;
    std::vector<std::pair<FontIconType, QString>> allFonts;

    // 先添加shx字体
    for (auto const& font : *DMFONTLIST)
    {
        if (font->getFontType() == FontType::ShxBigFont)
        {
            continue;
        }
        if (font->getFontType() == FontType::ShxASCII || font->getFontType() == FontType::ShxUnifont)
        {
            shxFonts.emplace_back(std::make_pair(FontIconType::Shx, font->getFileName()));
        }
    }

    // 再添加系统字体族
    for (auto kv : DMFONTLIST->getSysFontsMapConstRef())
    {
        QString family = kv.first;
        sysFonts.emplace_back(std::make_pair(FontIconType::SystemFont, family));
    }

    // 如果有不存在字体，标记它
    QString invalidFont;
    bool isInvalidSys = false;
    DmTextStyle* pSelectedStyle = ui->lswTextStyles->selectedStyle();
    auto sortFunc = [](const std::pair<FontIconType, QString>& item1, const std::pair<FontIconType, QString>& item2)
    {
        return item1.second < item2.second;
    };
    if (!pSelectedStyle->isValid())
    {
        auto data = pSelectedStyle->getData();
        if (!data.invalidAsciiFont.isEmpty())
        {
            invalidFont = data.invalidAsciiFont;
            isInvalidSys = false;
            shxFonts.emplace_back(std::make_pair(FontIconType::NotExist, invalidFont));
            std::sort(shxFonts.begin(), shxFonts.end(), sortFunc);
        }
        else if (!data.invalidSysFontFamily.isEmpty())
        {
            invalidFont = data.invalidSysFontFamily;
            isInvalidSys = true;
            sysFonts.emplace_back(std::make_pair(FontIconType::NotExist, invalidFont));
            std::sort(sysFonts.begin(), sysFonts.end(), sortFunc);
        }
    }

    // 添加所有字体，包括不存在字体
    allFonts.reserve(shxFonts.size() + sysFonts.size());
    allFonts.insert(allFonts.end(), shxFonts.begin(), shxFonts.end());
    allFonts.insert(allFonts.end(), sysFonts.begin(), sysFonts.end());
    ui->cboFont->addFonts(allFonts);
}

void UIDlgTextStyle::updateUI()
{
    if (!ui->lswTextStyles->isLoaded())
    {
        return;
    }
    m_bIsUpdatingToUI = true;

    // 设置字体
    if (m_data->isSystemFont)
    {
        ui->lbBigFont->setText(tr("Font style:"));
        if (m_data->pSysFont != nullptr)
        {
            ui->cboFont->setCurrentText(m_data->sysFontFamily);
            addCboSysFontStyleItems(m_data->sysFontFamily);
            updateCboSysFontStyle(*m_data);
        }
        else if (!m_data->invalidSysFontFamily.isEmpty())
        {
            ui->cboFont->setCurrentText(m_data->invalidSysFontFamily);
            ui->cboBigFont->clearFonts();
            ui->cboBigFont->setEnabled(false);
        }
        ui->chkUseBigFont->setEnabled(false);
        ui->chkUseBigFont->setChecked(false);
    }
    else if (m_data->isUseBigfont)
    {
        ui->lbBigFont->setText(tr("Big font:"));
        ui->cboBigFont->setEnabled(true);
        if (m_data->pAsciiFont != nullptr)
        {
            ui->cboFont->setCurrentText(m_data->pAsciiFont->getFileName());
        }
        else if (!m_data->invalidAsciiFont.isEmpty())
        {
            ui->cboFont->setCurrentText(m_data->invalidAsciiFont);
        }
        ui->chkUseBigFont->setEnabled(true);
        ui->chkUseBigFont->setChecked(true);
        initBigFontWidget();
        if (m_data->pBigFont != nullptr)
        {
            ui->cboBigFont->setCurrentText(m_data->pBigFont->getFileName());
        }
        else if (!m_data->invalidBigFont.isEmpty())
        {
            ui->cboBigFont->setCurrentText(m_data->invalidBigFont);
        }
    }
    else
    {
        ui->lbBigFont->setText(tr("Big font:"));
        if (m_data->pAsciiFont != nullptr)
        {
            ui->cboFont->setCurrentText(m_data->pAsciiFont->getFileName());
        }
        else if (!m_data->invalidAsciiFont.isEmpty())
        {
            ui->cboFont->setCurrentText(m_data->invalidAsciiFont);
        }
        ui->cboBigFont->clearFonts();
        ui->cboBigFont->setEnabled(false);
        ui->chkUseBigFont->setEnabled(true);
        ui->chkUseBigFont->setChecked(false);
    }

    // 按钮
    ui->btnRename->setEnabled(ui->lswTextStyles->canSelectedRename());
    ui->btnDelete->setEnabled(ui->lswTextStyles->canSelectedDelete());

    // 其他信息
    ui->chkVertical->setChecked(m_data->isVertical);
    ui->chkUpsidedown->setChecked(m_data->isUpsideDown);
    ui->chkReverse->setChecked(m_data->isReverseDirection);
    ui->leDefaultHeight->setText(QString("%0").arg(m_data->defaultHeight));
    ui->leObliqueAngle->setText(QString("%0").arg(Math2d::rad2deg(m_data->slashAngle)));
    ui->leWidthFactor->setText(QString("%0").arg(m_data->widhFactor));
    ui->lblCurTextStyle->setText(ui->lswTextStyles->activeStyle()->getName());
    updatePreview();
    m_bIsUpdatingToUI = false;
}

void UIDlgTextStyle::addCboSysFontStyleItems(const QString& sysFontFamily)
{
    auto& map = DMFONTLIST->getSysFontsMapConstRef();
    auto it = map.find(sysFontFamily);
    QStringList types;
    if (it == map.cend())
    {
        types.append(DMFONTLIST->getSysFontStyleName(false, false));
    }
    else
    {
        if (it->second.at(0))
        {
            types.append(DMFONTLIST->getSysFontStyleName(false, false));
        }
        if (it->second.at(1))
        {
            types.append(DMFONTLIST->getSysFontStyleName(false, true));
        }
        if (it->second.at(2))
        {
            types.append(DMFONTLIST->getSysFontStyleName(true, false));
        }
        if (it->second.at(3))
        {
            types.append(DMFONTLIST->getSysFontStyleName(true, true));
        }
    }
    ui->cboBigFont->setEnabled(true);
    ui->cboBigFont->clearFonts();
    std::vector<std::pair<FontIconType, QString>> fonts;
    for (auto type : types)
    {
        fonts.emplace_back(std::make_pair(FontIconType::None, type));
    }
    ui->cboBigFont->addFonts(fonts);
}

void UIDlgTextStyle::updateCboSysFontStyle(const DmTextStyleData& data)
{
    QString type = DMFONTLIST->getSysFontStyleName(data.isSysFontBold, data.isSysFontItalic);
    int count = ui->cboBigFont->count();
    QStringList types;
    for (int i = 0; i < count; i++)
    {
        types.append(ui->cboBigFont->itemText(i));
    }
    if (types.contains(type))
    {
        ui->cboBigFont->setCurrentText(type);
    }
    else
    {
        ui->cboBigFont->setCurrentIndex(0);
    }
}

void UIDlgTextStyle::updatePreview()
{
    if (m_pPreview == nullptr)
    {
        return;
    }

    if (!ui->preview->initialized())
    {
        return;
    }

    m_pTempTextStyle->getPreview(m_pPreview);
    ui->preview->specifyModified();
    ui->preview->zoomAuto();
}

void UIDlgTextStyle::initTempData()
{
    if (!ui->lswTextStyles->isLoaded())
    {
        return;
    }
    DmTextStyleData data = ui->lswTextStyles->selectedStyle()->getData();
    m_data.reset(new DmTextStyleData(data));
    m_pTempTextStyle.reset(new DmTextStyle(*m_data));
}

void UIDlgTextStyle::askSaveCurrrentStyle()
{
    if (m_isChanged)
    {
        QMessageBox::StandardButton btn = QMessageBox::information(this, tr("Tips"), tr("Current style has changed, save or not?"), QMessageBox::Yes | QMessageBox::No);
        if (btn == QMessageBox::Yes)
        {
            saveCurrent();
        }
        m_isChanged = false;
    }
}

void UIDlgTextStyle::saveCurrent()
{
    Transaction t(tr("Save TextStyle").toStdString(), m_pDocument);
    t.start();
    auto style = ui->lswTextStyles->selectedStyle();
    m_pDocument->getTextStyleTable()->startModify(style);
    style->setData(*m_data);
    t.commit();
}
