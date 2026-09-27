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

/// @file AppDocument.h
/// @brief 一份打开的图纸：持有图纸数据与它的存盘策略
///
/// 与 DmDocument 按层一一对应：DmDocument 是 Model 层的图纸数据，AppDocument 是 Application 层
/// 打开的图纸，对应 AutoCAD 的 AcApDocument。它不是控件，图纸窗口 MDIWindow 持有它、只负责显示
/// （doc/SELECTION_SET_PLAN.md 6.3 节）。

#ifndef APP_DOCUMENT_H
#define APP_DOCUMENT_H

#include <memory>

class DmDocument;
class DocumentFileService;
class IDocumentManager;

/// @brief 一份打开的图纸，见文件说明
class AppDocument
{
public:
    /// @brief 新建一份已初始化的空白文档，并接管它的存盘策略
    /// @param documents 宿主管理的打开图纸，交给文档文件服务取未命名文档的名字；必须比本对象活得久
    explicit AppDocument(const IDocumentManager& documents);

    /// @brief 先释放文档文件服务，再释放文档
    ~AppDocument();

    AppDocument(const AppDocument&) = delete;
    AppDocument& operator=(const AppDocument&) = delete;

    /// @brief 图纸数据
    DmDocument& document() const { return *m_document; }

    /// @brief 文档的存盘策略（自动保存、备份、打开失败的处理）
    DocumentFileService& fileService() const { return *m_fileService; }

private:
    std::unique_ptr<DmDocument>          m_document;     ///< 图纸数据
    std::unique_ptr<DocumentFileService> m_fileService;  ///< 文档的存盘策略；自动保存的定时器会写文档，先于文档释放
};

#endif  // APP_DOCUMENT_H
