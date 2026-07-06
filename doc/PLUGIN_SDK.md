# YiCAD 插件 SDK 使用指南

YiCAD 插件使用稳定 C ABI 与宿主交互。官方 C++23 SDK 是只包含头文件的便利封装，不依赖 Qt 或 YiCAD 私有头文件。

## 安装内容与构建要求

安装 YiCAD 后，SDK 交付物位于安装前缀下：

```text
include/YiCAD/plugin-sdk/
  YiCadPluginAbi.h
  YiCadPluginSdk.h
lib/cmake/YiCADPluginSdk/
  YiCADPluginSdkConfig.cmake
  YiCADPluginSdkConfigVersion.cmake
  YiCADPluginSdkTargets.cmake
share/YiCAD/sdk/
  PLUGIN_SDK.md
  PLUGIN_ABI_EVOLUTION.md
  PLUGIN_ABI_V3_REFERENCE.md
  PLUGIN_ABI_V3_RELEASE.md
  licenses/LICENSE
share/YiCAD/examples/demo_plugin/
```

插件工程需要 CMake 3.21 或更高版本、C++23 编译器和与 YiCAD 进程相同的 Windows 架构。
ABI v3 的正式支持范围是 Win64；头文件保留 Win32 布局断言以防止布局漂移，但当前发布
没有完成 Win32 依赖、构建和运行时验收，因此不构成 Win32 支持声明。最小
`CMakeLists.txt`：

YiCAD 使用两个安装组件。普通用户只安装运行时，插件开发者额外安装 SDK；不指定组件时，CMake 会安装两者：

```powershell
cmake --build --preset Release-Runtime
cmake --build --preset Release-PluginSDK
```

对应的 Debug 预设为 `Debug-Runtime` 和 `Debug-PluginSDK`。这些构建预设调用组件化安装目标，无需再手写 `cmake --install --component`。两个组件可以使用同一个安装前缀。`Runtime` 包含可执行程序、运行库和资源；`PluginSDK` 包含本节列出的公开头、CMake package、文档、许可证和示例源码。

最小插件工程的 `CMakeLists.txt`：

```cmake
cmake_minimum_required(VERSION 3.21)
project(MyYiCadPlugin LANGUAGES CXX)

find_package(YiCADPluginSdk CONFIG REQUIRED)

add_library(MyYiCadPlugin MODULE MyPlugin.cpp)
target_link_libraries(MyYiCadPlugin PRIVATE YiCAD::PluginSdk)
set_target_properties(MyYiCadPlugin PROPERTIES PREFIX "")
```

配置和构建：

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\path\to\YiCAD"
cmake --build build --config Release
```

也可将 `YiCADPluginSdk_DIR` 指向 `lib/cmake/YiCADPluginSdk`。`YiCAD::PluginSdk` 自动提供公开 include 目录、C++23 要求和插件导出定义；插件不得添加 YiCAD 源码目录作为 include 路径。

## 固定导出入口

插件 DLL 必须提供以下三个 C ABI 入口，名称和 `__cdecl` 调用约定不可改变：

```cpp
#include <YiCadPluginSdk.h>

YICAD_PLUGIN_EXPORT uint32_t YICAD_PLUGIN_CALL
yicad_plugin_get_abi_version(void)
{
    return YICAD_PLUGIN_ABI_MAX_VERSION;
}

YICAD_PLUGIN_EXPORT YiCadResult YICAD_PLUGIN_CALL
yicad_plugin_init(const YiCadHostApi* host, YiCadPluginApi* plugin)
{
    return yicad::plugin::invokeNoexcept<YiCadResult>([&]() {
        // 校验函数表、填写元数据并完成所有注册。
        return YICAD_SUCCESS;
    }, YICAD_FAILURE);
}

