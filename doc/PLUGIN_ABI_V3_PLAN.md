# YiCAD 插件 ABI v3 实施计划

## 文档状态

本文是 ABI v3 的实现和发布计划，不是已经发布的 ABI 契约。本轮不新增 CTest、
单元测试目录、专用测试插件或测试数据；构建检查、代码审查、兼容性审计和人工验证
仍属于实施范围。在本文最后一个阶段完成以前：

- `YICAD_PLUGIN_ABI_MAX_VERSION` 必须保持为 `YICAD_PLUGIN_ABI_V2`；
- v3 草案只能在开发分支或编译开关下使用；
- 不得发布声称支持 ABI v3 的 SDK 或插件；
- 每个阶段必须保持现有 v1/v2 插件的源码和二进制兼容性。

正式发布后，v3 的字段顺序、调用约定、结构大小语义和所有权规则永久冻结，
只能通过结构尾部追加字段或发布更高 ABI 版本扩展。

## 目标

ABI v3 首先解决高保真文件导入。插件负责解析外部格式，宿主负责创建 YiCAD
资源、实体和块。以 DXF 为首个使用场景，但接口不得包含 `libdxfrw` 类型或
DXF 专用类名，以便后续 SVG、DGN 等导入插件复用。

必须覆盖以下信息：

- 文档单位和必要的绘图变量；
- 图层、线型、文字样式和标注样式；
- 模型空间、块定义和块引用；
- 基础曲线、多段线和样条曲线；
- 单行文字、多行文字、属性定义和属性值；
- 标注、引线和填充；
- 实体公共属性，包括颜色、图层、线型、线宽、可见性和法向量；
- 原子导入、单步撤销、失败回滚和可诊断的错误结果。

## 非目标

以下内容不纳入本轮 ABI v3 候选：

- 直接暴露 `Dm*`、Qt、STL 或 libdxfrw 对象；
- 完整的高保真导出和任意实体只读枚举；
- 动态块参数、约束系统、MLEADER、TABLE、MLINE；
- Civil 3D、Architecture 等垂直产品的代理对象；
- 插件自定义实体和跨 DLL 的对象继承；
- 后台线程并行修改文档；
- 图纸空间、布局、视口和页面设置；
- CTest、单元测试、模糊测试、压力测试、专用测试插件和测试数据。

YiCAD 当前文档模型没有图纸空间，因此 v3 不为图纸空间、布局或视口预留容器类型、
枚举值或函数入口。

完整导出应在 v3 稳定后单独设计，必要时发布 ABI v4，不应为了接口对称扩大
本次范围。

## 总体设计

### 子函数表

`YiCadHostApi` 的 v1/v2 前缀保持不变，在尾部只追加一个导入子函数表指针：

```c
typedef struct YiCadImportApi YiCadImportApi;

typedef struct YiCadHostApi
{
    /* 已发布的 v1/v2 字段保持原样。 */
    const YiCadImportApi* importApi;
} YiCadHostApi;
```

`YiCadImportApi` 自带 `structSize` 和 `abiVersion`。插件访问任何尾字段前同时检查
版本、结构大小和函数指针。宿主持有该函数表，其有效期与 `YiCadHostApi` 相同。

采用子函数表的理由：

- 主宿主表只增长一个指针，v3 的 Win32/Win64 布局容易冻结和审计；
- 导入接口可以按能力追加尾字段，不影响命令、Ribbon 和旧文档 API；
- SDK 可以把导入能力封装为独立的 `ImportSession`，降低误用风险；
- 后续导出接口可以使用独立子表，不与导入接口耦合。

### 句柄和所有权

计划新增以下不透明句柄：

```c
typedef void* YiCadImportSessionHandle;
typedef void* YiCadImportContainerHandle;
typedef void* YiCadImportResourceHandle;
```

- `YiCadImportSessionHandle` 由宿主创建，成功提交或回滚后失效；
- `YiCadImportContainerHandle` 表示模型空间或块定义，只在所属会话内有效；
- `YiCadImportResourceHandle` 表示图层、线型、文字样式等资源，只在所属会话内有效；
- 插件不得缓存句柄供下一次回调使用；
- 插件不得释放任何宿主句柄；
- 宿主不得保存插件传入的裸指针，必须在函数返回前复制字符串和数组。

