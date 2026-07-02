# YiCAD 新插件系统最终验收表

本文记录任务 14 的源码级静态验收结论。按实施计划约束，本任务未执行 CMake 配置、编译、安装或真实 ProgramData 部署；运行时项目需由用户使用已有产物完成。

任务 13 已按用户要求取消，因此本次交付不包含 `PluginTrustPolicy`、Authenticode 强制签名选项或任务 13 的额外稳定性矩阵。

| 验收项 | 静态结论 | 用户运行验收 |
| --- | --- | --- |
| 固定目录第一层 XML 扫描 | `PluginManifestReader` 使用 `C:\ProgramData\YiCAD\plugins`，非递归读取 `*.xml` | 将同一 XML 放在固定目录第一层、子目录和其他目录，确认仅第一层生效 |
| 绝对/相对 DLL 路径 | 清单读取器以 XML 目录解析相对路径，并规范化为存在的绝对 `.dll` 文件 | 分别部署绝对和相对清单，确认 DLL 可位于扫描目录外 |
| 正常、坏、重复插件隔离 | Manager 逐清单处理失败；Registry 事务回滚重复 ID 和注册冲突 | 同时部署正常、缺导出和重复 ID 插件，确认正常插件仍可用 |
| 命令与 Ribbon | Registry 使用 `(pluginId, commandId)`，UI 适配器物化按钮和命令补全 | 点击 Demo 按钮并从命令行执行 `com.yicad.demo/demo.add-line` |
| 文档、几何、刷新、缩放与撤销 | HostApi 校验 handle；实体通过历史接口添加；SDK 暴露 regen/zoomAuto | 添加实体后确认视图刷新、自动缩放且可撤销 |
| 导入、导出和事务回滚 | FileIO 适配器路由活动插件；v2 事务 RAII 回滚；迭代器输出真实直线/圆 | 导入有效/无效 `.demo`，验证整体提交/回滚；导出后重新导入 |
| shutdown 顺序 | `shutdownAll()` 按成功初始化逆序调用，每个插件至多一次，随后卸载 DLL | 部署两个可观察关闭顺序的插件后退出应用 |
| 旧插件彻底移除 | 生产代码不存在旧 Qt/C++ ABI 与旧加载路径；新 Loader 只查找三个 C 导出 | 放置旧 Qt 插件 DLL，确认不会被加载 |
| 内置文件格式无回归 | FileIO 保留内置过滤器，同时追加原生插件过滤器路由 | 打开并保存 OCD 文件 |
| 仓库外 SDK 消费 | 安装导出 `YiCADPluginSdk` package 与 `YiCAD::PluginSdk`，公开头安装到稳定目录；Demo 只调用 `find_package` | 安装 YiCAD 后复制 Demo 到仓库外，分别配置 Debug/Release 并构建 |

## 安装与打包检查

- 安装规则分为 `Runtime` 和 `PluginSDK`；四个组件化 build preset 会调用对应安装目标，普通用户可以只安装可执行程序、运行库和资源，插件开发者可以在同一前缀追加 SDK。
- Debug、Release 使用同一只含头文件的 SDK target；生成的 package 不记录源码树、Qt 或 Conan 本机路径。
- SDK package 只导出 ABI/SDK 公开头，不导出 YiCAD 应用、Qt 或内部运行时 target。
- SDK 文档、ABI 演进规则、许可证和独立 Demo 源码安装到 `share/YiCAD`；真实插件 XML 和生成 DLL 不进入仓库。
- 应用启动在主窗口基础设施完成后调用 `PluginManager::loadAll()`；析构开始阶段先调用 `shutdownAll()`，再释放 UI、HostApi、Registry 和宿主上下文。
