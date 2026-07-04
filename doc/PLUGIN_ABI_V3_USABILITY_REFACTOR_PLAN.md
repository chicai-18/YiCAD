# Plugin ABI v3 易用性与可扩展布局改造计划

## 1. 背景和结论

当前 ABI v3 尚未发布，因此可以在不破坏已发布插件的前提下修正候选布局。该修改不是
单纯增加几个 SDK 工厂函数，而是同时涉及 ABI 结构布局、宿主解码、SDK 默认值、示例和
兼容性验证，必须在 `PLUGIN_ABI_V3_PLAN.md` 的阶段 7 和阶段 8 之前完成。

本计划采用以下分层：

- C ABI 继续使用 POD、固定宽度类型、函数表和 `structSize`，作为稳定的底层协议；
- C++ PluginSDK 负责设置 `structSize`、有效默认值和嵌套关系；
- 普通插件代码不直接读写函数表，也不直接填写 `structSize`；
- v3 发布后只允许扩展明确标记为可扩展的结构，不把所有公开结构都视为可扩展结构。

该工作应拆成独立、可构建的提交，不应在一个提交中同时重写全部 ABI、宿主和示例。

## 2. 当前问题

### 2.1 机械初始化泄漏到插件代码

当前最小直线示例要求调用者逐层填写：

```cpp
YiCadLineDataV3 line{};
line.structSize = sizeof(line);
line.attributes.structSize = sizeof(line.attributes);
line.attributes.color.structSize = sizeof(line.attributes.color);
line.attributes.color.method = YICAD_COLOR_BY_LAYER;
line.attributes.lineWidth = -1;
line.attributes.lineTypeScale = 1.0;
line.attributes.visible = 1;
line.attributes.normal.z = 1.0;
```

其中多数代码属于 ABI 记账或默认值，不是插件业务数据。漏写任意嵌套大小或非零默认值
只会在运行时失败。

### 2.2 可扩展结构不能安全地按值嵌套

`YiCadEntityAttributes` 位于多数实体结构的中间。如果以后给它追加尾字段，实体结构中
位于 attributes 后面的几何字段偏移会一起变化。`YiCadColorData` 按值嵌入 attributes、
图层和标注样式时也存在相同问题。嵌套结构自己的 `structSize` 无法告诉宿主外层后续字段
实际位于哪个偏移。

因此必须禁止以下布局：

```cpp
struct Outer
{
    uint32_t structSize;
    ExtensibleStruct nested;
    double laterField;
};
```

### 2.3 可扩展数组元素缺少步长

`YiCadHatchLoopDataV3` 和 `YiCadHatchEdgeDataV3` 通过 `data + count` 数组视图传递。
如果数组元素将来追加字段，旧端和新端使用不同的 `sizeof(Element)` 进行下标运算，元素
自身的 `structSize` 不能解决数组步长不同的问题。

### 2.4 部分宿主校验不能接受旧前缀

宿主对顶层输入大多使用“最后必需字段末端偏移”，但颜色和实体公共属性仍使用当前
`sizeof(结构)` 校验。结构追加字段后，新宿主会拒绝旧插件提供的合法前缀。

## 3. 目标使用方式

底层 C ABI 仍允许高级插件直接构造 POD：

```cpp
YiCadLineDataV3 line = {/* 完整的底层 ABI 初始化 */};
api->createLine(session, container, &line);
```

官方 C++ SDK 的常规用法不得要求 `structSize`：

```cpp
auto attributes = yicad::plugin::EntityAttributes{};
attributes.setLayer(layer);

const auto result = modelSpace.createLine(
    {x1, y1},
    {x2, y2},
    attributes);
```

复杂实体允许使用 SDK 数据工厂或 builder，但仍不得手工设置 ABI 元数据：