### 导入事务

一个导入会话内部持有一个宿主事务：

```text
beginImport(document)
  -> 创建资源和实体
  -> commitImport(session)   成功，形成一个撤销项
  -> rollbackImport(session) 失败，不保留部分结果
```

规则如下：

- 同一文档一次只允许一个活动导入会话；
- 不允许与现有插件事务或宿主事务嵌套；
- `commitImport` 和 `rollbackImport` 都释放会话及其子句柄；
- SDK RAII 包装在析构时自动回滚未提交会话；
- 文件过滤器返回失败前必须确保会话已回滚；
- 提交失败时宿主必须自行回滚，不能把半成品留在文档中。

### POD 数据结构

所有可扩展输入结构的第一个字段都是 `uint32_t structSize`。结构只能追加尾字段。
建议先定义以下公共结构：

- `YiCadPoint2d`、`YiCadPoint3d`；
- `YiCadVector2d`、`YiCadVector3d`；
- `YiCadColorData`：ByLayer、ByBlock、ACI、RGB；ACI 输入转换为 RGB，不保留索引语义；
- `YiCadEntityAttributes`：图层、线型、颜色、线宽、线型比例、可见性、法向量；
- `YiCadVertex2d`：位置、起始宽度、终止宽度、凸度；
- `YiCadStringView`：UTF-8 指针和显式字节长度；
- `YiCadDoubleArrayView`、`YiCadPointArrayView` 等只读数组视图。

所有浮点输入必须拒绝 NaN 和无穷大。枚举值必须验证范围。字符串可以包含空格和
非 ASCII 字符，但不得包含嵌入式 NUL；需要原样保留的 MTEXT 内容使用显式长度。

### 结果和诊断

不应把所有失败压缩成无法定位的 `YICAD_FAILURE`。导入子接口使用固定宽度结果码，
至少区分：

- 无效参数；
- 无效或过期句柄；
- 名称冲突；
- 引用的资源不存在；
- 当前宿主不支持该能力；
- 数据超出 YiCAD 模型可表示范围；
- 内存不足；
- 宿主事务失败。

详细文本通过调用方提供的缓冲区读取，或通过现有消息接口输出；不得返回要求插件
释放的宿主字符串。

## 阶段 0 冻结结果

本节是阶段 0 的代码审计结论，审计基线为 `develop` 分支提交
`883634fdb250822923af6f725fda6da5eb380cec`。后续阶段只能在这里记录的 v1/v2
前缀之后追加能力，不能重新解释已发布字段。

### 已发布 ABI 前缀

`YiCadHostApi` 和 `YiCadPluginApi` 都是标准布局 C 结构。Windows 上所有函数指针、
插件导出入口和回调均使用 `YICAD_PLUGIN_CALL`，其值为 `__cdecl`；导出入口使用
`extern "C"`。非 Windows 编译器上的 `YICAD_PLUGIN_CALL` 为空。`structSize` 表示调用方
实际提供且可访问的字节数，读取字段前必须同时检查协商版本、字段末端偏移和函数指针。

v1 宿主前缀固定到 `registerExportFilter`，大小为 Win32 48 字节、Win64 88 字节：

