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

/// @file UIBlockDelete.cpp
/// @brief 块删除对话框实现

#include "UIBlockDelete.h"

#include <QMessageBox>

#include "DmBlockTable.h"
#include "DmBlock.h"
#include "DmBlockReference.h"
#include "DmDocument.h"
#include "DmEntity.h"
#include "DmEntityContainer.h"
#include "EntityTable.h"
#include "Transaction.h"

UIBlockDelete::UIBlockDelete(QWidget* parent, Qt::WindowFlags fl)
    : QDialog(parent, fl)
    , m_pBlockTable(nullptr)
    , m_pDocument(nullptr)
{
    setupUi(this);

    connect(blockListWidget, &QListWidget::currentTextChanged, this, &UIBlockDelete::slotBlockSelectionChanged);
    connect(deleteButton, &QPushButton::clicked, this, [this]
    {
        slotDeleteClicked();
    });
    connect(closeButton, &QPushButton::clicked, this, [this]
    {
        reject();
    });
}

UIBlockDelete::~UIBlockDelete() = default;

void UIBlockDelete::setBlockTable(DmBlockTable* blockTable)
{
    m_pBlockTable = blockTable;
    updateBlockList();
}

void UIBlockDelete::setDocument(DmDocument* doc, std::shared_ptr<GsModel> graphics)
{
    m_pDocument = doc;
    m_graphics = std::move(graphics);
}

void UIBlockDelete::updateBlockList()
{
    blockListWidget->clear();
    if (!m_pBlockTable)
    {
        return;
    }

    for (auto block : *m_pBlockTable)
    {
        if (!block || block->isErased())
        {
            continue;
        }

        // 过滤匿名块
        std::wstring::size_type idx = block->getName().toStdWString().find(L"*");
        if (idx != std::wstring::npos)
        {
            continue;
        }

        // 过滤内部块
        auto strChar = block->getName().toStdWString().substr(0, 1);
        if (strChar == L"_")
        {
            continue;
        }

        blockListWidget->addItem(block->getName());
    }
    if (blockListWidget->count() > 0)
    {
        blockListWidget->setCurrentRow(0);
    }
}

void UIBlockDelete::slotBlockSelectionChanged(const QString& curBlock)
{
    if (!m_pBlockTable)
    {
        return;
    }

    // 直接画文档图形模型里的块几何（RENDER_PLAN.md 第 4.8 步）；没有这个块时什么也不画
    preview->setBlock(m_graphics, m_pBlockTable->find(curBlock));
    preview->zoomAuto();
}

void UIBlockDelete::slotDeleteClicked()
{
    if (!m_pBlockTable || !m_pDocument)
    {
        return;
    }

    auto items = blockListWidget->selectedItems();
    if (items.isEmpty())
    {
        QMessageBox::information(this, tr("Tips"), tr("Please select a block to delete."));
        return;
    }

    QString blockName = items.first()->text();
    auto block = m_pBlockTable->find(blockName);
    if (!block)
    {
        return;
    }

    // 检查是否存在引用该块的 DmBlockReference
    if (hasReferences(blockName))
    {
        QMessageBox::warning(this, tr("Warning"),
            tr("Block \"%1\" has references in the drawing and cannot be deleted.").arg(blockName));
        return;
    }

    // 确认删除
    QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Confirm Delete"),
        tr("Are you sure you want to delete block \"%1\"?").arg(blockName),
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes)
    {
        return;
    }

    Transaction t(tr("Delete Block").toStdString(), m_pDocument);
    t.start();
    m_pBlockTable->remove(block);
    t.commit();

    // 刷新列表并更新预览
    updateBlockList();
    if (blockListWidget->count() > 0)
    {
        blockListWidget->setCurrentRow(0);
    }
    else
    {
        preview->setBlock(m_graphics, nullptr);
    }
}

bool UIBlockDelete::hasReferences(const QString& blockName) const
{
    if (!m_pDocument)
    {
        return false;
    }

    auto entTable = m_pDocument->getEntityTable();
    for (auto e : *entTable)
    {
        if (e && e->getEntityType() == DM::EntityBlockReference)
        {
            DmBlockReference* ref = static_cast<DmBlockReference*>(e);
            if (ref->getName() == blockName && !ref->isErased())
            {
                return true;
            }
        }
    }
    return false;
}
