#ifndef YICAD_PLUGIN_SDK_H
#define YICAD_PLUGIN_SDK_H

#include "YiCadPluginAbi.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace yicad::plugin
{

class Host;
class Document;

/// @brief 在 C ABI 边界内执行插件代码，并把所有异常转换为指定失败值。
/// @param callable 不得把引用保存到本次调用之后的插件侧可调用对象。
/// @param failure 捕获异常时返回的值。
/// @return 插件代码的返回值，或捕获异常后的 failure。
/// @note 插件的导出入口和回调应使用本函数，禁止异常穿过 C ABI。
template<typename Result, typename Callable>
Result invokeNoexcept(Callable&& callable, Result failure) noexcept
{
    try
    {
        return static_cast<Result>(
            std::invoke(std::forward<Callable>(callable)));
    }
    catch (...)
    {
        return failure;
    }
}

/// @brief 在无返回值 C ABI 边界内执行插件代码并吞掉所有异常。
/// @param callable 不得把引用保存到本次调用之后的插件侧可调用对象。
template<typename Callable>
void invokeNoexcept(Callable&& callable) noexcept
{
    try
    {
        std::invoke(std::forward<Callable>(callable));
    }
    catch (...)
    {
    }
}

#if defined(YICAD_ENABLE_PLUGIN_ABI_V3_DRAFT)
class ImportSession;
class ImportContainer;
class ImportResource;
class EntityAttributes;
class DocumentSettings;
class LineTypeData;
class LayerData;
class TextStyleData;
class DimensionStyleData;
class PolylineData;
class SplineData;
class TextData;
class MTextData;
class BlockData;
class InsertData;
class AttributeDefinitionData;
class AttributeData;
class DimensionData;
class LeaderData;
class HatchData;
class ImageData;

namespace detail
{

inline bool fitsAbiCount(std::size_t size) noexcept
{
    return size <= std::numeric_limits<uint32_t>::max();
}

inline YiCadStringView stringView(const std::string& value) noexcept
{
    if (!fitsAbiCount(value.size()))
    {
        return {};
    }
    return {value.empty() ? nullptr : value.data(),
        static_cast<uint32_t>(value.size())};
}

template<typename Value>
auto arrayView(const std::vector<Value>& values) noexcept
{
    using View = std::conditional_t<std::is_same_v<Value, double>,
        YiCadDoubleArrayView,
        std::conditional_t<std::is_same_v<Value, YiCadPoint2d>,
            YiCadPoint2dArrayView,
            YiCadVertex2dArrayView>>;
    if (!fitsAbiCount(values.size()))
    {
        return View{};
    }
    return View{values.empty() ? nullptr : values.data(),
        static_cast<uint32_t>(values.size())};
}

inline bool validString(const std::string& value, bool required) noexcept
{
    return fitsAbiCount(value.size()) && (!required || !value.empty()) &&
           value.find('\0') == std::string::npos;
}

template<typename Data>
Data initializeImportData() noexcept = delete;

#define YICAD_SDK_DEFINE_ZERO_IMPORT_DATA(type)                         \
    template<>                                                         \
    inline type initializeImportData<type>() noexcept                  \
    {                                                                  \
        type data{};                                                   \
        data.structSize = static_cast<uint32_t>(sizeof(data));         \
        return data;                                                   \
    }

YICAD_SDK_DEFINE_ZERO_IMPORT_DATA(YiCadLineTypeDataV3)
YICAD_SDK_DEFINE_ZERO_IMPORT_DATA(YiCadPointDataV3)
YICAD_SDK_DEFINE_ZERO_IMPORT_DATA(YiCadLineDataV3)
YICAD_SDK_DEFINE_ZERO_IMPORT_DATA(YiCadArcDataV3)
YICAD_SDK_DEFINE_ZERO_IMPORT_DATA(YiCadCircleDataV3)
YICAD_SDK_DEFINE_ZERO_IMPORT_DATA(YiCadEllipseDataV3)
YICAD_SDK_DEFINE_ZERO_IMPORT_DATA(YiCadPolylineDataV3)
YICAD_SDK_DEFINE_ZERO_IMPORT_DATA(YiCadSplineDataV3)
YICAD_SDK_DEFINE_ZERO_IMPORT_DATA(YiCadBlockDataV3)
YICAD_SDK_DEFINE_ZERO_IMPORT_DATA(YiCadAttributeDefinitionDataV3)
YICAD_SDK_DEFINE_ZERO_IMPORT_DATA(YiCadAttributeDataV3)
YICAD_SDK_DEFINE_ZERO_IMPORT_DATA(YiCadLeaderDataV3)
YICAD_SDK_DEFINE_ZERO_IMPORT_DATA(YiCadHatchEdgeDataV3)
YICAD_SDK_DEFINE_ZERO_IMPORT_DATA(YiCadImageDataV3)

#undef YICAD_SDK_DEFINE_ZERO_IMPORT_DATA

template<>
inline YiCadDocumentSettings
initializeImportData<YiCadDocumentSettings>() noexcept
{
    YiCadDocumentSettings data{};
    data.structSize = static_cast<uint32_t>(sizeof(data));
    data.globalLineTypeScale = 1.0;
    return data;
}

template<>
inline YiCadLayerDataV3 initializeImportData<YiCadLayerDataV3>() noexcept
{
    YiCadLayerDataV3 data{};
    data.structSize = static_cast<uint32_t>(sizeof(data));
    data.plottable = 1;
    data.color.method = YICAD_COLOR_BY_LAYER;
    data.lineWidth = -1;
    return data;
}

template<>
inline YiCadTextStyleDataV3
initializeImportData<YiCadTextStyleDataV3>() noexcept
{
    YiCadTextStyleDataV3 data{};
    data.structSize = static_cast<uint32_t>(sizeof(data));
    data.widthFactor = 1.0;
    return data;
}

template<>
inline YiCadDimensionStyleDataV3
initializeImportData<YiCadDimensionStyleDataV3>() noexcept
{
    YiCadDimensionStyleDataV3 data{};
    data.structSize = static_cast<uint32_t>(sizeof(data));
    data.dimLineColor.method = YICAD_COLOR_BY_LAYER;
    data.extensionLineColor.method = YICAD_COLOR_BY_LAYER;
    data.textColor.method = YICAD_COLOR_BY_LAYER;
    data.textFillColor.method = YICAD_COLOR_BY_LAYER;
    data.dimLineWidth = -1;
    data.extensionLineWidth = -1;
    data.fractionHeightScale = 1.0;
    data.measurementScale = 1.0;
    return data;
}

template<>
inline YiCadEntityAttributes
initializeImportData<YiCadEntityAttributes>() noexcept
{
    YiCadEntityAttributes data{};
    data.structSize = static_cast<uint32_t>(sizeof(data));
    data.color.method = YICAD_COLOR_BY_LAYER;
    data.lineWidth = -1;
    data.lineTypeScale = 1.0;
    data.visible = 1;
    data.normal.z = 1.0;
    return data;
}

template<>
inline YiCadRayDataV3 initializeImportData<YiCadRayDataV3>() noexcept
{
    YiCadRayDataV3 data{};
    data.structSize = static_cast<uint32_t>(sizeof(data));
    data.direction.x = 1.0;
    return data;
}

template<>
inline YiCadXLineDataV3 initializeImportData<YiCadXLineDataV3>() noexcept
{
    YiCadXLineDataV3 data{};
    data.structSize = static_cast<uint32_t>(sizeof(data));
    data.direction.x = 1.0;
    return data;
}

template<>
inline YiCadTextDataV3 initializeImportData<YiCadTextDataV3>() noexcept
{
    YiCadTextDataV3 data{};
    data.structSize = static_cast<uint32_t>(sizeof(data));
    data.widthFactor = 1.0;
    return data;
}

template<>
inline YiCadMTextBackgroundData
initializeImportData<YiCadMTextBackgroundData>() noexcept
{
    YiCadMTextBackgroundData data{};
    data.structSize = static_cast<uint32_t>(sizeof(data));
    data.color.method = YICAD_COLOR_BY_LAYER;
    data.borderScaleFactor = 1.0;
    return data;
}

template<>
inline YiCadMTextDataV3 initializeImportData<YiCadMTextDataV3>() noexcept
{
    YiCadMTextDataV3 data{};
    data.structSize = static_cast<uint32_t>(sizeof(data));
    data.direction.x = 1.0;
    data.lineSpacingFactor = 1.0;
    data.attachment = YICAD_MTEXT_TOP_LEFT;
    return data;
}

template<>
inline YiCadInsertDataV3 initializeImportData<YiCadInsertDataV3>() noexcept
{
    YiCadInsertDataV3 data{};
    data.structSize = static_cast<uint32_t>(sizeof(data));
    data.scale.x = 1.0;
    data.scale.y = 1.0;
    data.scale.z = 1.0;
    data.columnCount = 1;
    data.rowCount = 1;
    return data;
}

template<>
inline YiCadDimensionDataV3
initializeImportData<YiCadDimensionDataV3>() noexcept
{
    YiCadDimensionDataV3 data{};
    data.structSize = static_cast<uint32_t>(sizeof(data));
    data.lineSpacingFactor = 1.0;
    return data;
}

template<>
inline YiCadHatchLoopDataV3
initializeImportData<YiCadHatchLoopDataV3>() noexcept
{
    YiCadHatchLoopDataV3 data{};
    data.structSize = static_cast<uint32_t>(sizeof(data));
    data.outerLoopIndex = UINT32_MAX;
    return data;
}

template<>
inline YiCadHatchDataV3 initializeImportData<YiCadHatchDataV3>() noexcept
{
    YiCadHatchDataV3 data{};
    data.structSize = static_cast<uint32_t>(sizeof(data));
    data.patternScale = 1.0;
    return data;
}

} // namespace detail

/**
 * @brief 创建已清零并带有完整 ABI 大小和协议默认值的 v3 输入 POD。
 * @tparam Data `YiCadPluginAbi.h` 中受 SDK 支持的可扩展输入类型。
 * @return 可继续填写业务字段并传给底层 POD 重载的输入结构。
 * @note 不支持固定布局值类型或任意自定义类型；这些类型会在编译期被拒绝。
 */
template<typename Data>
Data makeImportData() noexcept
{
    return detail::initializeImportData<Data>();
}

/// @brief 从 SDK 持有的连续容器生成填充边数组 ABI 视图。
/// @return 元素数超过 ABI 可表示范围时返回空视图。
inline YiCadHatchEdgeArrayView makeHatchEdgeArrayView(
    std::span<const YiCadHatchEdgeDataV3> edges) noexcept
{
    if (edges.size() > std::numeric_limits<uint32_t>::max())
    {
        return {};
    }
    return {
        edges.empty() ? nullptr : edges.data(),
        static_cast<uint32_t>(edges.size()),
        static_cast<uint32_t>(sizeof(YiCadHatchEdgeDataV3))};
}

/// @brief 从 SDK 持有的连续容器生成填充环数组 ABI 视图。
/// @return 元素数超过 ABI 可表示范围时返回空视图。
inline YiCadHatchLoopArrayView makeHatchLoopArrayView(
    std::span<const YiCadHatchLoopDataV3> loops) noexcept
{
    if (loops.size() > std::numeric_limits<uint32_t>::max())
    {
        return {};
    }
    return {
        loops.empty() ? nullptr : loops.data(),
        static_cast<uint32_t>(loops.size()),
        static_cast<uint32_t>(sizeof(YiCadHatchLoopDataV3))};
}

namespace detail
{

/// @brief 判断导入子表字段是否同时满足版本和可访问字节数要求。
inline bool hasImportField(
    const YiCadImportApi* api,
    std::size_t offset,
    std::size_t size) noexcept
{
    constexpr auto headerSize =
        offsetof(YiCadImportApi, abiVersion) + sizeof(uint32_t);
    return api != nullptr && api->structSize >= headerSize &&
           api->abiVersion >= YICAD_PLUGIN_ABI_V3_DRAFT &&
           api->structSize >= offset && size <= api->structSize - offset;
}

struct ImportState
{
    const YiCadImportApi* api = nullptr;
    YiCadImportSessionHandle session = nullptr;
};

template<typename Callable>
YiCadImportResult callImport(Callable&& callable) noexcept
{
    return invokeNoexcept<YiCadImportResult>(
        std::forward<Callable>(callable),
        YICAD_IMPORT_ERROR_TRANSACTION_FAILED);
}

} // namespace detail

#define YICAD_SDK_HAS_IMPORT_FUNCTION(api, field)                         \
    (::yicad::plugin::detail::hasImportField(                            \
         (api),                                                          \
         offsetof(YiCadImportApi, field),                                \
         sizeof(((YiCadImportApi*)nullptr)->field)) &&                   \
     (api)->field != nullptr)

/**
 * @brief 导入会话内的非拥有资源句柄包装。
 * @note 资源由宿主持有，只在创建它的活动会话内有效；插件不得释放或跨回调缓存。
 */
class ImportResource
{
public:
    ImportResource() noexcept = default;

    explicit operator bool() const noexcept
    {
        return nativeHandle() != nullptr;
    }

    /// @brief 返回仅供填充 ABI POD 输入结构使用的句柄。
    /// @return 会话已结束或资源为空时返回 nullptr。
    YiCadImportResourceHandle nativeHandle() const noexcept
    {
        return m_state != nullptr && m_state->session != nullptr
            ? m_handle
            : nullptr;
    }

private:
    friend class ImportSession;
    friend class ImportContainer;

    ImportResource(
        std::shared_ptr<detail::ImportState> state,
        YiCadImportResourceHandle handle) noexcept
        : m_state(std::move(state)),
          m_handle(handle)
    {
    }

    std::shared_ptr<detail::ImportState> m_state;
    YiCadImportResourceHandle m_handle = nullptr;
};

/**
 * @brief 插件侧实体公共属性，默认值与宿主的空 attributes 语义一致。
 * @note 本对象不跨 DLL 边界；实体语义重载仅在宿主调用期间借用其内部 POD。
 */
class EntityAttributes
{
public:
    EntityAttributes() noexcept
        : m_data(makeImportData<YiCadEntityAttributes>())
    {
    }

    /// @brief 设置图层；空或过期资源恢复为活动图层。
    EntityAttributes& setLayer(const ImportResource& layer) noexcept
    {
        m_data.layer = layer.nativeHandle();
        return *this;
    }

    /// @brief 设置线型；空或过期资源恢复为 ByLayer。
    EntityAttributes& setLineType(const ImportResource& lineType) noexcept
    {
        m_data.lineType = lineType.nativeHandle();
        return *this;
    }

    /// @brief 设置固定布局 ABI 颜色值。
    EntityAttributes& setColor(const YiCadColorData& color) noexcept
    {
        m_data.color = color;
        return *this;
    }

    /// @brief 设置标准线宽枚举值。
    EntityAttributes& setLineWidth(int32_t lineWidth) noexcept
    {
        m_data.lineWidth = lineWidth;
        return *this;
    }

    /// @brief 设置实体线型比例。
    EntityAttributes& setLineTypeScale(double scale) noexcept
    {
        m_data.lineTypeScale = scale;
        return *this;
    }

    /// @brief 设置实体可见性。
    EntityAttributes& setVisible(bool visible) noexcept
    {
        m_data.visible = visible ? 1U : 0U;
        return *this;
    }

    /// @brief 设置实体法向量。
    EntityAttributes& setNormal(YiCadVector3d normal) noexcept
    {
        m_data.normal = normal;
        return *this;
    }

private:
    friend class ImportContainer;
    friend class PolylineData;
    friend class SplineData;
    friend class TextData;
    friend class MTextData;
    friend class InsertData;
    friend class DimensionData;
    friend class LeaderData;
    friend class HatchData;
    friend class ImageData;

    YiCadEntityAttributes m_data;
};

/** @brief 插件侧拥有字符串的文档设置。 */
class DocumentSettings
{
public:
    DocumentSettings& setInsertionUnits(int32_t value) noexcept
    {
        m_insertionUnits = value;
        return *this;
    }

    DocumentSettings& setMeasurement(int32_t value) noexcept
    {
        m_measurement = value;
        return *this;
    }

    DocumentSettings& setGlobalLineTypeScale(double value) noexcept
    {
        m_globalLineTypeScale = value;
        return *this;
    }

    DocumentSettings& setSourceCodePage(std::string value)
    {
        m_sourceCodePage = std::move(value);
        return *this;
    }

private:
    friend class ImportSession;

    YiCadImportResult makeAbi(YiCadDocumentSettings& data) const noexcept
    {
        if (!detail::validString(m_sourceCodePage, false))
        {
            return YICAD_IMPORT_ERROR_INVALID_ARGUMENT;
        }
        data = makeImportData<YiCadDocumentSettings>();
        data.insertionUnits = m_insertionUnits;
        data.measurement = m_measurement;
        data.globalLineTypeScale = m_globalLineTypeScale;
        data.sourceCodePage = detail::stringView(m_sourceCodePage);
        return YICAD_IMPORT_SUCCESS;
    }

    int32_t m_insertionUnits = 0;
    int32_t m_measurement = 0;
    double m_globalLineTypeScale = 1.0;
    std::string m_sourceCodePage;
};

/** @brief 插件侧拥有名称、说明和元素序列的简单线型定义。 */
class LineTypeData
{
public:
    explicit LineTypeData(std::string name = {}) : m_name(std::move(name)) {}

    LineTypeData& setDescription(std::string value)
    {
        m_description = std::move(value);
        return *this;
    }

    LineTypeData& setElements(std::vector<double> value)
    {
        m_elements = std::move(value);
        return *this;
    }

    /// @brief 标记复杂线型；宿主将按 ABI 契约明确返回不支持。
    LineTypeData& setComplex(bool value) noexcept
    {
        m_complex = value;
        return *this;
    }

private:
    friend class ImportSession;

    YiCadImportResult makeAbi(YiCadLineTypeDataV3& data) const noexcept
    {
        if (!detail::validString(m_name, true) ||
            !detail::validString(m_description, false) ||
            !detail::fitsAbiCount(m_elements.size()) ||
            (!m_complex && m_elements.empty()))
        {
            return YICAD_IMPORT_ERROR_INVALID_ARGUMENT;
        }
        data = makeImportData<YiCadLineTypeDataV3>();
        data.name = detail::stringView(m_name);
        data.description = detail::stringView(m_description);
        data.elements = detail::arrayView(m_elements);
        data.complex = m_complex ? 1U : 0U;
        return YICAD_IMPORT_SUCCESS;
    }

    std::string m_name;
    std::string m_description;
    std::vector<double> m_elements;
    bool m_complex = false;
};

/** @brief 插件侧拥有名称并引用会话资源的图层定义。 */
class LayerData
{
public:
    explicit LayerData(std::string name = {}) : m_name(std::move(name)) {}

    LayerData& setFrozen(bool value) noexcept { m_frozen = value; return *this; }
    LayerData& setLocked(bool value) noexcept { m_locked = value; return *this; }
    LayerData& setPlottable(bool value) noexcept { m_plottable = value; return *this; }
    LayerData& setColor(YiCadColorData value) noexcept { m_color = value; return *this; }
    LayerData& setLineType(const ImportResource& value) noexcept
    {
        m_lineType = value;
        return *this;
    }
    LayerData& setLineWidth(int32_t value) noexcept { m_lineWidth = value; return *this; }

private:
    friend class ImportSession;

    YiCadImportResult makeAbi(YiCadLayerDataV3& data) const noexcept
    {
        if (!detail::validString(m_name, true))
        {
            return YICAD_IMPORT_ERROR_INVALID_ARGUMENT;
        }
        data = makeImportData<YiCadLayerDataV3>();
        data.name = detail::stringView(m_name);
        data.frozen = m_frozen ? 1U : 0U;
        data.locked = m_locked ? 1U : 0U;
        data.plottable = m_plottable ? 1U : 0U;
        data.color = m_color;
        data.lineType = m_lineType.nativeHandle();
        data.lineWidth = m_lineWidth;
        return YICAD_IMPORT_SUCCESS;
    }

    std::string m_name;
    bool m_frozen = false;
    bool m_locked = false;
    bool m_plottable = true;
    YiCadColorData m_color{YICAD_COLOR_BY_LAYER, 0, 0, 0, 0, 0};
    ImportResource m_lineType;
    int32_t m_lineWidth = -1;
};

/** @brief 插件侧拥有字体路径字符串的文字样式定义。 */
class TextStyleData
{
public:
    explicit TextStyleData(std::string name = {}) : m_name(std::move(name)) {}

    TextStyleData& setFontFiles(std::string fontFile, std::string bigFontFile = {})
    {
        m_fontFile = std::move(fontFile);
        m_bigFontFile = std::move(bigFontFile);
        return *this;
    }
    TextStyleData& setMetrics(double fixedHeight, double widthFactor,
        double obliqueAngle) noexcept
    {
        m_fixedHeight = fixedHeight;
        m_widthFactor = widthFactor;
        m_obliqueAngle = obliqueAngle;
        return *this;
    }
    TextStyleData& setGenerationFlags(uint32_t value) noexcept
    {
        m_generationFlags = value;
        return *this;
    }

private:
    friend class ImportSession;

    YiCadImportResult makeAbi(YiCadTextStyleDataV3& data) const noexcept
    {
        if (!detail::validString(m_name, true) ||
            !detail::validString(m_fontFile, false) ||
            !detail::validString(m_bigFontFile, false))
        {
            return YICAD_IMPORT_ERROR_INVALID_ARGUMENT;
        }
        data = makeImportData<YiCadTextStyleDataV3>();
        data.name = detail::stringView(m_name);
        data.fontFile = detail::stringView(m_fontFile);
        data.bigFontFile = detail::stringView(m_bigFontFile);
        data.fixedHeight = m_fixedHeight;
        data.widthFactor = m_widthFactor;
        data.obliqueAngle = m_obliqueAngle;
        data.generationFlags = m_generationFlags;
        return YICAD_IMPORT_SUCCESS;
    }

    std::string m_name;
    std::string m_fontFile;
    std::string m_bigFontFile;
    double m_fixedHeight = 0.0;
    double m_widthFactor = 1.0;
    double m_obliqueAngle = 0.0;
    uint32_t m_generationFlags = 0;
};

/** @brief 插件侧拥有格式字符串并保留全部 v3 字段的标注样式定义。 */
class DimensionStyleData
{
public:
    explicit DimensionStyleData(std::string name = {}) : m_name(std::move(name)) {}

    DimensionStyleData& setResources(const ImportResource& textStyle,
        const ImportResource& dimLineType,
        const ImportResource& extensionLineType) noexcept
    {
        m_textStyle = textStyle; m_dimLineType = dimLineType;
        m_extensionLineType = extensionLineType; return *this;
    }
    DimensionStyleData& setColors(YiCadColorData dimLine,
        YiCadColorData extensionLine, YiCadColorData text,
        YiCadColorData textFill) noexcept
    {
        m_data.dimLineColor = dimLine; m_data.extensionLineColor = extensionLine;
        m_data.textColor = text; m_data.textFillColor = textFill; return *this;
    }
    DimensionStyleData& setLineWidths(int32_t dimLine, int32_t extensionLine) noexcept
    {
        m_data.dimLineWidth = dimLine; m_data.extensionLineWidth = extensionLine;
        return *this;
    }
    DimensionStyleData& setLineSuppression(bool dim1, bool dim2,
        bool extension1, bool extension2) noexcept
    {
        m_data.hideDimLine1 = dim1; m_data.hideDimLine2 = dim2;
        m_data.hideExtensionLine1 = extension1;
        m_data.hideExtensionLine2 = extension2; return *this;
    }
    DimensionStyleData& setExtensionGeometry(double beyond, double originOffset,
        bool fixedEnabled, double fixedLength) noexcept
    {
        m_data.extensionBeyondDimLine = beyond;
        m_data.extensionOriginOffset = originOffset;
        m_data.fixedExtensionLineLengthEnabled = fixedEnabled;
        m_data.fixedExtensionLineLength = fixedLength; return *this;
    }
    DimensionStyleData& setArrows(int32_t first, int32_t second,
        int32_t leader, double size) noexcept
    {
        m_data.firstArrow = first; m_data.secondArrow = second;
        m_data.leaderArrow = leader; m_data.arrowSize = size; return *this;
    }
    DimensionStyleData& setTextLayout(double height, double fractionScale,
        bool drawBoundary, int32_t verticalPosition, int32_t horizontalPosition,
        int32_t direction, double offset) noexcept
    {
        m_data.textHeight = height; m_data.fractionHeightScale = fractionScale;
        m_data.drawTextBoundary = drawBoundary;
        m_data.textVerticalPosition = verticalPosition;
        m_data.textHorizontalPosition = horizontalPosition;
        m_data.textDirection = direction; m_data.textOffset = offset; return *this;
    }
    DimensionStyleData& setLinearFormat(int32_t unitFormat, int32_t precision,
        int32_t fractionFormat, int32_t decimalSeparator, double roundOff,
        std::string prefix, std::string suffix, double measurementScale,
        bool suppressLeadingZeros, bool suppressTrailingZeros)
    {
        m_data.linearUnitFormat = unitFormat; m_data.linearPrecision = precision;
        m_data.fractionFormat = fractionFormat;
        m_data.decimalSeparator = decimalSeparator; m_data.roundOff = roundOff;
        m_prefix = std::move(prefix); m_suffix = std::move(suffix);
        m_data.measurementScale = measurementScale;
        m_data.suppressLeadingZeros = suppressLeadingZeros;
        m_data.suppressTrailingZeros = suppressTrailingZeros; return *this;
    }
    DimensionStyleData& setAngularFormat(int32_t unitFormat, int32_t precision,
        bool suppressLeadingZeros, bool suppressTrailingZeros) noexcept
    {
        m_data.angularUnitFormat = unitFormat; m_data.angularPrecision = precision;
        m_data.suppressAngularLeadingZeros = suppressLeadingZeros;
        m_data.suppressAngularTrailingZeros = suppressTrailingZeros; return *this;
    }
    DimensionStyleData& allowUnsupportedFields(uint64_t mask, bool allow) noexcept
    {
        m_data.unsupportedFieldMask = mask;
        m_data.allowUnsupportedFields = allow; return *this;
    }

private:
    friend class ImportSession;

    YiCadImportResult makeAbi(YiCadDimensionStyleDataV3& data) const noexcept
    {
        if (!detail::validString(m_name, true) ||
            !detail::validString(m_prefix, false) ||
            !detail::validString(m_suffix, false))
        {
            return YICAD_IMPORT_ERROR_INVALID_ARGUMENT;
        }
        data = m_data;
        data.name = detail::stringView(m_name);
        data.textStyle = m_textStyle.nativeHandle();
        data.dimLineType = m_dimLineType.nativeHandle();
        data.extensionLineType = m_extensionLineType.nativeHandle();
        data.prefix = detail::stringView(m_prefix);
        data.suffix = detail::stringView(m_suffix);
        return YICAD_IMPORT_SUCCESS;
    }

    std::string m_name;
    std::string m_prefix;
    std::string m_suffix;
    ImportResource m_textStyle;
    ImportResource m_dimLineType;
    ImportResource m_extensionLineType;
    YiCadDimensionStyleDataV3 m_data = makeImportData<YiCadDimensionStyleDataV3>();
};

/** @brief 插件侧拥有顶点数组的二维多段线输入。 */
class PolylineData
{
public:
    explicit PolylineData(std::vector<YiCadVertex2d> vertices = {})
        : m_vertices(std::move(vertices)) {}

    PolylineData& setClosed(bool value) noexcept { m_closed = value; return *this; }
    PolylineData& setAttributes(EntityAttributes value) noexcept
    {
        m_attributes = std::move(value); return *this;
    }

private:
    friend class ImportContainer;

    YiCadImportResult makeAbi(YiCadPolylineDataV3& data) const noexcept
    {
        if (m_vertices.empty() || !detail::fitsAbiCount(m_vertices.size()))
        {
            return YICAD_IMPORT_ERROR_INVALID_ARGUMENT;
        }
        data = makeImportData<YiCadPolylineDataV3>();
        data.attributes = &m_attributes.m_data;
        data.vertices = detail::arrayView(m_vertices);
        data.closed = m_closed ? 1U : 0U;
        return YICAD_IMPORT_SUCCESS;
    }

    std::vector<YiCadVertex2d> m_vertices;
    EntityAttributes m_attributes;
    bool m_closed = false;
};

/** @brief 插件侧拥有控制点、拟合点、节点和权重数组的样条输入。 */
class SplineData
{
public:
    SplineData& setControlPoints(uint32_t degree,
        std::vector<YiCadPoint2d> points, std::vector<double> knots,
        std::vector<double> weights = {})
    {
        m_definition = YICAD_SPLINE_CONTROL_POINTS; m_degree = degree;
        m_controlPoints = std::move(points); m_knots = std::move(knots);
        m_weights = std::move(weights); m_fitPoints.clear(); return *this;
    }
    SplineData& setFitPoints(uint32_t degree, std::vector<YiCadPoint2d> points)
    {
        m_definition = YICAD_SPLINE_FIT_POINTS; m_degree = degree;
        m_fitPoints = std::move(points); m_controlPoints.clear();
        m_knots.clear(); m_weights.clear(); return *this;
    }
    SplineData& setClosed(bool value) noexcept { m_closed = value; return *this; }
    SplineData& setRational(bool value) noexcept { m_rational = value; return *this; }
    SplineData& setPeriodic(bool value) noexcept { m_periodic = value; return *this; }
    SplineData& setAttributes(EntityAttributes value) noexcept
    {
        m_attributes = std::move(value); return *this;
    }

private:
    friend class ImportContainer;

    YiCadImportResult makeAbi(YiCadSplineDataV3& data) const noexcept
    {
        if (!detail::fitsAbiCount(m_controlPoints.size()) ||
            !detail::fitsAbiCount(m_fitPoints.size()) ||
            !detail::fitsAbiCount(m_knots.size()) ||
            !detail::fitsAbiCount(m_weights.size()))
        {
            return YICAD_IMPORT_ERROR_OUT_OF_RANGE;
        }
        if ((m_definition == YICAD_SPLINE_CONTROL_POINTS && m_controlPoints.empty()) ||
            (m_definition == YICAD_SPLINE_FIT_POINTS && m_fitPoints.empty()))
        {
            return YICAD_IMPORT_ERROR_INVALID_ARGUMENT;
        }
        data = makeImportData<YiCadSplineDataV3>();
        data.attributes = &m_attributes.m_data;
        data.definition = m_definition; data.degree = m_degree;
        data.closed = m_closed ? 1U : 0U;
        data.rational = m_rational ? 1U : 0U;
        data.periodic = m_periodic ? 1U : 0U;
        data.controlPoints = detail::arrayView(m_controlPoints);
        data.knots = detail::arrayView(m_knots);
        data.weights = detail::arrayView(m_weights);
        data.fitPoints = detail::arrayView(m_fitPoints);
        return YICAD_IMPORT_SUCCESS;
    }

    YiCadSplineDefinition m_definition = YICAD_SPLINE_CONTROL_POINTS;
    uint32_t m_degree = 0;
    bool m_closed = false;
    bool m_rational = false;
    bool m_periodic = false;
    std::vector<YiCadPoint2d> m_controlPoints;
    std::vector<double> m_knots;
    std::vector<double> m_weights;
    std::vector<YiCadPoint2d> m_fitPoints;
    EntityAttributes m_attributes;
};

/** @brief 插件侧拥有 UTF-8 内容的单行文字输入。 */
class TextData
{
public:
    explicit TextData(std::string text = {}) : m_text(std::move(text)) {}

    TextData& setPlacement(YiCadPoint2d insertionPoint,
        YiCadPoint2d alignmentPoint = {}) noexcept
    {
        m_insertionPoint = insertionPoint; m_alignmentPoint = alignmentPoint;
        return *this;
    }
    TextData& setMetrics(double height, double rotation = 0.0,
        double widthFactor = 1.0, double obliqueAngle = 0.0) noexcept
    {
        m_height = height; m_rotation = rotation; m_widthFactor = widthFactor;
        m_obliqueAngle = obliqueAngle; return *this;
    }
    TextData& setAlignment(YiCadTextHorizontalAlignment horizontal,
        YiCadTextVerticalAlignment vertical) noexcept
    {
        m_horizontalAlignment = horizontal; m_verticalAlignment = vertical;
        return *this;
    }
    TextData& setStyle(const ImportResource& value) noexcept
    {
        m_textStyle = value; return *this;
    }
    TextData& setAttributes(EntityAttributes value) noexcept
    {
        m_attributes = std::move(value); return *this;
    }

private:
    friend class ImportContainer;
    friend class AttributeDefinitionData;
    friend class AttributeData;
    friend class LeaderData;

    YiCadImportResult makeAbi(YiCadTextDataV3& data) const noexcept
    {
        if (!detail::validString(m_text, true))
        {
            return YICAD_IMPORT_ERROR_INVALID_ARGUMENT;
        }
        data = makeImportData<YiCadTextDataV3>();
        data.attributes = &m_attributes.m_data;
        data.text = detail::stringView(m_text);
        data.insertionPoint = m_insertionPoint;
        data.alignmentPoint = m_alignmentPoint;
        data.height = m_height; data.rotation = m_rotation;
        data.widthFactor = m_widthFactor; data.obliqueAngle = m_obliqueAngle;
        data.horizontalAlignment = m_horizontalAlignment;
        data.verticalAlignment = m_verticalAlignment;
        data.textStyle = m_textStyle.nativeHandle();
        return YICAD_IMPORT_SUCCESS;
    }

    std::string m_text;
    YiCadPoint2d m_insertionPoint{};
    YiCadPoint2d m_alignmentPoint{};
    double m_height = 0.0;
    double m_rotation = 0.0;
    double m_widthFactor = 1.0;
    double m_obliqueAngle = 0.0;
    YiCadTextHorizontalAlignment m_horizontalAlignment = YICAD_TEXT_ALIGN_LEFT;
    YiCadTextVerticalAlignment m_verticalAlignment = YICAD_TEXT_ALIGN_BASELINE;
    ImportResource m_textStyle;
    EntityAttributes m_attributes;
};

/** @brief 插件侧拥有 UTF-8 格式串和可选背景的多行文字输入。 */
class MTextData
{
public:
    explicit MTextData(std::string contents = {}) : m_contents(std::move(contents)) {}

    MTextData& setPlacement(YiCadPoint2d insertionPoint,
        YiCadVector2d direction = {1.0, 0.0}) noexcept
    {
        m_insertionPoint = insertionPoint; m_direction = direction; return *this;
    }
    MTextData& setLayout(double characterHeight, double rectangleWidth,
        double lineSpacingFactor, YiCadMTextAttachment attachment) noexcept
    {
        m_characterHeight = characterHeight; m_rectangleWidth = rectangleWidth;
        m_lineSpacingFactor = lineSpacingFactor; m_attachment = attachment;
        return *this;
    }
    MTextData& setStyle(const ImportResource& value) noexcept
    {
        m_textStyle = value; return *this;
    }
    MTextData& setAttributes(EntityAttributes value) noexcept
    {
        m_attributes = std::move(value); return *this;
    }
    MTextData& setBackground(bool useDrawingBackgroundColor,
        YiCadColorData color, double borderScaleFactor) noexcept
    {
        m_background = makeImportData<YiCadMTextBackgroundData>();
        m_background->useDrawingBackgroundColor = useDrawingBackgroundColor ? 1U : 0U;
        m_background->color = color;
        m_background->borderScaleFactor = borderScaleFactor;
        return *this;
    }
    MTextData& clearBackground() noexcept { m_background.reset(); return *this; }

private:
    friend class ImportContainer;

    YiCadImportResult makeAbi(YiCadMTextDataV3& data) const noexcept
    {
        if (!detail::validString(m_contents, true))
        {
            return YICAD_IMPORT_ERROR_INVALID_ARGUMENT;
        }
        data = makeImportData<YiCadMTextDataV3>();
        data.attributes = &m_attributes.m_data;
        data.contents = detail::stringView(m_contents);
        data.insertionPoint = m_insertionPoint; data.direction = m_direction;
        data.characterHeight = m_characterHeight;
        data.rectangleWidth = m_rectangleWidth;
        data.lineSpacingFactor = m_lineSpacingFactor;
        data.attachment = m_attachment;
        data.textStyle = m_textStyle.nativeHandle();
        data.background = m_background ? &*m_background : nullptr;
        return YICAD_IMPORT_SUCCESS;
    }

    std::string m_contents;
    YiCadPoint2d m_insertionPoint{};
    YiCadVector2d m_direction{1.0, 0.0};
    double m_characterHeight = 0.0;
    double m_rectangleWidth = 0.0;
    double m_lineSpacingFactor = 1.0;
    YiCadMTextAttachment m_attachment = YICAD_MTEXT_TOP_LEFT;
    ImportResource m_textStyle;
    EntityAttributes m_attributes;
    std::optional<YiCadMTextBackgroundData> m_background;
};

/** @brief 插件侧拥有块名称、说明和外部引用路径的块定义。 */
class BlockData
{
public:
    explicit BlockData(std::string name = {}) : m_name(std::move(name)) {}

    BlockData& setBasePoint(YiCadPoint2d value) noexcept { m_basePoint = value; return *this; }
    BlockData& setFlags(uint32_t value) noexcept { m_flags = value; return *this; }
    BlockData& setDescription(std::string value)
    {
        m_description = std::move(value); return *this;
    }
    BlockData& setExternalReferencePath(std::string value)
    {
        m_externalReferencePath = std::move(value); return *this;
    }

private:
    friend class ImportSession;

    YiCadImportResult makeAbi(YiCadBlockDataV3& data) const noexcept
    {
        if (!detail::validString(m_name, true) ||
            !detail::validString(m_description, false) ||
            !detail::validString(m_externalReferencePath, false))
        {
            return YICAD_IMPORT_ERROR_INVALID_ARGUMENT;
        }
        data = makeImportData<YiCadBlockDataV3>();
        data.name = detail::stringView(m_name); data.basePoint = m_basePoint;
        data.flags = m_flags; data.description = detail::stringView(m_description);
        data.externalReferencePath = detail::stringView(m_externalReferencePath);
        return YICAD_IMPORT_SUCCESS;
    }

    std::string m_name;
    YiCadPoint2d m_basePoint{};
    uint32_t m_flags = 0;
    std::string m_description;
    std::string m_externalReferencePath;
};

/** @brief 插件侧保存块引用资源与变换参数的块引用输入。 */
class InsertData
{
public:
    explicit InsertData(const ImportResource& block = {}) : m_block(block) {}

    InsertData& setPlacement(YiCadPoint2d point, YiCadVector3d scale = {1.0, 1.0, 1.0},
        double rotation = 0.0) noexcept
    {
        m_insertionPoint = point; m_scale = scale; m_rotation = rotation; return *this;
    }
    InsertData& setArray(uint32_t columns, uint32_t rows,
        double columnSpacing, double rowSpacing) noexcept
    {
        m_columnCount = columns; m_rowCount = rows;
        m_columnSpacing = columnSpacing; m_rowSpacing = rowSpacing; return *this;
    }
    InsertData& setAttributes(EntityAttributes value) noexcept
    {
        m_attributes = std::move(value); return *this;
    }

private:
    friend class ImportContainer;

    YiCadImportResult makeAbi(YiCadInsertDataV3& data) const noexcept
    {
        const auto block = m_block.nativeHandle();
        if (block == nullptr)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        data = makeImportData<YiCadInsertDataV3>();
        data.attributes = &m_attributes.m_data; data.block = block;
        data.insertionPoint = m_insertionPoint; data.scale = m_scale;
        data.rotation = m_rotation; data.columnCount = m_columnCount;
        data.rowCount = m_rowCount; data.columnSpacing = m_columnSpacing;
        data.rowSpacing = m_rowSpacing; return YICAD_IMPORT_SUCCESS;
    }

    ImportResource m_block;
    EntityAttributes m_attributes;
    YiCadPoint2d m_insertionPoint{};
    YiCadVector3d m_scale{1.0, 1.0, 1.0};
    double m_rotation = 0.0;
    uint32_t m_columnCount = 1;
    uint32_t m_rowCount = 1;
    double m_columnSpacing = 0.0;
    double m_rowSpacing = 0.0;
};

/** @brief 插件侧拥有文字、标记、提示和默认值的属性定义输入。 */
class AttributeDefinitionData
{
public:
    AttributeDefinitionData(TextData text = TextData{}, std::string tag = {})
        : m_text(std::move(text)), m_tag(std::move(tag)) {}

    AttributeDefinitionData& setPrompt(std::string value)
    {
        m_prompt = std::move(value); return *this;
    }
    AttributeDefinitionData& setDefaultValue(std::string value)
    {
        m_defaultValue = std::move(value); return *this;
    }
    AttributeDefinitionData& setFlags(uint32_t value) noexcept
    {
        m_flags = value; return *this;
    }

private:
    friend class ImportContainer;

    YiCadImportResult makeAbi(YiCadAttributeDefinitionDataV3& data,
        YiCadTextDataV3& text) const noexcept
    {
        auto result = m_text.makeAbi(text);
        if (result != YICAD_IMPORT_SUCCESS)
        {
            return result;
        }
        if (!detail::validString(m_tag, true) ||
            !detail::validString(m_prompt, false) ||
            !detail::validString(m_defaultValue, false))
        {
            return YICAD_IMPORT_ERROR_INVALID_ARGUMENT;
        }
        data = makeImportData<YiCadAttributeDefinitionDataV3>();
        data.text = &text; data.tag = detail::stringView(m_tag);
        data.prompt = detail::stringView(m_prompt);
        data.defaultValue = detail::stringView(m_defaultValue);
        data.flags = m_flags; return YICAD_IMPORT_SUCCESS;
    }

    TextData m_text;
    std::string m_tag;
    std::string m_prompt;
    std::string m_defaultValue;
    uint32_t m_flags = 0;
};

/** @brief 插件侧拥有文字、标记和值的块引用属性输入。 */
class AttributeData
{
public:
    AttributeData(TextData text = TextData{}, const ImportResource& insert = {},
        std::string tag = {}, std::string value = {})
        : m_text(std::move(text)), m_insert(insert), m_tag(std::move(tag)),
          m_value(std::move(value)) {}

    AttributeData& setFlags(uint32_t value) noexcept { m_flags = value; return *this; }

private:
    friend class ImportContainer;

    YiCadImportResult makeAbi(YiCadAttributeDataV3& data,
        YiCadTextDataV3& text) const noexcept
    {
        auto result = m_text.makeAbi(text);
        if (result != YICAD_IMPORT_SUCCESS)
        {
            return result;
        }
        const auto insert = m_insert.nativeHandle();
        if (insert == nullptr)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        if (!detail::validString(m_tag, true) ||
            !detail::validString(m_value, false))
        {
            return YICAD_IMPORT_ERROR_INVALID_ARGUMENT;
        }
        data = makeImportData<YiCadAttributeDataV3>();
        data.text = &text; data.insert = insert;
        data.tag = detail::stringView(m_tag); data.value = detail::stringView(m_value);
        data.flags = m_flags; return YICAD_IMPORT_SUCCESS;
    }

    TextData m_text;
    ImportResource m_insert;
    std::string m_tag;
    std::string m_value;
    uint32_t m_flags = 0;
};

/** @brief 插件侧拥有文字覆盖并保留全部定义点的语义标注输入。 */
class DimensionData
{
public:
    explicit DimensionData(YiCadDimensionKind kind = YICAD_DIMENSION_LINEAR)
        : m_kind(kind) {}

    DimensionData& setStyle(const ImportResource& value) noexcept
    {
        m_dimensionStyle = value; return *this;
    }
    DimensionData& setText(std::string overrideText, YiCadPoint2d position,
        double rotation = 0.0, double lineSpacingFactor = 1.0)
    {
        m_textOverride = std::move(overrideText); m_textPosition = position;
        m_textRotation = rotation; m_lineSpacingFactor = lineSpacingFactor;
        return *this;
    }
    DimensionData& setDefinitionPoints(YiCadPoint2d definitionPoint,
        YiCadPoint2d extensionPoint1, YiCadPoint2d extensionPoint2) noexcept
    {
        m_definitionPoint = definitionPoint; m_extensionPoint1 = extensionPoint1;
        m_extensionPoint2 = extensionPoint2; return *this;
    }
    DimensionData& setAngularLines(YiCadPoint2d line1Start, YiCadPoint2d line1End,
        YiCadPoint2d line2Start, YiCadPoint2d line2End,
        YiCadPoint2d arcPoint) noexcept
    {
        m_line1Start = line1Start; m_line1End = line1End;
        m_line2Start = line2Start; m_line2End = line2End;
        m_arcPoint = arcPoint; return *this;
    }
    DimensionData& setFeaturePoint(YiCadPoint2d value,
        double leaderLength = 0.0) noexcept
    {
        m_featurePoint = value; m_leaderLength = leaderLength; return *this;
    }
    DimensionData& setAttributes(EntityAttributes value) noexcept
    {
        m_attributes = std::move(value); return *this;
    }

private:
    friend class ImportContainer;

    YiCadImportResult makeAbi(YiCadDimensionDataV3& data) const noexcept
    {
        if (!detail::validString(m_textOverride, false))
        {
            return YICAD_IMPORT_ERROR_INVALID_ARGUMENT;
        }
        data = makeImportData<YiCadDimensionDataV3>();
        data.attributes = &m_attributes.m_data; data.kind = m_kind;
        data.dimensionStyle = m_dimensionStyle.nativeHandle();
        data.textOverride = detail::stringView(m_textOverride);
        data.definitionPoint = m_definitionPoint; data.textPosition = m_textPosition;
        data.textRotation = m_textRotation;
        data.lineSpacingFactor = m_lineSpacingFactor;
        data.extensionPoint1 = m_extensionPoint1;
        data.extensionPoint2 = m_extensionPoint2;
        data.line1Start = m_line1Start; data.line1End = m_line1End;
        data.line2Start = m_line2Start; data.line2End = m_line2End;
        data.arcPoint = m_arcPoint; data.featurePoint = m_featurePoint;
        data.leaderLength = m_leaderLength; return YICAD_IMPORT_SUCCESS;
    }

    YiCadDimensionKind m_kind;
    ImportResource m_dimensionStyle;
    std::string m_textOverride;
    YiCadPoint2d m_definitionPoint{};
    YiCadPoint2d m_textPosition{};
    double m_textRotation = 0.0;
    double m_lineSpacingFactor = 1.0;
    YiCadPoint2d m_extensionPoint1{};
    YiCadPoint2d m_extensionPoint2{};
    YiCadPoint2d m_line1Start{};
    YiCadPoint2d m_line1End{};
    YiCadPoint2d m_line2Start{};
    YiCadPoint2d m_line2End{};
    YiCadPoint2d m_arcPoint{};
    YiCadPoint2d m_featurePoint{};
    double m_leaderLength = 0.0;
    EntityAttributes m_attributes;
};

/** @brief 插件侧拥有顶点数组和可选文字的引线输入。 */
class LeaderData
{
public:
    explicit LeaderData(std::vector<YiCadPoint2d> vertices = {})
        : m_vertices(std::move(vertices)) {}

    LeaderData& setArrow(bool value) noexcept { m_hasArrow = value; return *this; }
    LeaderData& setStyle(const ImportResource& value) noexcept
    {
        m_dimensionStyle = value; return *this;
    }
    LeaderData& setText(TextData value)
    {
        m_text = std::move(value); return *this;
    }
    LeaderData& clearText() noexcept { m_text.reset(); return *this; }
    LeaderData& setAttributes(EntityAttributes value) noexcept
    {
        m_attributes = std::move(value); return *this;
    }

private:
    friend class ImportContainer;

    YiCadImportResult makeAbi(YiCadLeaderDataV3& data,
        YiCadTextDataV3& text) const noexcept
    {
        if (m_vertices.size() < 2 || !detail::fitsAbiCount(m_vertices.size()))
        {
            return YICAD_IMPORT_ERROR_INVALID_ARGUMENT;
        }
        if (m_text)
        {
            const auto result = m_text->makeAbi(text);
            if (result != YICAD_IMPORT_SUCCESS)
            {
                return result;
            }
        }
        data = makeImportData<YiCadLeaderDataV3>();
        data.attributes = &m_attributes.m_data;
        data.vertices = detail::arrayView(m_vertices);
        data.hasArrow = m_hasArrow ? 1U : 0U;
        data.dimensionStyle = m_dimensionStyle.nativeHandle();
        data.text = m_text ? &text : nullptr; return YICAD_IMPORT_SUCCESS;
    }

    std::vector<YiCadPoint2d> m_vertices;
    bool m_hasArrow = false;
    ImportResource m_dimensionStyle;
    std::optional<TextData> m_text;
    EntityAttributes m_attributes;
};

/** @brief 插件侧拥有多环边界及所有嵌套数组的填充输入。 */
class HatchData
{
public:
    class Edge
    {
    public:
        static Edge line(YiCadPoint2d startPoint, YiCadPoint2d endPoint) noexcept
        {
            Edge edge; edge.m_type = YICAD_HATCH_EDGE_LINE;
            edge.m_startPoint = startPoint; edge.m_endPoint = endPoint; return edge;
        }
        static Edge circularArc(YiCadPoint2d center, double radius,
            double startParameter, double endParameter, bool counterClockwise) noexcept
        {
            Edge edge; edge.m_type = YICAD_HATCH_EDGE_CIRCULAR_ARC;
            edge.m_center = center; edge.m_radius = radius;
            edge.m_startParameter = startParameter; edge.m_endParameter = endParameter;
            edge.m_counterClockwise = counterClockwise; return edge;
        }
        static Edge ellipticArc(YiCadPoint2d center, YiCadVector2d majorAxis,
            double minorToMajorRatio, double startParameter, double endParameter,
            bool counterClockwise) noexcept
        {
            Edge edge; edge.m_type = YICAD_HATCH_EDGE_ELLIPTIC_ARC;
            edge.m_center = center; edge.m_majorAxis = majorAxis;
            edge.m_minorToMajorRatio = minorToMajorRatio;
            edge.m_startParameter = startParameter; edge.m_endParameter = endParameter;
            edge.m_counterClockwise = counterClockwise; return edge;
        }
        static Edge spline(uint32_t degree, std::vector<YiCadPoint2d> controlPoints,
            std::vector<double> knots, std::vector<double> weights = {},
            bool rational = false, bool periodic = false)
        {
            Edge edge; edge.m_type = YICAD_HATCH_EDGE_SPLINE; edge.m_degree = degree;
            edge.m_controlPoints = std::move(controlPoints);
            edge.m_knots = std::move(knots); edge.m_weights = std::move(weights);
            edge.m_rational = rational; edge.m_periodic = periodic;
            return edge;
        }

    private:
        friend class HatchData;

        YiCadImportResult makeAbi(YiCadHatchEdgeDataV3& data) const noexcept
        {
            if (!detail::fitsAbiCount(m_controlPoints.size()) ||
                !detail::fitsAbiCount(m_knots.size()) ||
                !detail::fitsAbiCount(m_weights.size()))
            {
                return YICAD_IMPORT_ERROR_OUT_OF_RANGE;
            }
            if (m_type == YICAD_HATCH_EDGE_SPLINE && m_controlPoints.empty())
            {
                return YICAD_IMPORT_ERROR_INVALID_ARGUMENT;
            }
            data = makeImportData<YiCadHatchEdgeDataV3>();
            data.type = m_type; data.startPoint = m_startPoint;
            data.endPoint = m_endPoint; data.center = m_center;
            data.majorAxis = m_majorAxis; data.radius = m_radius;
            data.minorToMajorRatio = m_minorToMajorRatio;
            data.startParameter = m_startParameter;
            data.endParameter = m_endParameter;
            data.counterClockwise = m_counterClockwise ? 1U : 0U;
            data.degree = m_degree;
            data.rational = m_rational ? 1U : 0U;
            data.periodic = m_periodic ? 1U : 0U;
            data.controlPoints = detail::arrayView(m_controlPoints);
            data.knots = detail::arrayView(m_knots);
            data.weights = detail::arrayView(m_weights);
            return YICAD_IMPORT_SUCCESS;
        }

        YiCadHatchEdgeType m_type = YICAD_HATCH_EDGE_LINE;
        YiCadPoint2d m_startPoint{};
        YiCadPoint2d m_endPoint{};
        YiCadPoint2d m_center{};
        YiCadVector2d m_majorAxis{};
        double m_radius = 0.0;
        double m_minorToMajorRatio = 0.0;
        double m_startParameter = 0.0;
        double m_endParameter = 0.0;
        bool m_counterClockwise = false;
        uint32_t m_degree = 0;
        bool m_rational = false;
        bool m_periodic = false;
        std::vector<YiCadPoint2d> m_controlPoints;
        std::vector<double> m_knots;
        std::vector<double> m_weights;
    };

    HatchData& setPattern(std::string name, double scale = 1.0,
        double angle = 0.0)
    {
        m_solid = false; m_patternName = std::move(name);
        m_patternScale = scale; m_patternAngle = angle; return *this;
    }
    HatchData& setSolid(bool value = true) noexcept
    {
        m_solid = value; return *this;
    }
    HatchData& addPolylineLoop(std::vector<YiCadVertex2d> vertices,
        YiCadHatchLoopRole role = YICAD_HATCH_LOOP_OUTER,
        uint32_t outerLoopIndex = UINT32_MAX)
    {
        Loop loop; loop.kind = YICAD_HATCH_LOOP_POLYLINE; loop.role = role;
        loop.outerLoopIndex = outerLoopIndex; loop.vertices = std::move(vertices);
        m_loops.push_back(std::move(loop)); return *this;
    }
    HatchData& addEdgeLoop(std::vector<Edge> edges,
        YiCadHatchLoopRole role = YICAD_HATCH_LOOP_OUTER,
        uint32_t outerLoopIndex = UINT32_MAX)
    {
        Loop loop; loop.kind = YICAD_HATCH_LOOP_EDGES; loop.role = role;
        loop.outerLoopIndex = outerLoopIndex; loop.edges = std::move(edges);
        m_loops.push_back(std::move(loop)); return *this;
    }
    HatchData& setAttributes(EntityAttributes value) noexcept
    {
        m_attributes = std::move(value); return *this;
    }

private:
    friend class ImportContainer;

    struct Loop
    {
        YiCadHatchLoopKind kind = YICAD_HATCH_LOOP_POLYLINE;
        YiCadHatchLoopRole role = YICAD_HATCH_LOOP_OUTER;
        uint32_t outerLoopIndex = UINT32_MAX;
        std::vector<YiCadVertex2d> vertices;
        std::vector<Edge> edges;
    };

    struct Scratch
    {
        std::vector<std::vector<YiCadHatchEdgeDataV3>> edges;
        std::vector<YiCadHatchLoopDataV3> loops;
    };

    YiCadImportResult makeAbi(YiCadHatchDataV3& data, Scratch& scratch) const
    {
        if (!detail::validString(m_patternName, !m_solid) || m_loops.empty() ||
            !detail::fitsAbiCount(m_loops.size()))
        {
            return YICAD_IMPORT_ERROR_INVALID_ARGUMENT;
        }
        scratch.edges.resize(m_loops.size());
        scratch.loops.resize(m_loops.size());
        for (std::size_t index = 0; index < m_loops.size(); ++index)
        {
            const auto& source = m_loops[index];
            if (source.kind == YICAD_HATCH_LOOP_POLYLINE)
            {
                if (source.vertices.size() < 3 ||
                    !detail::fitsAbiCount(source.vertices.size()))
                {
                    return YICAD_IMPORT_ERROR_INVALID_ARGUMENT;
                }
            }
            else
            {
                if (source.edges.empty() || !detail::fitsAbiCount(source.edges.size()))
                {
                    return YICAD_IMPORT_ERROR_INVALID_ARGUMENT;
                }
                auto& edges = scratch.edges[index];
                edges.resize(source.edges.size());
                for (std::size_t edgeIndex = 0; edgeIndex < source.edges.size(); ++edgeIndex)
                {
                    const auto result = source.edges[edgeIndex].makeAbi(edges[edgeIndex]);
                    if (result != YICAD_IMPORT_SUCCESS)
                    {
                        return result;
                    }
                }
            }
            auto loop = makeImportData<YiCadHatchLoopDataV3>();
            loop.kind = source.kind; loop.role = source.role;
            loop.outerLoopIndex = source.outerLoopIndex;
            loop.polylineVertices = detail::arrayView(source.vertices);
            loop.edges = makeHatchEdgeArrayView(scratch.edges[index]);
            scratch.loops[index] = loop;
        }
        data = makeImportData<YiCadHatchDataV3>();
        data.attributes = &m_attributes.m_data;
        data.solid = m_solid ? 1U : 0U;
        data.patternName = detail::stringView(m_patternName);
        data.patternScale = m_patternScale; data.patternAngle = m_patternAngle;
        data.loops = makeHatchLoopArrayView(scratch.loops);
        return YICAD_IMPORT_SUCCESS;
    }

    bool m_solid = false;
    std::string m_patternName;
    double m_patternScale = 1.0;
    double m_patternAngle = 0.0;
    std::vector<Loop> m_loops;
    EntityAttributes m_attributes;
};

/** @brief 插件侧拥有图像路径和可选裁剪边界的图像引用输入。 */
class ImageData
{
public:
    explicit ImageData(std::string path = {}) : m_path(std::move(path)) {}

    ImageData& setGeometry(YiCadPoint2d insertionPoint, YiCadVector2d uVector,
        YiCadVector2d vVector, YiCadVector2d size) noexcept
    {
        m_insertionPoint = insertionPoint; m_uVector = uVector;
        m_vVector = vVector; m_size = size; return *this;
    }
    ImageData& setDisplay(int32_t brightness, int32_t contrast, int32_t fade) noexcept
    {
        m_brightness = brightness; m_contrast = contrast; m_fade = fade; return *this;
    }
    ImageData& setClipBoundary(std::vector<YiCadPoint2d> value)
    {
        m_clipBoundary = std::move(value); return *this;
    }
    ImageData& setAttributes(EntityAttributes value) noexcept
    {
        m_attributes = std::move(value); return *this;
    }

private:
    friend class ImportContainer;

    YiCadImportResult makeAbi(YiCadImageDataV3& data) const noexcept
    {
        if (!detail::validString(m_path, true) ||
            !detail::fitsAbiCount(m_clipBoundary.size()))
        {
            return YICAD_IMPORT_ERROR_INVALID_ARGUMENT;
        }
        data = makeImportData<YiCadImageDataV3>();
        data.attributes = &m_attributes.m_data; data.path = detail::stringView(m_path);
        data.insertionPoint = m_insertionPoint; data.uVector = m_uVector;
        data.vVector = m_vVector; data.size = m_size;
        data.brightness = m_brightness; data.contrast = m_contrast; data.fade = m_fade;
        data.clipBoundary = detail::arrayView(m_clipBoundary);
        return YICAD_IMPORT_SUCCESS;
    }

    std::string m_path;
    YiCadPoint2d m_insertionPoint{};
    YiCadVector2d m_uVector{};
    YiCadVector2d m_vVector{};
    YiCadVector2d m_size{};
    int32_t m_brightness = 0;
    int32_t m_contrast = 0;
    int32_t m_fade = 0;
    std::vector<YiCadPoint2d> m_clipBoundary;
    EntityAttributes m_attributes;
};

/**
 * @brief 导入会话内的非拥有模型空间或块定义容器包装。
 * @note 容器只在所属会话内有效；块容器在 endBlock 成功后立即失效。
 * @note 创建函数返回宿主的确定结果码；无效包装返回 INVALID_HANDLE，截短子表或
 * 空函数指针返回 UNSUPPORTED。输入字符串、数组和嵌套结构指针只需保持到该函数返回。
 */
class ImportContainer
{
public:
    ImportContainer() noexcept = default;

    explicit operator bool() const noexcept
    {
        return m_state != nullptr && m_state->session != nullptr &&
               m_handle != nullptr;
    }

    /**
     * @brief 使用语义参数向容器添加点实体。
     * @note SDK 在调用期间生成 ABI POD 和公共属性指针。
     */
    YiCadImportResult createPoint(
        YiCadPoint2d position,
        const EntityAttributes& attributes = {}) const noexcept
    {
        auto data = makeImportData<YiCadPointDataV3>();
        data.attributes = &attributes.m_data;
        data.position = position;
        return createPoint(data);
    }

    /**
     * @brief 使用语义参数向容器添加线段实体。
     * @note SDK 在调用期间生成 ABI POD 和公共属性指针。
     */
    YiCadImportResult createLine(
        YiCadPoint2d startPoint,
        YiCadPoint2d endPoint,
        const EntityAttributes& attributes = {}) const noexcept
    {
        auto data = makeImportData<YiCadLineDataV3>();
        data.attributes = &attributes.m_data;
        data.startPoint = startPoint;
        data.endPoint = endPoint;
        return createLine(data);
    }

    /**
     * @brief 使用语义参数向容器添加圆实体。
     * @note SDK 在调用期间生成 ABI POD 和公共属性指针。
     */
    YiCadImportResult createCircle(
        YiCadPoint2d center,
        double radius,
        const EntityAttributes& attributes = {}) const noexcept
    {
        auto data = makeImportData<YiCadCircleDataV3>();
        data.attributes = &attributes.m_data;
        data.center = center;
        data.radius = radius;
        return createCircle(data);
    }

    YiCadImportResult createRay(YiCadPoint2d basePoint, YiCadVector2d direction,
        const EntityAttributes& attributes = {}) const noexcept
    {
        auto data = makeImportData<YiCadRayDataV3>();
        data.attributes = &attributes.m_data; data.basePoint = basePoint;
        data.direction = direction; return createRay(data);
    }

    YiCadImportResult createXLine(YiCadPoint2d basePoint, YiCadVector2d direction,
        const EntityAttributes& attributes = {}) const noexcept
    {
        auto data = makeImportData<YiCadXLineDataV3>();
        data.attributes = &attributes.m_data; data.basePoint = basePoint;
        data.direction = direction; return createXLine(data);
    }

    YiCadImportResult createArc(YiCadPoint2d center, double radius,
        double startAngle, double endAngle,
        const EntityAttributes& attributes = {}) const noexcept
    {
        auto data = makeImportData<YiCadArcDataV3>();
        data.attributes = &attributes.m_data; data.center = center;
        data.radius = radius; data.startAngle = startAngle; data.endAngle = endAngle;
        return createArc(data);
    }

    YiCadImportResult createEllipse(YiCadPoint2d center, YiCadVector2d majorAxis,
        double minorToMajorRatio, double startParameter, double endParameter,
        bool closed, const EntityAttributes& attributes = {}) const noexcept
    {
        auto data = makeImportData<YiCadEllipseDataV3>();
        data.attributes = &attributes.m_data; data.center = center;
        data.majorAxis = majorAxis; data.minorToMajorRatio = minorToMajorRatio;
        data.startParameter = startParameter; data.endParameter = endParameter;
        data.closed = closed ? 1U : 0U; return createEllipse(data);
    }

    YiCadImportResult createPolyline(const PolylineData& value) const noexcept
    {
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        YiCadPolylineDataV3 data{};
        const auto result = value.makeAbi(data);
        return result == YICAD_IMPORT_SUCCESS ? createPolyline(data) : result;
    }

    YiCadImportResult createSpline(const SplineData& value) const noexcept
    {
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        YiCadSplineDataV3 data{};
        const auto result = value.makeAbi(data);
        return result == YICAD_IMPORT_SUCCESS ? createSpline(data) : result;
    }

    YiCadImportResult createText(const TextData& value) const noexcept
    {
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        YiCadTextDataV3 data{};
        const auto result = value.makeAbi(data);
        return result == YICAD_IMPORT_SUCCESS ? createText(data) : result;
    }

    YiCadImportResult createMText(const MTextData& value) const noexcept
    {
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        YiCadMTextDataV3 data{};
        const auto result = value.makeAbi(data);
        return result == YICAD_IMPORT_SUCCESS ? createMText(data) : result;
    }

    YiCadImportResult createAttributeDefinition(
        const AttributeDefinitionData& value) const noexcept
    {
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        YiCadAttributeDefinitionDataV3 data{};
        YiCadTextDataV3 text{};
        const auto result = value.makeAbi(data, text);
        return result == YICAD_IMPORT_SUCCESS
            ? createAttributeDefinition(data) : result;
    }

    YiCadImportResult createAttribute(const AttributeData& value) const noexcept
    {
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        YiCadAttributeDataV3 data{};
        YiCadTextDataV3 text{};
        const auto result = value.makeAbi(data, text);
        return result == YICAD_IMPORT_SUCCESS ? createAttribute(data) : result;
    }

    YiCadImportResult createDimension(const DimensionData& value) const noexcept
    {
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        YiCadDimensionDataV3 data{};
        const auto result = value.makeAbi(data);
        return result == YICAD_IMPORT_SUCCESS ? createDimension(data) : result;
    }

    YiCadImportResult createLeader(const LeaderData& value) const noexcept
    {
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        YiCadLeaderDataV3 data{};
        YiCadTextDataV3 text{};
        const auto result = value.makeAbi(data, text);
        return result == YICAD_IMPORT_SUCCESS ? createLeader(data) : result;
    }

    YiCadImportResult createHatch(const HatchData& value) const noexcept
    {
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        try
        {
            YiCadHatchDataV3 data{};
            HatchData::Scratch scratch;
            const auto result = value.makeAbi(data, scratch);
            return result == YICAD_IMPORT_SUCCESS ? createHatch(data) : result;
        }
        catch (...)
        {
            return YICAD_IMPORT_ERROR_OUT_OF_MEMORY;
        }
    }

    YiCadImportResult createImage(const ImageData& value) const noexcept
    {
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        YiCadImageDataV3 data{};
        const auto result = value.makeAbi(data);
        return result == YICAD_IMPORT_SUCCESS ? createImage(data) : result;
    }

    /// @brief 使用底层 ABI POD 向容器添加点实体。
    YiCadImportResult createPoint(const YiCadPointDataV3& data) const noexcept
    {
        return createEntity(data, offsetof(YiCadImportApi, createPoint),
            sizeof(((YiCadImportApi*)nullptr)->createPoint),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createPoint)
                ? m_state->api->createPoint
                : nullptr);
    }

    /// @brief 使用底层 ABI POD 向容器添加线段实体。
    YiCadImportResult createLine(const YiCadLineDataV3& data) const noexcept
    {
        return createEntity(data, offsetof(YiCadImportApi, createLine),
            sizeof(((YiCadImportApi*)nullptr)->createLine),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createLine)
                ? m_state->api->createLine
                : nullptr);
    }

    /// @brief 向容器添加射线实体。
    YiCadImportResult createRay(const YiCadRayDataV3& data) const noexcept
    {
        return createEntity(data, offsetof(YiCadImportApi, createRay),
            sizeof(((YiCadImportApi*)nullptr)->createRay),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createRay)
                ? m_state->api->createRay
                : nullptr);
    }

    /// @brief 向容器添加无限长线实体。
    YiCadImportResult createXLine(const YiCadXLineDataV3& data) const noexcept
    {
        return createEntity(data, offsetof(YiCadImportApi, createXLine),
            sizeof(((YiCadImportApi*)nullptr)->createXLine),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createXLine)
                ? m_state->api->createXLine
                : nullptr);
    }

    /// @brief 向容器添加圆弧实体。
    YiCadImportResult createArc(const YiCadArcDataV3& data) const noexcept
    {
        return createEntity(data, offsetof(YiCadImportApi, createArc),
            sizeof(((YiCadImportApi*)nullptr)->createArc),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createArc)
                ? m_state->api->createArc
                : nullptr);
    }

    /// @brief 使用底层 ABI POD 向容器添加圆实体。
    YiCadImportResult createCircle(const YiCadCircleDataV3& data) const noexcept
    {
        return createEntity(data, offsetof(YiCadImportApi, createCircle),
            sizeof(((YiCadImportApi*)nullptr)->createCircle),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createCircle)
                ? m_state->api->createCircle
                : nullptr);
    }

    /// @brief 向容器添加椭圆或椭圆弧实体。
    YiCadImportResult createEllipse(
        const YiCadEllipseDataV3& data) const noexcept
    {
        return createEntity(data, offsetof(YiCadImportApi, createEllipse),
            sizeof(((YiCadImportApi*)nullptr)->createEllipse),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createEllipse)
                ? m_state->api->createEllipse
                : nullptr);
    }

    /// @brief 向容器添加二维多段线实体。
    YiCadImportResult createPolyline(
        const YiCadPolylineDataV3& data) const noexcept
    {
        return createEntity(data, offsetof(YiCadImportApi, createPolyline),
            sizeof(((YiCadImportApi*)nullptr)->createPolyline),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createPolyline)
                ? m_state->api->createPolyline
                : nullptr);
    }

    /// @brief 向容器添加非有理、非周期样条实体。
    YiCadImportResult createSpline(
        const YiCadSplineDataV3& data) const noexcept
    {
        return createEntity(data, offsetof(YiCadImportApi, createSpline),
            sizeof(((YiCadImportApi*)nullptr)->createSpline),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createSpline)
                ? m_state->api->createSpline
                : nullptr);
    }

    /// @brief 向容器添加单行文字实体。
    YiCadImportResult createText(const YiCadTextDataV3& data) const noexcept
    {
        return createEntity(data, offsetof(YiCadImportApi, createText),
            sizeof(((YiCadImportApi*)nullptr)->createText),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createText)
                ? m_state->api->createText
                : nullptr);
    }

    /// @brief 向容器添加多行文字实体。
    YiCadImportResult createMText(const YiCadMTextDataV3& data) const noexcept
    {
        return createEntity(data, offsetof(YiCadImportApi, createMText),
            sizeof(((YiCadImportApi*)nullptr)->createMText),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createMText)
                ? m_state->api->createMText
                : nullptr);
    }

    /// @brief 向容器添加属性定义实体。
    YiCadImportResult createAttributeDefinition(
        const YiCadAttributeDefinitionDataV3& data) const noexcept
    {
        return createEntity(data,
            offsetof(YiCadImportApi, createAttributeDefinition),
            sizeof(((YiCadImportApi*)nullptr)->createAttributeDefinition),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createAttributeDefinition)
                ? m_state->api->createAttributeDefinition
                : nullptr);
    }

    /// @brief 向容器添加块引用属性实体。
    YiCadImportResult createAttribute(
        const YiCadAttributeDataV3& data) const noexcept
    {
        return createEntity(data, offsetof(YiCadImportApi, createAttribute),
            sizeof(((YiCadImportApi*)nullptr)->createAttribute),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createAttribute)
                ? m_state->api->createAttribute
                : nullptr);
    }

    /// @brief 向容器添加语义标注实体。
    YiCadImportResult createDimension(
        const YiCadDimensionDataV3& data) const noexcept
    {
        return createEntity(data, offsetof(YiCadImportApi, createDimension),
            sizeof(((YiCadImportApi*)nullptr)->createDimension),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createDimension)
                ? m_state->api->createDimension
                : nullptr);
    }

    /// @brief 向容器添加引线实体。
    YiCadImportResult createLeader(
        const YiCadLeaderDataV3& data) const noexcept
    {
        return createEntity(data, offsetof(YiCadImportApi, createLeader),
            sizeof(((YiCadImportApi*)nullptr)->createLeader),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createLeader)
                ? m_state->api->createLeader
                : nullptr);
    }

    /// @brief 向容器添加填充实体。
    YiCadImportResult createHatch(const YiCadHatchDataV3& data) const noexcept
    {
        return createEntity(data, offsetof(YiCadImportApi, createHatch),
            sizeof(((YiCadImportApi*)nullptr)->createHatch),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createHatch)
                ? m_state->api->createHatch
                : nullptr);
    }

    /// @brief 向容器添加图像引用实体。
    YiCadImportResult createImage(const YiCadImageDataV3& data) const noexcept
    {
        return createEntity(data, offsetof(YiCadImportApi, createImage),
            sizeof(((YiCadImportApi*)nullptr)->createImage),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createImage)
                ? m_state->api->createImage
                : nullptr);
    }

    /// @brief 创建块引用，并返回可供属性值引用的非拥有资源包装。
    YiCadImportResult createInsert(
        const InsertData& value,
        ImportResource& insert) const noexcept
    {
        insert = {};
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        YiCadInsertDataV3 data{};
        const auto result = value.makeAbi(data);
        return result == YICAD_IMPORT_SUCCESS ? createInsert(data, insert) : result;
    }

    /// @brief 使用底层 ABI POD 创建块引用。
    YiCadImportResult createInsert(
        const YiCadInsertDataV3& data,
        ImportResource& insert) const noexcept
    {
        insert = {};
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        const auto* api = m_state->api;
        if (!YICAD_SDK_HAS_IMPORT_FUNCTION(api, createInsert))
        {
            return YICAD_IMPORT_ERROR_UNSUPPORTED;
        }
        YiCadImportResourceHandle handle = nullptr;
        const auto result = detail::callImport([&]() {
            return api->createInsert(
                m_state->session, m_handle, &data, &handle);
        });
        if (result == YICAD_IMPORT_SUCCESS && handle != nullptr)
        {
            insert = ImportResource(m_state, handle);
        }
        return result;
    }

    /// @brief 结束块定义并消费当前块容器；模型空间容器不支持此操作。
    YiCadImportResult endBlock() noexcept
    {
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        const auto* api = m_state->api;
        if (!YICAD_SDK_HAS_IMPORT_FUNCTION(api, endBlock))
        {
            return YICAD_IMPORT_ERROR_UNSUPPORTED;
        }
        const auto handle = m_handle;
        const auto result = detail::callImport(
            [&]() { return api->endBlock(m_state->session, handle); });
        if (result == YICAD_IMPORT_SUCCESS)
        {
            m_handle = nullptr;
        }
        return result;
    }

