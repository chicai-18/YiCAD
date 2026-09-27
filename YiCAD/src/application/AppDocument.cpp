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

/// @file AppDocument.cpp
/// @brief 一份打开的图纸的实现

#include "AppDocument.h"

#include "DmDocument.h"
#include "DocumentFileService.h"
#include "SelectionSet.h"

AppDocument::AppDocument(const IDocumentManager& documents)
    : m_document(std::make_unique<DmDocument>())
{
    m_document->initDoc();
    m_fileService = std::make_unique<DocumentFileService>(*m_document, &documents);
    m_selection = std::make_unique<SelectionSet>(*m_document);
}

/// 选择集引用文档，文档文件服务的自动保存定时器会写文档，两者都要在文档之前释放。
/// 显式按顺序释放，不依赖成员的声明顺序。
AppDocument::~AppDocument()
{
    m_selection.reset();
    m_fileService.reset();
    m_document.reset();
}
