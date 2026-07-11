# YiCAD DXF 插件实施计划

## 1. 目标与实现原则

在现有插件系统中增加 `YiCadDxfPlugin`，使用 Conan 已引入的
`libdxfrw/2.2.0` 实现 `.dxf` 导入和导出。

本实现遵循以下原则：

1. 只转换 YiCAD 已有且可编辑、可持久化的数据。DXF 有而 YiCAD 没有的数据不读取、不保存、不导出。
2. `DmLineStrip`、`DmChar`、`DmCharTemplate`、`DmRegion`、`DmTriangle`、文字段落/行、标注展开实体、预览和 Overlay 等内部或临时对象不参与导入导出。
3. 插件业务代码只使用 `YiCadPluginSdk.h` 提供的 C++ API。底层函数表只作为 DLL 边界实现细节，不在 DXF 插件中直接操作。
4. 继续使用 `YICAD_PLUGIN_ABI_V3`。SDK 尚未发布，需要的新接口直接加入 v3，不新增 v4，也不实现版本协商和旧版兼容分支。
5. 不建立测试工程、`tests/` 目录、测试数据目录或测试代码。通过构建、YiCAD 实际操作和第三方 CAD 打开结果进行人工验收。
6. 不做重复和过度校验。文件语法交给 `libdxfrw`，YiCAD 数据约束交给宿主 API；插件只检查空路径、解析结果、必要引用和 API 调用结果。
7. 类和中间模型保持最少。没有明确生命周期或职责价值的对象不增加。

## 2. 支持范围

### 2.1 文档和资源

只处理 YiCAD 当前模型已有字段：

| YiCAD 数据 | DXF 数据 | 规则 |
|---|---|---|
| 插入单位 | `$INSUNITS` | 直接映射 |
| 测量制式 | `$MEASUREMENT` | 直接映射 |
| 全局线型比例 | `$LTSCALE` | 直接映射 |
| 源代码页 | `$DWGCODEPAGE` | 作为源文件元数据 |
| `DmLineType` | `LTYPE` | 只处理 YiCAD 可表达的简单 dash/gap 线型 |
| `DmLayer` | `LAYER` | 名称、状态、颜色、线型、线宽和打印属性 |
| `DmTextStyle` | `STYLE` | 只处理 YiCAD 已有字体、字高、宽度和倾斜字段 |
| `DmDimensionStyle` | `DIMSTYLE` | 只处理 `DmDimensionStyleData` 已有字段 |
| `DmBlock` | `BLOCK` | 名称、基点和块内一等实体 |

DXF 表记录中的扩展数据、句柄、所有者链、材质、字典、布局、视口、打印样式和 YiCAD 没有的样式字段直接忽略。

### 2.2 一等实体

| YiCAD 类型 | DXF 类型 | 说明 |
|---|---|---|
| `DmPoint` | `POINT` | 二维坐标 |
| `DmLine` | `LINE` | 起点、终点 |
| `DmRay` | `RAY` | 基点、方向 |
| `DmXline` | `XLINE` | 基点、方向 |
| `DmArc` | `ARC` | 圆心、半径、起止角 |
| `DmCircle` | `CIRCLE` | 圆心、半径 |
| `DmEllipse` | `ELLIPSE` | 椭圆和椭圆弧 |
| `DmPolyline` | `LWPOLYLINE`、二维 `POLYLINE` | 顶点、闭合、bulge 和宽度 |
| `DmSpline` | `SPLINE` | 只处理 YiCAD 已有的控制点、拟合点和节点数据 |
| `DmSolid` | `SOLID`、`TRACE` | 三点或四点实体填充 |
| `DmText` | `TEXT` | 内容、位置、样式、字高、旋转和对齐 |
| `DmMText` | `MTEXT` | 内容、位置、宽度、方向和基础格式 |
| 五类 `DmDimension` | `DIMENSION` | 对齐、线性、角度、半径和直径标注 |
| `DmDimLeader` | `LEADER` | 顶点和现有引线属性 |
| `DmHatch` | `HATCH` | YiCAD 可表达的边界和图案字段 |
| `DmBlockReference` | `INSERT` | 块名、插入点、比例、旋转和阵列 |
| `DmAttributeDefinition` | `ATTDEF` | YiCAD 已有字段 |
| `DmAttribute` | `ATTRIB` | 按 tag 关联块引用 |
| `DmImage` | `IMAGE`、`IMAGEDEF` | 文件路径、位置、U/V 向量、尺寸、亮度、对比度和淡入 |