private:
    friend class ImportSession;

    ImportContainer(
        std::shared_ptr<detail::ImportState> state,
        YiCadImportContainerHandle handle) noexcept
        : m_state(std::move(state)),
          m_handle(handle)
    {
    }

    template<typename Data, typename Function>
    YiCadImportResult createEntity(
        const Data& data,
        std::size_t,
        std::size_t,
        Function function) const noexcept
    {
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        if (function == nullptr)
        {
            return YICAD_IMPORT_ERROR_UNSUPPORTED;
        }
        return detail::callImport([&]() {
            return function(m_state->session, m_handle, &data);
        });
    }

    std::shared_ptr<detail::ImportState> m_state;
    YiCadImportContainerHandle m_handle = nullptr;
};

/**
 * @brief 不可复制、可移动的导入会话；析构时自动回滚未结束会话。
 * @note 仅允许在 YiCAD UI 主线程和创建该会话的文件导入回调内使用。
 * @note 包装方法返回宿主的确定结果码；无效包装返回 INVALID_HANDLE，截短子表或
 * 空函数指针返回 UNSUPPORTED。commit 和 rollback 都会消费会话及全部子句柄。
 */
class ImportSession
{
public:
    ImportSession() noexcept = default;
    ImportSession(const ImportSession&) = delete;
    ImportSession& operator=(const ImportSession&) = delete;
    ImportSession(ImportSession&&) noexcept = default;