YICAD_PLUGIN_EXPORT void YICAD_PLUGIN_CALL
yicad_plugin_shutdown(void)
{
    yicad::plugin::invokeNoexcept([&]() {
        // 停止线程并释放所有宿主回调引用。
    });
}
```

`init` 必须填写宿主提供的 `YiCadPluginApi`：保留宿主给出的 `structSize` 和协商后的 `abiVersion`，并设置非空 UTF-8 `pluginId`、`pluginName` 和 `pluginVersion`。这些字符串至少保持有效到 `shutdown` 返回。推荐使用反向 DNS 形式的稳定 `pluginId`，例如 `com.vendor.product`。

如果任一注册失败，`init` 应返回 `YICAD_FAILURE`。宿主会整体回滚本次注册，并在卸载 DLL 前调用一次 `shutdown`。成功插件在应用退出时按初始化的逆序关闭。

## XML 清单与部署

YiCAD 只扫描字面路径 `C:\ProgramData\YiCAD\plugins` 第一层的 `*.xml`，不递归扫描。每个 XML 只允许一个根元素和一个 `dll` 属性：

```xml
<?xml version="1.0" encoding="UTF-8"?>
<plugin dll="D:\YiCADPlugins\MyPlugin\MyYiCadPlugin.dll"/>
```

相对路径以 XML 所在目录为基准，例如：

```xml
<plugin dll="my-plugin/MyYiCadPlugin.dll"/>
```

DLL 可以位于扫描目录之外。YiCAD 不展开环境变量；最终路径必须是存在的绝对 `.dll` 文件。真实 ProgramData 目录应只允许管理员或受信任安装程序写入，因为原生插件与 YiCAD 同进程运行并拥有相同权限。

## 命令与 Ribbon

用 `yicad::plugin::Host` 包装宿主函数表，并在 `init` 期间先注册命令，再注册引用该命令的 Ribbon 按钮：

```cpp
yicad::plugin::Host sdkHost(host);
sdkHost.registerCommand(
    "com.vendor.product",
    "draw.sample",
    "Draw sample",
    &executeSample,
    userData);
sdkHost.registerRibbonButton(
    "com.vendor.product",
    "Vendor",
    "Draw",
    "draw.sample",
    "icons/sample.svg");
```

命令行规范名为 `pluginId/commandId`。回调和 `userData` 必须保持有效到 `shutdown`。图标相对路径以 DLL 所在目录为基准；图标缺失只会产生无图标按钮。

## 文档与几何

`Host::currentDocument()` 返回非拥有的 `Document`。不要缓存文档或底层 `YiCadDocumentHandle`；每次命令执行时重新获取。可用的基础操作包括添加直线、添加圆、重生成和自动缩放：

```cpp
const auto document = sdkHost.currentDocument();
if (document && document.addLine(0.0, 0.0, 100.0, 100.0))
{
    document.regen();
    document.zoomAuto();
}
```

ABI v2 提供 `Document::beginTransaction()`。`DocumentTransaction` 成功提交后形成一个撤销项；未提交对象析构时自动回滚。ABI v2 的 `Document::entities()` 返回只读 POD 快照迭代器，目前可查询直线和圆；`EntityIterator` 析构时归还宿主句柄。

## 导入与导出

导入和导出在 `init` 期间注册：

```cpp
sdkHost.registerImportFilter(
    pluginId, "sample", "Sample Drawing", "sample", importFile, userData);
sdkHost.registerExportFilter(
    pluginId, "sample", "Sample Drawing", "sample", exportFile, userData);
```

扩展名不带前导点。导出格式的规范名为 `pluginId/formatId`。文件回调收到有效期仅限本次调用的文档句柄和 UTF-8 路径；可通过 `Host::document(handle)` 获取 SDK 文档对象。批量导入优先使用 v3 导入会话；协商到 v2 时可显式回退到文档事务。任何解析或添加失败都应让 RAII 对象回滚并返回 `YICAD_FAILURE`。导出应使用只读实体快照，不得访问 YiCAD 内部实体指针。

## ABI 版本兼容

插件从 `yicad_plugin_get_abi_version()` 返回自身支持的最高版本，宿主选择双方支持的最高共同版本。插件必须同时检查 `abiVersion` 和 `structSize`；官方 SDK 封装会在读取函数指针前检查字段边界。缺失的尾部能力返回失败或空对象，不应越界访问。

ABI v1 包含命令、Ribbon、当前文档、基础几何、刷新、缩放和导入导出注册。ABI v2 只在 v1 尾部追加事务与只读实体枚举。详细规则和兼容矩阵见 [PLUGIN_ABI_EVOLUTION.md](PLUGIN_ABI_EVOLUTION.md)。

ABI v3 已正式发布，`YICAD_PLUGIN_ABI_MAX_VERSION` 和 `YICAD_PLUGIN_ABI_VERSION` 均为
`YICAD_PLUGIN_ABI_V3`。C++ 封装提供 `ImportSession`、`ImportContainer` 和
`ImportResource`：会话析构
自动回滚，容器负责创建实体，资源包装通过 `nativeHandle()` 填入关联资源的 POD 字段。
每个包装方法都会分别检查协商版本、子表 `structSize` 和函数指针；截短子表只让对应
尾部能力返回 `YICAD_IMPORT_ERROR_UNSUPPORTED`，不会影响已有前缀能力。完整实施范围和
底层契约见 [PLUGIN_ABI_V3_REFERENCE.md](PLUGIN_ABI_V3_REFERENCE.md)。

v3 普通 SDK 用法不得直接填写 `structSize`、数组字节步长或嵌套 ABI 指针，也不得依赖
零初始化偶然产生有效业务默认值。SDK 必须通过默认构造、语义工厂或 builder 自动生成
临时 C ABI POD，并设置公共属性默认值、所有可扩展结构大小、嵌套借用指针、数组数量和
字节步长。普通示例只展示几何、资源和导入事务语义。

正式 SDK 提供默认构造层和按领域划分的高层输入类型。`EntityAttributes` 默认生成活动
图层、ByLayer 线型和颜色、标准线宽 `-1`、线型比例 `1.0`、可见以及法向量
`(0, 0, 1)`。基础几何直接使用语义重载；SDK 在调用宿主前生成临时 POD 并连接公共属性
指针：

```cpp
auto attributes = yicad::plugin::EntityAttributes{};
attributes.setLayer(layer);

