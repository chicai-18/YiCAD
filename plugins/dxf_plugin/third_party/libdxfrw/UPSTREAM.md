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
校验值，因此下列本地修改文件与清单中的对应校验值不同是预期行为，其余文件应与清单
一致。后续为 YiCAD 构建或功能修复而修改上游文件时，应保留原版权声明，在修改处显著
记录修改事实和日期，并同步更新下面的清单。
同步新上游版本时，应先更新纯上游快照及清单，再单独提交 YiCAD 适配改动。

libdxfrw 编译为静态库，只链接进 `YiCadDxfPlugin.dll`，不单独部署。

## 本地修改清单

2026-07-11，随导入一并提交的功能修改（当时未登记，2026-09-27 补记于此，并在修改处加注）：

- `src/libdxfrw.h`、`src/libdxfrw.cpp`：文件名由 `std::string` 改为
  `std::filesystem::path`，构造时按 UTF-8 解析（`std::filesystem::u8path`），各处
  `filestr.open` 直接传入 path，以支持非 ASCII 文件路径。
- `src/libdxfrw.h`、`src/libdxfrw.cpp`：新增 `textStyleNames`，读取 STYLE 表时记录
  句柄到名称的映射；读取 DIMSTYLE 时，若文字样式（组码 340）是能在映射中找到的句柄，
  换成样式名再交给 `DRW_Interface`。

2026-10-04，渲染方案阶段 5：

- `src/libdxfrw.cpp`：`writeEntity` 在 R13 及以上版本写出实体线型比例（组码 48，
  `DRW_Entity::ltypeScale`），为缺省值 1 时不写。上游读入时解析组码 48，写出时不写。

2026-10-05，渲染方案阶段 8（自定义实体照 AutoCAD 写进 DXF、读回）：

- `src/drw_interface.h`：新增 `DRW_UnknownEntity`（记录名与全部组码的原文），
  `DRW_Interface` 新增三个带默认空实现的虚函数 `addClass`、`addUnknownEntity`、
  `writeClasses`，上游的接口实现不受影响。
- `src/libdxfrw.h`、`src/libdxfrw.cpp`：读 CLASSES 段（`processClasses`，上游为 TODO、
  整段跳过），逐条交给 `addClass`；不认识类型的实体收集全部组码的原文交给
  `addUnknownEntity`（上游逐条跳过）；写文件时在 CLASSES 段里调 `writeClasses`，
  新增 `writeClass`（写一条类登记）与 `writeCustomEntity`（记录名、公共属性，之后原样写
  给出的组码；只支持 R13 起的 ASCII DXF）。
- `src/intern/dxfreader.h`、`src/intern/dxfreader.cpp`：新增 `getRawString()`，给出当前
  记录的原文（ASCII 为文件里的那一行，二进制按类型格式化）；二进制 DXF 的二进制块读成
  十六进制文字（与 ASCII DXF 相同），上游跳过。

## 已撤销的修改

2026-07-11 曾为构建 `YiCadLibdxfrw220` Windows DLL 新增 `src/yicad_libdxfrw_export.h`，
并在 `drw_base.h`、`drw_classes.h`、`drw_entities.h`、`drw_header.h`、
`drw_interface.h`、`drw_objects.h`、`libdxfrw.h` 中添加 `YICAD_LIBDXFRW_API` 导出声明。
2026-09-27 改为静态链接后已全部撤销：导出宏头文件已删除，前六个头文件恢复为上游原样。

仓库内新增的 `CMakeLists.txt` 和本说明不属于上游快照。