```cpp
auto hatch = yicad::plugin::HatchData{};
hatch.setPattern("ANSI31", 1.0, 0.0);
hatch.addPolylineLoop(vertices, yicad::plugin::HatchLoopRole::Outer);
modelSpace.createHatch(hatch);
```

SDK 对象只能在插件侧存在。跨 DLL 边界时，SDK 必须临时生成 C ABI POD，并继续遵守
“宿主在调用返回前复制输入”的规则。

## 4. v3 候选 ABI 布局规则

在修改代码前，先在 `PLUGIN_ABI_V3_PLAN.md` 和 `PLUGIN_ABI_EVOLUTION.md` 固化以下规则：

1. 只有独立传入函数或通过指针引用的结构才能声明为可扩展结构。
2. 可扩展结构首字段为 `uint32_t structSize`，只允许追加尾字段。
3. 可扩展结构不得按值嵌入另一个结构；如需组合，使用调用期间借用的 `const T*`。
4. 按值嵌入的点、向量、字符串视图、数组视图、颜色等值类型是固定布局类型，不带
   `structSize`；扩展时定义新类型或在拥有它的可扩展结构中追加新字段。
5. 可扩展数组元素必须携带显式字节步长；固定布局数组元素可以只携带指针和数量。
6. `structSize` 表示调用方实际可访问字节数，不要求等于接收方的 `sizeof(T)`。
7. 宿主只读取 `structSize` 已覆盖的字段；缺失尾字段使用文档规定的默认值。
8. 大于宿主已知大小的结构合法，宿主忽略未知尾字段。
9. 空的可选嵌套指针必须具有明确语义，禁止让宿主猜测。

同时修正阶段 7 的“所有公开结构都有 `structSize`”表述，改为“所有可扩展公开结构都有
`structSize`，所有固定布局值类型都有冻结的大小和对齐断言”。

## 5. 分步实施

### 步骤 0：冻结规则和建立修改基线

涉及文件：

- `doc/PLUGIN_ABI_V3_PLAN.md`
- `doc/PLUGIN_ABI_EVOLUTION.md`
- `doc/PLUGIN_SDK.md`

修改内容：

- 写入第 4 节的布局规则；
- 把本改造插入原阶段 6 和阶段 7 之间；
- 明确 v3 仍为草案，修改期间不得提升 `YICAD_PLUGIN_ABI_MAX_VERSION`；
- 列出当前 Win32/Win64 候选结构大小和关键偏移，作为重构前快照；
- 明确普通 SDK 示例不允许直接填写 `structSize`。

完成条件：

- 文档不再声称所有公开结构都需要 `structSize`；
- 对固定布局类型、可扩展类型、嵌套指针和数组步长的规则没有歧义；
- 只修改文档，现有构建保持不变。

建议提交：`define abi v3 extensible layout rules`

### 步骤 1：收口固定布局值类型

涉及文件：

- `YiCAD/src/plugin_runtime/YiCadPluginAbi.h`
- `YiCAD/src/plugin_runtime/HostApi.cpp`
- `examples/demo_plugin/DemoPlugin.cpp`

修改内容：

- 将 `YiCadColorData` 定义为固定布局值类型并移除其 `structSize`；
- 保持点、向量、字符串视图、基础数组视图和 `YiCadVertex2d` 为固定布局值类型；
- 修改 `toDmColor` 及所有颜色调用点，不再校验嵌套颜色大小；
- 为固定布局类型增加 Win32/Win64 通用的大小、对齐和字段偏移静态断言；
- 暂不增加高层 SDK API，只确保底层候选 ABI 和宿主同步。

完成条件：

- 颜色按值嵌入不再具有虚假的独立扩展承诺；
- demo 和宿主中不存在 `color.structSize`；
- Debug 和 Release 均可编译宿主与 demo plugin。

建议提交：`fix abi v3 fixed value layouts`

### 步骤 2：消除可扩展结构的按值嵌套

涉及文件：