    ImportSession& operator=(ImportSession&& other) noexcept
    {
        if (this != &other)
        {
            rollback();
            m_state = std::move(other.m_state);
        }
        return *this;
    }

    ~ImportSession()
    {
        rollback();
    }

    explicit operator bool() const noexcept
    {
        return m_state != nullptr && m_state->api != nullptr &&
               m_state->session != nullptr;
    }

    /// @brief 提交并消费会话句柄；失败时宿主自动回滚。
    YiCadImportResult commit() noexcept
    {
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        const auto* api = m_state->api;
        if (!YICAD_SDK_HAS_IMPORT_FUNCTION(api, commitImport))
        {
            return YICAD_IMPORT_ERROR_UNSUPPORTED;
        }
        const auto handle = std::exchange(m_state->session, nullptr);
        return detail::callImport([&]() { return api->commitImport(handle); });
    }

    /// @brief 回滚并消费会话句柄。
    YiCadImportResult rollback() noexcept
    {
        if (m_state == nullptr || m_state->session == nullptr)
        {
            return YICAD_IMPORT_SUCCESS;
        }
        const auto* api = m_state->api;
        if (!YICAD_SDK_HAS_IMPORT_FUNCTION(api, rollbackImport))
        {
            return YICAD_IMPORT_ERROR_UNSUPPORTED;
        }
        const auto handle = std::exchange(m_state->session, nullptr);
        return detail::callImport(
            [&]() { return api->rollbackImport(handle); });
    }

