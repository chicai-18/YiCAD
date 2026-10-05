# YiCAD Demo 插件

该插件只依赖 `YiCAD::PluginSdk` 公开接口目标，不链接 Qt 或 YiCAD 内部库。它注册三条命令（Ribbon 的 **Demo > Draw** 下各有一个按钮）、一个实体类"管道"（`com.yicad.demo.Pipe`），以及 `.demo` 导入和 `com.yicad.demo/demo` 导出格式。

- **Add demo line**（`com.yicad.demo/demo.add-line`）：在当前文档里添加一条从 `(0, 0)` 到 `(100, 100)` 的直线，然后重生成并自动缩放视图。
- **Add demo pipe**（`demo.add-pipe`）：在文档事务里新建一根管道，折点 `(0, 0)`、`(100, 0)`、`(100, 60)`，管径 10。管道画成两侧边线、两端半圆端头、中心线（文档里有 CENTER 线型时用它）、第一段中点一个红色实心箭头和 `DN管径` 的标注；能选中、拖夹点（每个折点一个，起点旁边一个改管径）、捕捉端点、中点与最近点、移动旋转缩放镜像、炸开，撤销一步恢复。
- **Double demo pipe diameters**（`demo.pipe-grow`）：把模型空间里所有管道的管径加倍，一次撤销全部恢复。

`.demo` 导入通过 `ImportSession` 批量添加直线、圆和管道：全部解析成功后一次提交，任意记录失败则整体回滚。选择 **YiCAD Demo Drawing (*.demo)** 导出时，插件通过拥有型实体变体枚举输出当前文档中的真实直线、圆和管道数据。

demo 固定声明 ABI v4，只通过常规 C++ SDK 的语义接口（`ImportSession`、`LayerData`、`EntityAttributes`、`ImportContainer`、`DocumentTransaction`、`EntityClass`）工作，不直接构造 ABI POD 或填写 ABI 元数据。管道的写法见 `DemoPipe.h`、`DemoPipe.cpp`：数据编码、`worldDraw`、包围框、变换、夹点、捕捉与炸开；插件不在时（例如图纸带到没装这个插件的电脑上），管道读成代理，按存盘时记下的图形显示，可以删除、变换、复制、改图层与颜色，数据原样保存。

示例文件解析不依赖具体库。
真实格式插件应自行链接 `libdxfrw` 等解析库，PluginSDK 不包含或传播这些依赖。
仓库中的 `plugins/dxf_plugin` 展示了如何内置此类解析库的源码、静态链接进插件 DLL，
并随插件提供许可证；第三方插件应按其依赖许可证履行对应义务。

## Demo 文件格式

文件使用无 BOM 的 UTF-8 文本。首行固定为 `YICAD_DEMO_V2`，后续每行是一条实体记录：

```text
YICAD_DEMO_V2
LINE 0 0 100 100
CIRCLE 50 50 25
PIPE 10 3 0 0 100 0 100 60
```

- `LINE` 后依次为起点 `x y` 和终点 `x y`。
- `CIRCLE` 后依次为圆心 `x y` 和半径。
- `PIPE` 后依次为管径、折点数（至少 2）和各折点的 `x y`。
- 空行会被忽略；未知类型、缺少参数、多余参数或无效几何会使整个导入失败并回滚。
- demo 只选择完整只读实体变体中的直线、圆和管道，其他实体不会写入 `.demo` 文件。

## 独立构建

该目录是仓库外插件工程的完整示例。它不作为 YiCAD 主工程的子目录参与构建，也不读取 `YiCAD/src`。先安装 YiCAD 的 `PluginSDK` 组件，再把 `CMAKE_PREFIX_PATH` 指向 YiCAD 的安装前缀：

```powershell
cmake --build --preset Release-PluginSDK
```

然后在本示例目录中配置和构建：

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\path\to\YiCAD"
cmake --build build --config Release
```

也可以在 YiCAD 仓库根目录直接构建 demo 目标：

```powershell
cmake --preset Debug
cmake --build build/Debug --config Debug --target YiCadDemoPlugin
```

也可以把 `YiCADPluginSdk_DIR` 直接设为安装后的 `lib/cmake/YiCADPluginSdk` 目录。Visual Studio 生成器通常把 DLL 写到 `build/Release/YiCadDemoPlugin.dll`；实际位置以 CMake 构建输出为准。Debug 构建使用 `--config Debug`。

安装 YiCAD 后，示例源码副本位于 `share/YiCAD/examples/demo_plugin`。复制该目录到任意仓库外位置后，仍只需已安装的 SDK 即可配置和构建。

## 绝对路径部署

以管理员身份打开 PowerShell，由用户创建真实清单。将占位路径替换为已构建 DLL 的绝对路径：

```xml
<?xml version="1.0" encoding="UTF-8"?>
<plugin dll="D:\path\to\YiCadDemoPlugin.dll"/>
```

将内容保存为 `C:\ProgramData\YiCAD\plugins\demo.xml`。DLL 可以位于清单目录之外。

## 相对路径部署

1. 创建 `C:\ProgramData\YiCAD\plugins\demo\`。
2. 把 `YiCadDemoPlugin.dll` 复制到该子目录。
3. 把 [`demo.xml.example`](demo.xml.example) 复制为 `C:\ProgramData\YiCAD\plugins\demo.xml`。

清单中的相对 DLL 路径以 XML 所在目录为基准。YiCAD 只扫描 `C:\ProgramData\YiCAD\plugins` 第一层的 `*.xml`；其他目录中的相同清单不会被发现。

相对路径和绝对路径都是受支持的部署方式。只要最终解析到同一个 DLL，加载效果相同。开发时可让清单直接指向外部插件工程构建目录下的绝对路径；固定部署建议使用 `demo/YiCadDemoPlugin.dll`，避免依赖本机构建目录。不要同时部署两个指向不同 Demo DLL 的清单，否则相同插件 ID 和格式注册会发生冲突。

如果插件还有私有 DLL 依赖，应把这些 DLL 与插件 DLL 放在同一个插件子目录中，并将
它们视为不可拆分的部署单元。`PluginSDK` 组件不会自动收集第三方插件的私有依赖或
许可证；插件发布者需要自行随部署包提供这些文件。也可以像内置 DXF 插件那样把依赖库
静态链接进插件 DLL，从而不需要私有 DLL。

## 手工验收

1. 启动安装后的 YiCAD，确认 Demo Ribbon 按钮出现。
2. 无活动文档时点击按钮，确认应用显示提示且不崩溃。
3. 新建或打开文档后再次点击，确认直线出现、视图刷新并可撤销。
4. 打开包含多条 `LINE`/`CIRCLE` 记录的有效 `.demo` 文件，确认实体全部出现；执行一次撤销，确认本次导入的所有实体同时消失。
5. 打开先包含有效记录、后包含无效记录的 `.demo` 文件，确认导入失败且没有留下已解析的部分实体。
6. 将包含直线和圆的文档另存为 `.demo`，确认文件包含真实坐标数据，并可重新导入得到相同的直线和圆。
7. 分别使用绝对路径和相对路径清单验证加载。
8. 将同一 XML 放到其他目录，确认 YiCAD 不会扫描它。
