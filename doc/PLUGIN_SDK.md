# YiCAD 插件 SDK

YiCAD 插件 SDK 当前只支持 `YICAD_PLUGIN_ABI_V4`（布局与约定见 `PLUGIN_ABI_V4_REFERENCE.md`）。
插件包含 `YiCadPluginSdk.h`，实现三个 C linkage 入口，并在初始化期间通过 `Host`
注册命令、文件过滤器或实体类。插件业务代码应使用 C++ SDK，不直接读取底层函数表。

## 构建与入口

插件使用 C++23，并与 YiCAD 使用相同的 MSVC 运行库配置。入口必须捕获所有异常：

```cpp
YICAD_PLUGIN_EXPORT uint32_t YICAD_PLUGIN_CALL
yicad_plugin_get_abi_version()
{
    return YICAD_PLUGIN_ABI_V4;
}
```

`yicad_plugin_init` 接收 `YiCadHostApi` 和 `YiCadPluginApi`。当前宿主只接受 v4，
插件应填写稳定的 UTF-8 `pluginId`、`pluginName` 和 `pluginVersion`。这些字符串
至少保持有效到 `yicad_plugin_shutdown` 返回。

## 导入

文件回调通过 `Host::document(handle)` 获得 `Document`，再调用
`Document::beginImport()`。`ImportSession` 未提交时析构会自动回滚；一次文件导入
只提交一次。资源通过 `createLineType`、`createLayer`、`createTextStyle` 和
`createDimensionStyle` 创建，实体通过 `ImportContainer` 创建。

支持点、线、射线、无限长线、圆弧、圆、椭圆、多段线、样条、三点/四点
实体填充、单行文字、多行文字、块、块引用、属性、五类标注、引线、填充、图像和
自定义实体（v4）。`createSolid(const SolidData&)` 用于创建 `DmSolid`。

`createCustomEntity(const CustomEntityData&)` 按类名、数据版本与字节建自定义实体：类登记了且
读得了数据时建原实体，否则建代理实体，保留数据，按 `setProxyFlags` 的权限放行操作。代理显示的
图形先经 `ImportSession::beginProxyGraphics` 取得一个收集容器、在里面建基本实体，再作为
`createCustomEntity` 的第二个参数交出；容器里属性为空、图层为空、线型为空、颜色与线宽随块的实体
沿用自定义实体自己的属性。DXF 插件读别的程序的自定义实体就是这样：类名与数据原样保留，
按 DXF 里的代理图形显示。

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

自定义实体（插件的、进程内扩展的与代理）以 `CustomEntityData` 交出：类名、数据版本、字节、
代理权限、代理累计的变换、是否代理，以及可交给 `DocumentTransaction::setCustomEntityData` 的
实体引用 `entity()`。要把它写进别的格式的代理图形时，在 `next` 之后调
`EntityIterator::graphics()`：宿主把它的图形做成基本实体逐个交出（块展开成内容，实心填充为
填充，图案填充为切好的线与点，文字为笔画，样条离散成多段线）。

## 自定义实体

插件实体的数据是宿主保管的一段字节，编码由插件定义；撤销、存盘、读盘与插件缺失时的代理显示
都由宿主处理。插件写一个派生自 `EntityClass<Data>` 的类，SDK 负责字节与 `Data` 的转换、
实例缓存、函数表与异常隔离（函数里抛出的异常在 C ABI 边界转成失败，宿主不采用结果）：

```cpp
class PipeClass final : public yicad::plugin::EntityClass<PipeData>
{
public:
    PipeData decode(std::span<const uint8_t> bytes) const override;     // 读不了时抛异常
    std::vector<uint8_t> encode(const PipeData& data) const override;
    void worldDraw(const PipeData& data, const yicad::plugin::Gi& gi) const override;
    YiCadExtents2d extents(const PipeData& data) const override;
    void transform(PipeData& data, const YiCadMatrix2d& matrix) const override;
    // 可选：grips 与 moveGrips（一起覆盖）、snapPoints、explode、upgrade
};
```

