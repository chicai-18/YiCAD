# YiCAD 插件 C ABI 演进规则

ABI v3 已于 YiCAD 0.20.0 正式冻结，支持范围为 Win64。公开真值由
`YiCadPluginAbi.h`、本文和 [PLUGIN_ABI_V3_REFERENCE.md](PLUGIN_ABI_V3_REFERENCE.md)
共同定义。

## 版本协商

- `YICAD_PLUGIN_ABI_V1`、`YICAD_PLUGIN_ABI_V2`、`YICAD_PLUGIN_ABI_V3` 分别固定为 `1`、`2`、`3`。`YICAD_PLUGIN_ABI_MIN_VERSION` 和 `YICAD_PLUGIN_ABI_MAX_VERSION` 定义当前 SDK/宿主支持范围；`YICAD_PLUGIN_ABI_VERSION` 保持为最高支持版本。
- `yicad_plugin_get_abi_version()` 返回插件实现的最高 ABI 版本。宿主选择 `min(插件最高版本, 宿主最高版本)`；低于宿主最低版本时直接拒绝。
- 宿主在调用 `yicad_plugin_init()` 前，把选择结果同时写入 `YiCadHostApi::abiVersion` 和 `YiCadPluginApi::abiVersion`，并按协商版本裁剪宿主函数表的 `structSize`。插件必须检查宿主函数表的版本和大小；若不能按该版本运行，应返回失败。
- init 成功后，插件必须在 `YiCadPluginApi::abiVersion` 中保留协商版本。未来插件只有明确确认当前宿主给出的旧版本，才视为安全降级；否则宿主回滚注册并拒绝加载。

## 可扩展结构的 `structSize` 规则

- `structSize` 只属于明确声明为可扩展的结构；并非所有公开结构都需要该字段。
- 可扩展结构必须独立传入函数或通过指针引用，不得按值嵌入另一个结构；需要组合时
  使用调用期间借用的 `const T*`，并明确空指针语义。
- `structSize` 必须是可扩展结构的首字段，表示调用方实际提供且可访问的字节数，
  不是期望版本的固定标签，也不要求等于接收方的 `sizeof(T)`。
- 读取任意字段前，必须先确认 `structSize >= offsetof(结构, 字段) + sizeof(字段)`；不得仅凭 `abiVersion` 访问字段。
- 新版本只能在可扩展结构尾部追加字段。旧调用方传入较小合法前缀时，新实现不得读写
  其边界以外的内容，并对缺失尾字段使用文档规定的默认值。
- 调用方传入大于接收方已知大小的结构合法；接收方必须忽略未知尾字段。
- `YICAD_HOST_API_V1_SIZE` 与 `YICAD_PLUGIN_API_V1_SIZE` 固定 v1 前缀大小，`YICAD_HOST_API_V2_SIZE` 固定 v2 宿主前缀，`YICAD_HOST_API_V3_SIZE` 和 `YICAD_IMPORT_API_V3_SIZE` 固定 v3 函数表；未来整个结构可以增长，但已有前缀大小不得改变。
- 函数表尾字段按能力处理：字段缺失或函数指针为空时，该能力不可用，SDK 返回失败或空对象，不影响更早且可用的能力。
- `YiCadPluginApi` 的 v1 三项元数据均为必填字段。v2 未扩展插件输出表，插件不得报告超过宿主提供容量的输出结构。

## 固定布局值类型、嵌套指针和数组

- 点、向量、字符串视图、基础数组视图、颜色及其他按值嵌入的 POD 是固定布局值类型，
  不携带 `structSize`。每个此类公开类型必须冻结 Win32/Win64 的大小、对齐和关键偏移。
- 固定布局值类型不能通过追加字段原地扩展；需要更多信息时定义新类型，或在拥有它的
  可扩展结构尾部追加字段。
- 可扩展嵌套对象只能通过调用期间借用的指针组合。空指针必须有唯一、公开的含义；
  宿主不得保存该指针，并须在 API 返回前复制需要保留的数据。
- 固定布局数组元素可以使用 `data + count`。可扩展数组元素必须额外传递显式字节步长，
  宿主以经过空指针、最小前缀、对齐和乘法溢出检查的字节运算访问元素。
- 数组元素自身的 `structSize` 只说明单个元素的可访问前缀，不能替代数组字节步长。

## ABI v2 文档对象所有权

