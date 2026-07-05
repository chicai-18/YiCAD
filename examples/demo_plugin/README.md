# YiCAD Demo 插件

该插件只依赖 `YiCAD::PluginSdk` 公开接口目标，不链接 Qt 或 YiCAD 内部库。它注册命令 `com.yicad.demo/demo.add-line`、Ribbon 中的 **Demo > Draw > Add demo line** 按钮，以及 `.demo` 导入和 `com.yicad.demo/demo` 导出格式。

执行命令会重新获取当前文档，添加一条从 `(0, 0)` 到 `(100, 100)` 的直线，然后重生成并自动缩放视图。ABI v2 的 `.demo` 导入会在一个文档事务中批量添加直线和圆：全部解析成功后一次提交，任意记录失败则整体回滚。选择 **YiCAD Demo Drawing (*.demo)** 导出时，插件通过只读实体迭代 API 输出当前文档中的真实直线和圆数据。

demo 默认声明支持 ABI v3，并只通过常规 C++ SDK 的 `ImportSession`、`LayerData`、`EntityAttributes` 和
`ImportContainer` 语义接口创建导入图层、直线和圆，不直接构造 ABI POD 或填写 ABI
元数据；面对只支持 v2 的宿主仍回退到上述 v2 事务流程。示例文件解析不依赖具体库。
真实格式插件应自行链接 `libdxfrw` 等解析库，PluginSDK 不包含或传播这些依赖。

## Demo 文件格式

文件使用无 BOM 的 UTF-8 文本。首行固定为 `YICAD_DEMO_V2`，后续每行是一条实体记录：

```text
YICAD_DEMO_V2
LINE 0 0 100 100
CIRCLE 50 50 25
```

- `LINE` 后依次为起点 `x y` 和终点 `x y`。
- `CIRCLE` 后依次为圆心 `x y` 和半径。
- 空行会被忽略；未知类型、缺少参数、多余参数或无效几何会使整个导入失败并回滚。
- 当前只读 ABI 仅定义直线和圆数据，导出时其他实体类型不会写入 `.demo` 文件。

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

## 手工验收

1. 启动安装后的 YiCAD，确认 Demo Ribbon 按钮出现。
2. 无活动文档时点击按钮，确认应用显示提示且不崩溃。
3. 新建或打开文档后再次点击，确认直线出现、视图刷新并可撤销。
4. 打开包含多条 `LINE`/`CIRCLE` 记录的有效 `.demo` 文件，确认实体全部出现；执行一次撤销，确认本次导入的所有实体同时消失。
5. 打开先包含有效记录、后包含无效记录的 `.demo` 文件，确认导入失败且没有留下已解析的部分实体。
6. 将包含直线和圆的文档另存为 `.demo`，确认文件包含真实坐标数据，并可重新导入得到相同的直线和圆。
7. 分别使用绝对路径和相对路径清单验证加载。
8. 将同一 XML 放到其他目录，确认 YiCAD 不会扫描它。