    /// @brief 读取最后一条导入错误，返回包含 NUL 的所需字节数。
    uint32_t lastError(char* buffer, uint32_t bufferSize) const noexcept
    {
        const auto* api = m_state != nullptr ? m_state->api : nullptr;
        if (!YICAD_SDK_HAS_IMPORT_FUNCTION(api, getLastError))
        {
            if (buffer != nullptr && bufferSize > 0)
            {
                buffer[0] = '\0';
            }
            return 1;
        }
        return invokeNoexcept<uint32_t>(
            [&]() { return api->getLastError(buffer, bufferSize); }, 1);
    }

    /// @brief 设置文档级导入元数据；输入字符串在返回前由宿主复制。
    YiCadImportResult setDocumentSettings(
        const DocumentSettings& value) const noexcept
    {
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        YiCadDocumentSettings data{};
        const auto result = value.makeAbi(data);
        return result == YICAD_IMPORT_SUCCESS ? setDocumentSettings(data) : result;
    }

    /// @brief 使用底层 ABI POD 设置文档级导入元数据。
    YiCadImportResult setDocumentSettings(
        const YiCadDocumentSettings& settings) const noexcept
    {
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        const auto* api = m_state->api;
        if (!YICAD_SDK_HAS_IMPORT_FUNCTION(api, setDocumentSettings))
        {
            return YICAD_IMPORT_ERROR_UNSUPPORTED;
        }
        return detail::callImport(
            [&]() { return api->setDocumentSettings(
                m_state->session, &settings); });
    }

