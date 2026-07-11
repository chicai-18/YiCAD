# libdxfrw 源码 DLL 引入计划

## 1. 目标

将当前由 Conan 提供的 `libdxfrw/2.2.0` 替换为仓库内维护的源码快照，构建为
Windows 动态库，并由 `YiCadDxfPlugin` 私有使用。允许 YiCAD 在上游源码基础上修复和扩展
DXF 读写能力，同时保留清晰的来源、许可证和本地修改记录。

本计划只改变 DXF 插件的实现依赖，不应把 libdxfrw 类型暴露到 YiCAD Plugin SDK，
也不应改变 `YICAD_PLUGIN_ABI_V3`。

## 2. 已确认的授权结论

- 上游项目：<https://github.com/LibreCAD/libdxfrw>
- 上游标签：`LC2.2.0`
- 上游提交：`d73a25c61fa6b7f41000b38b4b4c8b32ed4e2fd1`
- libdxfrw 许可证：`GPL-2.0-or-later`
- YiCAD 许可证：`GPL-3.0-only`
- 组合发行结论：可以按 GPLv3 发行。

动态链接不会改变 GPL 合规要求。发行 `libdxfrw` DLL 时，必须同时提供其对应源码、
本地修改及控制编译和安装所需的脚本。必须保留上游版权声明及许可证声明；修改过的上游
文件应显著标记修改事实和日期。

仓库已有 `licenses/gpl-2.0.txt` 和 `licenses/gpl-3.0.txt`。导入目录仍应保留上游原始
`COPYING`，以便源码快照自身具备完整授权信息。

## 3. 目标目录结构

```text
plugins/dxf_plugin/
├── CMakeLists.txt
├── README.md
├── src/
└── third_party/
    └── libdxfrw/
        ├── CMakeLists.txt
        ├── COPYING
        ├── UPSTREAM.md
        └── src/
```

`UPSTREAM.md` 至少记录上游地址、标签、提交哈希、导入日期、许可证和本地修改策略。
不要保留嵌套的 `.git` 目录，不使用浮动分支作为构建输入。

第一版优先导入上游完整 `src/`，排除示例程序、Visual Studio 工程、Autotools 文件、
CI 配置和上游仓库元数据。完成构建后再根据实际编译闭包评估是否裁剪 DWG 专用源码，
避免过早裁剪内部编码表或公共实现文件。

## 4. 目标构建架构

新增共享库目标，建议使用不会与系统中其他 libdxfrw 冲突的名称：

```cmake
add_library(YiCadDxfLibdxfrw SHARED ...)
add_library(YiCAD::DxfLibdxfrw ALIAS YiCadDxfLibdxfrw)
```

约束如下：

- `YiCadDxfPlugin` 通过 `PRIVATE` 链接 `YiCAD::DxfLibdxfrw`。
- libdxfrw 的头文件不进入 `YiCadPluginSdk` 的接口或安装内容。
- DLL 输出名建议包含 YiCAD 和版本信息，例如 `YiCadLibdxfrw220.dll`。
- DLL 与插件必须使用相同编译器、架构、C++ 配置和 MSVC 运行库。
- Debug 和 Release 输出必须隔离。
- libdxfrw ABI 改变时，必须同步重新编译 DXF 插件。
- Windows 安装产物必须保证插件加载时能够解析依赖 DLL。

优先把 `YiCadDxfPlugin.dll` 和 libdxfrw DLL 安装到同一个插件目录。实施前必须检查当前
插件加载器的 Windows DLL 搜索路径策略；不能仅假定 `LoadLibrary` 会自动搜索插件目录。
如果当前策略不支持，应在不扩大全局 DLL 搜索风险的前提下设计加载方案，并先向用户说明。

## 5. Windows DLL 导出改造

实施时先审计 `LC2.2.0` 是否完整支持 MSVC DLL 导出。重点检查插件使用的 `dxfRW`、
`DRW_Interface`、实体、头部和对象类型是否具有正确的 `dllexport`/`dllimport` 声明。

如果上游缺少完整导出宏，应新增统一宏，例如：

```cpp
#if defined(_WIN32)
#  if defined(YICAD_LIBDXFRW_BUILD)
#    define YICAD_LIBDXFRW_API __declspec(dllexport)
#  else
#    define YICAD_LIBDXFRW_API __declspec(dllimport)
#  endif
#else
#  define YICAD_LIBDXFRW_API
#endif
```

只导出插件实际跨 DLL 使用的公开类型和函数。不要依靠
`CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS` 作为长期方案，因为它无法表达稳定的导出边界，且可能
无意暴露内部符号。

需要特别验证：

- 跨 DLL 创建和销毁对象是否使用相同运行库；
- 是否跨边界传递 STL 容器、字符串、异常或具有内联实现的复杂类型；
- 类静态成员、虚表和 RTTI 是否正确导出；
- Release 使用 `/MD`，Debug 使用与项目一致的 Debug 运行库；
- 插件卸载前，所有 libdxfrw 对象均已销毁。

## 6. 分阶段实施

### 阶段 A：基线和只读审计

1. 检查工作树，保护用户已有修改。
2. 核对 Conan 锁定的 `libdxfrw/2.2.0` 与上游 `LC2.2.0` 的源码差异。
3. 列出当前 DXF 插件实际包含和使用的 libdxfrw API。
4. 审计上游 CMake 源文件列表、传递依赖和 Windows 导出支持。
5. 审计插件加载器及安装布局的依赖 DLL 搜索行为。
6. 报告导入文件范围、预计修改文件以及发现的关键风险。

