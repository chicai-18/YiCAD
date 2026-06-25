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

/// @file PreviewActionInterface.h
/// @brief 需要预览功能的操作类接口，管理预览的绘制和删除

#ifndef PREVIEWACTIONINTERFACE_H
#define PREVIEWACTIONINTERFACE_H

#include <memory>

#include "ActionInterface.h"

class Preview;
class DmDocument;

/// @brief 预览操作接口
/// 所有需要预览功能的操作类必须实现此接口
class PreviewActionInterface : public ActionInterface
{
public:
    PreviewActionInterface(const char* name, DmDocument* doc, GuiDocumentView* docView);
    ~PreviewActionInterface() override;

    void init(int status = 0) override;
    void finish(bool updateTB = true) override;
    void suspend() override;
    void resume() override;
    void trigger() override;
    bool isViewAction() override;

    /// @brief 绘制当前预览
    void drawPreview();

    /// @brief 从屏幕上删除预览
    void deletePreview();

protected:
    std::unique_ptr<Preview>    preview;    ///< 持有预览实体的预览对象
    bool                        hasPreview; ///< 预览是否正在使用中
    DmDocument*                 pDocument;  ///< 关联的文档指针
};

#endif