| 顺序 | 字段 | 精确类型 | Win32/Win64 偏移 | 已发布语义 |
| ---: | --- | --- | --- | --- |
| 1 | `structSize` | `uint32_t` | 0 / 0 | 当前协商表的可访问字节数。 |
| 2 | `abiVersion` | `uint32_t` | 4 / 4 | 宿主与插件协商后的 ABI 版本。 |
| 3 | `message` | `YiCadMessageFn` | 8 / 8 | 显示 UTF-8 消息；宿主在返回前复制文本。 |
| 4 | `registerCommand` | `YiCadRegisterCommandFn` | 12 / 16 | 注册命令及插件回调。 |
| 5 | `registerRibbonButton` | `YiCadRegisterRibbonButtonFn` | 16 / 24 | 注册指向同插件命令的 Ribbon 按钮。 |
| 6 | `currentDocument` | `YiCadCurrentDocumentFn` | 20 / 32 | 返回当前打开文档的非拥有句柄，无当前文档时为空。 |
| 7 | `documentAddLine` | `YiCadDocumentAddLineFn` | 24 / 40 | 向文档添加二维直线。 |
| 8 | `documentAddCircle` | `YiCadDocumentAddCircleFn` | 28 / 48 | 向文档添加二维圆。 |
| 9 | `documentRegen` | `YiCadDocumentRegenFn` | 32 / 56 | 重生成文档。 |
| 10 | `documentZoomAuto` | `YiCadDocumentZoomAutoFn` | 36 / 64 | 对文档当前视图执行自动缩放。 |
| 11 | `registerImportFilter` | `YiCadRegisterImportFilterFn` | 40 / 72 | 注册扩展名、导入回调和插件上下文。 |
| 12 | `registerExportFilter` | `YiCadRegisterExportFilterFn` | 44 / 80 | 注册格式、导出回调和插件上下文。 |

v2 不修改上述字段，只在尾部追加以下字段，完整宿主前缀大小为 Win32 80 字节、
Win64 152 字节：

| 顺序 | 字段 | 精确类型 | Win32/Win64 偏移 | 已发布语义 |
| ---: | --- | --- | --- | --- |
| 13 | `documentBeginTransaction` | `YiCadDocumentBeginTransactionFn` | 48 / 88 | 为文档开始一个非嵌套事务。 |
| 14 | `documentCommitTransaction` | `YiCadDocumentCommitTransactionFn` | 52 / 96 | 提交并消费事务句柄。 |
| 15 | `documentRollbackTransaction` | `YiCadDocumentRollbackTransactionFn` | 56 / 104 | 回滚并消费事务句柄。 |
| 16 | `documentCreateEntityIterator` | `YiCadDocumentCreateEntityIteratorFn` | 60 / 112 | 创建宿主持有的只读实体快照迭代器。 |
| 17 | `entityIteratorNext` | `YiCadEntityIteratorNextFn` | 64 / 120 | 前进到下一个快照，并复制实体类型。 |
| 18 | `entityIteratorGetLine` | `YiCadEntityIteratorGetLineFn` | 68 / 128 | 把当前直线复制到插件提供的 POD。 |
| 19 | `entityIteratorGetCircle` | `YiCadEntityIteratorGetCircleFn` | 72 / 136 | 把当前圆复制到插件提供的 POD。 |
| 20 | `entityIteratorDestroy` | `YiCadEntityIteratorDestroyFn` | 76 / 144 | 释放宿主持有的迭代器。 |

`YiCadPluginApi` 在 v2 没有扩展，v1/v2 均使用同一前缀：

| 顺序 | 字段 | 精确类型 | Win32/Win64 偏移 | 所有权 |
| ---: | --- | --- | --- | --- |
| 1 | `structSize` | `uint32_t` | 0 / 0 | 宿主设置容量，插件确认实际使用的大小。 |
| 2 | `abiVersion` | `uint32_t` | 4 / 4 | 宿主设置协商版本，插件必须原值确认。 |
| 3 | `pluginId` | `const char*` | 8 / 8 | 插件持有的 UTF-8 NUL 结尾字符串。 |
| 4 | `pluginName` | `const char*` | 12 / 16 | 插件持有的 UTF-8 NUL 结尾字符串。 |
| 5 | `pluginVersion` | `const char*` | 16 / 24 | 插件持有的 UTF-8 NUL 结尾字符串。 |

该前缀大小为 Win32 20 字节、Win64 32 字节。`YiCadResult` 和 `YiCadEntityType` 均为
`int32_t`；结果值只冻结 `YICAD_FAILURE == 0` 和 `YICAD_SUCCESS == 1`。三个不透明句柄
均为 `void*`。`YiCadLineData` 是依次排列的四个 `double`，大小 32 字节；
`YiCadCircleData` 是依次排列的三个 `double`，大小 24 字节。插件入口顺序和签名固定为
`YiCadPluginGetAbiVersionFn`、`YiCadPluginInitFn` 和 `YiCadPluginShutdownFn` 所声明的形式。

