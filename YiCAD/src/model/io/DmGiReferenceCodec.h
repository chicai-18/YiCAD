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

/// @file DmGiReferenceCodec.h
/// @brief GI 流里的引用按文档换成名字、按名字找回（代理图形存盘用，RENDER_PLAN.md 第 4.8.3 节）

#ifndef DMGIREFERENCECODEC_H
#define DMGIREFERENCECODEC_H

#include "GiStream.h"

class DmDocument;

/// @brief 文档级的 GI 引用编解码
/// @details 图层、线型、块按文档里的名字；字体按字体文件名（字体列表是全进程的）；
///          实体里已解码的图片像素没有名字，存不下来（来自文件的图片在图元里另有路径）
class DmGiReferenceCodec final : public IGiReferenceCodec
{
public:
    /// @param document 引用所属的文档，须比本对象活得久
    explicit DmGiReferenceCodec(DmDocument& document);

    std::string layerName(const DmLayer* layer) const override;
    const DmLayer* findLayer(const std::string& name) const override;

    std::string lineTypeName(const DmLineType* lineType) const override;
    const DmLineType* findLineType(const std::string& name) const override;

    std::string drawableName(const IGiDrawable* drawable) const override;
    const IGiDrawable* findDrawable(const std::string& name) const override;

    std::string fontName(const IGiFont* font) const override;
    const IGiFont* findFont(const std::string& name) const override;

    std::string imageName(const QImage* image) const override;
    const QImage* findImage(const std::string& name) const override;

private:
    DmDocument& m_document;
};

#endif // DMGIREFERENCECODEC_H
