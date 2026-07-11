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

执行 Runtime 组件安装后，部署文件位于：

```text
build/Release/bin/plugins/
  dxf.xml
  dxf/YiCadDxfPlugin.dll
```

YiCAD 扫描插件目录第一层的 `*.xml`，清单中的 DLL 相对路径以清单所在目录为基准。

## 第三方许可证

DXF 读写使用仓库内置的 `libdxfrw 2.2.0` 源码，并以独立 DLL 方式链接。`libdxfrw`
使用 `GPL-2.0-or-later` 许可证；对应 GPLv2 文本位于仓库的
`licenses/gpl-2.0.txt`，并由 Runtime 组件安装到 `bin/licenses/gpl-2.0.txt`。
YiCAD 及本插件仍按仓库 `LICENSE` 所述的 GPLv3 条款发布。