没有覆盖的可选函数不进函数表：没有夹点时实体只能整体移动，捕捉与炸开由宿主按 `worldDraw`
的图元推导。`worldDraw` 经 `Gi` 输出图元（多段线、圆、圆弧、椭圆弧、样条、填充、三角形、文字、
图像、点、射线、构造线、块，以及属性、变换栈与屏幕空间），资源用 `Gi::lineType`、`layer`、
`textStyle`、`block` 按名字在实体所属文档里找。`ByteWriter`、`ByteReader` 是小端序编码的辅助（越界时抛异常）。

实体类的实例要存活到 `shutdown`（一般是插件的成员），在 init 里登记：

```cpp
yicad::plugin::EntityClassInfo info;
info.className = "com.example.Pipe";          // 必须以 pluginId 加点开头
info.classVersion = 1;                        // 数据编码的版本；读回的旧版本经 upgrade 升级
info.proxyFlags = YICAD_PROXY_ERASE | YICAD_PROXY_TRANSFORM;  // 插件不在时代理允许的操作
info.threadSafeDraw = false;                  // 为真时图形系统在工作线程上直接调 worldDraw
host.registerEntityClass(pluginId, m_pipe, info);
```

`threadSafeDraw` 为假（缺省）时，宿主在实体更新时于 UI 线程调一次 `worldDraw` 记下图元，
图形系统只回放记下的图形。命令里新建与改动实体走文档事务，一个事务是一步撤销：

```cpp
auto transaction = document.beginTransaction("Add pipe");
yicad::plugin::EntityRef created;
transaction.createCustomEntity(
    yicad::plugin::CustomEntityData("com.example.Pipe", 1, encoded), created);
transaction.commit();
```

`setCustomEntityData(entity, bytes)` 换一个已有实体的数据，实体引用来自只读枚举。插件只在程序
退出时卸载：宿主在 `shutdown` 之前注销它的实体类，之后文档里的实体只画记下的图形。

## 线程、所有权与错误

SDK 只允许在 YiCAD UI 主线程调用。文档、资源、块和底层迭代器句柄均由宿主持有；
插件不得释放或跨文件回调保存。C++ 返回值拥有其中的字符串和数组，可以在下一次
宿主调用后继续使用。导入函数返回 `YiCadImportResult`，文件过滤器最终转换为
`YICAD_SUCCESS` 或 `YICAD_FAILURE`。

完整工程参见 `plugins/demo_plugin`（示例实体"管道"在 `DemoPipe.h`、`DemoPipe.cpp`）。

## SDK 安装与插件部署

先在 YiCAD 仓库根目录安装与插件配置一致的 SDK 组件：

```powershell
cmake --build --preset Release-PluginSDK
```

仓库外插件通过 `find_package(YiCADPluginSdk CONFIG REQUIRED)` 查找 SDK，并只链接
`YiCAD::PluginSdk`。已安装的 `share/YiCAD/examples/demo_plugin` 是可复制到仓库外
独立构建的示例。SDK 只包含公开头文件、CMake package、文档、许可证和 Demo 源码，
不包含 DXF 插件或 libdxfrw 的头文件与库。

运行时插件采用“第一层 XML 清单 + 子目录 DLL”的布局。生产环境下 YiCAD 只扫描
`C:\ProgramData\YiCAD\plugins\*.xml`，并以清单所在目录为基准解析 `dll` 属性：

```text
C:\ProgramData\YiCAD\plugins\
  example.xml
  example/ExamplePlugin.dll
```

开发时也可在清单中使用插件 DLL 的绝对路径。固定部署应使用相对路径，并将插件的
所有私有 DLL 依赖放在插件子目录中。不要部署多个使用相同 `pluginId` 的 DLL。

仓库内的 DXF 插件是使用第三方库的实际示例：它把内置的 libdxfrw 2.2.0 源码编成静态库
链接进插件 DLL，因此没有私有 DLL 依赖。安装 `Runtime` 后，先从 `bin/plugins` 取得
安装产物，再按以下布局部署：

```text
C:\ProgramData\YiCAD\plugins\
  dxf.xml
  dxf/YiCadDxfPlugin.dll
```

libdxfrw 的许可证为 `GPL-2.0-or-later`；上游 `COPYING` 保留在源码目录，GPLv2 全文
位于 `licenses/gpl-2.0.txt`，并随 Runtime 安装到 `bin/licenses`。