modelSpace.createPoint({10.0, 20.0}, attributes);
modelSpace.createLine({0.0, 0.0}, {100.0, 50.0}, attributes);
modelSpace.createCircle({50.0, 50.0}, 25.0, attributes);
modelSpace.createArc({50.0, 50.0}, 25.0, 0.0, 3.1415926, attributes);
modelSpace.createRay({0.0, 0.0}, {1.0, 1.0}, attributes);
```

`layer` 和 `modelSpace` 分别是当前活动导入会话中的 `ImportResource` 和
`ImportContainer`。省略最后一个参数即可使用相同的标准公共属性。

资源输入拥有名称、说明、字体路径和元素数组。资源句柄仍由当前活动会话约束有效期：

```cpp
auto lineTypeData = yicad::plugin::LineTypeData("DASHED");
lineTypeData.setDescription("Dashed").setElements({12.0, -6.0});

yicad::plugin::ImportResource lineType;
session.createLineType(lineTypeData, YICAD_RESOURCE_CONFLICT_FAIL, lineType);

auto layerData = yicad::plugin::LayerData("STRUCTURE");
layerData.setLineType(lineType).setColor({YICAD_COLOR_RGB, 0, 255, 0, 0, 0});

yicad::plugin::ImportResource layer;
session.createLayer(layerData, YICAD_RESOURCE_CONFLICT_FAIL, layer);
```

数组几何由 SDK 对象拥有连续容器，调用期间才生成数组视图：

```cpp
auto polyline = yicad::plugin::PolylineData({
    {{0.0, 0.0}, 0.0, 0.0, 0.0},
    {{100.0, 0.0}, 0.0, 0.0, 0.0},
    {{100.0, 50.0}, 0.0, 0.0, 0.0}});
polyline.setClosed(true).setAttributes(attributes);
modelSpace.createPolyline(polyline);

auto spline = yicad::plugin::SplineData{};
spline.setControlPoints(3, controlPoints, knots);
modelSpace.createSpline(spline);
```

文字和块输入同样拥有 UTF-8 字符串；嵌套文字、属性和块引用句柄只在宿主调用期间连接：

```cpp
auto text = yicad::plugin::TextData("设备编号");
text.setPlacement({20.0, 30.0}).setMetrics(2.5).setStyle(textStyle);
modelSpace.createText(text);

auto blockData = yicad::plugin::BlockData("VALVE");
yicad::plugin::ImportResource block;
yicad::plugin::ImportContainer blockContainer;
session.beginBlock(blockData, block, blockContainer);
// 向 blockContainer 添加实体后调用 blockContainer.endBlock()。

auto insertData = yicad::plugin::InsertData(block);
insertData.setPlacement({80.0, 40.0});
yicad::plugin::ImportResource insert;
modelSpace.createInsert(insertData, insert);
```

标注、引线、填充和图像使用拥有型输入保留全部 ABI 能力。填充会自动生成环/边步长，
不会把样条边或孔环静默降级：

```cpp
auto hatch = yicad::plugin::HatchData{};
hatch.setPattern("ANSI31", 1.0, 0.0)
    .addPolylineLoop(boundary, YICAD_HATCH_LOOP_OUTER);
modelSpace.createHatch(hatch);

auto leader = yicad::plugin::LeaderData({{0.0, 0.0}, {20.0, 10.0}});
leader.setArrow(true).setText(yicad::plugin::TextData("说明"));
modelSpace.createLeader(leader);