实体公共属性只映射 YiCAD 已有的图层、线型、颜色、线宽、可见性和法向量。当前模型固定或不保存的 DXF 属性不引入新字段。

`IMAGE` 首先只支持模型空间。当前 `DmBlock::isSaveEntType` 明确排除了块内图像，因此块内 `IMAGE` 不导入、不导出；不要为了 DXF 扩大 YiCAD 块模型。

### 2.3 明确排除

以下内容不进入任何转换流程：

- `DmLineStrip`：多段线、文字或其他对象的绘制展开结果；
- `DmChar`、`DmCharTemplate`、`DmMTextLine`、`DmMTextParagraph`：文字内部排版对象；
- `DmRegion`、`DmTriangle`：填充边界计算和三角化结果；
- `DmConstructionLine`：内部构造对象；DXF `XLINE` 只对应 `DmXline`；
- 标注、引线、块引用生成的子实体；
- `DmPreview`、Overlay、缓存和选择辅助对象；
- 3D 实体、网格、ACIS、代理对象、布局、视口、材质、字典、XDATA、外部参照递归加载；
- 所有其他只有 DXF 定义而 YiCAD 没有对应一等数据的内容。

`libdxfrw` 对这些对象产生回调时使用空实现，不创建替代几何，不展开成线段，也不报导入失败。

## 3. 架构取舍

### 3.1 不使用 `DxfDocument`

原计划中的 `DxfDocument` 用于完整复制 `libdxfrw` 回调数据，再统一校验、排序并写入 YiCAD。它会产生第二套文档模型，必须为每种资源和实体重复定义结构、所有权和引用关系，同时增加一次全量内存复制。

本项目不需要它，理由如下：

- `libdxfrw` 已按 HEADER、TABLES、BLOCKS、ENTITIES 提供回调；大部分对象可在回调中直接转换；
- `ImportSession` 已提供原子提交和回滚，不需要中立文档承担事务职责；
- 导出时可从 YiCAD C++ SDK 直接枚举数据并交给 `libdxfrw`；
- 只有前向块引用需要延迟处理，保存少量 `PendingInsert` 即可，不应为此复制整个文档。

因此不创建 `DxfDocument.h/.cpp`。也不创建 `DxfDiagnostics`、`AtomicOutputFile` 或通用 Repository/Factory/Visitor 层。

### 3.2 数据流

```text
导入：DXF -> libdxfrw 回调 -> DxfImporter -> C++ ImportSession -> DmDocument

导出：DmDocument -> C++ Document/EntityIterator -> DxfExporter -> libdxfrw -> DXF
```

导入过程中只保留：资源名称到 SDK 句柄的映射、当前块容器和尚未解析的块引用。解析或创建失败时不提交 `ImportSession`，由 RAII 回滚。

### 3.3 必要校验

只保留以下检查：

- 文件路径非空且 `libdxfrw::read/write` 返回成功；
- `ImportSession`、模型空间和必要资源句柄创建成功；
- 块引用目标存在；
- 宿主 C++ API 返回成功。

不增加 API 版本范围判断、结构大小逐字段判断、DXF 二次解析、严格/宽松模式、unsupported mask、重复的有限值扫描或损坏文件专项逻辑。

