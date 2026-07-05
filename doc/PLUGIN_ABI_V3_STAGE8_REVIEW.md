# ABI v3 阶段 8 发布候选审查记录

## 1. 结论与范围

阶段 8 于 2026-07-05 完成。ABI v3 仍为草案，
`YICAD_PLUGIN_ABI_MAX_VERSION == YICAD_PLUGIN_ABI_V2`，安装后的
`YiCAD::PluginSdk` 不传播 `YICAD_ENABLE_PLUGIN_ABI_V3_DRAFT`。

本候选只声明支持 Win64。验证环境没有 Win32 Qt、SARibbonBar 和 CDT 依赖，未执行 Win32
完整构建；公开头保留 Win32 候选布局断言，但这些断言不构成支持声明。

环境为 Windows 11、Visual Studio 2022 17.10、MSVC 19.38、CMake 3.26.3、Conan
2.29.1 和 Qt 5.15.2。构建使用仓库锁定的 Debug/Release Conan 配置。

## 2. 构建、安装和语言边界

| 配置 | 草案 | 宿主 | demo | 安装 | 结论 |
| --- | --- | --- | --- | --- | --- |
| Win64 Debug | OFF | 通过 | 不参与构建 | 通过 | 正式 SDK 仅公开 v1/v2。 |
| Win64 Release | OFF | 通过 | 通过安装 SDK 在独立目录构建 v2 示例 | 通过 | 安装 target 不传播草案宏。 |
| Win64 Debug | ON | 通过 | 通过 | 通过 | v3 子表和 Debug demo 可用。 |
| Win64 Release | ON | 通过 | 通过 | 通过 | v3 宿主、demo 和安装树生成成功。 |

直接把 `YiCadPluginAbi.h` 交给 MSVC 执行了四次 `/Zs` 语法检查：C11/C++23 分别在草案
OFF 和 ON 下均通过。OFF 时 v3 声明受条件编译隔离；C11 路径不依赖 C++、Qt 或 STL。

安装后的 Release SDK 在仓库构建树之外通过 `find_package(YiCADPluginSdk)` 构建现有
demo。`dumpbin /exports` 确认 DLL 只导出：

- `yicad_plugin_get_abi_version`；
- `yicad_plugin_init`；
- `yicad_plugin_shutdown`。

Conan 同时生成的 Debug/Release 用户预设包含同名 `conan-default`，本机直接解析根目录
预设会报告 `Duplicate presets`。本次使用相同的 `build/Debug`、`build/Release` 缓存和
配置参数执行 `cmake --build <dir> --config <config>`；该环境问题不影响生成器、工具链或
产物。MSBuild 启动前还移除了工具执行环境重复注入的大小写不同 `PATH` 项。

## 3. ABI 和版本协商审查

生产头文件现在同时冻结以下内容：

- Win64 v1/v2 宿主表和插件输出表的大小、对齐与逐字段偏移；
- Win64 v3 草案宿主表 160 字节、导入子表 248 字节及 8 字节对齐；
- Win32 候选快照、固定值类型、数组视图和每个 `YICAD_*_MIN_SIZE`；
- v1/v2 宿主表、v3 导入子表和三个插件入口的精确函数类型及 `__cdecl`；
- v3 `importApi` 仅位于 v2 前缀末尾，入口使用 C 链接声明。

`PluginManager` 按 `min(pluginVersion, hostVersion)` 协商，按 v1/v2/v3 的精确前缀复制宿主
表，并拒绝不能覆盖协商前缀的宿主表。插件输出表过短、超过容量或未原值确认协商版本均
失败。版本 0 在 init 前拒绝；高于 v3 的插件在草案宿主上协商 v3；v3 插件面对正式 v2
宿主时协商 v2，插件可确认或从 init 拒绝。init 返回失败或抛出异常都会回滚本次注册、
调用一次 shutdown、回滚遗留导入会话并在最后卸载 DLL。

## 4. 输入和能力策略审查

所有 v3 入口复用 `validStructPrefix` 读取公开最小前缀。固定数组和可扩展 HATCH 数组在
解引用前统一检查空指针、数量、元素对齐、字节步长和地址乘加溢出；UTF-8 视图拒绝嵌入
NUL、无效编码和地址溢出。颜色保留位、枚举、标志位和全部布尔字段在写入模型前验证。

HATCH 在添加任何实体前完成全部环、边、步长、孔环归属、闭合性和样条数据检查，且拒绝
非零未使用字段。SPLINE、POLYLINE、块引用、属性和资源依赖同样先完成整体检查。资源句柄
记录在修改资源表前预分配，避免句柄分配失败后留下可提交的部分资源修改。

能力策略与实现一致：复杂线型、有理/周期样条、MTEXT 背景、外部块、属性标志、坐标标注
和图像裁剪明确返回 `YICAD_IMPORT_ERROR_UNSUPPORTED`；标注样式只有调用方显式允许时才
忽略 `unsupportedFieldMask` 并留下诊断；缺失字体和图像文件保留原始引用并留下诊断。
三维多段线/网格没有公开创建入口，格式插件必须在调用宿主前拒绝，不能映射到二维入口。

## 5. 会话、撤销和清理审查

导入会话拒绝同文档第二会话以及与普通事务、命令组的嵌套。事务启动后的异常会尝试立即
回滚；空会话提交执行回滚，不产生撤销项；非空会话只提交一个宏命令。提交、回滚及提交
失败均消费会话并使容器和资源句柄失效。

文档关闭前由 `UITabDrawWidget::documentAboutToClose` 通知宿主回滚对应会话。插件 init
失败、插件 shutdown 后、DLL 卸载前以及 HostApi 析构时都会回滚并失效剩余会话。C ABI
最外层和 SDK 入口捕获全部异常，跨边界只传递 POD、借用指针和宿主持有的无类型句柄。

现有 demo 只覆盖 v3 图层、公共属性、LINE/CIRCLE、提交和 SDK RAII 回滚，无法构造裁剪
函数表、错误步长、复杂 HATCH、文档关闭中途失败等路径。按阶段 8 约束，这些路径仅通过
上述生产代码和编译期断言审查，没有增加测试开关、测试插件、夹具或运行时验证分支。

## 6. 已知非阻塞项

完整宿主构建仍报告项目既有的编码、窄化转换和弃用 API 警告；本阶段没有新增编译警告或
错误。步骤 9 必须从干净目录重新执行同一矩阵，并完成正式版本提升、名称冻结和发布安装
树检查；本记录不授权提前移除草案隔离。
