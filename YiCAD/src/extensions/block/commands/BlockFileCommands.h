/**
 * Copyright (c) 2011-2018 by Andrew Mustun. All rights reserved.
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is part of the YiCAD project.
 *
 * YiCAD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YiCAD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/// @file BlockFileCommands.h
/// @brief 块的删除、保存、另存为、导入四条即时命令的实现，取代原 ActionBlocksDelete、
///        ActionBlocksSave、ActionBlocksSaveAs、ActionBlocksImport

#ifndef BLOCKFILECOMMANDS_H
#define BLOCKFILECOMMANDS_H

#include <memory>

#include <QCoreApplication>

class DmBlockReference;
class DmDocument;
class DmEntity;
class GsModel;
class QWidget;

/// @brief 块的文件类命令；parent 是对话框的父窗口（扩展上下文的主窗口），graphics 是文档的图形模型
///        （对话框里的块预览直接画其中的块几何，可以为空）
class BlockFileCommands
{
    Q_DECLARE_TR_FUNCTIONS(BlockFileCommands)

public:
    /// @brief 弹出删除块对话框（ext.block.delete）
    static void deleteBlocks(DmDocument* doc, std::shared_ptr<GsModel> graphics, QWidget* parent);
    /// @brief 把当前激活的块连同它引用的样式与嵌套块存成文件（ext.block.save）
    static void saveActiveBlock(DmDocument* doc, QWidget* parent);
    /// @brief 弹出块另存为对话框，选好块后按"另存为"执行 saveActiveBlock（ext.block.save_as）
    static void showSaveAs(DmDocument* doc, std::shared_ptr<GsModel> graphics, QWidget* parent);
    /// @brief 从文件导入块定义与它们用到的线型、图层、文字样式、标注样式（ext.block.import）
    static void importBlocks(DmDocument* doc, QWidget* parent);

private:
    /// @brief 递归把块参照引用的块定义加到目标文档
    static void addBlock(DmBlockReference* ref, DmDocument* doc);
    /// @brief 把源文档的线型、图层、文字样式、标注样式复制到目标文档
    static void copyStyles(DmDocument* src, DmDocument* dst);
    /// @brief 递归把实体及其子实体的图层、线型、文字样式、标注样式指向本文档的同名项
    static void repointEntityStyles(DmDocument* doc, DmEntity* entity);
};

#endif  // BLOCKFILECOMMANDS_H