## 4. v3 C++ SDK 最小扩展

### 4.1 原则

插件入口直接返回：

```cpp
return YICAD_PLUGIN_ABI_V3;
```

`YiCadPluginAbi.h` 中直接补充 v3 所需底层函数和数据结构，最小和最大版本都固定为 v3：

```cpp
#define YICAD_PLUGIN_ABI_MIN_VERSION YICAD_PLUGIN_ABI_V3
#define YICAD_PLUGIN_ABI_MAX_VERSION YICAD_PLUGIN_ABI_V3
#define YICAD_PLUGIN_ABI_VERSION YICAD_PLUGIN_ABI_V3
```

不新增 `YICAD_PLUGIN_ABI_V4`、v4 size 宏、v4 发布文档或兼容验证代码。现有 demo 也改为只走 v3，删除 `importV2`、`supportsImport()` 分支和旧版降级路径。

DXF 插件不包含 `YiCadPluginAbi.h` 的业务用法，只通过 `YiCadPluginSdk.h` 的 C++ 类型工作。DLL 入口仍使用 C linkage，这是 Windows 插件加载所必需的稳定入口，不等于让业务代码使用 C ABI。

### 4.2 `Host` 只接受 v3

`Host` 只在构造后做一次版本检查，后续 C++ 方法不再逐字段检查 ABI 版本和函数表长度：

```cpp
class Host
{
public:
    explicit Host(const YiCadHostApi* api = nullptr) noexcept
        : m_api(api)
    {
    }

    explicit operator bool() const noexcept
    {
        return m_api != nullptr &&
               m_api->abiVersion == YICAD_PLUGIN_ABI_V3;
    }

private:
    const YiCadHostApi* m_api = nullptr;
};
```

保留参数空值和宿主调用结果检查；删除 v1/v2 fallback、版本范围判断和“尾字段不存在则降级”逻辑。因为 SDK 未发布，不承担历史二进制兼容成本。

### 4.3 导入侧补充

现有导入 C++ API 已覆盖大部分类型，只需给 `ImportContainer` 增加 `createSolid`：

```cpp
class ImportContainer
{
public:
    /// @brief 创建三点或四点实体填充。
    YiCadImportResult createSolid(
        const SolidData& value,
        const EntityAttributes& attributes = {}) const noexcept
    {
        const auto data = value.makeAbi(attributes);
        return m_state->api->createSolid(
            m_state->session, m_handle, &data);
    }
};
```

底层仅增加对应的 `YiCadSolidDataV3`、`YiCadImportCreateSolidFn` 和 `YiCadImportApi::createSolid`，宿主直接构造 `DmSolid`。

### 4.4 导出侧补充

沿用现有 `Document` 和 `EntityIterator`，不要增加 `ExportSnapshot`、`ExportDocument` 或一组 Range 类。导出在主线程同步执行，期间文档不被修改，因此直接只读枚举即可。

`Document` 增加文档设置、资源表、块和容器枚举；表数据量小，C++ 包装层返回值对象最简单：

```cpp
class Document
{
public:
    DocumentSettings settings() const;
    std::vector<LineTypeData> lineTypes() const;
    std::vector<LayerData> layers() const;
    std::vector<TextStyleData> textStyles() const;
    std::vector<DimensionStyleData> dimensionStyles() const;
    std::vector<BlockData> blocks() const;

    EntityIterator entities() const noexcept;
    EntityIterator entities(const BlockData& block) const noexcept;
};
```

`EntityIterator` 将现有仅支持直线和圆的接口扩展到第 2.2 节列出的类型。使用一个值类型 `EntityData` 承载当前实体，不为每种实体增加 iterator 类：