### v1/v2 调用和所有权规则

- 宿主选择 `min(插件最高版本, 宿主最高版本)`，并按 v1 或 v2 裁剪
  `YiCadHostApi::structSize`。插件必须在 `YiCadPluginApi::abiVersion` 中确认协商值。
- `YiCadHostApi` 及其函数指针由宿主持有，从 `yicad_plugin_init` 开始到
  `yicad_plugin_shutdown` 返回前有效；插件不得修改或释放函数表。
- `YiCadPluginApi` 的存储由宿主提供。插件持有三个元数据字符串并保证它们至少到
  `shutdown` 返回前有效；宿主复制字符串，不跨 DLL 释放插件内存。
- 所有跨 ABI 字符串都是 UTF-8 NUL 结尾字符串。宿主函数输入字符串只借用到该次调用
  返回，并在需要保留时于返回前复制。导入/导出回调收到的 `filePath` 只在该次回调内有效。
- 命令、导入和导出回调以及相应 `userData` 由插件持有，注册后保持有效到
  `shutdown`；宿主只保存地址，不释放 `userData`。
- `YiCadDocumentHandle` 非拥有，只在对应文档保持打开期间有效，插件不得释放，也不得
  假定关闭文档后地址仍可使用。
- `YiCadTransactionHandle` 由宿主持有。成功开始后插件必须且只能调用一次 commit 或
  rollback；成功结束后句柄立即失效。同一文档只允许一个插件事务，且不能与宿主事务嵌套。
- `YiCadEntityIteratorHandle` 由宿主持有，插件必须调用 `entityIteratorDestroy`。迭代器
  保存创建时的 POD 快照；getter 写入插件提供的结构，返回后结构内容归插件所有。
- 注册、宿主函数和插件回调只允许在 YiCAD UI 主线程调用。异常不得穿过 C ABI；任何一侧
  都不得要求另一侧释放自己分配的内存，也不得跨边界传递 Qt、STL 或 YiCAD C++ 对象。

### v3 能力策略

矩阵中的策略含义如下：

- **支持**：v3 发布前必须能保留该项的语义和本文列出的字段；缺字段属于发布阻断问题。
- **降级**：只允许调用方显式启用，必须产生可查询诊断；未启用时按“拒绝”处理，禁止静默降级。
- **拒绝**：返回确定的“不支持”结果，且本次调用不得修改文档。

资源、文档设置和公共属性矩阵：

| 能力 | 策略 | v3 边界 |
| --- | --- | --- |
| 插入单位、测量制式、全局线型比例、源代码页 | 支持 | 代码页只作为源文件元数据保留；跨 ABI 文本始终为 UTF-8。 |
| 图层名称、可见性、锁定、打印、颜色、线型、线宽 | 支持 | YiCAD 使用统一可见性，不区分外部格式的关闭与冻结语义。 |
| 简单线型（线段/间隔序列） | 支持 | 保留名称、说明和序列；周期总长度需要时由序列计算。 |
| 含文字或形文件的复杂线型 | 拒绝 | v3 不把它转换为连续线，留待后续 ABI。 |
| 文字样式 | 支持 | 保留可解析字体或缺失字体名、固定高度、宽度、倾斜和生成标志。 |
| 缺失字体的显示 | 降级 | 使用现有 `invalid*` 字段保留无法解析的字体名，以替代字体显示并报告诊断。 |
| YiCAD 可表达的标注样式字段 | 支持 | 逐字段映射，不能只保存样式名。 |
| YiCAD 暂时不可表达的标注样式字段 | 降级 | 显式允许时忽略该字段并逐项诊断；默认拒绝对应资源。 |
| 实体图层、线型、颜色、线宽和可见性 | 支持 | 保留 ByLayer、ByBlock 和 RGB；ACI 输入按固定映射转换为 RGB。 |
| 实体线型比例和法向量/OCS | 支持 | 阶段 2/3 必须补齐公共存储和统一转换；不能表示的非平面数据拒绝。 |
| 透明度、材质、打印样式和其他未列公共属性 | 拒绝 | 不在 v3 候选范围内。 |