    /// @brief 创建或解析简单线型资源。
    YiCadImportResult createLineType(
        const LineTypeData& value,
        YiCadResourceConflictPolicy conflictPolicy,
        ImportResource& resource) const noexcept
    {
        resource = {};
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        YiCadLineTypeDataV3 data{};
        const auto result = value.makeAbi(data);
        return result == YICAD_IMPORT_SUCCESS
            ? createLineType(data, conflictPolicy, resource) : result;
    }

    /// @brief 使用底层 ABI POD 创建或解析简单线型资源。
    YiCadImportResult createLineType(
        const YiCadLineTypeDataV3& data,
        YiCadResourceConflictPolicy conflictPolicy,
        ImportResource& resource) const noexcept
    {
        return createResource(data, conflictPolicy, resource,
            offsetof(YiCadImportApi, createLineType),
            sizeof(((YiCadImportApi*)nullptr)->createLineType),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createLineType)
                ? m_state->api->createLineType
                : nullptr);
    }

    /// @brief 创建或解析图层资源。
    YiCadImportResult createLayer(
        const LayerData& value,
        YiCadResourceConflictPolicy conflictPolicy,
        ImportResource& resource) const noexcept
    {
        resource = {};
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        YiCadLayerDataV3 data{};
        const auto result = value.makeAbi(data);
        return result == YICAD_IMPORT_SUCCESS
            ? createLayer(data, conflictPolicy, resource) : result;
    }

    /// @brief 使用底层 ABI POD 创建或解析图层资源。
    YiCadImportResult createLayer(
        const YiCadLayerDataV3& data,
        YiCadResourceConflictPolicy conflictPolicy,
        ImportResource& resource) const noexcept
    {
        return createResource(data, conflictPolicy, resource,
            offsetof(YiCadImportApi, createLayer),
            sizeof(((YiCadImportApi*)nullptr)->createLayer),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createLayer)
                ? m_state->api->createLayer
                : nullptr);
    }

    /// @brief 创建或解析文字样式资源。
    YiCadImportResult createTextStyle(
        const TextStyleData& value,
        YiCadResourceConflictPolicy conflictPolicy,
        ImportResource& resource) const noexcept
    {
        resource = {};
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        YiCadTextStyleDataV3 data{};
        const auto result = value.makeAbi(data);
        return result == YICAD_IMPORT_SUCCESS
            ? createTextStyle(data, conflictPolicy, resource) : result;
    }

    /// @brief 使用底层 ABI POD 创建或解析文字样式资源。
    YiCadImportResult createTextStyle(
        const YiCadTextStyleDataV3& data,
        YiCadResourceConflictPolicy conflictPolicy,
        ImportResource& resource) const noexcept
    {
        return createResource(data, conflictPolicy, resource,
            offsetof(YiCadImportApi, createTextStyle),
            sizeof(((YiCadImportApi*)nullptr)->createTextStyle),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createTextStyle)
                ? m_state->api->createTextStyle
                : nullptr);
    }

    /// @brief 创建或解析标注样式资源。
    YiCadImportResult createDimensionStyle(
        const DimensionStyleData& value,
        YiCadResourceConflictPolicy conflictPolicy,
        ImportResource& resource) const noexcept
    {
        resource = {};
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        YiCadDimensionStyleDataV3 data{};
        const auto result = value.makeAbi(data);
        return result == YICAD_IMPORT_SUCCESS
            ? createDimensionStyle(data, conflictPolicy, resource) : result;
    }

    /// @brief 使用底层 ABI POD 创建或解析标注样式资源。
    YiCadImportResult createDimensionStyle(
        const YiCadDimensionStyleDataV3& data,
        YiCadResourceConflictPolicy conflictPolicy,
        ImportResource& resource) const noexcept
    {
        return createResource(data, conflictPolicy, resource,
            offsetof(YiCadImportApi, createDimensionStyle),
            sizeof(((YiCadImportApi*)nullptr)->createDimensionStyle),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createDimensionStyle)
                ? m_state->api->createDimensionStyle
                : nullptr);
    }

    /// @brief 获取文档唯一模型空间容器。
    YiCadImportResult modelSpace(ImportContainer& container) const noexcept
    {
        container = {};
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        const auto* api = m_state->api;
        if (!YICAD_SDK_HAS_IMPORT_FUNCTION(api, getModelSpace))
        {
            return YICAD_IMPORT_ERROR_UNSUPPORTED;
        }
        YiCadImportContainerHandle handle = nullptr;
        const auto result = detail::callImport([&]() {
            return api->getModelSpace(m_state->session, &handle);
        });
        if (result == YICAD_IMPORT_SUCCESS && handle != nullptr)
        {
            container = ImportContainer(m_state, handle);
        }
        return result;
    }

    /// @brief 开始块定义并返回块资源与活动块容器。
    YiCadImportResult beginBlock(
        const BlockData& value,
        ImportResource& block,
        ImportContainer& container) const noexcept
    {
        block = {};
        container = {};
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        YiCadBlockDataV3 data{};
        const auto result = value.makeAbi(data);
        return result == YICAD_IMPORT_SUCCESS
            ? beginBlock(data, block, container) : result;
    }

    /// @brief 使用底层 ABI POD 开始块定义。
    YiCadImportResult beginBlock(
        const YiCadBlockDataV3& data,
        ImportResource& block,
        ImportContainer& container) const noexcept
    {
        block = {};
        container = {};
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        const auto* api = m_state->api;
        if (!YICAD_SDK_HAS_IMPORT_FUNCTION(api, beginBlock))
        {
            return YICAD_IMPORT_ERROR_UNSUPPORTED;
        }
        YiCadImportResourceHandle blockHandle = nullptr;
        YiCadImportContainerHandle containerHandle = nullptr;
        const auto result = detail::callImport([&]() {
            return api->beginBlock(m_state->session, &data,
                &blockHandle, &containerHandle);
        });
        if (result == YICAD_IMPORT_SUCCESS && blockHandle != nullptr &&
            containerHandle != nullptr)
        {
            block = ImportResource(m_state, blockHandle);
            container = ImportContainer(m_state, containerHandle);
        }
        return result;
    }