```cpp
using EntityData = std::variant<
    PointData, LineData, RayData, XLineData, ArcData, CircleData,
    EllipseData, PolylineData, SplineData, SolidData, TextData,
    MTextData, DimensionData, LeaderData, HatchData, InsertData,
    AttributeDefinitionData, AttributeData, ImageData>;

class EntityIterator
{
public:
    /// @brief 读取下一项；结束或读取失败时返回 false。
    bool next(EntityData& value) noexcept;

private:
    YiCadEntityIteratorHandle m_handle = nullptr;
    const YiCadHostApi* m_api = nullptr;
};
```

底层 v3 函数表为资源枚举、块枚举和各实体 getter 提供必要入口。C++ SDK 在一个位置完成 POD/view 到拥有型 C++ 值的转换；DXF 插件不检查 `structSize`、函数指针偏移或 ABI 版本范围。

## 5. 目录和类

```text
plugins/
  CMakeLists.txt
  demo_plugin/
    ...
  dxf_plugin/
    CMakeLists.txt
    README.md
    YiCadDxfPlugin.def
    dxf.xml.example
    src/
      DxfPlugin.cpp
      DxfInterfaceAdapter.h
      DxfImporter.h
      DxfImporter.cpp
      DxfExporter.h
      DxfExporter.cpp
      DxfMapping.h
      DxfMapping.cpp
```

只有三个有状态类：

- `DxfPlugin`：插件注册和文件回调入口；
- `DxfImporter`：实现 `libdxfrw` 读取回调并写入 `ImportSession`；
- `DxfExporter`：实现 `libdxfrw` 写出回调并读取 YiCAD 文档。

另有一个无状态基类 `DxfInterfaceAdapter`。`DRW_Interface` 把读取和写出方法都定义为纯虚函数；如果不提供该基类，导入器和导出器都要重复几十个空方法。该类只提供默认空实现，不保存数据，也不形成新的业务抽象。

`DxfMapping` 只提供无状态自由函数，不定义类。简单映射不需要对象生命周期。

仓库不增加任何 `tests/`、`test_*.cpp`、样例语料或 CTest 配置。

## 6. 每个类的核心代码

以下代码用于固定职责和调用关系。实现时补齐第 2 节支持字段，不增加新的架构层。

### 6.1 `DxfInterfaceAdapter`

```cpp
/// @brief 为 libdxfrw 未使用的纯虚回调提供空实现。
class DxfInterfaceAdapter : public DRW_Interface
{
public:
    void addHeader(const DRW_Header*) override {}
    void addLType(const DRW_LType&) override {}
    void addLayer(const DRW_Layer&) override {}
    void addDimStyle(const DRW_Dimstyle&) override {}
    void addVport(const DRW_Vport&) override {}
    void addTextStyle(const DRW_Textstyle&) override {}
    void addAppId(const DRW_AppId&) override {}
    void addBlock(const DRW_Block&) override {}
    void setBlock(const int) override {}
    void endBlock() override {}
    void addPoint(const DRW_Point&) override {}
    void addLine(const DRW_Line&) override {}
    void addRay(const DRW_Ray&) override {}
    void addXline(const DRW_Xline&) override {}
    void addArc(const DRW_Arc&) override {}
    void addCircle(const DRW_Circle&) override {}
    void addEllipse(const DRW_Ellipse&) override {}
    void addLWPolyline(const DRW_LWPolyline&) override {}
    void addPolyline(const DRW_Polyline&) override {}
    void addSpline(const DRW_Spline*) override {}
    void addKnot(const DRW_Entity&) override {}
    void addInsert(const DRW_Insert&) override {}
    void addTrace(const DRW_Trace&) override {}
    void add3dFace(const DRW_3Dface&) override {}
    void addSolid(const DRW_Solid&) override {}
    void addMText(const DRW_MText&) override {}
    void addText(const DRW_Text&) override {}
    void addDimAlign(const DRW_DimAligned*) override {}
    void addDimLinear(const DRW_DimLinear*) override {}
    void addDimRadial(const DRW_DimRadial*) override {}
    void addDimDiametric(const DRW_DimDiametric*) override {}
    void addDimAngular(const DRW_DimAngular*) override {}
    void addDimAngular3P(const DRW_DimAngular3p*) override {}
    void addDimOrdinate(const DRW_DimOrdinate*) override {}
    void addLeader(const DRW_Leader*) override {}
    void addHatch(const DRW_Hatch*) override {}
    void addViewport(const DRW_Viewport&) override {}
    void addImage(const DRW_Image*) override {}
    void linkImage(const DRW_ImageDef*) override {}
    void addComment(const char*) override {}
    void addPlotSettings(const DRW_PlotSettings*) override {}

    void writeHeader(DRW_Header&) override {}
    void writeBlocks() override {}
    void writeBlockRecords() override {}
    void writeEntities() override {}
    void writeLTypes() override {}
    void writeLayers() override {}
    void writeTextstyles() override {}
    void writeVports() override {}
    void writeDimstyles() override {}
    void writeObjects() override {}
    void writeAppId() override {}
};
```

