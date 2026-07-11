# YiCAD DXF 插件

该目录包含 YiCAD 的 DXF 文件格式插件。插件使用 `libdxfrw 2.2.0` 读取和写出
DXF，并通过公开的 `YiCAD::PluginSdk` C++ API 访问文档数据。

当前已注册 DXF 导入和导出过滤器。导入在单个 `ImportSession` 中创建文档设置、
资源、块和实体；导出直接枚举 SDK 只读文档快照，写出 ASCII DXF 2013
（`AC1027`）。

支持范围包括文档单位、测量制式、全局线型比例、源代码页、线型、图层、文字样式、
标注样式、块，以及点、直线、射线、构造线、圆弧、圆、椭圆、多段线、控制点样条、
实体填充、文字、多行文字、五类语义标注、引线、填充、块引用和模型空间图像。
不受 YiCAD 模型支持的 DXF 数据会被忽略。

`libdxfrw 2.2.0` 当前没有可靠的 `ATTDEF`、`ATTRIB` 写出接口，也不会写出
fit-point spline 的拟合点数据；导出时这些对象保持跳过，不伪装成其他 DXF 类型。
插件不包含 YiCAD 内部头文件，也不链接 Qt 或 YiCAD 应用目标。

## 构建和安装

在仓库根目录准备依赖并完成 CMake 配置后，可单独构建插件：

```powershell
cmake --preset Release
cmake --build --preset Release --target YiCadDxfPlugin
```

仅构建目标不会形成完整部署目录。请通过 Runtime 预设同时安装应用、插件清单、两个
DLL 和运行时许可证：

```powershell
cmake --build --preset Release-Runtime
```

执行 Runtime 组件安装后，待部署文件位于：

```text
build/Release/bin/plugins/
  dxf.xml
  dxf/YiCadDxfPlugin.dll
  dxf/YiCadLibdxfrw220.dll
```

YiCAD 扫描插件目录第一层的 `*.xml`，清单中的 DLL 相对路径以清单所在目录为基准。
`YiCadLibdxfrw220.dll` 与插件 DLL 安装到同一目录，由插件加载器的受限 DLL 搜索路径
解析；部署时两者不可拆分。Plugin SDK 安装组件不包含 libdxfrw 的头文件、导入库或 DLL。
手工复制部署时，还必须保留 `dxf.xml` 的第一层位置和其中的相对路径，并随发布包提供
适用的许可证文本。不要只复制 `YiCadDxfPlugin.dll`。

生产环境的实际插件加载目录是 `C:\ProgramData\YiCAD\plugins`，不是安装输出中的
`build/Release/bin/plugins`。将安装输出中的 `dxf.xml` 和整个 `dxf` 子目录复制到该
目录；最终应形成 `C:\ProgramData\YiCAD\plugins\dxf.xml` 及其旁边的 `dxf` 子目录。

## 内置 libdxfrw 维护

内置源码固定来自上游标签 `LC2.2.0` 的提交
`d73a25c61fa6b7f41000b38b4b4c8b32ed4e2fd1`。来源、导入范围、本地修改策略和修改
文件清单记录在 `third_party/libdxfrw/UPSTREAM.md`；原始许可证保留在同目录的
`COPYING` 中，原始快照校验值记录在 `MANIFEST.sha256` 中。

同步上游时，先更新纯上游源码快照及校验清单，再单独应用 YiCAD 的 DLL 导出适配。
不得移除上游文件已有的版权或许可证声明；修改上游文件时，应在文件中记录修改事实和
日期，并同步更新 `UPSTREAM.md` 的本地修改清单。发布源码包必须包含本目录中的实际
构建源码、CMake 脚本、`COPYING`、`UPSTREAM.md` 和 `MANIFEST.sha256`。

## 第三方许可证

DXF 读写使用仓库内置的 `libdxfrw 2.2.0` 源码，并以独立 DLL 方式链接。`libdxfrw`
使用 `GPL-2.0-or-later` 许可证；对应 GPLv2 文本位于仓库的
`licenses/gpl-2.0.txt`，并由 Runtime 组件安装到 `bin/licenses/gpl-2.0.txt`。
YiCAD 及本插件仍按仓库 `LICENSE` 所述的 GPLv3 条款发布。
内置源码的原始 `COPYING`、来源与本地修改记录分别保留在
`third_party/libdxfrw/COPYING` 和 `third_party/libdxfrw/UPSTREAM.md`；源代码发布包
必须同时保留这些文件及实际参与构建的修改后源码。