实体矩阵：

| 实体或能力 | 策略 | v3 边界 |
| --- | --- | --- |
| POINT、LINE、RAY、XLINE | 支持 | 使用明确的 WCS/OCS 或块局部坐标。 |
| ARC、CIRCLE、ELLIPSE | 支持 | 保留中心、半径/轴、参数范围和法向量。 |
| LWPOLYLINE、二维 POLYLINE | 支持 | 保留闭合、顶点、凸度和每段起止宽度，并校验数组数量一致。 |
| 三维 POLYLINE、POLYFACE、网格 | 拒绝 | v3 只承诺二维绘图实体。 |
| 非有理、非周期 SPLINE | 支持 | 保留次数、节点、控制点/拟合点和闭合语义。 |
| 当前模型不能精确表示的有理或周期 SPLINE | 降级 | 仅可显式选择按公差近似，并报告误差；默认拒绝。 |
| TEXT | 支持 | 保留内容、位置、对齐、样式和文字变换。 |
| MTEXT | 支持 | 保留原始 UTF-8 格式串和可表达的排版语义；背景填充等现有模型缺口必须在阶段 4 补齐。 |
| 不可表达的 MTEXT 控制码 | 降级 | 保留原始内容并逐项诊断，禁止炸成线段。 |
| BLOCK、INSERT | 支持 | 块定义保持共享，INSERT 不自动炸开；阶段 4 补齐块说明/标志，并在提交前检测递归和悬空引用。 |
| ATTDEF、ATTRIB | 支持 | 按 tag 关联并保留文字与标志语义。 |
| 线性、对齐、角度、半径、直径标注 | 支持 | 保留语义实体、定义点、文字覆盖和样式引用。 |
| 坐标标注 | 降级 | 当前没有对应 `Dm*` 语义实体；仅可显式转为可编辑图形和文字并诊断，默认拒绝。 |
| LEADER | 支持 | 保留顶点、箭头、文字和标注样式引用。 |
| HATCH | 支持 | 支持实体填充、图案及由折线/线/圆弧/椭圆弧/样条组成的多环边界。 |
| IMAGE 基本引用 | 支持 | 保留路径、插入点、U/V 方向和尺寸；文件缺失保留引用并诊断。 |
| IMAGE 裁剪边界 | 拒绝 | 当前模型只有图像包围矩形，不能保真表达裁剪语义，延期到模型具备对应能力后。 |

“支持”项只描述 v3 的发布目标，不表示当前公开 ABI 已经提供该能力。上表指出的模型缺口
必须在对应后续阶段关闭；不能关闭时，必须在发布前把该项明确改为“降级”或“拒绝”并重新评审。

### 容器和延期边界

- 每个文档只有一个模型空间根容器，由 `beginImport` 返回或通过会话取得；插件不能创建、
  删除或缓存第二个模型空间根。
- 块定义是 v3 唯一允许新增的容器类型。块容器属于创建它的导入会话，结束块定义、提交、
  回滚或关闭文档后失效。实体添加函数只接受模型空间或活动块定义容器。
- `DmEntityContainer`、块引用展开结果、标注内部图形、填充边界临时对象和选择集均不作为
  ABI 容器暴露。
- 图纸空间、布局、视口和页面设置明确延期；v3 不预留枚举值、空句柄或占位函数。
- 完整导出、任意实体只读枚举、动态块、约束、MLEADER、TABLE、MLINE、代理对象、
  自定义实体、3D 面/实体/曲面、后台线程修改和跨 DLL 对象继承均延期到后续独立设计。
- CTest、单元测试、模糊/压力测试、专用测试插件和测试数据不属于本轮；它们的延期不降低
  阶段 7 的构建、静态布局审计、兼容性审查和人工验证要求。

## 分阶段实施

### 阶段 0：冻结设计边界（已完成）

状态：已按上述审计基线完成文档冻结；未修改公开头文件、代码或测试结构。

目标：在修改 ABI 前明确现有 v1/v2 前缀、v3 能力范围和延期项。

代码范围：