导入器只覆盖需要读取的 `add*`，导出器只覆盖需要写出的 `write*`。DXF 独有数据自然落入空实现。

### 6.2 `DxfPlugin`

```cpp
namespace
{

constexpr auto PluginId = "com.yicad.dxf";
constexpr auto FormatId = "dxf";

class DxfPlugin final
{
public:
    bool initialize(const YiCadHostApi* api, YiCadPluginApi* plugin) noexcept
    {
        if (api == nullptr || plugin == nullptr)
        {
            return false;
        }

        m_host = yicad::plugin::Host(api);
        plugin->pluginId = PluginId;
        plugin->pluginName = "YiCAD DXF Plugin";
        plugin->pluginVersion = "1.0.0";

        return m_host.registerImportFilter(
                   PluginId, FormatId, "DXF Drawing", "dxf",
                   &DxfPlugin::importFile, this) &&
               m_host.registerExportFilter(
                   PluginId, FormatId, "DXF Drawing", "dxf",
                   &DxfPlugin::exportFile, this);
    }

    void shutdown() noexcept
    {
        m_host = {};
    }

private:
    static YiCadResult YICAD_PLUGIN_CALL importFile(
        YiCadDocumentHandle handle, const char* path, void* userData) noexcept
    {
        return yicad::plugin::invokeNoexcept<YiCadResult>([&]() {
            auto& self = *static_cast<DxfPlugin*>(userData);
            const auto document = self.m_host.document(handle);
            return path != nullptr && DxfImporter(document).read(path)
                ? YICAD_SUCCESS : YICAD_FAILURE;
        }, YICAD_FAILURE);
    }

    static YiCadResult YICAD_PLUGIN_CALL exportFile(
        YiCadDocumentHandle handle, const char* path, void* userData) noexcept
    {
        return yicad::plugin::invokeNoexcept<YiCadResult>([&]() {
            auto& self = *static_cast<DxfPlugin*>(userData);
            const auto document = self.m_host.document(handle);
            return path != nullptr && DxfExporter(document).write(path)
                ? YICAD_SUCCESS : YICAD_FAILURE;
        }, YICAD_FAILURE);
    }

    yicad::plugin::Host m_host;
};

DxfPlugin g_plugin;

} // namespace

YICAD_PLUGIN_EXPORT uint32_t YICAD_PLUGIN_CALL
yicad_plugin_get_abi_version()
{
    return YICAD_PLUGIN_ABI_V3;
}
```

入口文件继续提供 `yicad_plugin_init` 和 `yicad_plugin_shutdown`，直接转发给 `g_plugin`，与现有 demo 插件相同。

### 6.3 `DxfImporter`

