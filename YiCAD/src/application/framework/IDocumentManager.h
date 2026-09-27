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

/// @file IDocumentManager.h
/// @brief 宿主管理的打开图纸：取当前与全部文档、视图与选择集，新建、打开、保存、导出图纸
///
/// 由壳层实现（委托给图纸标签页），扩展经 IExtensionContext::documentManager() 取得，
/// ui/ 的控件与文档文件服务由宿主注入。使用方因此不认识具体的标签页控件与主窗口
/// （doc/LAYER_RESTRUCTURE_PLAN.md 9.2 节）。
///
/// 文件操作面向用户：打开、另存为、导出会弹出文件对话框，结果与提示都由宿主处理，
/// 没有返回值。返回的文档、视图与选择集归宿主所有，调用方不得保留到图纸关闭之后。

#ifndef IDOCUMENTMANAGER_H
#define IDOCUMENTMANAGER_H

#include <vector>

#include <QString>

class DmDocument;
class GuiDocumentView;
class SelectionSet;

/// @brief 宿主管理的打开图纸，见文件说明
class IDocumentManager
{
public:
    virtual ~IDocumentManager() = default;

    /// @brief 当前活动文档；无打开文档时为 nullptr
    virtual DmDocument* currentDocument() const = 0;

    /// @brief 当前活动文档的视图；无打开文档时为 nullptr
    virtual GuiDocumentView* currentDocumentView() const = 0;

    /// @brief 全部打开的文档，按标签页顺序
    virtual std::vector<DmDocument*> documents() const = 0;

    /// @brief 绘图区域里全部图纸的视图，按子窗口顺序（如改了显示选项后逐个刷新）
    virtual std::vector<GuiDocumentView*> documentViews() const = 0;

    /// @brief 文档的选择集
    /// @param document 文档
    /// @return 不是宿主打开的文档（含空指针）时返回 nullptr
    virtual SelectionSet* selection(const DmDocument* document) const = 0;

    /// @brief 新建一张空白图纸，设为当前
    virtual void newDocument() = 0;

    /// @brief 弹出文件对话框，打开选中的图纸
    virtual void openDocument() = 0;

    /// @brief 保存当前图纸；从未保存过时转为另存为
    virtual void saveDocument() = 0;

    /// @brief 弹出文件对话框，把当前图纸另存为
    virtual void saveDocumentAs() = 0;

    /// @brief 弹出文件对话框，把当前图纸导出为图片
    virtual void exportImage() = 0;

    /// @brief 从未保存过的文档的名字，自动保存副本的文件名用
    /// @param document 文档
    /// @return 文档所在标签页的标题；不是宿主打开的文档时返回空字符串
    virtual QString untitledDocumentName(const DmDocument* document) const = 0;
};

#endif  // IDOCUMENTMANAGER_H
