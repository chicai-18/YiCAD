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

/// @file UIBlockListWidget.cpp
/// @brief 块列表控件实现

#include "UIBlockListWidget.h"

#include <QScrollBar>
#include <QBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QListWidget>
#include <QScrollArea>

#include <algorithm>

#include "DmBlockTable.h"
#include "UIActionHandler.h"
#include "Debug.h"
#include "GuiPreviewWidget.h"

namespace
{
    constexpr int PREVIEW_PANEL_SIZE = 150;
    constexpr int PREVIEW_WIDGET_WIDTH = 140;
    constexpr int PREVIEW_WIDGET_HEIGHT = 125;
    constexpr int PREVIEW_LABEL_Y_OFFSET = 125;
    constexpr int PANEL_SPACING = 5;
    constexpr int BACK_WIDGET_WIDTH = 300;
    constexpr int BACK_WIDGET_INITIAL_HEIGHT = 600;
    constexpr int COL_COUNT = 2;
}

ModelWidget::ModelWidget(QWidget* parent)
{
}

void ModelWidget::mouseReleaseEvent(QMouseEvent* ev)
{
    emit clicked();
}

UIBlockListWidget::UIBlockListWidget(UIActionHandler* pActionHandler, QWidget* parent, const char* name, Qt::WindowFlags f)
    : QWidget(parent, f)
    , m_pBlockList(nullptr)
    , m_pActionHandler(pActionHandler)
    , m_pBackWidget(new QWidget(parent))
{
}

UIBlockListWidget::~UIBlockListWidget()
{
}

void UIBlockListWidget::setBlockList(DmBlockTable* blockTable)
{
    m_pBlockList = blockTable;
    update();
}

DmBlockTable* UIBlockListWidget::getBlockTable()
{
    return m_pBlockList;
}

void UIBlockListWidget::update()
{
    m_pBackWidget->close();
    m_pBackWidget = new QWidget(this);
    m_pBackWidget->resize(BACK_WIDGET_WIDTH, BACK_WIDGET_INITIAL_HEIGHT);
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidget(m_pBackWidget);
    scrollArea->resize(BACK_WIDGET_WIDTH, BACK_WIDGET_INITIAL_HEIGHT);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 屏蔽滑动条
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->show();
    m_pBackWidget->show();

    int num = 0;
    int blockIdx = 0;
    for (auto block : *m_pBlockList)
    {
        // 过滤匿名块
        std::wstring::size_type idx = block->getName().toStdWString().find(L"*");
        if (idx != std::wstring::npos)
        {
            continue;
        }

        // 过滤以 _ 开头的内部块（如标注箭头块）
        auto strChar = block->getName().toStdWString().substr(0, 1);
        if (strChar == L"_")
        {
            continue;
        }

        ModelWidget* pan = new ModelWidget(m_pBackWidget);
        pan->setParent(m_pBackWidget);
        pan->resize(PREVIEW_PANEL_SIZE, PREVIEW_PANEL_SIZE);
        pan->show();

        if (num % COL_COUNT == 0)
        {
            pan->move(0, (num / COL_COUNT) * PREVIEW_PANEL_SIZE);
        }
        else
        {
            pan->move(PREVIEW_PANEL_SIZE, ((num - 1) / COL_COUNT) * PREVIEW_PANEL_SIZE);
        }

        // 预览框
        GuiPreviewWidget* preview = new GuiPreviewWidget(pan);
        preview->resize(PREVIEW_WIDGET_WIDTH, PREVIEW_WIDGET_HEIGHT);
        preview->move(PANEL_SPACING, 0);
        m_blockIdxMap[blockIdx] = std::make_unique<DmEntityContainer>();
        for (auto e : block->getEntityTable())
        {
            m_blockIdxMap[blockIdx]->addEntity(e->clone());
        }
        preview->setContainer(m_blockIdxMap[blockIdx].get());
        preview->zoomAuto();
        preview->show();

        // 标题
        QLabel* name = new QLabel(block->getName(), pan);
        name->move(PANEL_SPACING, PREVIEW_LABEL_Y_OFFSET);
        name->show();

        connect(pan, &ModelWidget::clicked, this, [this, block]
        {
            m_pBlockList->activate(block);
            m_pActionHandler->slotBlocksInsert();
        });

        num++;
        blockIdx++;
    }

    m_pBackWidget->resize(BACK_WIDGET_WIDTH, ((num + 1) / COL_COUNT) * PREVIEW_PANEL_SIZE);
}
