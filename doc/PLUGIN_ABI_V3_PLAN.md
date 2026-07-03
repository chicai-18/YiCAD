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
- `YiCadColorData`：ByLayer、ByBlock、ACI、RGB；
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

## 分阶段实施

### 阶段 0：冻结设计边界

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

- 图层：名称、开关、冻结、锁定、打印、颜色、线型和线宽；
- 线型：名称、说明、总长度和简单线段序列；
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
- 字体文件缺失时保留原始文件名并标记缺失，不得丢失 STYLE 记录。

完成条件：

- 中文资源名称和原始字体文件名可以无损传入；
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
- 缺失字体使用 YiCAD 替代字体显示，但资源仍保存原始字体名；
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