- `YiCAD/src/plugin_runtime/YiCadPluginAbi.h`
- `YiCAD/src/plugin_runtime/HostApi.cpp`
- `YiCAD/src/plugin_runtime/HostApi.h`
- `YiCAD/src/plugin_runtime/YiCadPluginSdk.h`

修改内容：

- 将所有实体结构中的公共属性改为调用期间借用的
  `const YiCadEntityAttributes* attributes`；
- 规定 `attributes == nullptr` 表示完整的标准默认属性；
- 将 `YiCadMTextDataV3::background` 改为可选指针，空指针表示不启用背景；
- 将 `YiCadAttributeDefinitionDataV3`、`YiCadAttributeDataV3` 和
  `YiCadLeaderDataV3` 中复用的 `YiCadTextDataV3` 改为借用指针，分别规定必需或可选语义；
- 宿主增加集中式公共属性规范化函数，负责空指针默认值、前缀大小检查和字段验证；
- 不允许宿主直接保存这些输入指针，所有数据必须在 API 返回前复制。

默认公共属性固定为：

- 活动图层；
- ByLayer 线型；
- ByLayer 颜色；
- 标准线宽 `-1`；
- 线型比例 `1.0`；
- 可见；
- 法向量 `(0, 0, 1)`。

完成条件：

- `YiCadPluginAbi.h` 中不存在可扩展结构按值嵌入另一个结构的情况；
- 空 attributes 的所有实体都得到相同默认值；
- 非空 attributes 按其可访问前缀读取，未知尾字段被忽略；
- 每次提交仍可独立构建。

建议提交：`fix abi v3 nested extensible inputs`

### 步骤 3：修复可扩展结构数组

涉及文件：

- `YiCAD/src/plugin_runtime/YiCadPluginAbi.h`
- `YiCAD/src/plugin_runtime/HostApi.cpp`
- `YiCAD/src/plugin_runtime/YiCadPluginSdk.h`

修改内容：

- 给 hatch loop 和 hatch edge 的数组视图增加 `byteStride`；
- 明确 `byteStride` 至少覆盖该版本必需前缀，并满足元素对齐要求；
- 宿主使用经过溢出检查的字节指针运算访问元素，不再使用普通 `data[index]`；
- 校验 `count * byteStride` 溢出、空指针、过小步长和错误对齐；
- SDK 从自身持有的连续容器自动生成 `data`、`count` 和 `byteStride`；
- 固定布局元素数组继续使用简单的 `data + count`，不增加无意义步长。

完成条件：

- 旧前缀元素和带未知尾字段的较大元素都能被正确遍历；
- 恶意 count/stride 组合在解引用前被拒绝；
- hatch 不再依赖宿主与插件具有相同的元素 `sizeof`。

建议提交：`add abi v3 extensible array strides`

### 步骤 4：统一宿主的前缀解码

涉及文件：

- `YiCAD/src/plugin_runtime/HostApi.cpp`
- `YiCAD/src/plugin_runtime/HostApi.h`
- `YiCAD/src/plugin_runtime/YiCadPluginAbi.h`

修改内容：

- 为每个 v3 可扩展输入定义“当前版本最小必需大小”，使用最后必需字段的末端偏移，
  不使用接收方当前 `sizeof(T)` 作为兼容性门槛；
- 增加统一的 `hasStructField`/`validStructPrefix` 辅助函数，使用防下溢写法检查字段范围；
- 审查所有输入函数，确保校验 `structSize` 后才读取对应字段；
- 对未来可选尾字段写清缺失默认值；当前没有可选尾字段的结构仍使用当前 v3 最小前缀；
- 删除 `sizeof(YiCadEntityAttributes)` 一类会随头文件增长的校验；
- 大于已知大小的输入只忽略尾部，不能报错。

完成条件：

- 每个可扩展输入只有一个权威最小前缀定义；
- 全局搜索不存在用完整 `sizeof(可扩展结构)` 拒绝旧前缀的代码；
- 宿主不会在大小检查前读取尾字段。

