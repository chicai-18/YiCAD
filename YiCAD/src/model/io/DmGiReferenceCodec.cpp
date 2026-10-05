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

/// @file DmGiReferenceCodec.cpp
/// @brief 文档级 GI 引用编解码的实现

#include "DmGiReferenceCodec.h"

#include "DmBlock.h"
#include "DmBlockTable.h"
#include "DmDocument.h"
#include "DmFont.h"
#include "DmFontList.h"
#include "DmLayer.h"
#include "DmLayerTable.h"
#include "DmLineType.h"
#include "DmLineTypeTable.h"

DmGiReferenceCodec::DmGiReferenceCodec(DmDocument& document)
    : m_document(document)
{
}

std::string DmGiReferenceCodec::layerName(const DmLayer* layer) const
{
    return layer ? layer->getName().toStdString() : std::string();
}

const DmLayer* DmGiReferenceCodec::findLayer(const std::string& name) const
{
    return m_document.getLayerTable()->find(QString::fromStdString(name));
}

std::string DmGiReferenceCodec::lineTypeName(const DmLineType* lineType) const
{
    return lineType ? const_cast<DmLineType*>(lineType)->getLineTypeName().toStdString() : std::string();
}

const DmLineType* DmGiReferenceCodec::findLineType(const std::string& name) const
{
    return m_document.getLineTypeTable()->find(QString::fromStdString(name));
}

std::string DmGiReferenceCodec::drawableName(const IGiDrawable* drawable) const
{
    const auto* block = dynamic_cast<const DmBlock*>(drawable);
    return block ? block->getName().toStdString() : std::string();
}

const IGiDrawable* DmGiReferenceCodec::findDrawable(const std::string& name) const
{
    return m_document.getBlockTable()->find(QString::fromStdString(name));
}

std::string DmGiReferenceCodec::fontName(const IGiFont* font) const
{
    const auto* dmFont = dynamic_cast<const DmFont*>(font);
    return dmFont ? dmFont->getFileName().toStdString() : std::string();
}

const IGiFont* DmGiReferenceCodec::findFont(const std::string& name) const
{
    // 找不到时不退回缺省字体：读回为空，这串字形不画，而不是换一种字体画
    return DmFontList::instance()->requestFont(QString::fromStdString(name), false);
}

std::string DmGiReferenceCodec::imageName(const QImage* /*image*/) const
{
    return std::string();
}

const QImage* DmGiReferenceCodec::findImage(const std::string& /*name*/) const
{
    return nullptr;
}