- 记录 v1/v2 已发布字段的顺序、类型、调用约定和所有权语义；
- 建立资源和实体能力矩阵，为每项指定“支持、降级或拒绝”策略；
- 明确模型空间是文档级唯一根容器，块定义是唯一可新增的容器类型；
- 明确图纸空间、布局、视口和页面设置不进入 ABI v3；
- 本阶段不修改公开头文件，不新增测试目录或测试数据。

完成条件：设计边界和延期项经过代码评审并写入本文档。

回滚边界：只包含文档，可以独立撤销。

### 阶段 1：导入子表和会话生命周期

目标：建立 v3 基础设施，但暂不提供资源和实体创建能力。

代码范围：

- 在 `YiCadPluginAbi.h` 定义 v3 草案常量、子函数表和句柄；
- 在 `HostApi` 增加受编译开关保护的 v3 函数表装配；
- 新增 `ImportSession` 宿主实现，内部管理事务；
- 在 `YiCadPluginSdk.h` 增加不可复制、可移动的 RAII `ImportSession`；
- 增加 begin、commit、rollback 和错误读取接口；
- 暂不提升公开的 `YICAD_PLUGIN_ABI_MAX_VERSION`。

完成条件：

- 空会话提交不创建无意义撤销项；
- 显式回滚和 RAII 析构都能释放会话；
- 同一文档最多存在一个活动会话；
- 文档关闭会使关联句柄失效；
- 主宿主表的 v3 增量只包含一个对齐后的指针；
- v1/v2 前缀不发生源码层面的改动。

回滚边界：可整体移除 v3 草案，不影响 v1/v2。

### 阶段 2：公共属性和资源表

目标：先创建被实体引用的资源，确定名称冲突和缺失资源策略。

代码范围：

- 图层：名称、统一可见性、锁定、打印、颜色、线型和线宽；
- 线型：名称、说明和简单线段序列；
- 文字样式：名称、字体文件、大字体、固定高度、宽度因子、倾斜角和生成标志；
- 标注样式：映射 YiCAD 当前模型能够表达的 DIMSTYLE 字段；
- 文档设置：插入单位、测量制式、全局线型比例和必要代码页信息；
- 公共实体属性和资源句柄解析；
- 复杂线型先明确返回“不支持”，不得静默转换成连续线。

名称策略：

- 资源名称按 YiCAD 当前规则比较，不在 ABI 层自行改变大小写；
- 同名且内容相同返回已有资源句柄；
- 同名但内容不同根据调用方策略选择覆盖、重命名或失败；
- `0` 图层和 `Standard` 样式等内建资源不得被非法删除；
- 字体文件缺失时使用现有 `invalid*` 字段保留文件名并标记缺失，不得丢失 STYLE 记录。

完成条件：

- 中文资源名称和字体文件名可以传入；
- 重复资源和名称冲突遵循统一策略；
- 未支持字段产生可查询诊断，不能静默丢弃；
- 资源创建纳入导入事务，不存在实体时也可以独立提交资源表。

### 阶段 3：基础几何实体

目标：覆盖常见二维曲线，同时统一坐标、公共属性和容器处理。

按以下顺序实现：

1. POINT、LINE；
2. RAY、XLINE；
3. ARC、CIRCLE、ELLIPSE；
4. LWPOLYLINE、POLYLINE；
5. SPLINE。

每种实体使用独立 POD 描述结构，不能使用通用属性字典代替必需字段。实体添加接口
接收 `YiCadImportContainerHandle`，因此同一实现可以写入模型空间和块定义。

坐标规则：

- ABI 明确每个字段是 WCS、OCS 还是局部块坐标；
- 优先由插件提供原始法向量和 OCS 数据，由宿主统一转换；
- 如果决定由插件转换为 WCS，所有实体必须采用同一策略并在公开文档冻结；
- 角度统一使用弧度；
- 椭圆轴比、多段线凸度、样条次数和节点数量必须做边界校验。

完成条件：

- 每类实体都有独立 POD 描述结构和明确的坐标语义；
- 非法半径、空数组、NaN、无穷大和数组长度不匹配返回确定错误；
- 所有实体创建都纳入同一导入事务；
- 实体添加路径避免按文档总实体数重复全量扫描。