auto image = yicad::plugin::ImageData("references/site.png");
image.setGeometry({0.0, 0.0}, {0.1, 0.0}, {0.0, 0.1}, {1920.0, 1080.0});
modelSpace.createImage(image);
```

上述 SDK 对象均只存在于插件侧，跨 DLL 边界的仍然只有调用期间生成的 C ABI POD。
无效必需字符串、超出 ABI 数量范围的容器和过期必需资源会在调用宿主前返回确定错误。
宿主仍负责校验几何范围、枚举值和资源所属会话，并返回与裸 POD 路径一致的结果码。

`makeImportData<T>()`、`makeHatchEdgeArrayView()` 和 `makeHatchLoopArrayView()` 继续作为
下述高级裸 ABI POD 接口的初始化工具。它们自动设置当前结构大小、协议默认值和数组
步长，但普通插件路径应优先使用上述高层类型。

普通 SDK 示例的固定准入规则如下：

- 示例中不出现对 `structSize` 或 `byteStride` 的赋值；
- 示例不直接读写宿主函数表，也不拼装借用指针；
- 默认公共属性由 SDK 产生，调用方只覆盖有业务意义的值；
- SDK 对象只存在于插件侧，跨 DLL 边界时临时转换为 C ABI POD；
- 宿主仍须在调用返回前复制输入，不能保存 SDK 生成的视图或指针。

### 高级裸 ABI POD 接口

`ImportSession` 和 `ImportContainer` 中接受 `const YiCad*DataV3&` 的重载保持可用，供
非官方 SDK、其他语言绑定及 ABI 兼容性验证使用。C++ 调用方应以
`makeImportData<T>()` 创建 POD；绕过官方 SDK 时，调用方必须自行清零结构、把
`structSize` 设置为实际可访问字节数，并保证所有借用指针和数组视图在宿主函数返回前
有效。宿主不会保存这些地址。

以下代码只说明不使用官方 C++ 初始化器时的底层协议规则，不是普通插件的推荐写法：

```cpp
YiCadLineDataV3 data{};
data.structSize = sizeof(data);
data.startPoint = {0.0, 0.0};
data.endPoint = {100.0, 50.0};

// attributes == nullptr 使用宿主定义的完整标准公共属性。
modelSpace.createLine(data);
```

这里的大小必须是调用方实际可访问的字节数，而不是接收方头文件中的完整结构大小。
自定义绑定若只提供已知前缀，应填写该前缀的实际大小；宿主按该范围读取字段并忽略未知
尾部。可扩展数组还必须提供真实元素字节步长，且相关存储在调用返回前持续有效。

文件解析器属于插件实现依赖。插件应自行链接 `libdxfrw` 或其他 DXF、SVG、DGN 解析库，
并把解析结果转换为 ABI POD；`YiCAD::PluginSdk` 不链接、不传播，也不跨 ABI 传递
`libdxfrw` 类型或运行库。

## 线程、异常、字符串与所有权

- 注册和宿主回调只允许在 YiCAD UI 主线程调用；当前 ABI 不承诺线程安全。
- 异常不得穿过任何 C ABI 入口或回调。入口与回调应为 `noexcept`，并可用
  `yicad::plugin::invokeNoexcept` 把全部异常转换为失败值或安全吞掉无返回值异常。
- 所有跨 ABI 字符串均为 UTF-8。宿主在注册函数返回前复制注册参数；插件元数据字符串保持有效到 `shutdown` 返回。
- 宿主函数表保持有效到 `shutdown` 返回。插件不得在此之后调用函数表。
- 文档 handle 非拥有且不可长期保存。事务和迭代器由宿主创建，由插件显式提交、回滚或销毁；官方 SDK 的 RAII 类型负责失败路径清理。
- 不跨 DLL 传递 C++、Qt、STL 对象、异常或由另一侧释放的内存。Debug/Release 和运行库差异不得影响 C ABI，但插件仍必须避免跨边界分配与释放。

## 旧插件迁移

旧 `QPluginLoader`、`PluginInterface`、`Document_Interface` 和 `Database_Interface` Qt/C++ 插件 ABI 已移除，没有二进制或源码兼容层。旧插件 DLL 无法由新运行时加载，必须使用本 SDK 重写边界、重新构建，并改用 XML 清单部署。

完整工程参见 `examples/demo_plugin`。SDK 头和示例按仓库根目录 `LICENSE` 的 GPLv3-or-later 条款分发；插件作者应自行确认其交付物的许可证兼容性。
