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

/// @file UILayerBox.cpp
/// @brief 图层选择下拉框控件，管理和切换CAD文档中的图层

#include "UILayerBox.h"

#include "DmLayer.h"
#include "DmLayerTable.h"

// Default Constructor. You must call init manually before using this class.
UILayerBox::UILayerBox(QWidget* parent)
    : QComboBox(parent)
{
    m_isShowByBlock = false;
    m_pLayerTable = nullptr;
    m_pCurrentLayer = nullptr;
}

UILayerBox::~UILayerBox()
{
}

/// Initialisation (called manually only once).
/// @param m_pLayerTable Layer list which provides the layer names that are available.
/// @param m_isShowByBlock true: Show attribute ByBlock.
void UILayerBox::init(DmLayerTable& m_pLayerTable, bool m_isShowByBlock)
{
    this->m_isShowByBlock = m_isShowByBlock;
    this->m_pLayerTable = &m_pLayerTable;

    for (auto it = m_pLayerTable.begin(); it != m_pLayerTable.end(); ++it)
    {
        DmLayer* lay = *it;
        if (lay && (lay->getName() != "ByBlock" || m_isShowByBlock))
        {
            addItem(lay->getName());
        }
    }

    connect(this, SIGNAL(activated(int)), this, SLOT(slotLayerChanged(int)));

    setCurrentIndex(0);

    slotLayerChanged(currentIndex());
}

DmLayer* UILayerBox::getLayer()
{
    return m_pCurrentLayer;
}

// Sets the layer shown in the combobox to the given layer.
void UILayerBox::setLayer(DmLayer& layer)
{
    m_pCurrentLayer = &layer;

    int i = findText(layer.getName());
    setCurrentIndex(i);

    slotLayerChanged(currentIndex());
}

// Sets the layer shown in the combobox to the given layer.
void UILayerBox::setLayer(QString& layer)
{
    int i = findText(layer);
    setCurrentIndex(i);

    slotLayerChanged(currentIndex());
}

// Called when the color has changed. This method
// sets the current color to the value chosen or even
// offers a dialog to the user that allows him/ her to
// choose an individual color.
void UILayerBox::slotLayerChanged(int index)
{
    m_pCurrentLayer = m_pLayerTable->find(itemText(index));

    emit layerChanged(m_pCurrentLayer);
}