### 阶段 4：文字、块和属性

目标：满足实际工程图最重要的语义结构，禁止默认炸开文字和块。

代码范围：

- TEXT：内容、位置、对齐点、高度、旋转、宽度因子、倾斜角、水平/垂直对齐和样式；
- MTEXT：原始格式串、插入点、方向、矩形宽度、行距、附着点、样式和背景填充；
- BLOCK：名称、基点、标志、说明和可选外部参照路径；
- INSERT：块句柄、插入点、三轴比例、旋转、阵列行列和间距；
- ATTDEF/ATTRIB：标记、提示、默认值/实际值、位置、样式和标志；
- 块可以引用已经定义的块，前向引用策略必须明确。

文字规则：

- 跨 ABI 文本统一为 UTF-8；
- MTEXT 格式串在宿主支持范围内解析，无法表达的控制码保留诊断；
- 缺失字体使用 YiCAD 替代字体显示，并通过现有 `invalid*` 字段保存无法解析的字体名；
- 不得为了显示文字把 TEXT/MTEXT 转换成线段轮廓；
- 不得为了简化导入自动炸开 INSERT。

块规则：

- 定义块时返回容器句柄，块内实体通过阶段 3/4 的同一接口添加；
- 块结束后容器句柄失效；
- 块递归引用必须在提交前检测；
- 未解析块引用根据策略失败或记录为不支持，不能创建指向空对象的引用；
- 块属性必须与属性定义按 tag 关联，不依赖数组顺序。

完成条件：

- TEXT、MTEXT、BLOCK、INSERT、ATTDEF 和 ATTRIB 均保留语义对象；
- 同一块的多个 INSERT 共享块定义；
- 循环块、未知块、重复 tag 和无效缩放返回确定错误；
- 块表、文字样式和相关实体统一纳入导入事务。

### 阶段 5：标注、引线、填充和图像

目标：完成 v3 候选接口所需的标准工程图信息。

按以下子阶段实现，每个子阶段单独提交：

1. DIMENSION：线性、对齐、角度、半径、直径、坐标标注；
2. LEADER：顶点、箭头、关联标注样式和文字；
3. HATCH：实体填充、图案名、比例、角度和多环边界；
4. IMAGE：路径、插入点、U/V 方向、尺寸和裁剪边界；如果当前模型不足则明确延期。

标注必须保留语义实体和标注样式，不能只导入 AutoCAD 生成的匿名块图形。只有
YiCAD 当前无法表达某类标注时，才允许显式配置为降级导入，并产生诊断。

填充边界至少支持折线、线、圆弧、椭圆弧和样条边。所有环先验证闭合性和引用范围，
再修改文档，防止部分边界进入模型。

完成条件：

- 标注保留定义点、文字位置、测量值覆盖和样式引用；
- 填充支持带孔、多外环以及规定的边界类型；
- 外部图片缺失时保留原始引用并产生诊断；
- 所有高级实体都参与导入会话回滚和单步撤销。

### 阶段 6：SDK 封装和候选接口收口

目标：让插件开发者不直接操作裸函数表，并形成可供后续验证的完整候选接口。

代码范围：

- `YiCadPluginSdk.h` 增加 `ImportSession`、`ImportContainer` 和资源句柄包装；
- 所有包装在访问字段前检查版本、结构大小和函数指针；
- SDK 捕获插件侧异常，不允许异常穿过 C ABI；
- 更新 demo plugin，演示不依赖具体文件解析库的最小导入流程；
- 文档说明文件解析库由插件自行链接，PluginSDK 不传递 `libdxfrw` 依赖；
- 补齐所有公开结构和函数的中文 Doxygen 注释，说明线程、所有权、字符串有效期、
  默认值、参数范围和失败语义；
- 固定候选字段顺序，但保持 `YICAD_PLUGIN_ABI_MAX_VERSION` 为 v2。

完成条件：

- SDK 包装不暴露 YiCAD 私有类型；
- 截短的 `YiCadImportApi` 只禁用对应尾部能力；
- 未实现能力返回明确的“不支持”结果；
- ABI v3 仍由编译开关保护，不进入安装包和正式 SDK。

