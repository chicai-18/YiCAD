# YiCAD 插件 SDK

YiCAD 插件 SDK 当前只支持 `YICAD_PLUGIN_ABI_V3`。插件包含
`YiCadPluginSdk.h`，实现三个 C linkage 入口，并在初始化期间通过 `Host`
注册命令或文件过滤器。插件业务代码应使用 C++ SDK，不直接读取底层函数表。

## 构建与入口

插件使用 C++23，并与 YiCAD 使用相同的 MSVC 运行库配置。入口必须捕获所有异常：

```cpp
YICAD_PLUGIN_EXPORT uint32_t YICAD_PLUGIN_CALL
yicad_plugin_get_abi_version()
{
    return YICAD_PLUGIN_ABI_V3;
}
```

`yicad_plugin_init` 接收 `YiCadHostApi` 和 `YiCadPluginApi`。当前宿主只接受 v3，
插件应填写稳定的 UTF-8 `pluginId`、`pluginName` 和 `pluginVersion`。这些字符串
至少保持有效到 `yicad_plugin_shutdown` 返回。

## 导入

文件回调通过 `Host::document(handle)` 获得 `Document`，再调用
`Document::beginImport()`。`ImportSession` 未提交时析构会自动回滚；一次文件导入
只提交一次。资源通过 `createLineType`、`createLayer`、`createTextStyle` 和
`createDimensionStyle` 创建，实体通过 `ImportContainer` 创建。

v3 支持点、线、射线、无限长线、圆弧、圆、椭圆、多段线、样条、三点/四点
实体填充、单行文字、多行文字、块、块引用、属性、五类标注、引线、填充和图像。
`createSolid(const SolidData&)` 用于创建 `DmSolid`。

## 导出与只读枚举

`Document` 直接读取当前文档并返回拥有型 C++ 值：

```cpp
DocumentSettings settings() const;
std::vector<LineTypeData> lineTypes() const;
std::vector<LayerData> layers() const;
std::vector<TextStyleData> textStyles() const;
std::vector<DimensionStyleData> dimensionStyles() const;
std::vector<BlockData> blocks() const;
EntityIterator entities() const noexcept;
EntityIterator entities(const BlockData& block) const noexcept;
```

`EntityIterator::next(EntityData&)` 把当前实体转换为拥有型 `std::variant`。
字符串和数组不引用宿主临时缓冲区。宿主只枚举 YiCAD 的一等、可持久化实体；文字
展开、标注展开、预览、Overlay、区域三角化和其他内部对象不会进入枚举结果。

## 线程、所有权与错误

SDK 只允许在 YiCAD UI 主线程调用。文档、资源、块和底层迭代器句柄均由宿主持有；
插件不得释放或跨文件回调保存。C++ 返回值拥有其中的字符串和数组，可以在下一次
宿主调用后继续使用。导入函数返回 `YiCadImportResult`，文件过滤器最终转换为
`YICAD_SUCCESS` 或 `YICAD_FAILURE`。

完整工程参见 `plugins/demo_plugin`。

## SDK 安装与插件部署

先在 YiCAD 仓库根目录安装与插件配置一致的 SDK 组件：

```powershell
cmake --build --preset Release-PluginSDK
```

仓库外插件通过 `find_package(YiCADPluginSdk CONFIG REQUIRED)` 查找 SDK，并只链接
`YiCAD::PluginSdk`。已安装的 `share/YiCAD/examples/demo_plugin` 是可复制到仓库外
独立构建的示例。SDK 只包含公开头文件、CMake package、文档、许可证和 Demo 源码，
不包含 DXF 插件或 libdxfrw 的头文件、导入库与 DLL。

运行时插件采用“第一层 XML 清单 + 子目录 DLL”的布局。生产环境下 YiCAD 只扫描
`C:\ProgramData\YiCAD\plugins\*.xml`，并以清单所在目录为基准解析 `dll` 属性：

```text
C:\ProgramData\YiCAD\plugins\
  example.xml
  example/ExamplePlugin.dll
```

开发时也可在清单中使用插件 DLL 的绝对路径。固定部署应使用相对路径，并将插件的
所有私有 DLL 依赖放在插件子目录中。不要部署多个使用相同 `pluginId` 的 DLL。

仓库内的 DXF 插件是带私有第三方依赖的实际示例。安装 `Runtime` 后，先从
`bin/plugins` 取得安装产物，再按以下布局部署：

```text
C:\ProgramData\YiCAD\plugins\
  dxf.xml
  dxf/YiCadDxfPlugin.dll
  dxf/YiCadLibdxfrw220.dll
```

`YiCadLibdxfrw220.dll` 由内置 libdxfrw 2.2.0 源码构建，必须与 DXF 插件 DLL 一起
部署。其许可证为 `GPL-2.0-or-later`；上游 `COPYING` 保留在源码目录，GPLv2 全文
位于 `licenses/gpl-2.0.txt`，并随 Runtime 安装到 `bin/licenses`。