建议提交：`harden abi v3 input prefix decoding`

### 步骤 5：增加 SDK 默认构造层

涉及文件：

- `YiCAD/src/plugin_runtime/YiCadPluginSdk.h`
- `doc/PLUGIN_SDK.md`

修改内容：

- 为每个可扩展 ABI 输入提供 SDK 初始化入口，自动清零并设置正确 `structSize`；
- 对具有非零默认值的类型使用显式语义工厂，禁止依赖零初始化恰好有效；
- 提供默认 `EntityAttributes` SDK 类型，内部生成合法的 `YiCadEntityAttributes`；
- SDK 在调用宿主前生成临时 POD，并设置所有嵌套指针、数组数量和步长；
- 保留接受裸 POD 的高级入口，但在文档中标记为底层接口；
- 不修改 C ABI 函数签名，不让 STL、Qt 或 C++ SDK 类型跨 DLL 边界。

建议至少提供以下两层入口：

```cpp
auto data = yicad::plugin::makeImportData<YiCadLineDataV3>();
```

以及常用实体的语义重载：

```cpp
modelSpace.createPoint(position, attributes);
modelSpace.createLine(startPoint, endPoint, attributes);
modelSpace.createCircle(center, radius, attributes);
```

完成条件：

- 使用官方 SDK 构造任意已支持输入时不需要手写 `structSize`；
- SDK 生成的默认公共属性可直接被宿主接受；
- 裸 POD 入口仍可用于高级调用和 ABI 验证。

建议提交：`add abi v3 sdk data initializers`

### 步骤 6：按领域增加高层 SDK 封装

不要一次实现所有 builder，按以下顺序提交，每一步都更新对应文档示例：

1. 资源：文档设置、线型、图层、文字样式、标注样式；
2. 基础几何：点、线、射线、构造线、圆弧、圆、椭圆；
3. 数组几何：多段线和样条；
4. 文字与块：TEXT、MTEXT、BLOCK、INSERT、ATTDEF、ATTRIB；
5. 标注、引线、填充和图像。

每个领域都遵守：

- SDK 对象拥有插件侧容器和字符串，C ABI 视图只在调用时临时生成；
- `ImportResource` 和 `ImportContainer` 的现有会话有效期检查继续生效；
- 无效 SDK 状态在调用宿主前返回确定错误，不制造悬空视图；
- 复杂能力不为了简化 API 而静默丢字段或降级。

完成条件：

- 每个 v3 导入能力至少有一个不直接操作 POD 元数据的 SDK 路径；
- 高层路径和裸 POD 路径返回相同的宿主结果码；
- SDK 只隐藏机械细节，不改变 ABI 的失败语义。

建议提交前缀：`add abi v3 sdk <domain> builders`

### 步骤 7：迁移 demo 和 SDK 文档

涉及文件：

- `examples/demo_plugin/DemoPlugin.cpp`
- `examples/demo_plugin/README.md`
- `doc/PLUGIN_SDK.md`
- `doc/PLUGIN_ABI_V3_PLAN.md`

修改内容：

- demo 只使用常规 C++ SDK，不直接填写 `structSize`；
- SDK 指南先展示高层用法，再单独说明底层 ABI POD 规则；
- 删除鼓励逐层手工初始化的最小示例；
- 保留一小段裸 ABI 示例，用于解释非官方 SDK 或其他语言绑定如何设置实际大小；
- 将原计划阶段 6 的完成条件补充为“demo 和常规 SDK 示例不写 ABI 元数据”。

完成条件：

```powershell
rg -n "structSize\s*=\s*sizeof" examples/demo_plugin doc/PLUGIN_SDK.md
```

上述命令在常规示例中无匹配；若底层 ABI 说明保留匹配，必须位于明确标记的高级章节。

建议提交：`migrate demo to abi v3 sdk builders`

### 步骤 8：兼容性验证和发布门禁