### 阶段 7：兼容性、健壮性和安全审查

目标：在提升公开版本号前完成 ABI 边界审查，不新增测试工程或测试用例。

必须完成：

- 为 Win32/Win64 的字段偏移、结构大小、对齐和调用约定增加编译期静态断言；
- 对照已发布头文件审查 v1/v2 前缀，确认字段顺序、类型和语义未改变；
- 审查 v1/v2/v3 版本协商以及 v3 插件面对 v2 宿主时的降级或拒绝路径；
- 审查每个公开函数对空指针、截短结构、无效枚举、超大数组和过期句柄的处理；
- 审查会话、容器和资源句柄在文档关闭、插件卸载和宿主退出时的清理路径；
- 确认插件与宿主之间没有跨模块分配、释放或异常传播；
- 在 Release 和 Debug 配置下完成宿主、PluginSDK 和 demo plugin 构建；
- 所有新增公开注释使用中文 Doxygen 格式并说明线程、所有权和失败语义。

完成条件：

- v1/v2 公开前缀保持不变；
- 所有公开结构都有 `structSize`、默认值和范围说明；
- 所有公开函数都有字符串有效期、句柄有效期和错误返回说明；
- 未实现能力返回明确的“不支持”结果，不依赖静默降级；
- 审查发现的问题全部关闭或明确阻止发布。

### 阶段 8：发布 ABI v3

只有前述阶段全部完成后才能执行：

1. 设置 `YICAD_PLUGIN_ABI_V3 = 3`；
2. 把 `YICAD_PLUGIN_ABI_MAX_VERSION` 和 `YICAD_PLUGIN_ABI_VERSION` 提升到 3；
3. 定义并冻结 `YICAD_HOST_API_V3_SIZE`；
4. 固化 Win32/Win64 的 v3 字段偏移和结构大小静态断言；
5. 更新版本协商、兼容矩阵、PluginSDK 指南和 demo plugin；
6. 安装并打包 ABI 文档、公开头文件和 PluginSDK；
7. 在 Release 和 Debug 下构建并安装发布候选；
8. 记录 ABI v3 发布基线，后续禁止修改已有字段。

构建验证：

```powershell
conan install . --output-folder=build/conan-release --profile=profiles/windows-msvc-release --lockfile=conan.lock --build="missing:libdxfrw/*"
cmake --preset Release
cmake --build --preset Release
cmake --install build/Release --config Release

conan install . --output-folder=build/conan-debug --profile=profiles/windows-msvc-debug --lockfile=conan.lock --build="missing:libdxfrw/*"
cmake --preset Debug
cmake --build --preset Debug
cmake --install build/Debug --config Debug
```

最终人工验证：

- 启动 Release 和 Debug 安装版本；
- 加载现有 v1/v2 插件以及 v3 demo plugin；
- 执行成功导入、失败回滚、撤销、关闭文档和卸载插件流程；
- 检查图层、样式、文字、块、标注、填充和图像；
- 确认一次导入只产生一个撤销项；
- 确认失败导入不会改变当前文档。

## 后续测试工作

本轮不创建 CTest 或其他测试用例。后续如需建立自动化回归，可单独规划：

- v1/v2/v3 ABI 布局和版本协商测试；
- 导入事务、过期句柄、异常路径和资源清理测试；
- DXF 样本、libdxfrw 测试适配器和模型快照测试；
- 模糊测试、压力测试、内存泄漏和 use-after-free 检查。

## 建议的提交边界

为便于审查和回滚，建议至少拆成以下提交或拉取请求：

1. `define plugin abi v3 scope`：阶段 0；
2. `add abi v3 import session draft`：阶段 1；
3. `add abi v3 resource import`：阶段 2；
4. `add abi v3 geometry import`：阶段 3；
5. `add abi v3 text and block import`：阶段 4；
6. `add abi v3 annotation import`：阶段 5；
7. `add abi v3 sdk wrappers`：阶段 6；
8. `harden plugin abi v3`：阶段 7；
9. `release plugin abi v3`：阶段 8。

除最后一个提交外，任何中间提交都不得提升公开 ABI 版本。