```cpp
struct PendingInsert
{
    std::string blockName;
    yicad::plugin::ImportContainer container;
    yicad::plugin::InsertData data;
};

class DxfImporter final : public DxfInterfaceAdapter
{
public:
    explicit DxfImporter(yicad::plugin::Document document)
        : m_document(std::move(document))
    {
    }

    bool read(const char* path)
    {
        m_session = m_document.beginImport();
        if (!m_session ||
            m_session.modelSpace(m_modelSpace) != YICAD_IMPORT_SUCCESS)
        {
            return false;
        }

        m_currentContainer = m_modelSpace;
        dxfRW file(path);
        if (!file.read(this, false) || m_failed || !resolvePendingInserts())
        {
            return false;
        }
        return m_session.commit() == YICAD_IMPORT_SUCCESS;
    }

    void addLayer(const DRW_Layer& source) override
    {
        auto data = dxf::toLayer(source, m_lineTypes);
        yicad::plugin::ImportResource layer;
        const auto result = m_session.createLayer(
            data, YICAD_RESOURCE_CONFLICT_RENAME, layer);
        if (result == YICAD_IMPORT_SUCCESS)
        {
            m_layers.insert_or_assign(source.name, layer);
        }
        else
        {
            m_failed = true;
        }
    }

    void addLine(const DRW_Line& source) override
    {
        const auto attributes = dxf::toAttributes(source, m_layers, m_lineTypes);
        setFailed(m_currentContainer.createLine(
            dxf::point(source.basePoint), dxf::point(source.secPoint),
            attributes));
    }

    void addCircle(const DRW_Circle& source) override
    {
        setFailed(m_currentContainer.createCircle(
            dxf::point(source.basePoint), source.radious,
            dxf::toAttributes(source, m_layers, m_lineTypes)));
    }

    void addSolid(const DRW_Solid& source) override
    {
        setFailed(m_currentContainer.createSolid(
            dxf::toSolid(source),
            dxf::toAttributes(source, m_layers, m_lineTypes)));
    }

    void addInsert(const DRW_Insert& source) override
    {
        const auto found = m_blocks.find(source.name);
        if (found == m_blocks.end())
        {
            m_pendingInserts.push_back({
                source.name, m_currentContainer, dxf::toInsert(source)});
            return;
        }
        createInsert(m_currentContainer, found->second, dxf::toInsert(source));
    }

    // 其余受支持回调按同一模式直接转换并调用 ImportContainer。
    // DXF 独有对象的回调保持空实现。

private:
    void setFailed(YiCadImportResult result)
    {
        m_failed = m_failed || result != YICAD_IMPORT_SUCCESS;
    }

    bool resolvePendingInserts();
    void createInsert(
        const yicad::plugin::ImportContainer& container,
        const yicad::plugin::ImportResource& block,
        yicad::plugin::InsertData data);

    yicad::plugin::Document m_document;
    yicad::plugin::ImportSession m_session;
    yicad::plugin::ImportContainer m_modelSpace;
    yicad::plugin::ImportContainer m_currentContainer;
    std::unordered_map<std::string, yicad::plugin::ImportResource> m_lineTypes;
    std::unordered_map<std::string, yicad::plugin::ImportResource> m_layers;
    std::unordered_map<std::string, yicad::plugin::ImportResource> m_blocks;
    std::vector<PendingInsert> m_pendingInserts;
    bool m_failed = false;
};
```

`DxfImporter` 必须补齐第 2.2 节列出的 `add*` 回调。每个回调只做字段映射和一次 SDK 创建调用。`read()` 在提交前还要检查 `m_failed`；任何失败都不提交。

块处理使用 `addBlock`/`endBlock` 切换 `m_currentContainer`。属性值如果跟随一个尚未解析的 `INSERT`，与该 `PendingInsert` 一起保存；不建立完整块依赖图。

### 6.4 `DxfExporter`