private:
    friend class Document;

    ImportSession(
        const YiCadImportApi* api,
        YiCadImportSessionHandle handle)
        : m_state(std::make_shared<detail::ImportState>(
              detail::ImportState{api, handle}))
    {
    }

    template<typename Data, typename Function>
    YiCadImportResult createResource(
        const Data& data,
        YiCadResourceConflictPolicy conflictPolicy,
        ImportResource& resource,
        std::size_t,
        std::size_t,
        Function function) const noexcept
    {
        resource = {};
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        if (function == nullptr)
        {
            return YICAD_IMPORT_ERROR_UNSUPPORTED;
        }
        YiCadImportResourceHandle handle = nullptr;
        const auto result = detail::callImport([&]() {
            return function(
                m_state->session, &data, conflictPolicy, &handle);
        });
        if (result == YICAD_IMPORT_SUCCESS && handle != nullptr)
        {
            resource = ImportResource(m_state, handle);
        }
        return result;
    }

    std::shared_ptr<detail::ImportState> m_state;
};

#undef YICAD_SDK_HAS_IMPORT_FUNCTION
#endif

/// @brief 宿主持有的不透明文档事务；析构时自动回滚未提交事务。
class DocumentTransaction
{
public:
    DocumentTransaction() noexcept = default;
    DocumentTransaction(const DocumentTransaction&) = delete;
    DocumentTransaction& operator=(const DocumentTransaction&) = delete;

