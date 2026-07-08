# YiCAD 插件 ABI v3 参考

`YiCadPluginAbi.h` 是 ABI v3 的布局真值。当前最低、最高和当前版本均为
`YICAD_PLUGIN_ABI_V3`，宿主与插件不进行旧版本协商或能力降级。

`YiCadHostApi` 提供注册、基础文档操作以及两个 v3 子表：

- `YiCadImportApi`：原子导入会话、资源创建、块创建和全部受支持实体创建；
- `YiCadReadApi`：文档设置、资源、块及全部一等实体的同步只读枚举。

所有字符串使用 UTF-8。输入数组和字符串视图只在调用期间借用，宿主在返回前复制；
只读 API 返回的视图保持到同一子表的下一次调用，C++ SDK 会立即转换为拥有型值。
所有句柄均由宿主持有，插件不得释放、解引用或跨文件回调保存。

插件入口固定为 `yicad_plugin_get_abi_version`、`yicad_plugin_init` 和
`yicad_plugin_shutdown`，并使用 `YICAD_PLUGIN_CALL` 调用约定。异常不得穿过这些入口
或宿主回调边界。