涉及文件：

- `YiCAD/src/plugin_runtime/YiCadPluginAbi.h`
- `YiCAD/src/plugin_runtime/HostApi.cpp`
- `YiCAD/src/plugin_runtime/YiCadPluginSdk.h`
- `CMakeLists.txt`
- 可选的 `tests/plugin_abi/`

验证内容：

- 为固定布局类型和 v1/v2 已发布前缀保留静态断言；
- 为 v3 当前最小前缀增加静态断言，但不把接收方完整 `sizeof` 当作读取条件；
- 验证较小合法前缀、当前大小和带未知尾部的较大结构；
- 验证空/默认 attributes、非空旧前缀 attributes 和非法截短 attributes；
- 验证 hatch 元素步长等于当前大小、大于当前大小、过小、未对齐和乘法溢出；
- 验证 SDK 截短函数表时只返回 `YICAD_IMPORT_ERROR_UNSUPPORTED`，不越界读取；
- 验证异常不穿过 C ABI，会话失败后完整回滚。

ABI 兼容性不适合只依靠人工运行 demo。建议允许增加一个不依赖 GUI 的轻量 CTest 目标，
并相应删除原阶段 7 中“不新增测试工程或测试用例”的限制。如果暂时不增加测试目标，
上述场景必须形成可重复执行的检查程序，否则不得进入阶段 8。

构建验证：

```powershell
conan install . --output-folder=build/conan-release --profile=profiles/windows-msvc-release --lockfile=conan.lock --build=never
cmake --preset Release "-DCMAKE_PREFIX_PATH=$env:Qt5_DIR"
cmake --build --preset Release

conan install . --output-folder=build/conan-debug --profile=profiles/windows-msvc-debug --lockfile=conan.lock --build=never
cmake --preset Debug "-DCMAKE_PREFIX_PATH=$env:Qt5_DIR"
cmake --build --preset Debug
```

完成条件：

- Release、Debug、demo plugin 和 ABI 检查目标全部通过；
- v1/v2 的字段顺序、大小、调用约定和行为没有变化；
- demo 和常规 SDK 文档不要求插件作者填写 `structSize`；
- 不存在可扩展结构按值嵌套或无步长的可扩展结构数组；
- 完成上述条件前不得执行原计划阶段 8。

建议提交：`verify abi v3 layout compatibility`

## 6. AI 执行约束

后续由 AI 按本计划实施时，每次只执行一个步骤，并遵守：

- 开始前先读取本计划、`PLUGIN_ABI_V3_PLAN.md`、`YiCadPluginAbi.h` 和该步骤涉及的实现；
- 不提前提升公开 ABI 版本；
- 不顺手修改 v1/v2 已发布结构和函数签名；
- 修改 ABI 结构时，同一提交必须同步修改宿主消费者并保持构建通过；
- 不用批量文本替换推断嵌套语义，每种结构逐项审查；
- 不把 C++、STL、Qt 类型写入 C ABI 头；
- 每一步报告修改文件、ABI 决策、构建命令和未完成风险；
- 当前步骤验收失败时停止，不继续叠加后续步骤。

## 7. 总体验收标准

全部完成后应同时满足：

1. C ABI 保留明确且可验证的前向/后向扩展机制；
2. 固定布局值类型不携带无意义的 `structSize`；
3. 不存在会因嵌套结构增长而移动外层后续字段的布局；
4. 不存在依赖双方 `sizeof(Element)` 相同的可扩展结构数组；
5. 宿主按字段可访问范围读取，不按当前完整大小拒绝旧前缀；
6. 官方 C++ SDK 用户不需要直接填写 `structSize` 或公共属性默认值；
7. 裸 POD 入口仍然可用，便于其他语言绑定和高级插件接入；
8. v1/v2 二进制和源码兼容性保持不变；
9. 在所有条件满足并完成审查前，ABI v3 不得发布或冻结。
