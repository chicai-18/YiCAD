#ifndef YICAD_PLUGIN_SDK_H
#define YICAD_PLUGIN_SDK_H

#include "YiCadPluginAbi.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <array>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
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
class SolidData;
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

inline std::string copyString(YiCadStringView value)
{
    return value.data == nullptr || value.size == 0
        ? std::string{}
        : std::string(value.data, value.size);
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
YICAD_SDK_DEFINE_ZERO_IMPORT_DATA(YiCadSolidDataV3)
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
    data.currentEntityLineTypeScale = 1.0;
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
 * @brief 创建已清零并带有完整 ABI 大小和协议默认值的输入 POD。
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
    (void)offset;
    (void)size;
    return api != nullptr && api->abiVersion == YICAD_PLUGIN_ABI_V4;
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

    /// @brief 设置实体线型比例（DXF 组码 48），必须大于 0。
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

    const YiCadEntityAttributes& abiData() const noexcept { return m_data; }
    const std::string& layerName() const noexcept { return m_layerName; }
    const std::string& lineTypeName() const noexcept { return m_lineTypeName; }

    static EntityAttributes fromAbi(
        const YiCadEntityAttributes* value,
        std::string layerName = {},
        std::string lineTypeName = {})
    {
        EntityAttributes result;
        if (value != nullptr)
        {
            result.m_data = *value;
        }
        result.m_layerName = std::move(layerName);
        result.m_lineTypeName = std::move(lineTypeName);
        return result;
    }

private:
    friend class ImportContainer;
    friend class PolylineData;
    friend class SplineData;
    friend class SolidData;
    friend class TextData;
    friend class MTextData;
    friend class InsertData;
    friend class DimensionData;
    friend class LeaderData;
    friend class HatchData;
    friend class ImageData;

    YiCadEntityAttributes m_data;
    std::string m_layerName;
    std::string m_lineTypeName;
};

struct PointData { YiCadPoint2d position{}; EntityAttributes attributes; };
struct LineData { YiCadPoint2d startPoint{}; YiCadPoint2d endPoint{}; EntityAttributes attributes; };
struct RayData { YiCadPoint2d basePoint{}; YiCadVector2d direction{}; EntityAttributes attributes; };
struct XLineData { YiCadPoint2d basePoint{}; YiCadVector2d direction{}; EntityAttributes attributes; };
struct ArcData { YiCadPoint2d center{}; double radius = 0.0; double startAngle = 0.0; double endAngle = 0.0; EntityAttributes attributes; };
struct CircleData { YiCadPoint2d center{}; double radius = 0.0; EntityAttributes attributes; };
struct EllipseData { YiCadPoint2d center{}; YiCadVector2d majorAxis{}; double minorToMajorRatio = 0.0; double startParameter = 0.0; double endParameter = 0.0; bool closed = false; EntityAttributes attributes; };

/** @brief 插件侧拥有字符串的文档设置。 */
class DocumentSettings
{
public:
    int32_t insertionUnits() const noexcept { return m_insertionUnits; }
    int32_t measurement() const noexcept { return m_measurement; }
    double globalLineTypeScale() const noexcept { return m_globalLineTypeScale; }
    /// @brief 新建实体的线型比例（CELTSCALE，v4）
    double currentEntityLineTypeScale() const noexcept { return m_currentEntityLineTypeScale; }
    const std::string& sourceCodePage() const noexcept { return m_sourceCodePage; }
    DocumentSettings& setCurrentEntityLineTypeScale(double value) noexcept
    {
        m_currentEntityLineTypeScale = value;
        return *this;
    }

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
        data.currentEntityLineTypeScale = m_currentEntityLineTypeScale;
        return YICAD_IMPORT_SUCCESS;
    }

    int32_t m_insertionUnits = 0;
    int32_t m_measurement = 0;
    double m_globalLineTypeScale = 1.0;
    double m_currentEntityLineTypeScale = 1.0;
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
    const std::string& name() const noexcept { return m_name; }
    const std::string& description() const noexcept { return m_description; }
    const std::vector<double>& elements() const noexcept { return m_elements; }
    bool complex() const noexcept { return m_complex; }

