# libdxfrw 上游来源

- 上游仓库：https://github.com/LibreCAD/libdxfrw
- 上游标签：`LC2.2.0`
- 上游提交：`d73a25c61fa6b7f41000b38b4b4c8b32ed4e2fd1`
- 导入日期：2026-07-11
- 许可证：GPL-2.0-or-later

## 导入范围

本目录保留上游原始 `COPYING`，并完整导入该提交下的 `src/` 目录。
未导入上游 `.git`、示例程序、Visual Studio 工程、Autotools 文件、CI 配置和仓库元数据。

`MANIFEST.sha256` 记录导入文件相对于本目录的路径及 SHA-256，可用于复核快照内容。

## 本地修改策略

导入时不修改上游源码、版权声明或许可证文本。`MANIFEST.sha256` 保留纯上游快照的
校验值，因此下列本地修改文件与清单中的对应校验值不同是预期行为。后续为 YiCAD 构建、
Windows DLL 导出或功能修复而修改上游文件时，应保留原版权声明，并在修改处显著记录
修改事实和日期。
同步新上游版本时，应先更新纯上游快照及清单，再单独提交 YiCAD 适配改动。

## 本地修改清单

2026-07-11，为构建 `YiCadLibdxfrw220` Windows DLL，新增
`src/yicad_libdxfrw_export.h`，并在以下上游头文件中为 DXF 插件跨 DLL 使用的公开类型
添加 `YICAD_LIBDXFRW_API` 导出声明：

- `src/drw_base.h`
- `src/drw_classes.h`
- `src/drw_entities.h`
- `src/drw_header.h`
- `src/drw_interface.h`
- `src/drw_objects.h`
- `src/libdxfrw.h`

仓库内新增的 `CMakeLists.txt`、导出宏头文件和本说明不属于上游快照。