```cpp
class DxfExporter final : public DxfInterfaceAdapter
{
public:
    explicit DxfExporter(yicad::plugin::Document document)
        : m_document(std::move(document))
    {
    }

    bool write(const char* path)
    {
        dxfRW file(path);
        m_writer = &file;
        const bool written = file.write(this, DRW::AC1027, false);
        m_writer = nullptr;
        return written;
    }

    void writeHeader(DRW_Header& header) override
    {
        dxf::writeHeader(m_document.settings(), header);
    }

    void writeLTypes() override
    {
        for (const auto& value : m_document.lineTypes())
        {
            auto data = dxf::toDxf(value);
            m_writer->writeLineType(&data);
        }
    }

    void writeLayers() override
    {
        for (const auto& value : m_document.layers())
        {
            auto data = dxf::toDxf(value);
            m_writer->writeLayer(&data);
        }
    }

    void writeTextstyles() override
    {
        for (const auto& value : m_document.textStyles())
        {
            auto data = dxf::toDxf(value);
            m_writer->writeTextstyle(&data);
        }
    }

    void writeDimstyles() override
    {
        for (const auto& value : m_document.dimensionStyles())
        {
            auto data = dxf::toDxf(value);
            m_writer->writeDimstyle(&data);
        }
    }

    void writeBlockRecords() override
    {
        for (const auto& block : m_document.blocks())
        {
            m_writer->writeBlockRecord(block.name());
        }
    }

    void writeBlocks() override
    {
        for (const auto& block : m_document.blocks())
        {
            auto data = dxf::toDxf(block);
            m_writer->writeBlock(&data);
            writeEntities(m_document.entities(block));
        }
    }

    void writeEntities() override
    {
        writeEntities(m_document.entities());
    }

private:
    void writeEntities(yicad::plugin::EntityIterator entities)
    {
        yicad::plugin::EntityData value;
        while (entities.next(value))
        {
            std::visit([&](const auto& entity) {
                dxf::writeEntity(*m_writer, entity);
            }, value);
        }
    }

    yicad::plugin::Document m_document;
    dxfRW* m_writer = nullptr;
};
```

具体的 `DRW_Interface` 写回调名称以 Conan 包中 `libdxfrw/2.2.0` 头文件为准。实现不缓存全量文档；`writeBlocks()` 和 `writeEntities()` 在 `libdxfrw` 请求对应段时直接枚举 YiCAD。

### 6.5 `DxfMapping` 自由函数

该文件没有类，核心接口如下：

```cpp
namespace dxf
{

yicad::plugin::Point2d point(const DRW_Coord& value) noexcept;
yicad::plugin::EntityAttributes toAttributes(
    const DRW_Entity& value,
    const ResourceMap& layers,
    const ResourceMap& lineTypes);
yicad::plugin::LayerData toLayer(
    const DRW_Layer& value,
    const ResourceMap& lineTypes);
yicad::plugin::SolidData toSolid(const DRW_Solid& value);
yicad::plugin::InsertData toInsert(const DRW_Insert& value);

void writeEntity(dxfRW& writer, const yicad::plugin::LineData& value)
{
    auto data = toDxf(value);
    writer.writeLine(&data);
}

void writeEntity(dxfRW& writer, const yicad::plugin::CircleData& value);
void writeEntity(dxfRW& writer, const yicad::plugin::PolylineData& value);
// 为第 2.2 节的其余 EntityData 候选类型提供同样的重载。

} // namespace dxf
```

颜色、线宽、角度、文本对齐等双向转换也放在该命名空间。只有被导入和导出共同使用的转换才提取函数；单次使用的字段赋值留在对应回调中。

## 7. 分步任务与责任分配

### 阶段 1：目录和构建

任务：

1. 将 `examples/demo_plugin` 移到 `plugins/demo_plugin`，同步根 CMake 和安装路径。
2. 创建 `plugins/dxf_plugin`，链接 `YiCAD::PluginSdk` 和 `libdxfrw::libdxfrw`。
3. 添加插件 `.def`、XML 示例、安装规则和第三方许可证说明。

