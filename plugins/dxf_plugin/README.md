# YiCAD DXF 插件

该目录包含 YiCAD 的 DXF 文件格式插件。阶段 3 已实现 DXF 导入过滤器，使用
`libdxfrw 2.2.0` 读取文件，并通过公开的 `YiCAD::PluginSdk` C++ API 在单个
`ImportSession` 中创建文档资源、块和实体。

当前导入范围包括文档单位、测量制式、全局线型比例、源代码页、线型、图层、
文字样式、标注样式，以及点、直线、射线、构造线、圆弧、圆、椭圆、多段线、
样条、实体填充、文字、多行文字、五类标注、引线、填充、块引用和模型空间图像。
不受 YiCAD 模型支持的 DXF 数据会被忽略。导出功能属于实施计划阶段 4，当前尚未
注册导出过滤器。

`libdxfrw 2.2.0` 的读取接口没有提供 `ATTDEF` 和 `ATTRIB` 回调，因此这两类对象
当前与其他库端不可达对象一样保持忽略；插件没有增加计划明确排除的二次 DXF
解析流程。

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

将 `plugins` 目录中的内容复制到 `C:\ProgramData\YiCAD\plugins`。YiCAD 扫描该
目录第一层的 `*.xml`，清单中的 DLL 相对路径以清单所在目录为基准。

## 第三方许可证

DXF 读取使用 `libdxfrw 2.2.0`，通过 Conan 以静态库方式链接。
`libdxfrw` 使用 `GPL-2.0-or-later` 许可证；对应 GPLv2 文本位于仓库的
`licenses/gpl-2.0.txt`，并由 Runtime 组件安装到 `bin/licenses/gpl-2.0.txt`。
YiCAD 及本插件仍按仓库 `LICENSE` 所述的 GPLv3 条款发布。