如果源码选择、DLL 搜索方案或导出方式出现多个同等合理的实现路径，遵守仓库
`AGENTS.md`，列出选项并等待用户选择。

### 阶段 B：源码快照导入

1. 从固定提交导入所需源码，不携带上游 `.git`。
2. 保留所有源文件原始版权和许可证头。
3. 复制上游原始 `COPYING`。
4. 创建 `UPSTREAM.md`，记录来源和本地修改规则。
5. 对导入文件生成可复核清单，确认没有二进制和无关生成物。

此阶段会新增大量第三方文件。执行前必须按 `AGENTS.md` 向用户确认影响范围。

### 阶段 C：建立共享库目标

1. 添加 `third_party/libdxfrw/CMakeLists.txt`。
2. 创建 `YiCadDxfLibdxfrw` 共享库及内部别名目标。
3. 设置 C++ 标准、包含目录、编译定义、输出名和输出目录。
4. 添加或修复 Windows 导出宏，并标记本地修改及日期。
5. 将 DXF 插件改为私有链接源码 DLL。
6. 确保公共 Plugin SDK 不传播 libdxfrw 的包含目录或链接依赖。

### 阶段 D：移除 Conan 依赖

确认源码 DLL 已能独立配置和编译后，再移除：

- `conanfile.py` 中的 `libdxfrw/2.2.0` 要求及选项；
- `cmake/dependencies.cmake` 中的 `find_package(libdxfrw ...)`；
- `conan.lock` 中的 libdxfrw 节点；
- 其他 Conan 生成/检查逻辑中的 libdxfrw 引用；
- README 和插件文档中旧的 Conan 构建说明。

锁文件应通过规范 Conan 工作流重新生成，不手工猜测依赖图。该阶段涉及核心配置和多个
文件，执行前必须单独报告范围并获得确认。

### 阶段 E：安装和运行时加载

1. 将 libdxfrw DLL 安装到最终选定的插件运行目录。
2. 验证构建树和安装树中的插件均能解析依赖。
3. 确认卸载插件时不存在仍存活的 libdxfrw 对象或回调。
4. 检查安装组件和打包流程不会遗漏 DLL。
5. 检查 Plugin SDK 安装组件不会误带 libdxfrw 开发文件。

### 阶段 F：文档和合规收尾

1. 更新根 `LICENSE` 中 libdxfrw 的描述为源码内置且可能包含本地修改。
2. 更新项目 README 的依赖获取和构建步骤。
3. 更新 `plugins/dxf_plugin/README.md` 的构建、运行时部署和维护说明。
4. 确保发行源码包含实际构建使用的源码和脚本。
5. 确保每个修改过的上游文件都有修改说明，不覆盖原版权声明。

## 7. 验证清单

最低验收要求：

- Conan 安装不再解析或构建 libdxfrw。
- Release 和 Debug 均能完成配置与编译。
- 生成独立的 libdxfrw DLL 和 import library。
- `YiCadDxfPlugin` 仅私有链接该 DLL。
- 构建树中可以加载 DXF 插件。
- 安装后的 `YiCAD.exe` 可以加载 DXF 插件及依赖 DLL。
- 插件关闭和应用退出时无崩溃、重复释放或仍存活对象。
- 完成代表性 ASCII DXF 和 binary DXF 的导入测试。
- 完成 DXF 导出及导入回读测试。
- 覆盖当前插件 README 已知限制相关的回归样例。
- 使用搜索确认不存在旧的 `find_package(libdxfrw)`、
  `libdxfrw::libdxfrw` 或 Conan requirement。
- 检查安装清单、许可证文本、`UPSTREAM.md` 和修改标记。

若完整构建因本地 Qt、Conan 二进制或其他外部依赖缺失而无法执行，应保存准确命令和错误，
不得将未运行的验证描述为成功。

## 8. 提交建议

建议拆分为便于审计的提交：

1. `vendor libdxfrw LC2.2.0 source`
2. `build libdxfrw as dxf plugin dll`
3. `remove libdxfrw conan dependency`
4. `update bundled libdxfrw documentation`

后续同步上游时，先提交纯上游快照更新，再提交 YiCAD 的适配和功能修改。不要把上游同步、
业务功能改动和格式化混在同一提交中。

## 9. 后续 AI 会话执行指令

可将以下内容作为新会话的任务说明：

```text
请按照 doc/LIBDXFRW_SOURCE_DLL_PLAN.md 执行 libdxfrw 源码 DLL 引入。

先只执行“阶段 A：基线和只读审计”，检查工作树、上游快照、Conan 配方、DXF 插件
CMake、Windows 导出支持、插件 DLL 搜索路径及安装布局。给出审计结果、预计导入文件范围、
预计修改文件清单和需要确认的关键决策，然后停止等待确认。

严格遵守 AGENTS.md：导入大量第三方文件、修改超过三个核心文件、调整 Conan 锁文件前
必须说明影响并等待确认；不要删除用户文件或覆盖已有修改。使用固定上游标签 LC2.2.0 和
提交 d73a25c61fa6b7f41000b38b4b4c8b32ed4e2fd1，保留上游许可证和版权头，不把
libdxfrw 暴露到公共 Plugin SDK。
```