    DocumentTransaction(DocumentTransaction&& other) noexcept
        : m_api(other.m_api),
          m_handle(other.m_handle)
    {
        other.m_api = nullptr;
        other.m_handle = nullptr;
    }

    DocumentTransaction& operator=(DocumentTransaction&& other) noexcept
    {
        if (this != &other)
        {
            if (!rollback())
            {
                return *this;
            }
            m_api = other.m_api;
            m_handle = other.m_handle;
            other.m_api = nullptr;
            other.m_handle = nullptr;
        }
        return *this;
    }

    ~DocumentTransaction()
    {
        rollback();
    }

    explicit operator bool() const noexcept
    {
        return m_api != nullptr && m_handle != nullptr;
    }

    bool commit() noexcept
    {
        if (!m_api || !m_handle || !m_api->documentCommitTransaction ||
            m_api->documentCommitTransaction(m_handle) != YICAD_SUCCESS)
        {
            return false;
        }
        m_handle = nullptr;
        return true;
    }

    bool rollback() noexcept
    {
        if (m_handle == nullptr)
        {
            return true;
        }
        if (m_api == nullptr || !m_api->documentRollbackTransaction ||
            m_api->documentRollbackTransaction(m_handle) != YICAD_SUCCESS)
        {
            return false;
        }
        m_handle = nullptr;
        return true;
    }

private:
    friend class Document;

    DocumentTransaction(
        const YiCadHostApi* api,
        YiCadTransactionHandle handle) noexcept
        : m_api(api),
          m_handle(handle)
    {
    }

    const YiCadHostApi* m_api = nullptr;
    YiCadTransactionHandle m_handle = nullptr;
};

/// @brief 只读实体快照迭代器；析构时将不透明句柄归还宿主。
class EntityIterator
{
public:
    EntityIterator() noexcept = default;
    EntityIterator(const EntityIterator&) = delete;
    EntityIterator& operator=(const EntityIterator&) = delete;

    EntityIterator(EntityIterator&& other) noexcept
        : m_api(other.m_api),
          m_handle(other.m_handle)
    {
        other.m_api = nullptr;
        other.m_handle = nullptr;
    }

