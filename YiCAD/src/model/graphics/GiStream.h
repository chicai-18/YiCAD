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

/// @file GiStream.h
/// @brief GI 流：worldDraw 输出的紧凑二进制记录，及其记录器、重放与序列化（RENDER_PLAN.md 第 4.2.3 节）
///
/// 用途：GS 编译 GPU 数据的输入（第 4 阶段）、插件实体的代理图形（第 7、8 阶段）、
/// 自定义实体默认实现的依据、单元测试（test_graphics）。
///
/// 记录的布局：[setAttributes 的属性记录] AttributesEnd [worldDraw 的记录]。
/// 嵌套绘制（IGiGeometry::draw）记成一个带长度的段，段内同样是"属性、AttributesEnd、图元"，
/// 重放时整段当作一个可绘制对象交给目标的 draw()，所以嵌套关系原样保留。
/// 层、线型、共享对象、字体、图片像素这些引用记成引用表的下标；内存里是指针，
/// 序列化时经 IGiReferenceCodec 换成名字。

#ifndef GISTREAM_H
#define GISTREAM_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "IGiDrawable.h"

class DmLayer;
class DmLineType;
class IGiFont;
class InputStream;
class OutputStream;
class QImage;

/// @brief 引用的种类
enum class GiReferenceKind : std::uint8_t
{
    Layer,
    LineType,
    Drawable,   ///< drawShared 的共享对象（块定义）
    Font,
    Image,      ///< 实体里已解码的图片像素
};

/// @brief 序列化时把引用换成名字、读回时按名字找回引用
/// @details 由调用方按所在文档实现；换不出名字（返回空串）的引用读回时为空
class IGiReferenceCodec
{
public:
    virtual ~IGiReferenceCodec() = default;

    virtual std::string layerName(const DmLayer* layer) const = 0;
    virtual const DmLayer* findLayer(const std::string& name) const = 0;

    virtual std::string lineTypeName(const DmLineType* lineType) const = 0;
    virtual const DmLineType* findLineType(const std::string& name) const = 0;

    virtual std::string drawableName(const IGiDrawable* drawable) const = 0;
    virtual const IGiDrawable* findDrawable(const std::string& name) const = 0;

    virtual std::string fontName(const IGiFont* font) const = 0;
    virtual const IGiFont* findFont(const std::string& name) const = 0;

    virtual std::string imageName(const QImage* image) const = 0;
    virtual const QImage* findImage(const std::string& name) const = 0;
};

/// @brief GI 流。坐标为 double；字节序与本机相同（Windows 上为小端）
class GiStream
{
public:
    static constexpr std::uint32_t kVersion = 2;    ///< 序列化格式的版本号（2：加了填充图案 setFill）

    /// @brief 是否没有任何记录
    bool isEmpty() const { return m_bytes.empty(); }

    /// @brief 清空
    void clear();

    /// @brief 记录占用的字节数（不含引用表）
    std::size_t byteSize() const { return m_bytes.size(); }

    /// @brief 写出：版本号、引用表（名字）、记录
    void write(OutputStream& out, const IGiReferenceCodec& codec) const;

    /// @brief 读回；版本号不认识时返回 false，流保持为空
    bool read(InputStream& in, const IGiReferenceCodec& codec);

    /// @brief 逐个替换引用表里的对象，记录不变
    /// @details 代理实体改归另一份文档时，把图层、线型、块换成目标文档的（DmProxyEntity::transferReferences）
    /// @param map 收到引用的种类与原对象，返回新对象（可为空）
    void remapReferences(const std::function<const void*(GiReferenceKind, const void*)>& map);

    /// @brief 记录与引用都相同
    bool operator==(const GiStream& other) const = default;

private:
    friend class GiStreamRecorder;
    friend class GiStreamReader;

    /// @brief 引用表的一项
    struct Reference
    {
        GiReferenceKind kind = GiReferenceKind::Layer;
        const void* object = nullptr;

        bool operator==(const Reference& other) const = default;
    };

    std::vector<std::byte> m_bytes;         ///< 记录
    std::vector<Reference> m_references;    ///< 引用表
};

/// @brief 把可绘制对象的输出记成 GI 流
class GiStreamRecorder
{
public:
    /// @brief 记录一个可绘制对象：先 setAttributes，再 worldDraw
    /// @param regenType 交给 worldDraw 的用途
    /// @param deviation 交给 worldDraw 的弦高容差
    static GiStream record(const IGiDrawable& drawable, GiRegenType regenType = GiRegenType::Display,
                           double deviation = 0.0);
};

/// @brief 把 GI 流当作可绘制对象：setAttributes 重放属性记录，worldDraw 重放图元
/// @details 不持有流，流要比它活得久。代理实体、测试都经它把记下的输出交给任意接收方
class GiStreamDrawable final : public IGiDrawable
{
public:
    explicit GiStreamDrawable(const GiStream& stream);

    void setAttributes(IGiSubEntityTraits& traits) const override;
    void worldDraw(IGiWorldDraw& wd) const override;

private:
    const GiStream& m_stream;
};

#endif // GISTREAM_H
