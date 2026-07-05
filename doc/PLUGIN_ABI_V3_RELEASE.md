# YiCAD 插件 ABI v3 发布记录

## 发布基线

- 应用与 PluginSDK 版本：YiCAD 0.20.0。
- ABI 版本：`YICAD_PLUGIN_ABI_V3 == 3`。
- 阶段 8 候选基线：`970052f`。
- 正式基线：包含本记录且主题为 `release plugin abi v3` 的提交；发布标签必须直接指向该
  提交，不能包含后续 ABI 布局修改。
- 支持架构：Win64。Win32 只保留编译期布局快照，不属于本次支持范围。

## 冻结布局

正式头文件无条件公开 v3，不需要构建宏。Win64 `YiCadHostApi` 为 160 字节、
`YiCadImportApi` 为 248 字节，均按 8 字节对齐；`importApi` 只追加在 v2 宿主表前缀尾部。
v1/v2 版本常量、函数表前缀、协商分支和插件入口保持不变。

## 验证环境与命令

发布验证环境沿用阶段 8：Windows 11、Visual Studio 2022 17.10、MSVC 19.38、CMake
3.26.3、Conan 2.29.1、Qt 5.15.2，以及仓库 `conan.lock` 锁定的依赖。Debug 和 Release
使用对应的 `profiles/windows-msvc-*` 配置。

```powershell
conan install . --output-folder=build/conan-release --profile=profiles/windows-msvc-release --lockfile=conan.lock --build=never
cmake --preset Release "-DCMAKE_PREFIX_PATH=$env:Qt5_DIR"
cmake --build --preset Release
cmake --install build/Release --config Release

conan install . --output-folder=build/conan-debug --profile=profiles/windows-msvc-debug --lockfile=conan.lock --build=never
cmake --preset Debug "-DCMAKE_PREFIX_PATH=$env:Qt5_DIR"
cmake --build --preset Debug
cmake --install build/Debug --config Debug
```

2026-07-05 已从全新 `build/stage9-clean-Debug` 和 `build/stage9-clean-Release` 目录完成
Win64 Debug/Release 配置、完整构建和安装。C11/C++23 公开头语法与静态布局断言通过；安装
树草案扫描无匹配；仓库外 demo 只通过安装的 `YiCAD::PluginSdk` 构建成功；
`dumpbin /exports` 仅列出三个固定插件入口。v1/v2/v3 运行时协商、v3 导入提交、失败回滚、
单步撤销、文档关闭和插件卸载清理沿用阶段 8 对同一候选行为的人工验收结论。本次发布收口
没有改变候选布局或运行时语义。

## 发布内容

PluginSDK 组件安装正式 ABI/SDK 头、CMake package、本指南、ABI 演进规则、ABI v3 参考、
本发布记录、许可证及可独立构建 demo。实施计划和阶段 8 内部审查记录保留在源码仓库，
不属于安装契约。

自本基线起，v3 既有字段、函数签名、调用约定、枚举值、默认值、所有权和失败语义永久
冻结。兼容扩展只能在明确可扩展结构或函数表尾部追加字段并提升后续 ABI 版本；破坏性
变化必须发布新 ABI。
