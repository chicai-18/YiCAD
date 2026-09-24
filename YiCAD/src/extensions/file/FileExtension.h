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

/// @file FileExtension.h
/// @brief 文件扩展（src/extensions/file/）的入口。
///
/// 业务工具化第三步第⑤批（doc/COMMAND_TOOL_MIGRATION_PLAN.md 5.2 节）：新建、打开、
/// 保存、另存为、导出图片五条即时命令与"文件"类目里的按钮。图纸的新建、读写与导出
/// 本身仍由宿主的图纸标签页（UITabDrawWidget）完成，命令经
/// IExtensionContext::tabDrawWidget() 调用它。宿主的快速访问栏、Ctrl+N/O/S 与标签栏的
/// "+"按钮也按 ID 启动这里的命令。

#ifndef FILEEXTENSION_H
#define FILEEXTENSION_H

#include "IExtension.h"

class FileExtension : public IExtension
{
public:
    /// @brief 注册文件命令与"文件""导出"面板里的按钮。
    void OnRegister(IExtensionContext& ctx) override;

    std::string_view Id() const override { return "ext.file"; }
};

#endif  // FILEEXTENSION_H