责任：AI 完成全部代码和文档；人工仅在本机依赖缺失时提供 Qt/Conan 环境。

### 阶段 2：直接扩展 v3 SDK

任务：

1. 将插件最低、最高和当前 ABI 都固定为 v3，删除 demo 的 v2 fallback。
2. 给 v3 导入 API 增加 `DmSolid` 创建入口。
3. 给 v3 导出 API 增加文档设置、资源、块和全部一等实体的只读枚举/getter。
4. 在 `YiCadPluginSdk.h` 中提供第 4 节的 C++ 包装。
5. 宿主实现直接读取现有 `DmDocument`，跳过第 2.3 节内部对象。
6. 更新 SDK 文档，只描述 `YICAD_PLUGIN_ABI_V3`。

责任：AI 完成接口盘点、代码、编译修复和文档；人工只审核 C++ API 是否符合项目长期使用习惯。

### 阶段 3：导入

任务：

1. 实现 HEADER 和四类资源映射。
2. 实现第 2.2 节实体回调。
3. 使用 `ImportSession` 保证一次导入一次提交。
4. 使用最小 `PendingInsert` 解决前向块引用和关联属性。
5. DXF 独有回调保持空实现。

责任：AI 完成全部实现和构建；人工在 YiCAD 中打开代表性 DXF，确认图形、中文文字、块和标注显示符合预期。

### 阶段 4：导出

任务：

1. 实现 HEADER、TABLES、BLOCKS、ENTITIES 的直接写出。
2. 只枚举一等实体，避免导出 `DmLineStrip`、`DmChar`、`DmRegion` 等子对象。
3. `IMAGEDEF` 仅为模型空间 `DmImage` 生成。
4. 默认写 ASCII DXF 2013（`AC1027`）。

责任：AI 完成全部实现和构建；人工使用 YiCAD、LibreCAD/QCAD 以及可用的商业 CAD 打开导出文件并目视确认。

### 阶段 5：交付

任务：

1. 完成 Debug/Release 构建和安装。
2. 更新中英文 README、插件部署说明和许可证清单。
3. 在无开发环境机器上复制插件目录并确认 YiCAD 可发现插件。

责任：AI 完成仓库内工作、构建命令执行和问题修复；人工完成外部 CAD 操作、安装包签名、发布机验证和最终验收。

## 8. 验收方式

不编写自动化测试代码。每阶段只执行与交付直接相关的验证：

1. `cmake --build --preset Debug` 和 `cmake --build --preset Release` 成功。
2. YiCAD 能发现插件，打开和另存为对话框出现 DXF。
3. 选取包含第 2.2 节对象的实际图纸，执行 DXF -> YiCAD 和 YiCAD -> DXF。
4. 人工检查实体数量、位置、颜色、图层、文字、标注、填充和块引用。
5. 用至少一个第三方 CAD 打开导出文件，确认可正常显示。
6. 导入失败时文档不留下本次部分数据。

不要求字节级往返一致，不要求保留 YiCAD 没有的数据，也不把被明确排除的 DXF 对象计入失败。

## 9. 完成定义

同时满足以下条件即完成：

1. DXF 插件位于 `plugins/dxf_plugin`，不包含 YiCAD 内部头文件。
2. 插件直接使用 `YICAD_PLUGIN_ABI_V3`，仓库没有新增 v4 定义和版本兼容逻辑。
3. 插件业务代码使用 C++ SDK，不直接操作底层 C 函数表。
4. 第 2.1 和 2.2 节列出的数据完成双向转换。
5. 第 2.3 节对象以及 YiCAD 不存在的 DXF 数据不导入、不导出。
6. 没有 `DxfDocument` 和其他全量中间文档模型。
7. 没有新增测试工程、测试代码或测试语料。
8. Debug/Release 构建通过，并完成人工跨 CAD 验收。
9. 安装、部署、README 和许可证信息同步更新。