private:
    friend class ImportSession;

    YiCadImportResult makeAbi(YiCadLineTypeDataV3& data) const noexcept
    {
        if (!detail::validString(m_name, true) ||
            !detail::validString(m_description, false) ||
            !detail::fitsAbiCount(m_elements.size()))
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
    LayerData& setLineTypeName(std::string value)
    {
        m_lineTypeName = std::move(value);
        return *this;
    }
    const std::string& name() const noexcept { return m_name; }
    bool frozen() const noexcept { return m_frozen; }
    bool locked() const noexcept { return m_locked; }
    bool plottable() const noexcept { return m_plottable; }
    YiCadColorData color() const noexcept { return m_color; }
    int32_t lineWidth() const noexcept { return m_lineWidth; }
    const std::string& lineTypeName() const noexcept { return m_lineTypeName; }

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
    std::string m_lineTypeName;
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
    const std::string& name() const noexcept { return m_name; }
    const std::string& fontFile() const noexcept { return m_fontFile; }
    const std::string& bigFontFile() const noexcept { return m_bigFontFile; }
    double fixedHeight() const noexcept { return m_fixedHeight; }
    double widthFactor() const noexcept { return m_widthFactor; }
    double obliqueAngle() const noexcept { return m_obliqueAngle; }
    uint32_t generationFlags() const noexcept { return m_generationFlags; }

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
    const std::string& name() const noexcept { return m_name; }
    const YiCadDimensionStyleDataV3& abiData() const noexcept { return m_data; }
    DimensionStyleData& setResourceNames(std::string textStyle,
        std::string dimLineType, std::string extensionLineType)
    {
        m_textStyleName = std::move(textStyle);
        m_dimLineTypeName = std::move(dimLineType);
        m_extensionLineTypeName = std::move(extensionLineType);
        return *this;
    }
    const std::string& textStyleName() const noexcept { return m_textStyleName; }
    const std::string& dimLineTypeName() const noexcept { return m_dimLineTypeName; }
    const std::string& extensionLineTypeName() const noexcept { return m_extensionLineTypeName; }
    const std::string& prefix() const noexcept { return m_prefix; }
    const std::string& suffix() const noexcept { return m_suffix; }
    static DimensionStyleData fromAbi(const YiCadDimensionStyleDataV3& data)
    {
        DimensionStyleData result(detail::copyString(data.name));
        result.m_data = data;
        result.m_prefix = detail::copyString(data.prefix);
        result.m_suffix = detail::copyString(data.suffix);
        result.m_data.name = {};
        result.m_data.prefix = {};
        result.m_data.suffix = {};
        return result;
    }

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
    std::string m_textStyleName;
    std::string m_dimLineTypeName;
    std::string m_extensionLineTypeName;
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
    const std::vector<YiCadVertex2d>& vertices() const noexcept { return m_vertices; }
    bool closed() const noexcept { return m_closed; }
    const EntityAttributes& attributes() const noexcept { return m_attributes; }

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
    YiCadSplineDefinition definition() const noexcept { return m_definition; }
    uint32_t degree() const noexcept { return m_degree; }
    bool closed() const noexcept { return m_closed; }
    bool rational() const noexcept { return m_rational; }
    bool periodic() const noexcept { return m_periodic; }
    const std::vector<YiCadPoint2d>& controlPoints() const noexcept { return m_controlPoints; }
    const std::vector<YiCadPoint2d>& fitPoints() const noexcept { return m_fitPoints; }
    const std::vector<double>& knots() const noexcept { return m_knots; }
    const std::vector<double>& weights() const noexcept { return m_weights; }
    const EntityAttributes& attributes() const noexcept { return m_attributes; }

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

/** @brief 插件侧拥有三个或四个顶点的二维实体填充。 */
class SolidData
{
public:
    explicit SolidData(std::vector<YiCadPoint2d> corners = {})
        : m_corners(std::move(corners))
    {
    }

    SolidData& setAttributes(EntityAttributes value) noexcept
    {
        m_attributes = std::move(value);
        return *this;
    }

    const std::vector<YiCadPoint2d>& corners() const noexcept
    {
        return m_corners;
    }
    const EntityAttributes& attributes() const noexcept { return m_attributes; }

private:
    friend class ImportContainer;

    YiCadImportResult makeAbi(YiCadSolidDataV3& data) const noexcept
    {
        if (m_corners.size() != 3 && m_corners.size() != 4)
        {
            return YICAD_IMPORT_ERROR_INVALID_ARGUMENT;
        }
        data = makeImportData<YiCadSolidDataV3>();
        data.attributes = &m_attributes.m_data;
        data.cornerCount = static_cast<uint32_t>(m_corners.size());
        for (std::size_t index = 0; index < m_corners.size(); ++index)
        {
            data.corners[index] = m_corners[index];
        }
        return YICAD_IMPORT_SUCCESS;
    }

    std::vector<YiCadPoint2d> m_corners;
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
    TextData& setStyleName(std::string value)
    {
        m_textStyleName = std::move(value); return *this;
    }
    TextData& setAttributes(EntityAttributes value) noexcept
    {
        m_attributes = std::move(value); return *this;
    }
    const std::string& text() const noexcept { return m_text; }
    YiCadPoint2d insertionPoint() const noexcept { return m_insertionPoint; }
    YiCadPoint2d alignmentPoint() const noexcept { return m_alignmentPoint; }
    double height() const noexcept { return m_height; }
    double rotation() const noexcept { return m_rotation; }
    double widthFactor() const noexcept { return m_widthFactor; }
    double obliqueAngle() const noexcept { return m_obliqueAngle; }
    YiCadTextHorizontalAlignment horizontalAlignment() const noexcept { return m_horizontalAlignment; }
    YiCadTextVerticalAlignment verticalAlignment() const noexcept { return m_verticalAlignment; }
    const EntityAttributes& attributes() const noexcept { return m_attributes; }
    const std::string& textStyleName() const noexcept { return m_textStyleName; }

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
    std::string m_textStyleName;
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
    MTextData& setStyleName(std::string value)
    {
        m_textStyleName = std::move(value); return *this;
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
    const std::string& contents() const noexcept { return m_contents; }
    YiCadPoint2d insertionPoint() const noexcept { return m_insertionPoint; }
    YiCadVector2d direction() const noexcept { return m_direction; }
    double characterHeight() const noexcept { return m_characterHeight; }
    double rectangleWidth() const noexcept { return m_rectangleWidth; }
    double lineSpacingFactor() const noexcept { return m_lineSpacingFactor; }
    YiCadMTextAttachment attachment() const noexcept { return m_attachment; }
    const EntityAttributes& attributes() const noexcept { return m_attributes; }
    const std::string& textStyleName() const noexcept { return m_textStyleName; }

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
    std::string m_textStyleName;
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
    const std::string& name() const noexcept { return m_name; }
    YiCadPoint2d basePoint() const noexcept { return m_basePoint; }
    uint32_t flags() const noexcept { return m_flags; }
    const std::string& description() const noexcept { return m_description; }
    const std::string& externalReferencePath() const noexcept { return m_externalReferencePath; }

private:
    friend class ImportSession;
    friend class Document;

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
    YiCadReadResourceHandle m_readHandle = nullptr;
};

/** @brief 插件侧保存块引用资源与变换参数的块引用输入。 */
class InsertData
{
public:
    explicit InsertData(const ImportResource& block = {}) : m_block(block) {}
    explicit InsertData(std::string blockName) : m_blockName(std::move(blockName)) {}

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
    const std::string& blockName() const noexcept { return m_blockName; }
    YiCadPoint2d insertionPoint() const noexcept { return m_insertionPoint; }
    YiCadVector3d scale() const noexcept { return m_scale; }
    double rotation() const noexcept { return m_rotation; }
    uint32_t columnCount() const noexcept { return m_columnCount; }
    uint32_t rowCount() const noexcept { return m_rowCount; }
    double columnSpacing() const noexcept { return m_columnSpacing; }
    double rowSpacing() const noexcept { return m_rowSpacing; }
    const EntityAttributes& attributes() const noexcept { return m_attributes; }

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
    std::string m_blockName;
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
    const TextData& text() const noexcept { return m_text; }
    const std::string& tag() const noexcept { return m_tag; }
    const std::string& prompt() const noexcept { return m_prompt; }
    const std::string& defaultValue() const noexcept { return m_defaultValue; }
    uint32_t flags() const noexcept { return m_flags; }

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
    const TextData& text() const noexcept { return m_text; }
    const std::string& tag() const noexcept { return m_tag; }
    const std::string& value() const noexcept { return m_value; }
    uint32_t flags() const noexcept { return m_flags; }

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
    DimensionData& setStyleName(std::string value)
    {
        m_dimensionStyleName = std::move(value); return *this;
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
    YiCadDimensionKind kind() const noexcept { return m_kind; }
    const std::string& styleName() const noexcept { return m_dimensionStyleName; }
    const std::string& textOverride() const noexcept { return m_textOverride; }
    YiCadPoint2d definitionPoint() const noexcept { return m_definitionPoint; }
    YiCadPoint2d textPosition() const noexcept { return m_textPosition; }
    double textRotation() const noexcept { return m_textRotation; }
    double lineSpacingFactor() const noexcept { return m_lineSpacingFactor; }
    YiCadPoint2d extensionPoint1() const noexcept { return m_extensionPoint1; }
    YiCadPoint2d extensionPoint2() const noexcept { return m_extensionPoint2; }
    YiCadPoint2d line1Start() const noexcept { return m_line1Start; }
    YiCadPoint2d line1End() const noexcept { return m_line1End; }
    YiCadPoint2d line2Start() const noexcept { return m_line2Start; }
    YiCadPoint2d line2End() const noexcept { return m_line2End; }
    YiCadPoint2d arcPoint() const noexcept { return m_arcPoint; }
    YiCadPoint2d featurePoint() const noexcept { return m_featurePoint; }
    double leaderLength() const noexcept { return m_leaderLength; }
    const EntityAttributes& attributes() const noexcept { return m_attributes; }

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
    std::string m_dimensionStyleName;
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
    LeaderData& setStyleName(std::string value)
    {
        m_dimensionStyleName = std::move(value); return *this;
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
    const std::vector<YiCadPoint2d>& vertices() const noexcept { return m_vertices; }
    bool hasArrow() const noexcept { return m_hasArrow; }
    const std::string& styleName() const noexcept { return m_dimensionStyleName; }
    const std::optional<TextData>& text() const noexcept { return m_text; }
    const EntityAttributes& attributes() const noexcept { return m_attributes; }

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
    std::string m_dimensionStyleName;
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

        YiCadHatchEdgeType type() const noexcept { return m_type; }
        YiCadPoint2d startPoint() const noexcept { return m_startPoint; }
        YiCadPoint2d endPoint() const noexcept { return m_endPoint; }
        YiCadPoint2d center() const noexcept { return m_center; }
        YiCadVector2d majorAxis() const noexcept { return m_majorAxis; }
        double radius() const noexcept { return m_radius; }
        double minorToMajorRatio() const noexcept { return m_minorToMajorRatio; }
        double startParameter() const noexcept { return m_startParameter; }
        double endParameter() const noexcept { return m_endParameter; }
        bool counterClockwise() const noexcept { return m_counterClockwise; }
        uint32_t degree() const noexcept { return m_degree; }
        bool rational() const noexcept { return m_rational; }
        bool periodic() const noexcept { return m_periodic; }
        const std::vector<YiCadPoint2d>& controlPoints() const noexcept { return m_controlPoints; }
        const std::vector<double>& knots() const noexcept { return m_knots; }
        const std::vector<double>& weights() const noexcept { return m_weights; }

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

    struct Loop
    {
        YiCadHatchLoopKind kind = YICAD_HATCH_LOOP_POLYLINE;
        YiCadHatchLoopRole role = YICAD_HATCH_LOOP_OUTER;
        uint32_t outerLoopIndex = UINT32_MAX;
        std::vector<YiCadVertex2d> vertices;
        std::vector<Edge> edges;
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
    bool solid() const noexcept { return m_solid; }
    const std::string& patternName() const noexcept { return m_patternName; }
    double patternScale() const noexcept { return m_patternScale; }
    double patternAngle() const noexcept { return m_patternAngle; }
    const std::vector<Loop>& loops() const noexcept { return m_loops; }
    const EntityAttributes& attributes() const noexcept { return m_attributes; }

private:
    friend class ImportContainer;

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
    const std::string& path() const noexcept { return m_path; }
    YiCadPoint2d insertionPoint() const noexcept { return m_insertionPoint; }
    YiCadVector2d uVector() const noexcept { return m_uVector; }
    YiCadVector2d vVector() const noexcept { return m_vVector; }
    YiCadVector2d size() const noexcept { return m_size; }
    int32_t brightness() const noexcept { return m_brightness; }
    int32_t contrast() const noexcept { return m_contrast; }
    int32_t fade() const noexcept { return m_fade; }
    const std::vector<YiCadPoint2d>& clipBoundary() const noexcept { return m_clipBoundary; }
    const EntityAttributes& attributes() const noexcept { return m_attributes; }

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

/* ==========================================================================
 * v4：自定义实体（RENDER_PLAN.md 第 4.8.3 节、第 8.2 步）
 * ========================================================================== */

/** @brief 恒等变换。 */
inline YiCadMatrix2d identityMatrix() noexcept
{
    return {1.0, 0.0, 0.0, 1.0, 0.0, 0.0};
}

/** @brief 变换一个点：x' = a·x + c·y + tx，y' = b·x + d·y + ty。 */
inline YiCadPoint2d transformPoint(const YiCadMatrix2d& m, YiCadPoint2d p) noexcept
{
    return {m.a * p.x + m.c * p.y + m.tx, m.b * p.x + m.d * p.y + m.ty};
}

/** @brief 变换一个向量（不含平移）。 */
inline YiCadVector2d transformVector(const YiCadMatrix2d& m, YiCadVector2d v) noexcept
{
    return {m.a * v.x + m.c * v.y, m.b * v.x + m.d * v.y};
}

/** @brief 线性部分的行列式；小于 0 表示含镜像。 */
inline double matrixDeterminant(const YiCadMatrix2d& m) noexcept
{
    return m.a * m.d - m.b * m.c;
}

/** @brief 合成：先做 second，再做 first。 */
inline YiCadMatrix2d composeMatrix(const YiCadMatrix2d& first, const YiCadMatrix2d& second) noexcept
{
    return {first.a * second.a + first.c * second.b,
        first.b * second.a + first.d * second.b,
        first.a * second.c + first.c * second.d,
        first.b * second.c + first.d * second.d,
        first.a * second.tx + first.c * second.ty + first.tx,
        first.b * second.tx + first.d * second.ty + first.ty};
}

/**
 * @brief 把数据写成字节：小端序，与平台无关；插件实体的数据编码可以用它。
 * @note 字符串写成 4 字节长度加 UTF-8 字节。
 */
class ByteWriter
{
public:
    ByteWriter& u8(uint8_t value) { m_bytes.push_back(value); return *this; }

    ByteWriter& u32(uint32_t value)
    {
        for (int i = 0; i < 4; ++i)
        {
            m_bytes.push_back(static_cast<uint8_t>(value >> (8 * i)));
        }
        return *this;
    }

    ByteWriter& i32(int32_t value) { return u32(static_cast<uint32_t>(value)); }

    ByteWriter& f64(double value)
    {
        uint64_t bits = 0;
        static_assert(sizeof(bits) == sizeof(value));
        std::memcpy(&bits, &value, sizeof(bits));
        for (int i = 0; i < 8; ++i)
        {
            m_bytes.push_back(static_cast<uint8_t>(bits >> (8 * i)));
        }
        return *this;
    }

    ByteWriter& point(YiCadPoint2d value) { return f64(value.x).f64(value.y); }

    ByteWriter& string(std::string_view value)
    {
        u32(static_cast<uint32_t>(value.size()));
        m_bytes.insert(m_bytes.end(), value.begin(), value.end());
        return *this;
    }

    const std::vector<uint8_t>& bytes() const noexcept { return m_bytes; }
    std::vector<uint8_t> take() noexcept { return std::move(m_bytes); }

private:
    std::vector<uint8_t> m_bytes;
};

/**
 * @brief 读 ByteWriter 写的字节；越界时抛 std::out_of_range。
 * @note 实体类的函数里抛出的异常由 SDK 在 C ABI 边界转成失败。
 */
class ByteReader
{
public:
    explicit ByteReader(std::span<const uint8_t> bytes) noexcept : m_bytes(bytes) {}

    uint8_t u8()
    {
        need(1);
        return m_bytes[m_offset++];
    }

    uint32_t u32()
    {
        need(4);
        uint32_t value = 0;
        for (int i = 0; i < 4; ++i)
        {
            value |= static_cast<uint32_t>(m_bytes[m_offset++]) << (8 * i);
        }
        return value;
    }

    int32_t i32() { return static_cast<int32_t>(u32()); }

    double f64()
    {
        need(8);
        uint64_t bits = 0;
        for (int i = 0; i < 8; ++i)
        {
            bits |= static_cast<uint64_t>(m_bytes[m_offset++]) << (8 * i);
        }
        double value = 0.0;
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    }

    YiCadPoint2d point()
    {
        const double x = f64();
        return {x, f64()};
    }

    std::string string()
    {
        const uint32_t size = u32();
        need(size);
        std::string value(reinterpret_cast<const char*>(m_bytes.data() + m_offset), size);
        m_offset += size;
        return value;
    }

    bool atEnd() const noexcept { return m_offset >= m_bytes.size(); }
    std::size_t remaining() const noexcept { return m_bytes.size() - m_offset; }

private:
    void need(std::size_t count) const
    {
        if (count > m_bytes.size() - m_offset)
        {
            throw std::out_of_range("YiCAD plugin data is truncated");
        }
    }

    std::span<const uint8_t> m_bytes;
    std::size_t m_offset = 0;
};

/** @brief 宿主持有的文档实体引用（非拥有）；所属文档打开且实体仍在文档里时有效。 */
class EntityRef
{
public:
    EntityRef() noexcept = default;
    explicit EntityRef(YiCadEntityHandle handle) noexcept : m_handle(handle) {}

    explicit operator bool() const noexcept { return m_handle != nullptr; }
    YiCadEntityHandle nativeHandle() const noexcept { return m_handle; }

private:
    YiCadEntityHandle m_handle = nullptr;
};

/**
 * @brief 自定义实体：类名、数据版本与数据字节，外加代理信息。
 * @note 只读枚举交出它（YICAD_ENTITY_CUSTOM）；导入（ImportContainer::createCustomEntity）与
 * 事务内新建（DocumentTransaction::createCustomEntity）用它作输入，isProxy 与 entity 只在输出里有意义。
 */
class CustomEntityData
{
public:
    CustomEntityData() = default;
    CustomEntityData(std::string className, uint32_t classVersion, std::vector<uint8_t> data)
        : m_className(std::move(className)),
          m_classVersion(classVersion),
          m_data(std::move(data))
    {
    }

    const std::string& className() const noexcept { return m_className; }
    uint32_t classVersion() const noexcept { return m_classVersion; }
    const std::vector<uint8_t>& data() const noexcept { return m_data; }
    /// @brief 代理权限（YICAD_PROXY_*）：导入时类不在、读成代理用；输出为实体当前的
    uint32_t proxyFlags() const noexcept { return m_proxyFlags; }
    /// @brief 代理累计的变换；不是代理时为恒等
    const YiCadMatrix2d& transform() const noexcept { return m_transform; }
    /// @brief 输出：是否代理（类不在，或数据读不了）
    bool isProxy() const noexcept { return m_isProxy; }
    /// @brief 输出：实体引用，可交给 DocumentTransaction::setCustomEntityData
    const EntityRef& entity() const noexcept { return m_entity; }
    const EntityAttributes& attributes() const noexcept { return m_attributes; }

    CustomEntityData& setClass(std::string className, uint32_t classVersion)
    {
        m_className = std::move(className);
        m_classVersion = classVersion;
        return *this;
    }
    CustomEntityData& setData(std::vector<uint8_t> value)
    {
        m_data = std::move(value);
        return *this;
    }
    CustomEntityData& setProxyFlags(uint32_t value) noexcept
    {
        m_proxyFlags = value;
        return *this;
    }
    CustomEntityData& setTransform(const YiCadMatrix2d& value) noexcept
    {
        m_transform = value;
        return *this;
    }
    CustomEntityData& setAttributes(EntityAttributes value) noexcept
    {
        m_attributes = std::move(value);
        return *this;
    }

    /// @brief 从只读枚举的 ABI 输出转成拥有型值
    static CustomEntityData fromAbi(const YiCadCustomEntityDataV4& data, EntityAttributes attributes)
    {
        CustomEntityData result(detail::copyString(data.className), data.classVersion,
            std::vector<uint8_t>(data.data.data, data.data.data + data.data.size));
        result.m_proxyFlags = data.proxyFlags;
        result.m_transform = data.transform;
        result.m_isProxy = data.isProxy != 0;
        result.m_entity = EntityRef(data.entity);
        result.m_attributes = std::move(attributes);
        return result;
    }

    /// @brief 生成 ABI 输入；只在本对象与 proxyGraphics 存活、未修改期间有效
    YiCadImportResult makeAbi(
        YiCadCustomEntityDataV4& data,
        YiCadImportContainerHandle proxyGraphics) const noexcept
    {
        if (!detail::validString(m_className, true) || !detail::fitsAbiCount(m_data.size()))
        {
            return YICAD_IMPORT_ERROR_INVALID_ARGUMENT;
        }
        data = {};
        data.structSize = static_cast<uint32_t>(sizeof(data));
        data.attributes = &m_attributes.abiData();
        data.className = detail::stringView(m_className);
        data.classVersion = m_classVersion;
        data.proxyFlags = m_proxyFlags;
        data.data = {m_data.empty() ? nullptr : m_data.data(), static_cast<uint32_t>(m_data.size())};
        data.transform = m_transform;
        data.proxyGraphics = proxyGraphics;
        return YICAD_IMPORT_SUCCESS;
    }

private:
    std::string m_className;
    uint32_t m_classVersion = 0;
    std::vector<uint8_t> m_data;
    uint32_t m_proxyFlags = 0;
    YiCadMatrix2d m_transform = identityMatrix();
    bool m_isProxy = false;
    EntityRef m_entity;
    EntityAttributes m_attributes;
};

using EntityData = std::variant<
    PointData, LineData, RayData, XLineData, ArcData, CircleData,
    EllipseData, PolylineData, SplineData, SolidData, TextData,
    MTextData, DimensionData, LeaderData, HatchData, InsertData,
    AttributeDefinitionData, AttributeData, ImageData, CustomEntityData>;

/**
 * @brief 导入会话内的非拥有模型空间或块定义容器包装。
 * @note 容器只在所属会话内有效；endBlock 会消费调用它的包装对象，之前复制的
 * 包装可在会话提交前用于解析延迟块引用。
 * @note 创建函数返回宿主的确定结果码；无效包装返回 INVALID_HANDLE，缺失函数或
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

    /**
     * @brief v4：创建自定义实体；类登记了且读得了数据时建原实体，否则建代理实体。
     * @param proxyGraphics ImportSession::beginProxyGraphics 收集的代理图形，可为空；用后失效。
     */
    YiCadImportResult createCustomEntity(
        const CustomEntityData& value,
        const ImportContainer* proxyGraphics = nullptr) const noexcept
    {
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        const auto* api = m_state->api;
        if (!YICAD_SDK_HAS_IMPORT_FUNCTION(api, createCustomEntity))
        {
            return YICAD_IMPORT_ERROR_UNSUPPORTED;
        }
        YiCadCustomEntityDataV4 data{};
        const auto result = value.makeAbi(data,
            proxyGraphics != nullptr ? proxyGraphics->m_handle : nullptr);
        if (result != YICAD_IMPORT_SUCCESS)
        {
            return result;
        }
        return detail::callImport([&]() {
            return api->createCustomEntity(m_state->session, m_handle, &data);
        });
    }

    /**
     * @brief 包装宿主交来的导入会话与容器（实体类的炸开由 SDK 用它把宿主的临时会话交给插件）。
     * @note 包装不拥有会话，析构时不回滚；只在宿主那次调用期间有效。
     */
    static ImportContainer wrapHostContainer(
        const YiCadImportApi* api,
        YiCadImportSessionHandle session,
        YiCadImportContainerHandle container)
    {
        return ImportContainer(
            std::make_shared<detail::ImportState>(detail::ImportState{api, session}),
            container);
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

    /// @brief 创建三点或四点二维实体填充。
    YiCadImportResult createSolid(const SolidData& value) const noexcept
    {
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        YiCadSolidDataV3 data{};
        const auto result = value.makeAbi(data);
        return result == YICAD_IMPORT_SUCCESS ? createSolid(data) : result;
    }

    /// @brief 使用底层 ABI POD 创建三点或四点二维实体填充。
    YiCadImportResult createSolid(
        const YiCadSolidDataV3& data) const noexcept
    {
        return createEntity(data, offsetof(YiCadImportApi, createSolid),
            sizeof(((YiCadImportApi*)nullptr)->createSolid),
            m_state != nullptr && YICAD_SDK_HAS_IMPORT_FUNCTION(
                                      m_state->api, createSolid)
                ? m_state->api->createSolid
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
 * @note 包装方法返回宿主的确定结果码；无效包装返回 INVALID_HANDLE，缺失函数或
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

    /**
     * @brief v4：开始收集一个代理的图形：之后在 graphics 里建的实体不进文档，交给
     * ImportContainer::createCustomEntity 后成为代理显示的图形。
     * @note 属性为空、图层为空、线型为空、颜色与线宽随块，表示沿用自定义实体自己的属性。
     */
    YiCadImportResult beginProxyGraphics(ImportContainer& graphics) const noexcept
    {
        graphics = {};
        if (!*this)
        {
            return YICAD_IMPORT_ERROR_INVALID_HANDLE;
        }
        const auto* api = m_state->api;
        if (!YICAD_SDK_HAS_IMPORT_FUNCTION(api, beginProxyGraphics))
        {
            return YICAD_IMPORT_ERROR_UNSUPPORTED;
        }
        YiCadImportContainerHandle handle = nullptr;
        const auto result = detail::callImport([&]() {
            return api->beginProxyGraphics(m_state->session, &handle);
        });
        if (result == YICAD_IMPORT_SUCCESS && handle != nullptr)
        {
            graphics = ImportContainer(m_state, handle);
        }
        return result;
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

    /**
     * @brief v4：在模型空间新建一个自定义实体（类必须已登记，数据为当前版本）。
     * @param[out] entity 成功时为新实体的引用。
     * @note 属性的图层、线型不能引用导入资源：取当前图层、随层。
     */
    YiCadImportResult createCustomEntity(
        const CustomEntityData& value,
        EntityRef& entity) const noexcept
    {
        entity = {};
        const auto* api = entityApi();
        if (m_handle == nullptr || api == nullptr || api->createCustomEntity == nullptr)
        {
            return YICAD_IMPORT_ERROR_UNSUPPORTED;
        }
        YiCadCustomEntityDataV4 data{};
        const auto result = value.makeAbi(data, nullptr);
        if (result != YICAD_IMPORT_SUCCESS)
        {
            return result;
        }
        YiCadEntityHandle handle = nullptr;
        const auto created = detail::callImport([&]() {
            return api->createCustomEntity(m_handle, &data, &handle);
        });
        if (created == YICAD_IMPORT_SUCCESS)
        {
            entity = EntityRef(handle);
        }
        return created;
    }

    /// @brief v4：换一个自定义实体的数据（当前数据版本），可撤销。
    YiCadImportResult setCustomEntityData(
        const EntityRef& entity,
        std::span<const uint8_t> data) const noexcept
    {
        const auto* api = entityApi();
        if (m_handle == nullptr || api == nullptr || api->setCustomEntityData == nullptr)
        {
            return YICAD_IMPORT_ERROR_UNSUPPORTED;
        }
        if (!detail::fitsAbiCount(data.size()))
        {
            return YICAD_IMPORT_ERROR_INVALID_ARGUMENT;
        }
        const YiCadByteView view{data.empty() ? nullptr : data.data(),
            static_cast<uint32_t>(data.size())};
        return detail::callImport([&]() {
            return api->setCustomEntityData(m_handle, entity.nativeHandle(), view);
        });
    }

private:
    friend class Document;

    const YiCadEntityApiV4* entityApi() const noexcept
    {
        return m_api != nullptr && m_api->abiVersion == YICAD_PLUGIN_ABI_V4 &&
                   m_api->entityApi != nullptr &&
                   m_api->entityApi->abiVersion == YICAD_PLUGIN_ABI_V4
            ? m_api->entityApi
            : nullptr;
    }

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

    /// @brief 读取下一项并转换为拥有全部字符串和数组的 C++ 值。
    bool next(EntityData& value) noexcept
    {
        if (m_api == nullptr || m_api->readApi == nullptr ||
            m_handle == nullptr)
        {
            return false;
        }
        const auto* read = m_api->readApi;
        YiCadEntityType type = YICAD_ENTITY_UNKNOWN;
        if (read->entityNext(m_handle, &type) != YICAD_SUCCESS)
        {
            return false;
        }
        auto attributes = [&](const YiCadEntityAttributes* source) {
            return EntityAttributes::fromAbi(source,
                resourceName(source == nullptr ? nullptr : source->layer),
                resourceName(source == nullptr ? nullptr : source->lineType));
        };
        switch (type)
        {
        case YICAD_ENTITY_POINT:
        {
            YiCadPointDataV3 data{};
            if (!readCurrent(data)) return false;
            value = PointData{data.position, attributes(data.attributes)};
            return true;
        }
        case YICAD_ENTITY_LINE:
        {
            YiCadLineDataV3 data{};
            if (!readCurrent(data)) return false;
            value = LineData{data.startPoint, data.endPoint,
                attributes(data.attributes)};
            return true;
        }
        case YICAD_ENTITY_RAY:
        case YICAD_ENTITY_XLINE:
        {
            YiCadRayDataV3 data{};
            if (!readCurrent(data)) return false;
            if (type == YICAD_ENTITY_RAY)
                value = RayData{data.basePoint, data.direction, attributes(data.attributes)};
            else
                value = XLineData{data.basePoint, data.direction, attributes(data.attributes)};
            return true;
        }
        case YICAD_ENTITY_ARC:
        {
            YiCadArcDataV3 data{};
            if (!readCurrent(data)) return false;
            value = ArcData{data.center, data.radius, data.startAngle,
                data.endAngle, attributes(data.attributes)};
            return true;
        }
        case YICAD_ENTITY_CIRCLE:
        {
            YiCadCircleDataV3 data{};
            if (!readCurrent(data)) return false;
            value = CircleData{data.center, data.radius, attributes(data.attributes)};
            return true;
        }
        case YICAD_ENTITY_ELLIPSE:
        {
            YiCadEllipseDataV3 data{};
            if (!readCurrent(data)) return false;
            value = EllipseData{data.center, data.majorAxis,
                data.minorToMajorRatio, data.startParameter, data.endParameter,
                data.closed != 0, attributes(data.attributes)};
            return true;
        }
        case YICAD_ENTITY_POLYLINE:
        {
            YiCadPolylineDataV3 data{};
            if (!readCurrent(data)) return false;
            std::vector<YiCadVertex2d> vertices(data.vertices.data,
                data.vertices.data + data.vertices.count);
            PolylineData result(std::move(vertices));
            result.setClosed(data.closed != 0).setAttributes(attributes(data.attributes));
            value = std::move(result);
            return true;
        }
        case YICAD_ENTITY_SPLINE:
        {
            YiCadSplineDataV3 data{};
            if (!readCurrent(data)) return false;
            SplineData result;
            if (data.definition == YICAD_SPLINE_FIT_POINTS)
                result.setFitPoints(data.degree,
                    std::vector<YiCadPoint2d>(data.fitPoints.data,
                        data.fitPoints.data + data.fitPoints.count));
            else
                result.setControlPoints(data.degree,
                    std::vector<YiCadPoint2d>(data.controlPoints.data,
                        data.controlPoints.data + data.controlPoints.count),
                    std::vector<double>(data.knots.data,
                        data.knots.data + data.knots.count),
                    std::vector<double>(data.weights.data,
                        data.weights.data + data.weights.count));
            result.setClosed(data.closed != 0).setRational(data.rational != 0)
                .setPeriodic(data.periodic != 0).setAttributes(attributes(data.attributes));
            value = std::move(result);
            return true;
        }
        case YICAD_ENTITY_SOLID:
        {
            YiCadSolidDataV3 data{};
            if (!readCurrent(data)) return false;
            SolidData result(std::vector<YiCadPoint2d>(
                data.corners, data.corners + data.cornerCount));
            result.setAttributes(attributes(data.attributes));
            value = std::move(result);
            return true;
        }
        case YICAD_ENTITY_TEXT:
        {
            YiCadTextDataV3 data{};
            if (!readCurrent(data)) return false;
            auto result = textValue(data, attributes(data.attributes));
            result.setStyleName(resourceName(data.textStyle));
            value = std::move(result);
            return true;
        }
        case YICAD_ENTITY_MTEXT:
        {
            YiCadMTextDataV3 data{};
            if (!readCurrent(data)) return false;
            MTextData result(detail::copyString(data.contents));
            result.setPlacement(data.insertionPoint, data.direction)
                .setLayout(data.characterHeight, data.rectangleWidth,
                    data.lineSpacingFactor, data.attachment)
                .setAttributes(attributes(data.attributes));
            result.setStyleName(resourceName(data.textStyle));
            if (data.background != nullptr)
                result.setBackground(data.background->useDrawingBackgroundColor != 0,
                    data.background->color, data.background->borderScaleFactor);
            value = std::move(result);
            return true;
        }
        case YICAD_ENTITY_INSERT:
        {
            YiCadInsertDataV3 data{};
            if (!readCurrent(data)) return false;
            InsertData result(resourceName(data.block));
            result.setPlacement(data.insertionPoint, data.scale, data.rotation)
                .setArray(data.columnCount, data.rowCount,
                    data.columnSpacing, data.rowSpacing)
                .setAttributes(attributes(data.attributes));
            value = std::move(result);
            return true;
        }
        case YICAD_ENTITY_ATTRIBUTE_DEFINITION:
        {
            YiCadAttributeDefinitionDataV3 data{};
            if (!readCurrent(data) || data.text == nullptr) return false;
            AttributeDefinitionData result(
                textValue(*data.text, attributes(data.text->attributes)),
                detail::copyString(data.tag));
            result.setPrompt(detail::copyString(data.prompt))
                .setDefaultValue(detail::copyString(data.defaultValue))
                .setFlags(data.flags);
            value = std::move(result);
            return true;
        }
        case YICAD_ENTITY_ATTRIBUTE:
        {
            YiCadAttributeDataV3 data{};
            if (!readCurrent(data) || data.text == nullptr) return false;
            AttributeData result(
                textValue(*data.text, attributes(data.text->attributes)), {},
                detail::copyString(data.tag), detail::copyString(data.value));
            result.setFlags(data.flags);
            value = std::move(result);
            return true;
        }
        case YICAD_ENTITY_DIMENSION:
        {
            YiCadDimensionDataV3 data{};
            if (!readCurrent(data)) return false;
            DimensionData result(data.kind);
            result.setText(detail::copyString(data.textOverride), data.textPosition,
                    data.textRotation, data.lineSpacingFactor)
                .setDefinitionPoints(data.definitionPoint,
                    data.extensionPoint1, data.extensionPoint2)
                .setAngularLines(data.line1Start, data.line1End,
                    data.line2Start, data.line2End, data.arcPoint)
                .setFeaturePoint(data.featurePoint, data.leaderLength)
                .setAttributes(attributes(data.attributes));
            result.setStyleName(resourceName(data.dimensionStyle));
            value = std::move(result);
            return true;
        }
        case YICAD_ENTITY_LEADER:
        {
            YiCadLeaderDataV3 data{};
            if (!readCurrent(data)) return false;
            LeaderData result(std::vector<YiCadPoint2d>(data.vertices.data,
                data.vertices.data + data.vertices.count));
            result.setArrow(data.hasArrow != 0).setAttributes(attributes(data.attributes));
            result.setStyleName(resourceName(data.dimensionStyle));
            if (data.text != nullptr)
                result.setText(textValue(*data.text, attributes(data.text->attributes)));
            value = std::move(result);
            return true;
        }
        case YICAD_ENTITY_HATCH:
        {
            YiCadHatchDataV3 data{};
            if (!readCurrent(data)) return false;
            HatchData result;
            if (data.solid != 0) result.setSolid();
            else result.setPattern(detail::copyString(data.patternName),
                data.patternScale, data.patternAngle);
            for (uint32_t loopIndex = 0; loopIndex < data.loops.count; ++loopIndex)
            {
                const auto* loop = reinterpret_cast<const YiCadHatchLoopDataV3*>(
                    reinterpret_cast<const std::byte*>(data.loops.data) +
                    static_cast<std::size_t>(loopIndex) * data.loops.byteStride);
                if (loop->kind == YICAD_HATCH_LOOP_POLYLINE)
                {
                    result.addPolylineLoop(
                        std::vector<YiCadVertex2d>(loop->polylineVertices.data,
                            loop->polylineVertices.data +
                                loop->polylineVertices.count),
                        loop->role, loop->outerLoopIndex);
                    continue;
                }
                std::vector<HatchData::Edge> edges;
                edges.reserve(loop->edges.count);
                for (uint32_t edgeIndex = 0; edgeIndex < loop->edges.count;
                     ++edgeIndex)
                {
                    const auto* edge = reinterpret_cast<const YiCadHatchEdgeDataV3*>(
                        reinterpret_cast<const std::byte*>(loop->edges.data) +
                        static_cast<std::size_t>(edgeIndex) * loop->edges.byteStride);
                    switch (edge->type)
                    {
                    case YICAD_HATCH_EDGE_LINE:
                        edges.push_back(HatchData::Edge::line(
                            edge->startPoint, edge->endPoint));
                        break;
                    case YICAD_HATCH_EDGE_CIRCULAR_ARC:
                        edges.push_back(HatchData::Edge::circularArc(
                            edge->center, edge->radius, edge->startParameter,
                            edge->endParameter, edge->counterClockwise != 0));
                        break;
                    case YICAD_HATCH_EDGE_ELLIPTIC_ARC:
                        edges.push_back(HatchData::Edge::ellipticArc(
                            edge->center, edge->majorAxis,
                            edge->minorToMajorRatio, edge->startParameter,
                            edge->endParameter, edge->counterClockwise != 0));
                        break;
                    case YICAD_HATCH_EDGE_SPLINE:
                        edges.push_back(HatchData::Edge::spline(edge->degree,
                            std::vector<YiCadPoint2d>(edge->controlPoints.data,
                                edge->controlPoints.data + edge->controlPoints.count),
                            std::vector<double>(edge->knots.data,
                                edge->knots.data + edge->knots.count),
                            std::vector<double>(edge->weights.data,
                                edge->weights.data + edge->weights.count),
                            edge->rational != 0, edge->periodic != 0));
                        break;
                    default:
                        return false;
                    }
                }
                result.addEdgeLoop(std::move(edges),
                    loop->role, loop->outerLoopIndex);
            }
            result.setAttributes(attributes(data.attributes));
            value = std::move(result);
            return true;
        }
        case YICAD_ENTITY_IMAGE:
        {
            YiCadImageDataV3 data{};
            if (!readCurrent(data)) return false;
            ImageData result(detail::copyString(data.path));
            result.setGeometry(data.insertionPoint, data.uVector, data.vVector, data.size)
                .setDisplay(data.brightness, data.contrast, data.fade)
                .setClipBoundary(std::vector<YiCadPoint2d>(
                    data.clipBoundary.data,
                    data.clipBoundary.data + data.clipBoundary.count))
                .setAttributes(attributes(data.attributes));
            value = std::move(result);
            return true;
        }
        case YICAD_ENTITY_CUSTOM:
        {
            YiCadCustomEntityDataV4 data{};
            if (!readCurrent(data)) return false;
            value = CustomEntityData::fromAbi(data, attributes(data.attributes));
            return true;
        }
        default:
            return false;
        }
    }

    /**
     * @brief v4：当前自定义实体的图形，做成基本实体逐个枚举（写进别的格式的代理图形用）。
     * @return 当前项不是自定义实体时返回空迭代器。块展开成内容，实心填充为填充，
     * 图案填充为线与点，文字为笔画，样条为多段线；属性是解析后的。
     */
    EntityIterator graphics() const noexcept
    {
        if (m_api == nullptr || m_api->readApi == nullptr || m_handle == nullptr ||
            m_api->readApi->structSize < YICAD_READ_API_V4_SIZE ||
            m_api->readApi->entityGraphics == nullptr)
        {
            return {};
        }
        return EntityIterator(m_api, m_api->readApi->entityGraphics(m_handle));
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

    template<typename Data>
    bool readCurrent(Data& data) const noexcept
    {
        return m_api != nullptr && m_api->readApi != nullptr &&
               m_api->readApi->entityData(m_handle, &data) == YICAD_SUCCESS;
    }

    std::string resourceName(YiCadReadResourceHandle resource) const
    {
        YiCadStringView name{};
        return resource != nullptr && m_api != nullptr && m_api->readApi != nullptr &&
               m_api->readApi->resourceName(resource, &name) == YICAD_SUCCESS
            ? detail::copyString(name) : std::string{};
    }

    static TextData textValue(
        const YiCadTextDataV3& data,
        EntityAttributes attributes)
    {
        TextData result(detail::copyString(data.text));
        result.setPlacement(data.insertionPoint, data.alignmentPoint)
            .setMetrics(data.height, data.rotation,
                data.widthFactor, data.obliqueAngle)
            .setAlignment(data.horizontalAlignment, data.verticalAlignment)
            .setAttributes(std::move(attributes));
        result.setStyleName({});
        return result;
    }

    void reset() noexcept
    {
        if (m_api && m_handle && m_api->readApi &&
            m_api->readApi->entityDestroy)
        {
            m_api->readApi->entityDestroy(m_handle);
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
        return m_api != nullptr && m_handle != nullptr &&
               m_api->abiVersion == YICAD_PLUGIN_ABI_V4;
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

    /// @brief 开始一个整体可撤销的文档事务。
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
        if (!*this || m_api->readApi == nullptr ||
            m_api->readApi->entities == nullptr)
        {
            return {};
        }
        return EntityIterator(
            m_api, m_api->readApi->entities(m_handle, nullptr));
    }

    /// @brief 枚举指定块定义中的一等实体。
    EntityIterator entities(const BlockData& block) const noexcept
    {
        if (!*this || block.m_readHandle == nullptr ||
            m_api->readApi == nullptr || m_api->readApi->entities == nullptr)
        {
            return {};
        }
        return EntityIterator(
            m_api, m_api->readApi->entities(m_handle, block.m_readHandle));
    }

    DocumentSettings settings() const
    {
        DocumentSettings result;
        YiCadDocumentSettings data{};
        if (*this && m_api->readApi != nullptr &&
            m_api->readApi->documentSettings(m_handle, &data) == YICAD_SUCCESS)
        {
            result.setInsertionUnits(data.insertionUnits)
                .setMeasurement(data.measurement)
                .setGlobalLineTypeScale(data.globalLineTypeScale)
                .setCurrentEntityLineTypeScale(data.currentEntityLineTypeScale)
                .setSourceCodePage(detail::copyString(data.sourceCodePage));
        }
        return result;
    }

    std::vector<LineTypeData> lineTypes() const
    {
        std::vector<LineTypeData> result;
        forEachResource(YICAD_READ_LINE_TYPE, [&](const void* handle) {
            YiCadLineTypeDataV3 data{};
            if (m_api->readApi->resourceData(handle, YICAD_READ_LINE_TYPE,
                    &data) == YICAD_SUCCESS)
            {
                LineTypeData value(detail::copyString(data.name));
                value.setDescription(detail::copyString(data.description))
                    .setElements(std::vector<double>(data.elements.data,
                        data.elements.data + data.elements.count))
                    .setComplex(data.complex != 0);
                result.push_back(std::move(value));
            }
        });
        return result;
    }

    std::vector<LayerData> layers() const
    {
        std::vector<LayerData> result;
        forEachResource(YICAD_READ_LAYER, [&](const void* handle) {
            YiCadLayerDataV3 data{};
            if (m_api->readApi->resourceData(handle, YICAD_READ_LAYER,
                    &data) == YICAD_SUCCESS)
            {
                LayerData value(detail::copyString(data.name));
                value.setFrozen(data.frozen != 0).setLocked(data.locked != 0)
                    .setPlottable(data.plottable != 0).setColor(data.color)
                    .setLineWidth(data.lineWidth)
                    .setLineTypeName(readResourceName(data.lineType));
                result.push_back(std::move(value));
            }
        });
        return result;
    }

    std::vector<TextStyleData> textStyles() const
    {
        std::vector<TextStyleData> result;
        forEachResource(YICAD_READ_TEXT_STYLE, [&](const void* handle) {
            YiCadTextStyleDataV3 data{};
            if (m_api->readApi->resourceData(handle, YICAD_READ_TEXT_STYLE,
                    &data) == YICAD_SUCCESS)
            {
                TextStyleData value(detail::copyString(data.name));
                value.setFontFiles(detail::copyString(data.fontFile),
                        detail::copyString(data.bigFontFile))
                    .setMetrics(data.fixedHeight, data.widthFactor,
                        data.obliqueAngle)
                    .setGenerationFlags(data.generationFlags);
                result.push_back(std::move(value));
            }
        });
        return result;
    }

    std::vector<DimensionStyleData> dimensionStyles() const
    {
        std::vector<DimensionStyleData> result;
        forEachResource(YICAD_READ_DIMENSION_STYLE, [&](const void* handle) {
            YiCadDimensionStyleDataV3 data{};
            if (m_api->readApi->resourceData(handle,
                    YICAD_READ_DIMENSION_STYLE, &data) == YICAD_SUCCESS)
            {
                auto value = DimensionStyleData::fromAbi(data);
                value.setResourceNames(readResourceName(data.textStyle),
                    readResourceName(data.dimLineType),
                    readResourceName(data.extensionLineType));
                result.push_back(std::move(value));
            }
        });
        return result;
    }

    std::vector<BlockData> blocks() const
    {
        std::vector<BlockData> result;
        if (!*this || m_api->readApi == nullptr)
        {
            return result;
        }
        const auto count = m_api->readApi->blockCount(m_handle);
        result.reserve(count);
        for (uint32_t index = 0; index < count; ++index)
        {
            const auto handle = m_api->readApi->blockAt(m_handle, index);
            YiCadBlockDataV3 data{};
            if (handle != nullptr &&
                m_api->readApi->blockData(handle, &data) == YICAD_SUCCESS)
            {
                BlockData value(detail::copyString(data.name));
                value.setBasePoint(data.basePoint).setFlags(data.flags)
                    .setDescription(detail::copyString(data.description))
                    .setExternalReferencePath(
                        detail::copyString(data.externalReferencePath));
                value.m_readHandle = handle;
                result.push_back(std::move(value));
            }
        }
        return result;
    }

    /// @brief 开始一个导入会话。
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

private:
    friend class Host;

    Document(
        const YiCadHostApi* api,
        YiCadDocumentHandle handle) noexcept
        : m_api(api),
          m_handle(handle)
    {
    }

    template<typename Callback>
    void forEachResource(YiCadReadResourceKind kind, Callback&& callback) const
    {
        if (!*this || m_api->readApi == nullptr)
        {
            return;
        }
        const auto count = m_api->readApi->resourceCount(m_handle, kind);
        for (uint32_t index = 0; index < count; ++index)
        {
            const auto handle = m_api->readApi->resourceAt(m_handle, kind, index);
            if (handle != nullptr)
            {
                callback(handle);
            }
        }
    }

    std::string readResourceName(YiCadReadResourceHandle handle) const
    {
        YiCadStringView value{};
        return handle != nullptr && m_api->readApi->resourceName(handle, &value) ==
                   YICAD_SUCCESS
            ? detail::copyString(value) : std::string{};
    }

    bool hasField(size_t offset, size_t size) const noexcept
    {
        (void)offset;
        (void)size;
        return m_api != nullptr && m_handle != nullptr &&
               m_api->abiVersion == YICAD_PLUGIN_ABI_V4;
    }

    bool hasV2Field(size_t offset, size_t size) const noexcept
    {
        return hasField(offset, size);
    }

    const YiCadImportApi* importApiForSession() const noexcept
    {
        if (!*this || m_api->importApi == nullptr)
        {
            return nullptr;
        }

        const auto* importApi = m_api->importApi;
        return importApi->abiVersion == YICAD_PLUGIN_ABI_V4 &&
               importApi->beginImport != nullptr &&
               importApi->commitImport != nullptr &&
               importApi->rollbackImport != nullptr
            ? importApi
            : nullptr;
    }

    const YiCadHostApi* m_api = nullptr;
    YiCadDocumentHandle m_handle = nullptr;
};

/** @brief GI 填充的一个闭合环：顶点与每段凸度（为空或与顶点等长）。 */
struct GiLoop
{
    std::vector<YiCadPoint2d> points;
    std::vector<double> bulges;
};

/** @brief 一行文字的位置：插入点、字高，其余取默认（左对齐、基线、不旋转、宽度系数 1）。 */
inline YiCadTextPlacementV4 makeTextPlacement(YiCadPoint2d insertionPoint, double height) noexcept
{
    YiCadTextPlacementV4 placement{};
    placement.structSize = static_cast<uint32_t>(sizeof(placement));
    placement.insertionPoint = insertionPoint;
    placement.alignmentPoint = insertionPoint;
    placement.height = height;
    placement.widthFactor = 1.0;
    placement.horizontalAlignment = YICAD_TEXT_ALIGN_LEFT;
    placement.verticalAlignment = YICAD_TEXT_ALIGN_BASELINE;
    return placement;
}

/**
 * @brief worldDraw 收到的 GI：宿主 GI 表（YiCadGiApiV4）的 C++ 包装，只在本次 worldDraw 期间有效。
 * @note 各函数返回宿主是否接受（参数无效时宿主忽略该图元）；资源句柄由 lineType、layer、
 * textStyle、block 按名字在实体所属文档里找，找不到为空。
 */
class Gi
{
public:
    Gi(const YiCadGiApiV4* api, YiCadGiContextHandle ctx) noexcept
        : m_api(api),
          m_ctx(ctx)
    {
    }

    explicit operator bool() const noexcept
    {
        return m_api != nullptr && m_ctx != nullptr &&
               m_api->abiVersion == YICAD_PLUGIN_ABI_V4;
    }

    bool setColor(const YiCadColorData& color) const noexcept
    {
        return *this && m_api->setColor(m_ctx, &color) == YICAD_SUCCESS;
    }
    bool setLayer(YiCadReadResourceHandle layer) const noexcept
    {
        return *this && m_api->setLayer(m_ctx, layer) == YICAD_SUCCESS;
    }
    /// @brief 线型；为空即随块
    bool setLineType(YiCadReadResourceHandle lineType) const noexcept
    {
        return *this && m_api->setLineType(m_ctx, lineType) == YICAD_SUCCESS;
    }
    bool setLineTypeScale(double scale) const noexcept
    {
        return *this && m_api->setLineTypeScale(m_ctx, scale) == YICAD_SUCCESS;
    }
    bool setLineWeight(int32_t weight) const noexcept
    {
        return *this && m_api->setLineWeight(m_ctx, weight) == YICAD_SUCCESS;
    }
    bool setTransparency(uint32_t alpha) const noexcept
    {
        return *this && m_api->setTransparency(m_ctx, alpha) == YICAD_SUCCESS;
    }
    bool setSelectionMarker(int32_t marker) const noexcept
    {
        return *this && m_api->setSelectionMarker(m_ctx, marker) == YICAD_SUCCESS;
    }
    /// @brief 之后的 fill 按这些图案线填；为空时恢复实心
    bool setFillPattern(std::span<const YiCadHatchPatternLineV4> lines) const noexcept
    {
        return *this && detail::fitsAbiCount(lines.size()) &&
               m_api->setFillPattern(m_ctx, lines.empty() ? nullptr : lines.data(),
                   static_cast<uint32_t>(lines.size())) == YICAD_SUCCESS;
    }

    /**
     * @brief 多段线；bulges 为空或每段一个，widths 为空或每段两个（起止宽度）。
     * @param flags YICAD_GI_POLYLINE_*。
     */
    bool polyline(std::span<const YiCadPoint2d> points,
        std::span<const double> bulges = {},
        std::span<const double> widths = {},
        uint32_t flags = 0) const noexcept
    {
        if (!*this || !detail::fitsAbiCount(points.size()) ||
            !detail::fitsAbiCount(bulges.size()) || !detail::fitsAbiCount(widths.size()))
        {
            return false;
        }
        const YiCadPoint2dArrayView pointView{points.data(), static_cast<uint32_t>(points.size())};
        const YiCadDoubleArrayView bulgeView{bulges.data(), static_cast<uint32_t>(bulges.size())};
        const YiCadDoubleArrayView widthView{widths.data(), static_cast<uint32_t>(widths.size())};
        return m_api->polyline(m_ctx, &pointView, bulges.empty() ? nullptr : &bulgeView,
                   widths.empty() ? nullptr : &widthView, flags) == YICAD_SUCCESS;
    }
    bool circle(YiCadPoint2d center, double radius) const noexcept
    {
        return *this && m_api->circle(m_ctx, center, radius) == YICAD_SUCCESS;
    }
    /// @brief 圆弧，从 startAngle 起转过 sweepAngle（弧度，正为逆时针）
    bool arc(YiCadPoint2d center, double radius, double startAngle, double sweepAngle) const noexcept
    {
        return *this && m_api->arc(m_ctx, center, radius, startAngle, sweepAngle) == YICAD_SUCCESS;
    }
    bool ellipseArc(YiCadPoint2d center, YiCadVector2d majorAxis, double ratio,
        double startParameter, double endParameter) const noexcept
    {
        return *this && m_api->ellipseArc(m_ctx, center, majorAxis, ratio,
                   startParameter, endParameter) == YICAD_SUCCESS;
    }
    bool nurbs(uint32_t degree, std::span<const YiCadPoint2d> controlPoints,
        std::span<const double> knots, bool closed = false) const noexcept
    {
        if (!*this || !detail::fitsAbiCount(controlPoints.size()) || !detail::fitsAbiCount(knots.size()))
        {
            return false;
        }
        const YiCadPoint2dArrayView pointView{controlPoints.data(),
            static_cast<uint32_t>(controlPoints.size())};
        const YiCadDoubleArrayView knotView{knots.data(), static_cast<uint32_t>(knots.size())};
        return m_api->nurbs(m_ctx, degree, &pointView, &knotView, closed ? 1U : 0U) == YICAD_SUCCESS;
    }
    /// @brief 填充区域；fillRule 为 YICAD_GI_FILL_*
    bool fill(std::span<const GiLoop> loops, uint32_t fillRule = YICAD_GI_FILL_EVEN_ODD) const
    {
        if (!*this || !detail::fitsAbiCount(loops.size()))
        {
            return false;
        }
        std::vector<YiCadGiLoopV4> views;
        views.reserve(loops.size());
        for (const auto& loop : loops)
        {
            if (!detail::fitsAbiCount(loop.points.size()) || !detail::fitsAbiCount(loop.bulges.size()))
            {
                return false;
            }
            views.push_back({{loop.points.data(), static_cast<uint32_t>(loop.points.size())},
                {loop.bulges.data(), static_cast<uint32_t>(loop.bulges.size())}});
        }
        return m_api->fill(m_ctx, views.data(), static_cast<uint32_t>(views.size()), fillRule) ==
               YICAD_SUCCESS;
    }
    bool triangles(std::span<const YiCadPoint2d> vertices, std::span<const uint32_t> indices) const noexcept
    {
        if (!*this || !detail::fitsAbiCount(vertices.size()) || !detail::fitsAbiCount(indices.size()))
        {
            return false;
        }
        const YiCadPoint2dArrayView view{vertices.data(), static_cast<uint32_t>(vertices.size())};
        return m_api->triangles(m_ctx, &view, indices.data(),
                   static_cast<uint32_t>(indices.size())) == YICAD_SUCCESS;
    }
    /// @brief 单行文字；textStyle 为空时用 Standard
    bool text(std::string_view value, YiCadReadResourceHandle textStyle,
        const YiCadTextPlacementV4& placement) const noexcept
    {
        if (!*this || !detail::fitsAbiCount(value.size()))
        {
            return false;
        }
        const YiCadStringView view{value.empty() ? nullptr : value.data(),
            static_cast<uint32_t>(value.size())};
        return m_api->text(m_ctx, view, textStyle, &placement) == YICAD_SUCCESS;
    }
    bool image(std::string_view path, YiCadPoint2d origin, YiCadVector2d u, YiCadVector2d v,
        uint32_t widthPixels, uint32_t heightPixels) const noexcept
    {
        if (!*this || !detail::fitsAbiCount(path.size()))
        {
            return false;
        }
        const YiCadStringView view{path.data(), static_cast<uint32_t>(path.size())};
        return m_api->image(m_ctx, view, origin, u, v, widthPixels, heightPixels) == YICAD_SUCCESS;
    }
    bool point(YiCadPoint2d position) const noexcept
    {
        return *this && m_api->point(m_ctx, position) == YICAD_SUCCESS;
    }
    bool ray(YiCadPoint2d base, YiCadVector2d direction) const noexcept
    {
        return *this && m_api->ray(m_ctx, base, direction) == YICAD_SUCCESS;
    }
    bool xline(YiCadPoint2d base, YiCadVector2d direction) const noexcept
    {
        return *this && m_api->xline(m_ctx, base, direction) == YICAD_SUCCESS;
    }
    /// @brief 画一个块定义；块里的随块属性取当前属性
    bool drawBlock(YiCadReadResourceHandle block, const YiCadMatrix2d& transform) const noexcept
    {
        return *this && m_api->drawBlock(m_ctx, block, &transform) == YICAD_SUCCESS;
    }
    bool pushTransform(const YiCadMatrix2d& transform) const noexcept
    {
        return *this && m_api->pushTransform(m_ctx, &transform) == YICAD_SUCCESS;
    }
    bool popTransform() const noexcept
    {
        return *this && m_api->popTransform(m_ctx) == YICAD_SUCCESS;
    }
    /// @brief 非空：之后的图元以像素为单位、锚定在该点；为空：恢复世界单位
    bool setScreenSpace(const YiCadPoint2d* anchor) const noexcept
    {
        return *this && m_api->setScreenSpace(m_ctx, anchor) == YICAD_SUCCESS;
    }

    YiCadReadResourceHandle lineType(std::string_view name) const noexcept
    {
        return find(YICAD_READ_LINE_TYPE, name);
    }
    YiCadReadResourceHandle layer(std::string_view name) const noexcept
    {
        return find(YICAD_READ_LAYER, name);
    }
    YiCadReadResourceHandle textStyle(std::string_view name) const noexcept
    {
        return find(YICAD_READ_TEXT_STYLE, name);
    }
    YiCadReadResourceHandle block(std::string_view name) const noexcept
    {
        return find(YICAD_READ_BLOCK, name);
    }

private:
    YiCadReadResourceHandle find(YiCadReadResourceKind kind, std::string_view name) const noexcept
    {
        if (!*this || !detail::fitsAbiCount(name.size()))
        {
            return nullptr;
        }
        return m_api->findResource(m_ctx, kind,
            {name.empty() ? nullptr : name.data(), static_cast<uint32_t>(name.size())});
    }

    const YiCadGiApiV4* m_api = nullptr;
    YiCadGiContextHandle m_ctx = nullptr;
};

/** @brief 实体类的登记信息。 */
struct EntityClassInfo
{
    /// @brief 类名，"pluginId.类名"
    std::string className;
    /// @brief 数据编码的版本；读回的版本低于它时调用 upgrade
    uint32_t classVersion = 1;
    /// @brief 代理权限（YICAD_PROXY_*）：插件不在时允许的操作
    uint32_t proxyFlags = 0;
    /// @brief worldDraw 线程安全：图形系统在工作线程上直接调用，不缓存图形
    bool threadSafeDraw = false;
    /// @brief 为每个实体缓存解码后的 Data（宿主随数据作废），省得每次调用都解码
    bool cacheDecodedData = true;
};

namespace detail
{
template<typename Class>
struct EntityClassTrampolines;
} // namespace detail

/**
 * @brief 插件写实体类的基类（第 8.2 步）：Data 是插件解码后的数据。
 * @details SDK 负责字节与 Data 的转换、实例缓存、函数表与异常隔离：派生类实现 decode、encode、
 * worldDraw、extents、transform；夹点（grips 与 moveGrips 一起）、捕捉点、炸开、数据升级覆盖了才提供，
 * 没覆盖的由宿主按 worldDraw 的图元推导（夹点则没有）。函数里抛出的异常（如 ByteReader 越界）
 * 在 C ABI 边界转成失败，宿主不采用结果。
 * @note 实例要在插件 shutdown 前一直存活（一般是插件的全局对象），经 Host::registerEntityClass 登记。
 */
template<typename Data>
class EntityClass
{
public:
    using DataType = Data;

    virtual ~EntityClass() = default;

    /// @brief 把当前版本的字节解码成 Data；读不了时抛异常
    virtual Data decode(std::span<const uint8_t> bytes) const = 0;
    /// @brief 把 Data 编码成当前版本的字节
    virtual std::vector<uint8_t> encode(const Data& data) const = 0;
    /// @brief 画实体
    virtual void worldDraw(const Data& data, const Gi& gi) const = 0;
    /// @brief 包围框
    virtual YiCadExtents2d extents(const Data& data) const = 0;
    /// @brief 按仿射变换改动（移动、旋转、缩放、镜像，可能非等比）
    virtual void transform(Data& data, const YiCadMatrix2d& matrix) const = 0;

    /// @brief 夹点位置（与 moveGrips 一起覆盖）
    virtual std::vector<YiCadPoint2d> grips(const Data& data) const
    {
        (void)data;
        return {};
    }
    /// @brief 拖动 indices 这几个夹点 offset
    virtual void moveGrips(Data& data, std::span<const uint32_t> indices, YiCadVector2d offset) const
    {
        (void)data;
        (void)indices;
        (void)offset;
    }
    /// @brief 拾取点附近某种捕捉点（YICAD_SNAP_*）的候选
    virtual std::vector<YiCadPoint2d> snapPoints(const Data& data, uint32_t snapMode, YiCadPoint2d pick) const
    {
        (void)data;
        (void)snapMode;
        (void)pick;
        return {};
    }
    /// @brief 炸开：在 out 里用导入函数建基本实体（属性为空时取被炸开的实体的）；返回 false 表示失败
    virtual bool explode(const Data& data, const ImportContainer& out) const
    {
        (void)data;
        (void)out;
        return false;
    }
    /// @brief 把 fromVersion 版的字节升级成当前版本的字节
    virtual std::vector<uint8_t> upgrade(uint32_t fromVersion, std::span<const uint8_t> bytes) const
    {
        (void)fromVersion;
        (void)bytes;
        return {};
    }

protected:
    /// @brief 宿主函数表（登记时由 Host 设置），炸开时包装宿主的导入会话用
    const YiCadHostApi* hostApi() const noexcept { return m_hostApi; }

private:
    friend class Host;
    template<typename Class>
    friend struct detail::EntityClassTrampolines;

    const YiCadHostApi* m_hostApi = nullptr;
};

namespace detail
{

/// @brief 为实体类 Class 生成 YiCadEntityClassV4 的各个函数，userData 是 Class 的实例
template<typename Class>
struct EntityClassTrampolines
{
    using Data = typename Class::DataType;
    using Base = EntityClass<Data>;

    static const Class& self(void* userData) noexcept
    {
        return *static_cast<const Class*>(userData);
    }

    static std::span<const uint8_t> bytesOf(YiCadByteView view) noexcept
    {
        return {view.data, view.size};
    }

    /// @brief 有实例缓存时直接用，否则现解码
    template<typename Body>
    static void withData(void* userData, YiCadByteView view, void* cache, Body&& body)
    {
        if (cache != nullptr)
        {
            body(*static_cast<const Data*>(cache));
            return;
        }
        const Data data = self(userData).decode(bytesOf(view));
        body(data);
    }

    static Data copyData(void* userData, YiCadByteView view, void* cache)
    {
        return cache != nullptr ? *static_cast<const Data*>(cache) : self(userData).decode(bytesOf(view));
    }

    static YiCadResult writeBytes(const YiCadByteSink* out, const std::vector<uint8_t>& bytes) noexcept
    {
        return out != nullptr && out->write != nullptr && fitsAbiCount(bytes.size()) && !bytes.empty() &&
                       out->write(out->context, bytes.data(), static_cast<uint32_t>(bytes.size())) ==
                           YICAD_SUCCESS
            ? YICAD_SUCCESS
            : YICAD_FAILURE;
    }

    static YiCadResult addPoints(const YiCadPointSink* out, const std::vector<YiCadPoint2d>& points) noexcept
    {
        if (out == nullptr || out->add == nullptr || !fitsAbiCount(points.size()))
        {
            return YICAD_FAILURE;
        }
        return points.empty() ? YICAD_SUCCESS
            : out->add(out->context, points.data(), static_cast<uint32_t>(points.size()));
    }

    static YiCadResult YICAD_PLUGIN_CALL worldDraw(void* userData, YiCadByteView data, void* cache,
        const YiCadGiApiV4* gi, YiCadGiContextHandle ctx) noexcept
    {
        return invokeNoexcept<YiCadResult>([&]() {
            const Gi wrapper(gi, ctx);
            withData(userData, data, cache, [&](const Data& value) { self(userData).worldDraw(value, wrapper); });
            return YICAD_SUCCESS;
        }, YICAD_FAILURE);
    }

    static YiCadResult YICAD_PLUGIN_CALL getExtents(void* userData, YiCadByteView data, void* cache,
        YiCadExtents2d* extents) noexcept
    {
        return invokeNoexcept<YiCadResult>([&]() {
            if (extents == nullptr)
            {
                return YICAD_FAILURE;
            }
            withData(userData, data, cache, [&](const Data& value) { *extents = self(userData).extents(value); });
            return YICAD_SUCCESS;
        }, YICAD_FAILURE);
    }

    static YiCadResult YICAD_PLUGIN_CALL transform(void* userData, YiCadByteView data, void* cache,
        const YiCadMatrix2d* matrix, const YiCadByteSink* out) noexcept
    {
        return invokeNoexcept<YiCadResult>([&]() {
            if (matrix == nullptr)
            {
                return YICAD_FAILURE;
            }
            Data value = copyData(userData, data, cache);
            self(userData).transform(value, *matrix);
            return writeBytes(out, self(userData).encode(value));
        }, YICAD_FAILURE);
    }

    static YiCadResult YICAD_PLUGIN_CALL getGrips(void* userData, YiCadByteView data, void* cache,
        const YiCadPointSink* out) noexcept
    {
        return invokeNoexcept<YiCadResult>([&]() {
            std::vector<YiCadPoint2d> points;
            withData(userData, data, cache, [&](const Data& value) { points = self(userData).grips(value); });
            return addPoints(out, points);
        }, YICAD_FAILURE);
    }

    static YiCadResult YICAD_PLUGIN_CALL moveGrips(void* userData, YiCadByteView data, void* cache,
        const uint32_t* indices, uint32_t count, YiCadVector2d offset, const YiCadByteSink* out) noexcept
    {
        return invokeNoexcept<YiCadResult>([&]() {
            if (indices == nullptr && count != 0)
            {
                return YICAD_FAILURE;
            }
            Data value = copyData(userData, data, cache);
            self(userData).moveGrips(value, std::span<const uint32_t>(indices, count), offset);
            return writeBytes(out, self(userData).encode(value));
        }, YICAD_FAILURE);
    }

    static YiCadResult YICAD_PLUGIN_CALL getSnapPoints(void* userData, YiCadByteView data, void* cache,
        uint32_t snapMode, YiCadPoint2d pick, const YiCadPointSink* out) noexcept
    {
        return invokeNoexcept<YiCadResult>([&]() {
            std::vector<YiCadPoint2d> points;
            withData(userData, data, cache,
                [&](const Data& value) { points = self(userData).snapPoints(value, snapMode, pick); });
            return addPoints(out, points);
        }, YICAD_FAILURE);
    }

    static YiCadResult YICAD_PLUGIN_CALL explode(void* userData, YiCadByteView data, void* cache,
        YiCadImportSessionHandle session, YiCadImportContainerHandle container) noexcept
    {
        return invokeNoexcept<YiCadResult>([&]() {
            const YiCadHostApi* api = self(userData).m_hostApi;
            if (api == nullptr || api->importApi == nullptr)
            {
                return YICAD_FAILURE;
            }
            const ImportContainer out = ImportContainer::wrapHostContainer(api->importApi, session, container);
            bool exploded = false;
            withData(userData, data, cache, [&](const Data& value) { exploded = self(userData).explode(value, out); });
            return exploded ? YICAD_SUCCESS : YICAD_FAILURE;
        }, YICAD_FAILURE);
    }

    static YiCadResult YICAD_PLUGIN_CALL upgrade(void* userData, uint32_t fromVersion, YiCadByteView data,
        const YiCadByteSink* out) noexcept
    {
        return invokeNoexcept<YiCadResult>([&]() {
            return writeBytes(out, self(userData).upgrade(fromVersion, bytesOf(data)));
        }, YICAD_FAILURE);
    }

    static YiCadResult YICAD_PLUGIN_CALL createCache(void* userData, YiCadByteView data, void** cache) noexcept
    {
        return invokeNoexcept<YiCadResult>([&]() {
            if (cache == nullptr)
            {
                return YICAD_FAILURE;
            }
            *cache = new Data(self(userData).decode(bytesOf(data)));
            return YICAD_SUCCESS;
        }, YICAD_FAILURE);
    }

    static void YICAD_PLUGIN_CALL destroyCache(void* userData, void* cache) noexcept
    {
        (void)userData;
        delete static_cast<Data*>(cache);
    }

    /// @brief 生成函数表：可选函数只在派生类覆盖了时提供
    static YiCadEntityClassV4 table(const Class& entityClass, const EntityClassInfo& info) noexcept
    {
        YiCadEntityClassV4 result{};
        result.structSize = static_cast<uint32_t>(sizeof(result));
        result.abiVersion = YICAD_PLUGIN_ABI_V4;
        result.className = stringView(info.className);
        result.classVersion = info.classVersion;
        result.proxyFlags = info.proxyFlags;
        result.flags = info.threadSafeDraw ? YICAD_ENTITY_CLASS_THREAD_SAFE_DRAW : 0U;
        result.userData = const_cast<Class*>(&entityClass);
        result.worldDraw = &worldDraw;
        result.getExtents = &getExtents;
        result.transform = &transform;
        if constexpr (!std::is_same_v<decltype(&Class::grips), decltype(&Base::grips)> &&
                      !std::is_same_v<decltype(&Class::moveGrips), decltype(&Base::moveGrips)>)
        {
            result.getGrips = &getGrips;
            result.moveGrips = &moveGrips;
        }
        if constexpr (!std::is_same_v<decltype(&Class::snapPoints), decltype(&Base::snapPoints)>)
        {
            result.getSnapPoints = &getSnapPoints;
        }
        if constexpr (!std::is_same_v<decltype(&Class::explode), decltype(&Base::explode)>)
        {
            result.explode = &explode;
        }
        if constexpr (!std::is_same_v<decltype(&Class::upgrade), decltype(&Base::upgrade)>)
        {
            result.upgrade = &upgrade;
        }
        if (info.cacheDecodedData)
        {
            result.createCache = &createCache;
            result.destroyCache = &destroyCache;
        }
        return result;
    }
};

} // namespace detail

class Host
{
public:
    explicit Host(const YiCadHostApi* api = nullptr) noexcept
        : m_api(api)
    {
    }

    /**
     * @brief v4：在 init 里登记一个实体类。
     * @param entityClass 派生自 EntityClass<Data> 的实例，要存活到 shutdown。
     * @return 宿主接受时为真；提交（与插件的其他注册项一起）时还会检查类名是否重名。
     */
    template<typename Class>
    bool registerEntityClass(
        const char* pluginId,
        Class& entityClass,
        const EntityClassInfo& info) const noexcept
    {
        static_assert(std::is_base_of_v<EntityClass<typename Class::DataType>, Class>,
            "实体类必须派生自 yicad::plugin::EntityClass<Data>");
        if (pluginId == nullptr || !isCompatible() || m_api->entityApi == nullptr ||
            m_api->entityApi->registerEntityClass == nullptr ||
            !detail::validString(info.className, true))
        {
            return false;
        }
        entityClass.m_hostApi = m_api;
        const YiCadEntityClassV4 table =
            detail::EntityClassTrampolines<Class>::table(entityClass, info);
        return invokeNoexcept<bool>([&]() {
            return m_api->entityApi->registerEntityClass(pluginId, &table) == YICAD_SUCCESS;
        }, false);
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
        return m_api != nullptr &&
               m_api->abiVersion == YICAD_PLUGIN_ABI_V4;
    }

    bool hasField(size_t offset, size_t size) const noexcept
    {
        (void)offset;
        (void)size;
        return isCompatible();
    }

    const YiCadHostApi* m_api = nullptr;
};

} // namespace yicad::plugin

#endif
