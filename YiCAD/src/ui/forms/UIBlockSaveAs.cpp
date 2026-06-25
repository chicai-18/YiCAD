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

/// @file UIBlockSaveAs.cpp
/// @brief 块另存为对话框实现

#include "UIBlockSaveAs.h"

#include <QPushButton>
#include <QCloseEvent>
#include <QComboBox>

#include "DmBlockTable.h"
#include "DmBlock.h"
#include "DmBlockReference.h"
#include "UIActionHandler.h"

namespace
{
    constexpr int COMBOBOX_MAX_WIDTH = 130;
    constexpr int COMBOBOX_MAX_HEIGHT = 23;
}

ModelComboBox::ModelComboBox(QWidget* parent)
    : QComboBox(parent)
{
    connect(this, SIGNAL(currentTextChanged(QString)), this, SLOT(slotsCurrentTextChanged(QString)));
}

void ModelComboBox::slotsCurrentTextChanged(const QString& name)
{
    emit currentTextChanged();
}

UIBlockSaveAs::UIBlockSaveAs(UIActionHandler* pActionHandler, QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
    , m_pBlockComboBox(nullptr)
    , m_pBlockList(nullptr)
    , m_pPreview(nullptr)
    , m_pActionHandler(pActionHandler)
{
    setModal(modal);
    setupUi(this);

    QObject::connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Save As"));

    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]
    {
        saveAs();
    });

    m_pPreview = new DmEntityContainer();
    preview->setContainer(m_pPreview);

    m_pBlockComboBox = new ModelComboBox(parent);
    m_pBlockComboBox->setObjectName(QString::fromUtf8("blockComboBox"));
    m_pBlockComboBox->setMaximumSize(QSize(COMBOBOX_MAX_WIDTH, COMBOBOX_MAX_HEIGHT));
    m_pBlockComboBox->setMinimumSize(QSize(COMBOBOX_MAX_WIDTH, COMBOBOX_MAX_HEIGHT));

    horizontalLayout_4->addWidget(m_pBlockComboBox);

    connect(m_pBlockComboBox, &ModelComboBox::currentTextChanged, this, [this]
    {
        slotComBoxTextChanged();
    });
}

UIBlockSaveAs::~UIBlockSaveAs()
{
    if (m_pPreview)
    {
        delete m_pPreview;
        m_pPreview = nullptr;
    }
}

void UIBlockSaveAs::setBlockList(DmBlockTable* blockTable)
{
    m_pBlockList = blockTable;
    updateBlockList();
    updatePreview();
}

void UIBlockSaveAs::updateBlockList()
{
    m_pBlockComboBox->clear();

    for (auto block : *m_pBlockList)
    {
        // 过滤匿名块
        std::wstring::size_type idx = block->getName().toStdWString().find(L"*");
        if (idx != std::wstring::npos)
        {
            continue;
        }

        // 过滤一些autocad内部自动生成的块 todo: 此做法会导致前缀_ 的块不被yicad块保存列表显示
        auto strChar = block->getName().toStdWString().substr(0, 1);
        if (strChar == L"_")
        {
            continue;
        }

        m_pBlockComboBox->addItem(block->getName());
    }

    updatePreview();
}

void UIBlockSaveAs::saveAs()
{
    m_pActionHandler->slotBlocksSave();
    this->close();
}

void UIBlockSaveAs::slotComBoxTextChanged()
{
    auto currentName = m_pBlockComboBox->currentText();
    auto block = m_pBlockList->find(currentName);
    if (block)
    {
        m_pPreview->clear();
        for (auto e : block->getEntityTable())
        {
            m_pPreview->addEntity(e->clone());
        }
        preview->setContainer(m_pPreview);
        preview->zoomAuto();
        preview->update();

        auto origin = block->getBasePoint();
        QString originText = QString::number(origin.x, 'f', 3) + "," + QString::number(origin.y, 'f', 3);
        pointLable->setText(originText);

        m_pBlockList->activate(block);
    }
}

void UIBlockSaveAs::updatePreview()
{
    if (m_pPreview == nullptr)
    {
        return;
    }

    auto currentName = m_pBlockComboBox->currentText();
    auto block = m_pBlockList->find(currentName);
    if (block)
    {
        m_pPreview->clear();
        for (auto e : block->getEntityTable())
        {
            m_pPreview->addEntity(e->clone());
        }
        preview->setContainer(m_pPreview);
        preview->zoomAuto();

        auto origin = block->getBasePoint();
        QString originText = QString::number(origin.x, 'f', 3) + "," + QString::number(origin.y, 'f', 3);
        pointLable->setText(originText);
    }
    else
    {
        preview->setContainer(new DmEntityContainer());
        preview->update();
    }
}