- v2 只在 `YiCadHostApi` 的 v1 尾部追加事务和只读实体枚举函数；v1 字段、函数签名和前缀大小不变。
- `YiCadDocumentHandle` 非拥有，仅在文档保持打开时有效。`YiCadTransactionHandle` 和 `YiCadEntityIteratorHandle` 都是不透明宿主对象，不暴露 Qt、STL 或 YiCAD C++ 类型。
- 成功开始的事务必须由插件调用一次提交或回滚。提交和回滚都会释放事务句柄；SDK `DocumentTransaction` 对未提交事务执行析构回滚，因此导入中途返回失败或抛出异常不会留下已添加的部分实体。
- 同一文档一次只允许一个插件事务，且不允许与宿主已有事务或事务组嵌套。事务内所有添加操作合并为一个撤销项。
- 实体迭代器在创建时复制只读 POD 快照。文档之后修改或关闭不会使快照数据悬空；插件必须调用 `entityIteratorDestroy`，SDK `EntityIterator` 在析构时自动调用。
- `entityIteratorGetLine` 和 `entityIteratorGetCircle` 把数据复制到插件提供的结构，函数返回后数据由插件持有，不存在需要跨模块释放的临时内存。

## ABI v3 导入会话与所有权

- v3 只在 `YiCadHostApi` 的 v2 尾部追加 `importApi`。导入子表独立裁剪，缺失尾字段或空函数指针只表示对应能力不可用。
- 同一文档最多有一个活动导入会话，且不得与普通事务或命令组嵌套。`commitImport` 和 `rollbackImport` 都消费会话；空会话提交不产生撤销项，非空成功提交至多形成一个撤销项。
- `YiCadImportSessionHandle`、`YiCadImportContainerHandle` 和 `YiCadImportResourceHandle` 均由宿主持有，只在所属活动会话内有效。提交、回滚、文档关闭、插件卸载和宿主销毁都会使其失效。
- 插件传入的 UTF-8 字符串、数组和嵌套结构指针仅在当前调用期间借用。宿主在返回前复制需要保留的数据；任一侧都不得释放另一侧分配的内存。
- v3 写入只允许在创建宿主 API 的 UI 线程调用。所有 C ABI 最外层必须捕获异常并转换为稳定结果码。

## 兼容处理矩阵

| 场景 | 处理 |
| --- | --- |
| v1 插件 / v3 宿主 | 协商为 v1，宿主只传入稳定的 v1 尺寸函数表。 |
| v2 插件 / v3 宿主 | 协商为 v2，只公开 v2 前缀，可使用事务和只读实体快照。 |
| v3 插件 / v3 宿主 | 协商为 v3，公开正式 `importApi` 导入子表。 |
| v3 插件 / v2 宿主 | 协商为 v2；插件原值确认则按 v2 加载，从 init 拒绝则回滚注册。 |
| 截短的 v1 宿主函数表 | SDK 只访问表内字段；缺失能力返回失败。插件可继续使用仍存在的能力，也可从 init 返回失败。 |
| 版本为 `0` 或低于宿主最低版本 | init 前拒绝。 |
| 高于 v3 的插件 / v3 宿主 | 协商为 v3；插件必须原值确认，否则回滚并拒绝。 |
| 缺少可选尾字段或尾字段为空 | 对应能力不可用；不越界读取，不影响前缀能力。 |
| 插件输出元数据结构过短或声称超过宿主容量 | 回滚注册、调用 shutdown 并拒绝。 |

## 布局演进约束

- ABI v1、v2、v3 的既有字段顺序、字段类型、调用约定、函数签名、枚举值和语义永久冻结。
- 兼容扩展只能追加尾字段并提升最高 ABI 版本；禁止插入、删除、重排或复用旧字段。
- 破坏性变化必须定义新的 ABI，不得通过修改 `YICAD_PLUGIN_ABI_VERSION` 掩盖 v1 布局变化。
- `YiCadPluginAbi.h` 保存 Win32/Win64 的 v1/v2 前缀、v3 函数表、固定值类型、最小可扩展前缀和入口签名断言，布局漂移应在编译期失败。
- v3 只能扩展明确标记为可扩展的结构或函数表尾部，并提升后续 ABI 版本；不能原位修改 v3 契约。
- 本规则只适用于新 C ABI；旧 Qt/C++ 插件 ABI 不在兼容范围内。
