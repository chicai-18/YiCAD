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

/// @file UINestedBlockSelectDialog.cpp
/// @brief 嵌套块选择对话框实现

#include "UINestedBlockSelectDialog.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <QLabel>

#include "DmBlock.h"
#include "DmBlockReference.h"
#include "DmBlockTable.h"
#include "DmDocument.h"
#include "DmEntity.h"
#include "DmEntityContainer.h"
#include "GuiPreviewWidget.h"

namespace
{
void appendPreviewEntities(DmEntityContainer* previewContainer, DmEntity* entity)
{
    if (!previewContainer || !entity || entity->isErased())
    {
        return;
    }

    DmEntity* clone = entity->clone();
    clone->setParent(previewContainer);
    clone->setSelected(false);
    clone->setHighlighted(false);
    previewContainer->addEntity(clone);
}
}

UINestedBlockSelectDialog::UINestedBlockSelectDialog(
    DmDocument* doc, const QStringList& blockNames, QWidget* parent)
    : QDialog(parent)
    , m_document(doc)
    , m_previewContainer(new DmEntityContainer())
    , m_blockNames(blockNames)
{
    setWindowTitle(tr("Select Block to Edit"));
    setMinimumSize(500, 350);

    auto* mainLayout = new QVBoxLayout(this);

    auto* contentLayout = new QHBoxLayout();

    // 左侧：块层级列表
    auto* leftLayout = new QVBoxLayout();
    auto* listLabel = new QLabel(tr("Block Levels:"), this);
    leftLayout->addWidget(listLabel);

    m_listWidget = new QListWidget(this);
    for (const QString& name : m_blockNames)
    {
        m_listWidget->addItem(name);
    }
    leftLayout->addWidget(m_listWidget);
    contentLayout->addLayout(leftLayout, 1);

    // 右侧：预览区
    auto* rightLayout = new QVBoxLayout();
    auto* previewLabel = new QLabel(tr("Preview:"), this);
    rightLayout->addWidget(previewLabel);

    m_previewWidget = new GuiPreviewWidget(this);
    m_previewWidget->setMinimumSize(250, 250);
    rightLayout->addWidget(m_previewWidget);
    contentLayout->addLayout(rightLayout, 2);

    mainLayout->addLayout(contentLayout);

    // 底部：操作按钮
    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(m_buttonBox);

    connect(m_listWidget, &QListWidget::currentRowChanged,
            this, &UINestedBlockSelectDialog::onSelectionChanged);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // 默认选中第一项
    if (m_listWidget->count() > 0)
    {
        m_listWidget->setCurrentRow(0);
    }
}

UINestedBlockSelectDialog::~UINestedBlockSelectDialog()
{
    delete m_previewContainer;
}

QString UINestedBlockSelectDialog::selectedBlockName() const
{
    int row = m_listWidget->currentRow();
    if (row >= 0 && row < m_blockNames.size())
        return m_blockNames.at(row);
    return QString();
}

void UINestedBlockSelectDialog::onSelectionChanged(int row)
{
    if (row < 0 || row >= m_blockNames.size())
        return;

    QString blockName = m_blockNames.at(row);
    DmBlockTable* blockTable = m_document->getBlockTable();
    if (!blockTable)
        return;

    DmBlock* block = blockTable->find(blockName);
    if (!block)
        return;

    // 清空并重新填充预览容器
    m_previewContainer->clear();
    for (auto e : block->getEntityTable())
    {
        if (e && !e->isErased())
        {
            appendPreviewEntities(m_previewContainer, e);
        }
    }
    m_previewContainer->forcedCalculateBorders();

    m_previewWidget->setContainer(m_previewContainer);
    m_previewWidget->specifyModified();
    m_previewWidget->zoomAuto();
    m_previewWidget->redraw();
}
