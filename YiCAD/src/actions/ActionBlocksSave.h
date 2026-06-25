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


/// @file ActionBlocksSave.h
/// @brief 块保存动作类头文件

#ifndef ACTIONBLOCKSSAVE_H
#define ACTIONBLOCKSSAVE_H

#include "ActionInterface.h"

class DmBlockReference;

/// @brief 保存当前激活的块到文件
class ActionBlocksSave : public ActionInterface
{
    Q_OBJECT
public:
    ActionBlocksSave(DmDocument* doc, GuiDocumentView* docView);

    void init(int status = 0) override;
    void trigger() override;

private:
    /// @brief 递归将块参照引用的块定义添加到目标文档
    void addBlock(DmBlockReference* ref, DmDocument* doc);
    /// @brief 将源文档的线型/图层/文字样式/标注样式复制到目标文档
    void copyStyles(DmDocument* src, DmDocument* dst);
};

#endif
