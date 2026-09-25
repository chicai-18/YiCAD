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

/// @file UIBlockEditOptions.cpp
/// @brief 块在位编辑选项栏实现

#include "UIBlockEditOptions.h"

#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QMessageBox>

#include "BlockEditTool.h"

UIBlockEditOptions::UIBlockEditOptions(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
    , m_session(nullptr)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(8);

    m_label = new QLabel(this);
    m_label->setText(tr("Editing Block:"));

    m_completeButton = new QPushButton(tr("Complete"), this);
    m_completeButton->setToolTip(tr("Complete block editing"));
    connect(m_completeButton, &QPushButton::clicked, this, &UIBlockEditOptions::onCompleteClicked);

    layout->addWidget(m_label);
    layout->addStretch();
    layout->addWidget(m_completeButton);

    setLayout(layout);
}

UIBlockEditOptions::~UIBlockEditOptions() = default;

void UIBlockEditOptions::setSession(BlockEditTool* session)
{
    m_session = session;
    if (m_session)
    {
        m_label->setText(tr("Editing Block: \"%1\"").arg(m_session->blockName()));
    }
}

void UIBlockEditOptions::onCompleteClicked()
{
    if (!m_session)
        return;

    // 如果存在修改，则弹出保存确认提示
    if (m_session->hasModifications())
    {
        int ret = QMessageBox::question(nullptr,
            tr("Block Edit"),
            tr("Save changes to block?"),
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        if (ret == QMessageBox::Yes)
        {
            m_session->completeEditing(true);
        }
        else if (ret == QMessageBox::No)
        {
            m_session->completeEditing(false);
        }
        // 取消：继续编辑
    }
    else
    {
        m_session->completeEditing(false);
    }
}
