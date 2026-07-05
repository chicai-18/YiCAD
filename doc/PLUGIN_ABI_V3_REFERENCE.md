# YiCAD 插件 ABI v3 参考

本文描述 YiCAD 0.20.0 正式发布的 ABI v3。类型定义、字段顺序、函数签名、调用约定和
编译期布局快照以安装 SDK 中的 `YiCadPluginAbi.h` 为最终真值。正式支持架构为 Win64。

## 版本与函数表

- `YICAD_PLUGIN_ABI_V3 == 3`，当前最低版本为 v1，最高版本为 v3。
- Win64 的 `YICAD_HOST_API_V3_SIZE` 为 160 字节，`YICAD_IMPORT_API_V3_SIZE` 为
  248 字节，两个函数表均按 8 字节对齐。
- `YiCadHostApi::importApi` 是 v2 前缀后的唯一 v3 字段。协商版本低于 v3 时宿主裁剪该
  字段；协商到 v3 但指针为空时，导入会话能力不可用。
- `YiCadImportApi` 的 `structSize` 和 `abiVersion` 是必需头。插件必须在读取每个函数指针
  前验证字段边界；字段缺失或函数指针为空只禁用相应能力。

`YiCadImportApi` 按冻结顺序包含以下函数：

| 函数 | 输入与结果 |
| --- | --- |
| `beginImport` | 为打开文档创建非嵌套会话，返回会话句柄。 |
| `commitImport` | 原子提交并消费会话；空会话不创建撤销项。 |
| `rollbackImport` | 回滚并消费会话；空句柄或过期句柄返回无效句柄。 |
| `getLastError` | 复制线程内最后一条 UTF-8 诊断，返回包含结尾 NUL 的所需字节数。 |
| `setDocumentSettings` | 设置插入单位、测量制式、全局线型比例和源代码页元数据。 |
| `createLineType` | 创建简单线型；复杂线型明确不支持。 |
| `createLayer` | 创建图层，可引用本会话线型。 |
| `createTextStyle` | 创建文字样式；缺失字体保留原始文件名并产生诊断。 |
| `createDimensionStyle` | 创建标注样式；仅显式允许时忽略不可表达字段并产生诊断。 |
| `getModelSpace` | 返回本会话模型空间容器。 |
| `createPoint`、`createLine`、`createRay`、`createXLine` | 创建基础点和线性实体。 |
| `createArc`、`createCircle`、`createEllipse` | 创建圆弧、圆、椭圆或椭圆弧。 |
| `createPolyline`、`createSpline` | 创建二维多段线和非有理、非周期样条。 |
| `createText`、`createMText` | 创建单行文字和多行文字。 |
| `beginBlock`、`endBlock` | 创建块定义容器并结束定义；未结束的块不能被引用。 |
| `createInsert` | 创建已完成块定义的引用并返回引用资源句柄。 |
| `createAttributeDefinition`、`createAttribute` | 创建块属性定义和块引用属性值。 |
| `createDimension`、`createLeader` | 创建语义标注和引线；坐标标注明确不支持。 |
| `createHatch` | 创建实体或图案填充，支持多个外环及孔环。 |
| `createImage` | 创建外部光栅图像引用；裁剪边界明确不支持。 |

所有函数只允许在创建宿主 API 的 UI 线程调用。除 `getLastError` 外，函数返回
`YiCadImportResult`；失败不得把 C++ 异常传播到插件，也不得留下当前调用的部分修改。

## 结果码

| 常量 | 值 | 含义 |
| --- | ---: | --- |
| `YICAD_IMPORT_SUCCESS` | 0 | 调用成功；若诊断非空，表示成功但发生显式允许的降级。 |
| `YICAD_IMPORT_ERROR_INVALID_ARGUMENT` | -1 | 指针、字符串、枚举、布尔、数组或结构前缀无效。 |
| `YICAD_IMPORT_ERROR_INVALID_HANDLE` | -2 | 句柄为空、过期、类型错误或不属于当前会话。 |
| `YICAD_IMPORT_ERROR_NAME_CONFLICT` | -3 | 资源名称冲突且策略不允许处理。 |
| `YICAD_IMPORT_ERROR_RESOURCE_NOT_FOUND` | -4 | 引用的资源或依赖不存在。 |
| `YICAD_IMPORT_ERROR_UNSUPPORTED` | -5 | 输入要求宿主不能保真表达的能力。 |
| `YICAD_IMPORT_ERROR_OUT_OF_RANGE` | -6 | 有限数值仍超出协议或宿主允许范围。 |
| `YICAD_IMPORT_ERROR_OUT_OF_MEMORY` | -7 | 分配失败。 |
| `YICAD_IMPORT_ERROR_TRANSACTION_FAILED` | -8 | 会话、事务、提交、回滚或模型修改失败。 |