    EntityIterator& operator=(EntityIterator&& other) noexcept
    {
        if (this != &other)
        {
            reset();
            m_api = other.m_api;
            m_handle = other.m_handle;
            other.m_api = nullptr;
            other.m_handle = nullptr;
        }
        return *this;
    }

    ~EntityIterator()
    {
        reset();
    }

    explicit operator bool() const noexcept
    {
        return m_api != nullptr && m_handle != nullptr;
    }

    bool next(YiCadEntityType& type) noexcept
    {
        return m_api && m_handle && m_api->entityIteratorNext &&
               m_api->entityIteratorNext(m_handle, &type) == YICAD_SUCCESS;
    }

    bool line(YiCadLineData& data) const noexcept
    {
        return m_api && m_handle && m_api->entityIteratorGetLine &&
               m_api->entityIteratorGetLine(m_handle, &data) ==
                   YICAD_SUCCESS;
    }

    bool circle(YiCadCircleData& data) const noexcept
    {
        return m_api && m_handle && m_api->entityIteratorGetCircle &&
               m_api->entityIteratorGetCircle(m_handle, &data) ==
                   YICAD_SUCCESS;
    }

private:
    friend class Document;

    EntityIterator(
        const YiCadHostApi* api,
        YiCadEntityIteratorHandle handle) noexcept
        : m_api(api),
          m_handle(handle)
    {
    }

    void reset() noexcept
    {
        if (m_api && m_handle && m_api->entityIteratorDestroy)
        {
            m_api->entityIteratorDestroy(m_handle);
        }
        m_handle = nullptr;
    }

    const YiCadHostApi* m_api = nullptr;
    YiCadEntityIteratorHandle m_handle = nullptr;
};

class Document
{
public:
    Document() noexcept = default;

    explicit operator bool() const noexcept
    {
        return hasField(
            offsetof(YiCadHostApi, abiVersion),
            sizeof(m_api->abiVersion));
    }

    bool addLine(
        double x1,
        double y1,
        double x2,
        double y2) const noexcept
    {
        return hasField(
                   offsetof(YiCadHostApi, documentAddLine),
                   sizeof(m_api->documentAddLine)) &&
               m_api->documentAddLine != nullptr &&
               m_api->documentAddLine(m_handle, x1, y1, x2, y2) ==
                   YICAD_SUCCESS;
    }

    bool addCircle(
        double centerX,
        double centerY,
        double radius) const noexcept
    {
        return hasField(
                   offsetof(YiCadHostApi, documentAddCircle),
                   sizeof(m_api->documentAddCircle)) &&
               m_api->documentAddCircle != nullptr &&
               m_api->documentAddCircle(
                   m_handle,
                   centerX,
                   centerY,
                   radius) == YICAD_SUCCESS;
    }

    bool regen() const noexcept
    {
        return hasField(
                   offsetof(YiCadHostApi, documentRegen),
                   sizeof(m_api->documentRegen)) &&
               m_api->documentRegen != nullptr &&
               m_api->documentRegen(m_handle) == YICAD_SUCCESS;
    }

    bool zoomAuto() const noexcept
    {
        return hasField(
                   offsetof(YiCadHostApi, documentZoomAuto),
                   sizeof(m_api->documentZoomAuto)) &&
               m_api->documentZoomAuto != nullptr &&
               m_api->documentZoomAuto(m_handle) == YICAD_SUCCESS;
    }

    /// @brief 开始一个整体可撤销的 ABI v2 文档事务。
    DocumentTransaction beginTransaction(const char* name) const noexcept
    {
        if (name == nullptr || *name == '\0' ||
            !hasV2Field(
                offsetof(YiCadHostApi, documentRollbackTransaction),
                sizeof(m_api->documentRollbackTransaction)) ||
            m_api->documentBeginTransaction == nullptr)
        {
            return {};
        }
        return DocumentTransaction(
            m_api, m_api->documentBeginTransaction(m_handle, name));
    }

    /// @brief 创建与文档后续修改无关的只读实体数据快照。
    EntityIterator entities() const noexcept
    {
        if (!hasV2Field(
                offsetof(YiCadHostApi, entityIteratorDestroy),
                sizeof(m_api->entityIteratorDestroy)) ||
            m_api->documentCreateEntityIterator == nullptr)
        {
            return {};
        }
        return EntityIterator(
            m_api, m_api->documentCreateEntityIterator(m_handle));
    }

#if defined(YICAD_ENABLE_PLUGIN_ABI_V3_DRAFT)
    /// @brief 判断当前协商宿主是否提供导入会话基础能力。
    bool supportsImport() const noexcept
    {
        return importApiForSession() != nullptr;
    }

    /// @brief 开始一个 ABI v3 草案导入会话。
    ImportSession beginImport() const noexcept
    {
        const auto* importApi = importApiForSession();
        if (importApi == nullptr)
        {
            return {};
        }

        YiCadImportSessionHandle session = nullptr;
        if (importApi->beginImport(m_handle, &session) !=
            YICAD_IMPORT_SUCCESS)
        {
            return {};
        }
        try
        {
            return ImportSession(importApi, session);
        }
        catch (...)
        {
            invokeNoexcept<YiCadImportResult>(
                [&]() { return importApi->rollbackImport(session); },
                YICAD_IMPORT_ERROR_TRANSACTION_FAILED);
            return {};
        }
    }

    /// @brief 读取最后一条导入错误，返回包含 NUL 的所需字节数。
    uint32_t importLastError(
        char* buffer,
        uint32_t bufferSize) const noexcept
    {
        const auto* importApi = importApiForSession();
        if (importApi == nullptr || importApi->getLastError == nullptr)
        {
            if (buffer != nullptr && bufferSize > 0)
            {
                buffer[0] = '\0';
            }
            return 1;
        }
        return importApi->getLastError(buffer, bufferSize);
    }
#endif

private:
    friend class Host;

    Document(
        const YiCadHostApi* api,
        YiCadDocumentHandle handle) noexcept
        : m_api(api),
          m_handle(handle)
    {
    }

    bool hasField(size_t offset, size_t size) const noexcept
    {
#if defined(YICAD_ENABLE_PLUGIN_ABI_V3_DRAFT)
        constexpr auto maximumVersion = YICAD_PLUGIN_ABI_V3_DRAFT;
#else
        constexpr auto maximumVersion = YICAD_PLUGIN_ABI_MAX_VERSION;
#endif
        return m_api != nullptr && m_handle != nullptr &&
               m_api->structSize >=
                   offsetof(YiCadHostApi, abiVersion) +
                       sizeof(m_api->abiVersion) &&
               m_api->abiVersion >= YICAD_PLUGIN_ABI_MIN_VERSION &&
               m_api->abiVersion <= maximumVersion &&
               m_api->structSize >= offset + size;
    }

    bool hasV2Field(size_t offset, size_t size) const noexcept
    {
        return hasField(offset, size) &&
               m_api->abiVersion >= YICAD_PLUGIN_ABI_V2;
    }

#if defined(YICAD_ENABLE_PLUGIN_ABI_V3_DRAFT)
    const YiCadImportApi* importApiForSession() const noexcept
    {
        if (!hasField(
                offsetof(YiCadHostApi, importApi),
                sizeof(m_api->importApi)) ||
            m_api->abiVersion < YICAD_PLUGIN_ABI_V3_DRAFT ||
            m_api->importApi == nullptr)
        {
            return nullptr;
        }

        const auto* importApi = m_api->importApi;
        const auto headerSize =
            offsetof(YiCadImportApi, abiVersion) +
            sizeof(importApi->abiVersion);
        const auto requiredSize =
            offsetof(YiCadImportApi, getLastError) +
            sizeof(importApi->getLastError);
        return importApi->structSize >= headerSize &&
               importApi->abiVersion >= YICAD_PLUGIN_ABI_V3_DRAFT &&
               importApi->structSize >= requiredSize &&
               importApi->beginImport != nullptr &&
               importApi->commitImport != nullptr &&
               importApi->rollbackImport != nullptr
            ? importApi
            : nullptr;
    }
#endif

    const YiCadHostApi* m_api = nullptr;
    YiCadDocumentHandle m_handle = nullptr;
};

class Host
{
public:
    explicit Host(const YiCadHostApi* api = nullptr) noexcept
        : m_api(api)
    {
    }

    explicit operator bool() const noexcept
    {
        return isCompatible();
    }

    void message(const char* text) const noexcept
    {
        if (text != nullptr &&
            hasField(
                offsetof(YiCadHostApi, message),
                sizeof(m_api->message)) &&
            m_api->message != nullptr)
        {
            m_api->message(text);
        }
    }

    bool registerCommand(
        const char* pluginId,
        const char* commandId,
        const char* displayName,
        YiCadCommandCallback callback,
        void* userData = nullptr) const noexcept
    {
        return pluginId != nullptr && commandId != nullptr &&
               displayName != nullptr && callback != nullptr &&
               hasField(
                   offsetof(YiCadHostApi, registerCommand),
                   sizeof(m_api->registerCommand)) &&
               m_api->registerCommand != nullptr &&
               m_api->registerCommand(
                   pluginId,
                   commandId,
                   displayName,
                   callback,
                   userData) == YICAD_SUCCESS;
    }

    bool registerRibbonButton(
        const char* pluginId,
        const char* tab,
        const char* group,
        const char* commandId,
        const char* iconPath) const noexcept
    {
        return pluginId != nullptr && tab != nullptr && group != nullptr &&
               commandId != nullptr && iconPath != nullptr &&
               hasField(
                   offsetof(YiCadHostApi, registerRibbonButton),
                   sizeof(m_api->registerRibbonButton)) &&
               m_api->registerRibbonButton != nullptr &&
               m_api->registerRibbonButton(
                   pluginId,
                   tab,
                   group,
                   commandId,
                   iconPath) == YICAD_SUCCESS;
    }

    bool registerImportFilter(
        const char* pluginId,
        const char* formatId,
        const char* displayName,
        const char* extension,
        YiCadImportCallback callback,
        void* userData = nullptr) const noexcept
    {
        return pluginId != nullptr && formatId != nullptr &&
               displayName != nullptr && extension != nullptr &&
               callback != nullptr &&
               hasField(
                   offsetof(YiCadHostApi, registerImportFilter),
                   sizeof(m_api->registerImportFilter)) &&
               m_api->registerImportFilter != nullptr &&
               m_api->registerImportFilter(
                   pluginId,
                   formatId,
                   displayName,
                   extension,
                   callback,
                   userData) == YICAD_SUCCESS;
    }

    bool registerExportFilter(
        const char* pluginId,
        const char* formatId,
        const char* displayName,
        const char* extension,
        YiCadExportCallback callback,
        void* userData = nullptr) const noexcept
    {
        return pluginId != nullptr && formatId != nullptr &&
               displayName != nullptr && extension != nullptr &&
               callback != nullptr &&
               hasField(
                   offsetof(YiCadHostApi, registerExportFilter),
                   sizeof(m_api->registerExportFilter)) &&
               m_api->registerExportFilter != nullptr &&
               m_api->registerExportFilter(
                   pluginId,
                   formatId,
                   displayName,
                   extension,
                   callback,
                   userData) == YICAD_SUCCESS;
    }

    Document currentDocument() const noexcept
    {
        if (!hasField(
                offsetof(YiCadHostApi, currentDocument),
                sizeof(m_api->currentDocument)) ||
            m_api->currentDocument == nullptr)
        {
            return {};
        }

        return Document(m_api, m_api->currentDocument());
    }

    /// @brief 将文件回调收到的文档句柄包装为 SDK 文档对象。
    Document document(YiCadDocumentHandle handle) const noexcept
    {
        return isCompatible() && handle != nullptr
            ? Document(m_api, handle)
            : Document();
    }

private:
    bool isCompatible() const noexcept
    {
#if defined(YICAD_ENABLE_PLUGIN_ABI_V3_DRAFT)
        constexpr auto maximumVersion = YICAD_PLUGIN_ABI_V3_DRAFT;
#else
        constexpr auto maximumVersion = YICAD_PLUGIN_ABI_MAX_VERSION;
#endif
        return m_api != nullptr &&
               m_api->structSize >=
                   offsetof(YiCadHostApi, abiVersion) +
                       sizeof(m_api->abiVersion) &&
               m_api->abiVersion >= YICAD_PLUGIN_ABI_MIN_VERSION &&
               m_api->abiVersion <= maximumVersion;
    }

    bool hasField(size_t offset, size_t size) const noexcept
    {
        return isCompatible() && m_api->structSize >= offset + size;
    }

    const YiCadHostApi* m_api = nullptr;
};

} // namespace yicad::plugin

#endif
