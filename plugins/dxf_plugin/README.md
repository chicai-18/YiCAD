# YiCAD DXF 插件

该目录是 DXF 文件格式插件工程。阶段 1 只提供可加载的插件骨架、构建与部署规则；当前版本尚未注册 DXF 导入和导出过滤器。后续阶段会按 `doc/DXF_PLUGIN_IMPLEMENTATION_PLAN.md` 扩展 ABI v3 SDK，并实现格式转换。

插件仅依赖公开的 `YiCAD::PluginSdk` C++ 接口和 Conan 提供的 `libdxfrw::libdxfrw`，不包含 YiCAD 内部头文件，也不链接 Qt 或 YiCAD 应用目标。

## 构建和安装

在仓库根目录准备依赖并配置后，可以单独构建插件：

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

将 `plugins` 目录中的内容复制到 `C:\ProgramData\YiCAD\plugins`。YiCAD 只扫描该目录第一层的 `*.xml`；清单中的 DLL 相对路径以清单所在目录为基准。

## 第三方许可证

DXF 读写使用 [libdxfrw 2.2](https://github.com/LibreCAD/libdxfrw)，通过 Conan 以静态库方式链接。libdxfrw 使用 `GPL-2.0-or-later` 许可证；对应 GPLv2 文本位于仓库的 `licenses/gpl-2.0.txt`，并由 Runtime 组件安装到 `bin/licenses/gpl-2.0.txt`。YiCAD 及本插件仍按仓库 `LICENSE` 所述的 GPLv3 条款发布。