## 结构与默认值

固定布局值类型包括 `YiCadStringView`、点、向量、颜色、基础数组视图和
`YiCadVertex2d`。它们不含 `structSize`，不得原地扩展。以下可扩展输入均以
`structSize` 开头，并由同名 `YICAD_*_V3_MIN_SIZE` 常量定义 v3 必需前缀：

| 类别 | 结构 | 主要语义 |
| --- | --- | --- |
| 文档与资源 | `YiCadDocumentSettings`、`YiCadLineTypeDataV3`、`YiCadLayerDataV3`、`YiCadTextStyleDataV3`、`YiCadDimensionStyleDataV3` | 文档元数据及四类会话资源。 |
| 公共属性 | `YiCadEntityAttributes` | 图层、线型、颜色、线宽、线型比例、可见性和法向量。 |
| 基础几何 | `YiCadPointDataV3`、`YiCadLineDataV3`、`YiCadRayDataV3`、`YiCadXLineDataV3`、`YiCadArcDataV3`、`YiCadCircleDataV3`、`YiCadEllipseDataV3`、`YiCadPolylineDataV3`、`YiCadSplineDataV3` | 二维 WCS 几何；角度使用弧度。 |
| 文字与块 | `YiCadTextDataV3`、`YiCadMTextBackgroundData`、`YiCadMTextDataV3`、`YiCadBlockDataV3`、`YiCadInsertDataV3`、`YiCadAttributeDefinitionDataV3`、`YiCadAttributeDataV3` | 文字、块定义、块引用和属性。 |
| 语义实体 | `YiCadDimensionDataV3`、`YiCadLeaderDataV3`、`YiCadHatchEdgeDataV3`、`YiCadHatchLoopDataV3`、`YiCadHatchDataV3`、`YiCadImageDataV3` | 标注、引线、填充和图像。 |

缺失可选尾字段使用公开默认值。官方 C++ SDK 的高层对象自动产生这些值：

- 公共属性：活动图层、ByLayer 线型与颜色、线宽 `-1`、线型比例 `1.0`、可见、法向量
  `(0, 0, 1)`；实体的 `attributes == nullptr` 具有相同语义。
- 文档设置：插入单位和测量制式为 `0`、全局线型比例 `1.0`、源代码页为空。
- 图层：未冻结、未锁定、可打印、ByLayer 颜色、Continuous 线型、线宽 `-1`。
- 文字样式：固定高度 `0`、宽度因子 `1.0`、倾斜角 `0`、生成标志 `0`。
- 实体布尔和角度默认 `0`；正比例、文字高度、半径及其他业务必需正值仍须调用方填写。
- 块引用缩放默认为 `(1, 1, 1)`，行列数默认为 `1`。
- 实体填充的图案比例默认为 `1.0`；图像亮度、对比度和淡化值默认均为 `0`。

自定义绑定传入较小合法前缀时，`structSize` 必须是实际可访问字节数；大于宿主已知大小
也合法，宿主忽略未知尾字段。固定数组使用 `data + count`。HATCH 边和环是可扩展结构
数组，必须额外提供真实 `byteStride`，并满足最小前缀、元素对齐和地址溢出检查。

## 所有权与生命周期

- 会话、容器和资源句柄均由宿主持有，插件不得释放、跨会话使用或跨回调长期缓存。
- `commitImport`、`rollbackImport`、文档关闭、插件卸载和宿主销毁都会使会话及全部子句柄
  立即失效。SDK `ImportSession` 析构时自动回滚仍活动的会话。
- 同一文档最多有一个活动导入会话，且不能与 v2 文档事务或宿主命令组嵌套。成功导入
  至多形成一个撤销项。
- 插件传入的字符串、数组、嵌套输入和 SDK 临时 POD 仅在函数调用期间借用；宿主在返回
  前复制需要保留的数据。
- `YiCadStringView` 是显式长度 UTF-8，拒绝嵌入 NUL。全部浮点数必须有限；枚举、标志、
  布尔、资源类型和句柄归属在修改模型前验证。

## 明确不支持或仅诊断保留

复杂线型、有理或周期样条、MTEXT 背景、外部块、不可表达的属性标志、坐标标注和图像
裁剪返回 `YICAD_IMPORT_ERROR_UNSUPPORTED`。三维多段线、网格、曲面和实体没有 v3 创建
入口。标注样式的未知字段只有在 `allowUnsupportedFields == 1` 时可忽略并留下诊断；缺失
字体或图像文件会保留原始引用并留下诊断。
