# YiCAD 插件 ABI v4 参考

`YiCadPluginAbi.h` 是 ABI v4 的布局真值。当前最低、最高和当前版本均为
`YICAD_PLUGIN_ABI_V4`，宿主与插件不进行旧版本协商或能力降级；v3 插件不再加载。

`YiCadHostApi` 提供注册、基础文档操作以及三个子表：

- `YiCadImportApi`：原子导入会话、资源创建、块创建和全部受支持实体创建；v4 在尾部追加
  `createCustomEntity`（自定义实体）与 `beginProxyGraphics`（收集代理图形的容器）；
- `YiCadReadApi`：文档设置、资源、块及全部一等实体的同步只读枚举；v4 起自定义实体以
  `YICAD_ENTITY_CUSTOM` 交出（数据为 `YiCadCustomEntityDataV4`，不再炸开），尾部追加
  `entityGraphics`（把当前自定义实体的图形做成基本实体逐个枚举，写别的格式的代理图形用）；
- `YiCadEntityApiV4`（v4 新增）：`registerEntityClass` 在 init 里登记实体类，
  `createCustomEntity`、`setCustomEntityData` 在文档事务里新建自定义实体、换它的数据（可撤销）。

v3 定义、v4 未改动的结构保留 `V3` 后缀（如 `YiCadLineDataV3`）；v4 新增的结构带 `V4` 后缀。
`YiCadDocumentSettings` 在尾部追加 `currentEntityLineTypeScale`（DXF 的 `$CELTSCALE`，缺省 1）。

所有字符串使用 UTF-8。输入数组和字符串视图只在调用期间借用，宿主在返回前复制；
只读 API 返回的视图保持到同一子表的下一次调用，C++ SDK 会立即转换为拥有型值。
所有句柄均由宿主持有，插件不得释放、解引用或跨文件回调保存。

插件入口固定为 `yicad_plugin_get_abi_version`、`yicad_plugin_init` 和
`yicad_plugin_shutdown`，并使用 `YICAD_PLUGIN_CALL` 调用约定。异常不得穿过这些入口
或宿主回调边界。

## 自定义实体（v4）

设计见 `RENDER_PLAN.md` 第 4.8.3 节：实体的数据是宿主保管的一段字节，编码由插件定义；
插件经 `YiCadEntityClassV4` 提供一组纯函数，输入都是这段字节与实例缓存。撤销（换字节）、
存盘与读盘（类名、版本、字节与代理图形）、插件缺失时的代理显示都由宿主处理，插件不参与。

- **类登记**：`className` 为 `pluginId.类名`，与插件的其他注册项一起原子提交，重名时整个插件
  加载失败。`classVersion` 是数据编码的版本：读回的数据版本低于它时调用 `upgrade`，没有
  `upgrade` 或数据比它新时实体读成代理。`proxyFlags` 只能组合 `YICAD_PROXY_*`（与 DXF CLASSES
  段组码 90、ODA 的 `OdDbProxyEntity::ProxyFlags` 相同），是插件不在时代理允许的操作。
- **必须提供**：`worldDraw`（经 `YiCadGiApiV4` 输出图元）、`getExtents`、`transform`（仿射变换，
  可能非等比，结果写进 `YiCadByteSink`）。**可为空**：`getGrips`/`moveGrips`（为空时只能整体
  移动）、`getSnapPoints`、`explode`（为空时宿主按 worldDraw 的图元推导）、`upgrade`；
  `createCache`/`destroyCache` 同时提供或同时为空。
- **线程**：除声明 `YICAD_ENTITY_CLASS_THREAD_SAFE_DRAW` 的类的 `worldDraw` 外，都只在 UI 线程
  调用。没有声明的类，宿主在实体更新时（UI 线程）调一次 `worldDraw` 记成 GI 流，图形系统的
  工作线程只回放记下的流。
- **GI 表**：`YiCadGiApiV4` 与宿主的 `IGiGeometry`、`IGiSubEntityTraits` 一一对应；属性设置
  对之后的图元生效，初值为实体自己的属性；资源（线型、图层、文字样式、块）经 `findResource`
  按名字在实体所属文档里找；参数无效的调用返回失败并被忽略；单次 `worldDraw` 输出超过
  100 万个点与图元时之后的被丢弃。
- **炸开**：`explode` 在宿主开的临时导入会话里用导入函数建基本实体；这个会话不能建资源与块，
  实体属性为空时取被炸开的实体的图层与画笔。
- **插件卸载**：只在程序退出时。宿主在插件 `shutdown` 之前注销它的实体类、收回全部实例缓存；
  之后文档里留下的实体只画记下的图形，存盘时仍写原来的类名、版本与字节。
- **导入**：`createCustomEntity` 在类登记了且读得了数据时建原实体，否则建代理实体（保留类名、
  版本与字节，按 `proxyFlags` 放行操作）；代理显示的图形在 `beginProxyGraphics` 返回的容器里
  建，容器里属性为空、图层为空、线型为空、颜色与线宽随块的实体沿用自定义实体自己的属性。
  `transform` 是代理累计的变换，原实体读回后按它改动。
- **事务**：`createCustomEntity` 在模型空间新建（数据版本必须等于类的当前版本，图层、线型句柄
  为空，取当前图层与随层）；`setCustomEntityData` 只接受登记了的类的原实体，实体句柄来自
  只读枚举（`YiCadCustomEntityDataV4::entity`）或 `createCustomEntity` 的输出。
