/// @file PluginAbiConvert.h
/// @brief 插件 ABI 的值与宿主类型之间的转换与校验，HostApi、插件实体类（PluginEntityClass）与 GI 表（PluginGi）共用

#ifndef PLUGIN_ABI_CONVERT_H
#define PLUGIN_ABI_CONVERT_H

#include "YiCadPluginAbi.h"

#include "Datamodel.h"
#include "DmColor.h"
#include "DmVector.h"
#include "GiTransform.h"

#include <QByteArray>
#include <QString>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>

namespace plugin_abi
{

/// @brief 定长元素数组视图是否可读：数量不超过上限、指针对齐、不越过地址空间
template<typename Element, typename View>
bool validFixedArrayView(
    const View& view,
    uint32_t maximumCount = 1000000) noexcept
{
    if (view.count == 0)
    {
        return true;
    }
    if (view.data == nullptr || view.count > maximumCount)
    {
        return false;
    }
    const auto address = reinterpret_cast<std::uintptr_t>(view.data);
    if (address % alignof(Element) != 0 ||
        view.count > std::numeric_limits<std::size_t>::max() /
            sizeof(Element))
    {
        return false;
    }
    const auto byteCount = static_cast<std::size_t>(view.count) *
        sizeof(Element);
    return byteCount <= std::numeric_limits<std::uintptr_t>::max() - address;
}

/// @brief 复制 UTF-8 字符串视图；不是合法 UTF-8 或含 NUL 时返回 false
inline bool copyStringView(const YiCadStringView& source, QString& target) noexcept
{
    try
    {
        if ((source.data == nullptr && source.size != 0) ||
            source.size > static_cast<uint32_t>(std::numeric_limits<int>::max()))
        {
            return false;
        }
        if (source.size == 0)
        {
            target.clear();
            return true;
        }
        const auto address = reinterpret_cast<std::uintptr_t>(source.data);
        if (source.size >
            std::numeric_limits<std::uintptr_t>::max() - address)
        {
            return false;
        }
        if (std::memchr(source.data, '\0', source.size) != nullptr)
        {
            return false;
        }
        const QByteArray bytes(source.data, static_cast<int>(source.size));
        target = QString::fromUtf8(bytes.constData(), bytes.size());
        return target.toUtf8() == bytes;
    }
    catch (...)
    {
        target.clear();
        return false;
    }
}

/// @brief 线宽是否为 YiCAD 定义的标准线宽或 -3、-2、-1
inline bool validLineWidth(int32_t width) noexcept
{
    switch (width)
    {
    case -3: case -2: case -1: case 0: case 5: case 9: case 13:
    case 15: case 18: case 20: case 25: case 30: case 35: case 40:
    case 50: case 53: case 60: case 70: case 80: case 90: case 100:
    case 106: case 120: case 140: case 158: case 200: case 211:
        return true;
    default:
        return false;
    }
}

/// @brief ABI 颜色转宿主颜色；ACI 换成 RGB
inline bool toDmColor(const YiCadColorData& source, DmColor& color)
{
    if (source.reserved != 0)
    {
        return false;
    }
    switch (source.method)
    {
    case YICAD_COLOR_BY_LAYER:
        color = DmColor(DM::FlagByLayer);
        return true;
    case YICAD_COLOR_BY_BLOCK:
        color = DmColor(DM::FlagByBlock);
        return true;
    case YICAD_COLOR_ACI:
        if (source.aci < 1 || source.aci > 255)
        {
            return false;
        }
        color = DM::indexColors[source.aci];
        return true;
    case YICAD_COLOR_RGB:
        color = DmColor(source.red, source.green, source.blue);
        return true;
    default:
        return false;
    }
}

inline bool finitePoint(const YiCadPoint3d& point) noexcept
{
    constexpr double limit = 1.0e150;
    return std::isfinite(point.x) && std::abs(point.x) <= limit &&
           std::isfinite(point.y) && std::abs(point.y) <= limit &&
           std::isfinite(point.z) && std::abs(point.z) <= limit;
}

inline bool finitePoint(const YiCadPoint2d& point) noexcept
{
    constexpr double limit = 1.0e150;
    return std::isfinite(point.x) && std::abs(point.x) <= limit &&
           std::isfinite(point.y) && std::abs(point.y) <= limit;
}

inline DmVector toDmVector(const YiCadPoint3d& point)
{
    return DmVector(point.x, point.y, point.z);
}

inline DmVector toDmVector(const YiCadPoint2d& point)
{
    return DmVector(point.x, point.y);
}

inline bool validPointArray(const YiCadPoint2dArrayView& points) noexcept
{
    if (!validFixedArrayView<YiCadPoint2d>(points))
    {
        return false;
    }
    for (uint32_t index = 0; index < points.count; ++index)
    {
        if (!finitePoint(points.data[index]))
        {
            return false;
        }
    }
    return true;
}

inline bool validDoubleArray(const YiCadDoubleArrayView& values) noexcept
{
    if (!validFixedArrayView<double>(values))
    {
        return false;
    }
    for (uint32_t index = 0; index < values.count; ++index)
    {
        if (!std::isfinite(values.data[index]) ||
            std::abs(values.data[index]) > 1.0e150)
        {
            return false;
        }
    }
    return true;
}

/// @brief 矩阵的系数都有限
inline bool finiteMatrix(const YiCadMatrix2d& m) noexcept
{
    return std::isfinite(m.a) && std::isfinite(m.b) && std::isfinite(m.c) &&
           std::isfinite(m.d) && std::isfinite(m.tx) && std::isfinite(m.ty);
}

inline GiTransform toGiTransform(const YiCadMatrix2d& m)
{
    return GiTransform(m.a, m.b, m.c, m.d, m.tx, m.ty);
}

inline YiCadMatrix2d toAbiMatrix(const GiTransform& t) noexcept
{
    return {t.a(), t.b(), t.c(), t.d(), t.tx(), t.ty()};
}

inline YiCadPoint2d readPoint(const DmVector& value) noexcept
{
    return {value.x, value.y};
}

/// @brief 把 QString 转成 UTF-8 存进 scratch，交出指向它的视图
inline YiCadStringView readString(const QString& value, std::string& scratch)
{
    const auto bytes = value.toUtf8();
    scratch.assign(bytes.constData(), static_cast<std::size_t>(bytes.size()));
    return {scratch.empty() ? nullptr : scratch.data(),
        static_cast<uint32_t>(scratch.size())};
}

/// @brief 宿主颜色转 ABI 颜色（随层、随块、RGB）
inline YiCadColorData readColor(const DmColor& value) noexcept
{
    if (value.isByLayer())
    {
        return {YICAD_COLOR_BY_LAYER, 0, 0, 0, 0, 0};
    }
    if (value.isByBlock())
    {
        return {YICAD_COLOR_BY_BLOCK, 0, 0, 0, 0, 0};
    }
    return {YICAD_COLOR_RGB, 0,
        static_cast<uint8_t>(value.red()),
        static_cast<uint8_t>(value.green()),
        static_cast<uint8_t>(value.blue()), 0};
}

} // namespace plugin_abi

#endif // PLUGIN_ABI_CONVERT_H
