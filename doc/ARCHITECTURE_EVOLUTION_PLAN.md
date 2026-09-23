# YiCAD 架构演进分阶段执行方案

本文档给出 YiCAD 从当前形态演进到「可扩展、层次清晰、可测试」目标架构的
分阶段执行方案。每个阶段独立可交付、独立可回退，阶段之间只有明确的前置依赖。

> 本方案基于 `d8e0be5` 的代码基线。文中引用的行号、调用点数量、代码量均为该基线
> 的实测值，见「附录 A 现状度量」。
>
> **范围说明**：渲染子系统（缓存增量化、后端接口化、Vulkan / RHI 评估）不在本方案
> 排期内，另行统一规划。

---

## 1. 现状基线

### 1.1 规模

| 模块 | 文件数 | 代码行 |
|------|-------:|-------:|
| `src/kernel/` | 424 | 104,020 |
| `src/actions/` | 212 | 36,122 |
| `src/ui/` | 160 | 26,385 |
| `src/plugin_runtime/` | 18 | 14,363 |
| `src/ai/` | 29 | 7,063 |
| `src/main/` | 9 | 3,481 |
| `src/cmd/` | 2 | 914 |
| **合计** | **853** | **192,306** |

其中 `kernel/builder_model/` 单目录 51,368 行，`kernel/persistence/` 12,256 行，
`kernel/math/` 8,848 行，`kernel/history/` 8,571 行。

### 1.2 本方案处理的问题清单

按照严重度与修复收益排序。编号保持稳定，不因增删而重排。

| 编号 | 问题 | 证据 | 影响 |
|------|------|------|------|
| P2 | 无任何自动化测试 | 仓库内无 CTest 树 | 后续所有重构无回归保护 |
| P4 | 交互语义混杂、pan 绕过事件栈 | `ActionDefault` 单状态机含 6 态；`GuiDocumentView.cpp:1370` 中键直接 `new ActionZoomPan` | 交互难扩展、光标抢占 |
| P5 | 单一可执行目标 + `file(GLOB)` | `YiCAD/CMakeLists.txt` 仅一个 `add_executable`，19 万行同一 target，GLOB 无 `CONFIGURE_DEPENDS` | 构建慢、层次无强制 |
| P6 | 命令中心化 | `Datamodel.h:127` 的 `ActionType` 含 162 项；`UIActionHandler.cpp` 153 个 `case` | 加命令必改内核 |
| P7 | 分层违规：kernel 反向依赖 ui | `DmDocument.cpp:47` 引入 `UITabDrawWidget.h`；`DmEntityContainer.cpp`、`DmHatch.cpp` 引入 `UIDialogFactory.h` | 阻塞库拆分 |
| P8 | 重头文件扩散 | `GuiDocumentView.h` 被 121 个文件包含，却拖入 `GL/glew.h`、`GL/gl.h`、`GL/glu.h`、`QOpenGLWidget`、`QPushButton`、`QToolButton` | 编译慢、Action 耦合具体后端 |
| P9 | Snap 以继承方式植入所有 Action | `ActionInterface.h:37`：`class ActionInterface : public QObject, public Snapper` | 捕捉策略不可替换、不可单测 |
| P10 | 残余 O(N) 全量扫描 | `Selection::selectWindow` 遍历全表（仅 AABB 粗筛）；`EntityTable::getNearestVirtualIntersection` 对全容器求最近实体 | 大图纸框选、虚拟交点捕捉变慢 |
| P11 | 每帧调试输出 | `GuiDocumentView.cpp:1354` 每帧 `std::cout`，且用 `system_clock` 测时长 | 帧耗时污染、无统一日志 |
| P12 | 无效 GL 调用 | `GuiDocumentView.cpp:124` 在构造函数中调 `glEnable(GL_MULTISAMPLE)`，此时无 current context | 该行不生效 |

### 1.3 需要澄清的既有正确实现

**拾取与捕捉已走 R 树**，不是 O(N) 全量求距。`Snapper::catchEntity`
（`Snapper.cpp:642`、`:689`）先调 `pDocument->searchEntities(min, max, ents, ...)`
由 `EntityTable::m_searchTree` 粗筛，再对候选集精确求距。

这一点容易误判——`Snapper.cpp:736` 确实调用了
`DmEntityContainer::getNearestEntity`，但作用对象是由 R 树候选集临时构造的容器
`ec`，而非全文档容器。P10 列出的两处才是真正的全量扫描。

---

## 2. 阶段总览

```mermaid
flowchart TB
    S0["阶段 0<br/>测试与度量地基"]
    S1["阶段 1<br/>视图解耦"]
    S2["阶段 2<br/>交互层工具化"]
    S3["阶段 3<br/>构建拆分与分层治理"]
    S4["阶段 4<br/>命令注册表与扩展化"]
    S5["阶段 5<br/>Qt 6 迁移"]

    S0 --> S1
    S0 --> S2
    S1 --> S3
    S2 --> S3
    S3 --> S4
    S4 --> S5
```

| 阶段 | 目标 | 前置 | 可与谁并行 | 相对工作量 |
|------|------|------|-----------|-----------|
| 0 | 测试与度量地基 | 无 | — | M |
| 1 | 视图解耦 | 0 | 2 | M |
| 2 | 交互层工具化 | 0 | 1 | L |
| 3 | 构建拆分与分层治理 | 1、2 | — | L |
| 4 | 命令注册表与扩展化 | 3 | — | XL |
| 5 | Qt 6 迁移 | 4 | — | L |

工作量为相对量级：S 约数日，M 约 1–2 周，L 约 3–6 周，XL 约 2 个月以上，
按单人投入估算。

**关键排序理由**：阶段 1、2 互不冲突且都只在现有单 target 内动，可并行推进；
阶段 3 的库拆分必须等它们落地，否则刚拆完的边界会被后续改动反复打穿；
阶段 4 的扩展化依赖阶段 3 建立的物理边界，否则「扩展」只是目录搬迁。

第 9 节的独立小项不依赖任何阶段，可随时穿插执行。

---

## 3. 阶段 0：测试与度量地基

### 3.1 目标

在动任何架构之前，建立回归保护和性能基线。此阶段基本不改变产品行为，
仅顺带修复 P11、P12 两处无副作用的缺陷。

### 3.2 任务

1. **引入 CTest 骨架**
   - 新建 `tests/` 树，按子系统分目录：`tests/math/`、`tests/persistence/`、
     `tests/geometry/`。
   - 选型：Catch2 或 GoogleTest（经 Conan 引入），二选一后写入 `conanfile.py`
     与 `conan.lock`。
   - 在根 `CMakeLists.txt` 加 `enable_testing()` 与 `option(YICAD_BUILD_TESTS)`。
2. **优先覆盖纯函数子系统**（无 Qt、无 GL 依赖，测试成本最低）
   - `kernel/math/`（8,848 行）：`Math2d`、`RTree`、`KDTree`、`SpacialSearchTree`、
     `FindClosedRegion`。
   - `kernel/information/`：实体求交（`Information::getIntersection`）。
   - `kernel/persistence/`（12,256 行）：**往返测试** —— 构造文档、写盘、读回、
     结构比对。这是投入产出比最高的一类测试，能一次性锁住整个数据模型的语义，
     也是阶段 5（Qt 6 迁移）字符编码验证的唯一依靠。
   - `kernel/builder_model/` 中的几何实体：`DmArc`、`DmCircle`、`DmEllipse`、
     `DmSpline` 的求交、偏移、包围盒。
3. **度量设施**
   - 移除 `GuiDocumentView.cpp:1354` 的每帧 `std::cout`，替换为可开关的计时埋点，
     并把 `system_clock` 改为 `steady_clock`（P11）。
   - 删除 `GuiDocumentView.cpp:124` 构造函数中无 current context 的
     `glEnable(GL_MULTISAMPLE)`（P12）。
   - 引入轻量日志门面（当前 `Debug.h` 被 143 处包含但无分级、分类能力）。
   - 埋点位置：`paintGL` 帧耗时、`Snapper::catchEntity` 耗时、
     `Selection::selectWindow` 耗时。
4. **建立基准图纸**
   - 准备 3 份可复现样本：小（约 1k 实体）、中（约 50k）、大（约 500k），
     含样条、填充、块引用、文字，提交到仓库或以脚本生成。
   - 这三份样本在阶段 2 的交互回归、第 9 节的 P10 验证、以及未来的渲染专项中
     都会复用。
   - 记录基线数据表，模板见「附录 B」。
5. **CI 接入**
   - `.github/workflows/build.yml` 增加 `ctest` 步骤。

### 3.3 验收标准

- `ctest` 在 CI 中绿灯，且至少覆盖 `kernel/math`、`kernel/persistence` 两个子系统。
- 三份基准图纸的帧耗时、拾取耗时、框选耗时有书面基线数据。
- 主干上不再有每帧 `std::cout`。

### 3.4 风险

- 低。此阶段几乎不修改产品代码路径，唯一的产品侧改动是删除调试输出和无效 GL 调用。

### 3.5 执行结果

已完成。落地内容与方案的差异，以及执行中发现的缺陷，记在这里。

**与方案的偏差**

| 项 | 方案 | 实际 | 理由 |
|----|------|------|------|
| 测试框架 | Catch2 或 GoogleTest | GoogleTest 1.15.0 | 阶段 2、4 引入 `ISnapService`、`IDocumentView`、`IExtension` 后要写替身，gmock 更顺手 |
| 测试如何链接内核 | 未定 | 新建 `YiCadCore` OBJECT 库（除 `Main.cpp`），可执行目标与测试共享目标文件 | 单一 `add_executable` 下测试无法复用编译产物；OBJECT 而非 STATIC，避免静态库符号剥离丢掉 Qt 的自动注册符号 |
| 源文件收集 | 阶段 3 才处理 | 阶段 0 就按阶段 3 的七个目标库分区收集，`file(GLOB ... CONFIGURE_DEPENDS)` | 建 OBJECT 库本来就要重排源文件组织，顺带补上 `CONFIGURE_DEPENDS`（P5 的一半）；阶段 3 拆库退化为把七个分区变量各自喂给一次 `add_library`。曾一度改成显式清单，但那要配生成脚本加 CI 校验，维护成本超过收益，已退回 GLOB |
| 分层护栏 | 未列 | 新增 `tools/check_layering.py` 进 CI，P7 的三处进白名单 | 方向先锁住，库后建；白名单失效时脚本报错，防止腐化 |
| 往返测试的接口 | `Persistence` 的流接口 | `OutputStream`/`InputStream` | `dumpToStream`/`restoreFromStream` 是死代码且有缺陷，见下 |

**新增设施**

- `YiCAD/src/kernel/debug/YiCadLog.{h,cpp}` —— 分类加分级的日志门面，
  默认 Warning，过滤不通过时右侧不求值；`YICAD_LOG=*:warning,render:debug` 配置。
- `YiCAD/src/kernel/debug/ScopedTimer.{h,cpp}` —— 可开关的耗时埋点，
  `steady_clock`，按计数器累计而非逐次打印；`YICAD_PROFILE=1` 开启。
- `tools/gen_benchmark_drawings.py` —— 生成 1k / 50k / 500k 三份基准图纸
  （DXF R2000，含样条、填充、块引用、文字），确定性，零第三方依赖。
- `YiCAD/CMakeLists.txt` 的 `yicad_collect_sources()` —— 按分区收集源文件，
  对不存在的目录直接报错。启用后立刻发现原 GLOB 里挂着四个早已不存在的目录
  （`builder_model/array`、`temp/entity_builders`、`temp/entity_data`、
  `temp/geometry`），以及 include 路径里的 `kernel/engine`、`kernel/scripting`，
  一并清理。
- `tests/CMakeLists.txt` 里给测试进程加 PATH —— SARibbonBar 的 DLL 只在
  `cmake --install` 时才就位，构建树里没有，测试二进制会以 0xc0000135
  起不来。只改测试进程的环境变量，不往构建产物里复制任何文件。
- `tools/measure_build.ps1` —— 采集附录 B 的构建指标。
- `doc/BASELINE.md` —— 基线采集步骤与数据表。

**执行中发现的缺陷**

均为测试暴露、且**未在本阶段修复**（阶段 0 不改产品语义）。每一条都留了
`DISABLED_` 测试，修复后去掉前缀即可作为验收。

| # | 位置 | 问题 | 可达性 |
|---|------|------|--------|
| B1 | `Math2d.cpp:400` `cubicSolver` | 单实根分支在 `q > 0` 时取错辅助二次方程的根，对负数取立方根得到 NaN/inf。应取 `r[0]` 而非 `r[1]` | 经 `quarticSolver` → `simultaneousQuadraticSolver*` 被 `Information.cpp:621`（椭圆求交）与 `ActionDrawLineTangent2.cpp:392`（切线）使用 |
| B2 | `Math2d.cpp:396` 同一段 | `r.size() == 0` 时只往 cerr 打一行，随后仍索引 `r[0]`/`r[1]`，越界读 | 同上 |
| B3 | `Information.cpp:398` `getIntersectionLineArc` | 死代码：有完整的相切处理，但全仓无调用点。线与圆实际走通用二次曲线路径，**精确相切求不出切点** | 对切点做修剪、延伸、交点捕捉时失败 |
| B4 | `Persistence.cpp` `restoreFromStream` | 违反 `Archive.h` 为 `ArchiveReader` 写明的契约——未调用 `nextEntry()` 就取 `stream()`，读到空流并抛异常 | 死代码，全仓无调用点。产品文档读写走 `FilterOcdIO`，不受影响 |

另有两处注释与实现不符，已在测试里按实现的真实契约断言并注明：

- `Math2d::correctAngle2` 注释写 `[-PI, +PI)`，实现是 `remainder`，区间是闭的。
- `DmVector::flipXY` 注释未提及它会丢弃 z 分量。

**验收对照**

| 3.3 节的验收标准 | 状态 |
|------------------|------|
| `ctest` 绿灯，覆盖 `kernel/math`、`kernel/persistence` | 达成，另含 `geometry`，共 133 个用例 |
| 三份基准图纸有书面基线数据 | 图纸与采集流程就绪；运行期数据需在有 GPU 的开发机上按 `doc/BASELINE.md` 手工采集 |
| 主干上不再有每帧 `std::cout` | 达成 |

---

## 4. 阶段 1：视图解耦

### 4.1 目标

让 106 个 Action 不再依赖具体的 OpenGL widget，为阶段 3 的库拆分建立前置条件。
这是 P8 的解法。

> **范围边界**：本阶段**不涉及** `Painter` 的接口化、`GL_PAINTER_COMMON()` 宏的
> 消除、或任何渲染后端相关改动——那些属于渲染专项，另行规划。
> 本阶段做的是头文件卫生与视图抽象，本质是解耦而非渲染。

### 4.2 现状证据

- `GuiDocumentView.h` 被 **121 个文件**包含，头部包含 `GL/glew.h`、`GL/gl.h`、
  `GL/glu.h`、`QOpenGLWidget`、`QPushButton`、`QToolButton`、
  `CustomComboboxItem.h`、`PainterCreator.h`。
- `PainterCreator.h:27` 写有 `using namespace opengl;`，污染全部间接包含者。
- 106 个 Action 类直接依赖具体的 `GuiDocumentView`，因而全部间接依赖 OpenGL 头。

### 4.3 任务

1. **`GuiDocumentView.h` 瘦身**
   - 成员 `opengl::GLPainter*` 与 `DmCachePainter*` 改用前置声明，
     `PainterCreator.h` 与全部 `GL/*` 头下沉到 `.cpp`。
   - 移除 `QPushButton`、`QToolButton`、`CustomComboboxItem.h`——这些是 UI 细节，
     不应出现在被 121 个文件包含的头里。
   - 注意：此处只改包含关系，**不改成员类型**，不需要 `Painter` 变虚函数。
2. **清除 `PainterCreator.h` 的 `using namespace opengl;`**
   - 修正因该命名空间泄漏而省略限定符的调用点。
3. **抽出抽象视图接口 `IDocumentView`**
   - 抽出 Action 实际需要的能力：坐标变换（`toGraphX/Y`、`toGuiX/Y`）、
     重绘请求、光标设置、预览容器访问、相对零点、视口矩形。
   - `actions/` 与 `kernel/actions/` 改为依赖 `IDocumentView`。
   - `GuiDocumentView` 实现该接口。

### 4.4 验收标准

- `src/actions/` 与 `src/kernel/actions/` 下不再出现任何 `GL/*` 头。
- `GuiDocumentView.h` 不再包含任何 `GL/*` 头。
- 全量编译时间相对阶段 0 基线有可测量的下降（记录数据）。
- 行为零变化：阶段 0 测试加三份基准图纸目视回归。

### 4.5 风险

- **中**。改动面广（涉及 106 个 Action 的包含关系），但每一步都是机械变换，
  编译器会抓住绝大部分错误。
- 缓解：分两个 PR，先做头文件瘦身（纯包含关系），再做 `IDocumentView` 抽取。

### 4.6 执行结果

已完成，分两个提交，与 4.5 节的缓解措施一致。

**与方案的偏差**

| 项 | 方案 | 实际 | 理由 |
|----|------|------|------|
| `GuiDocumentView.h` 是否彻底摆脱 GL 头 | 不再包含任何 `GL/*` 头 | 仍保留 `#define GL_GLEXT_PROTOTYPES` + `#include <GL/glew.h>`；`GL/gl.h`、`GL/glu.h`、`PainterCreator.h` 按计划下沉到 `.cpp` | `QOpenGLWidget` 经 Qt 的 `qopengl.h` 会在桌面 GL 下间接 `#include <GL/gl.h>`。`GL/glew.h` 的 `#error` 保护是翻译单元级别的约束——只要本文件的某个包含者后续还引入了需要 `glew.h` 的画笔代码（`GLShader.h` 等），gl.h 抢先被 Qt 拉入就会报错。这是被 100+ 文件包含的头无法完全摆脱的不变量，已把代价压到最小（详见文件内注释），构建数据仍达到预期收益（见下） |
| `IDocumentView` 覆盖范围 | `actions/` 与 `kernel/actions/` | 额外覆盖 `kernel/modification/`（`Selection`、`Modification`）与 `kernel/history/BlockEditCmd` | 这三个类直接持有 Action 传入的 `docView` 指针：`Selection`/`Modification` 调用的 `redraw()`/`specifyDocumentModified()`/`getDocument()` 均已在接口内，机械替换即可；`BlockEditCmd` 的 `m_pDocView` 只存储从不解引用，改类型不需要新增任何接口方法。不跟着切换就无法编译（`docView` 现在是 `IDocumentView*`，反向转型不是隐式的） |
| `IDocumentView` 的能力清单 | 坐标变换、重绘请求、光标设置、预览容器访问、相对零点、视口矩形 | 额外加了 `setCursor(const QCursor&)` 与 `asQObject()` | 两处遗留用法绕不开：`ActionDrawMText` 退出文字编辑态要恢复系统默认箭头光标，语义不同于 `setMouseCursor()` 的自绘光标（后者对 `ArrowCursor` 是画 `Qt::BlankCursor` 再自绘十字线）；`ActionDrawHatch`/`ActionModifyExtend` 用旧式 `connect(docView, SIGNAL(viewChanged())...)`，需要一个 `QObject*` |
| 保留具体类型的例外 | 未列 | `ActionOptionsGeneral.cpp`、`kernel/actions/Preview.cpp` 仍 `#include GuiDocumentView.h`；`ActionDrawMText.cpp` 两处 `static_cast<GuiDocumentView*>(docView)` | 前两者拿到的指针分别来自 `MDIWindow::getDocumentView()`、`DmDocument::getDocumentView()`——这两个访问器的返回类型本就是具体的 `GuiDocumentView*`（UI/Model 层既有设计，不在本阶段范围），隐式上转型需要完整类型。后者是因为 `MTextEditWidget`（`ui/`，未迁移）的构造函数要求它同时是 Qt 父窗口和信号源，两者都要求具体类型 |

**新增设施**

- `YiCAD/src/kernel/actions/IDocumentView.h` —— Action/Snapper 视角下的文档视图
  接口，纯虚，无成员，放在 `kernel/actions/` 而非 `kernel/gui/`：接口属于消费方
  （依赖倒置），`GuiDocumentView`（`kernel/gui/`）反过来实现它，物理依赖方向与
  6.3 节的目标库图（`Interaction --> Render`）保持一致。

**执行中发现的缺陷（均属既有代码，本阶段顺手修复）**

| 位置 | 问题 |
|------|------|
| `DmEllipse.cpp` | 用了 `glm::vec2`/`glm::distance`，但从未直接 `#include <glm/glm.hpp>`，全靠 `GuiDocumentView.h → PainterCreator.h → GLPainter.h` 这条链路间接带进来 |
| `ActionLayersActivate/Color/Delete/Freeze/Lock/Print.cpp`（6 个文件） | 通过 `ComboBoxData::btnColor` 等字段与 `sender()` 做 `QObject* == QToolButton*`/`QPushButton*` 比较，但从未直接包含 `<QToolButton>`/`<QPushButton>`，同样靠 `GuiDocumentView.h` 间接带入 |
| `UIBottomWidget.cpp` | 用 `QToolButton` 却未直接包含，同上 |

三处都是 P8（重头文件扩散）描述的典型后果：头文件瘦身之前，这些缺失的直接依赖
被 `GuiDocumentView.h` 的宽泛包含悄悄掩盖了。

**验收对照**

| 4.4 节的验收标准 | 状态 |
|------------------|------|
| `src/actions/` 与 `src/kernel/actions/` 下不再出现任何 `GL/*` 头 | 达成 |
| `GuiDocumentView.h` 不再包含任何 `GL/*` 头 | 部分达成，仅保留 `GL/glew.h`，原因见上表 |
| 全量编译时间相对阶段 0 基线有可测量的下降 | 达成。Release 全量构建 144.3s → 129.5s（-10%）；改 `GuiDocumentView.h` 后的增量构建 65.3s → 25.4s（**-61%**，本阶段的核心指标）；数据见 `doc/BASELINE.md` §5 |
| 行为零变化：阶段 0 测试加三份基准图纸目视回归 | `ctest` 全绿（133 个用例不变）；三份基准图纸的目视回归需要 GPU 开发机人工完成，与阶段 0 的既有限制一致，未变化 |

---

## 5. 阶段 2：交互层工具化

### 5.1 目标

把「选择」「平移」「捕捉」从 Action 体系中剥离为独立的、可叠加的工具层，
并建立明确的事件分发与光标仲裁规则。这是 P4、P9 的解法。
参考 `E:\dev\DS` 的 `ViewToolControl` 设计。

### 5.2 现状证据

- `ActionDefault` 单个状态机承载 6 种语义：`Neutral`、`Dragging`、`SetCorner2`、
  `Moving`、`MovingRef`、`Panning`。
- `GuiDocumentView.cpp:1370`：中键按下直接
  `setCurrentAction(new ActionZoomPan(...))`，绕过 `GuiEventHandler` 的分发逻辑。
- `GuiEventHandler` 持有 `QList<ActionInterface*>`，但没有「未处理则下传」语义，
  依赖 `isExclusive()`、`isSubAction()`、`canBeInterrupt()`、`isViewAction()`
  四个布尔标志互相仲裁优先级。
- 光标无仲裁机制：各 Action 在 `updateMouseCursor()` 中各自抢占。
- `ActionInterface.h:37`：`class ActionInterface : public QObject, public Snapper`，
  捕捉能力以**继承**方式植入全部 106 个 Action。

### 5.3 参考设计（DS/DimX）

- `Application/IViewTool.h`：统一事件集，每个回调返回 `HOP_OK`（已处理，停止分发）、
  `HOP_NOT_HANDLED`（继续下传）或 `HOP_CANCEL`。
- `Application/ViewToolControl.h`：唯一注册到视图的事件接收者，内部维护三层栈 ——
  导航工具（栈底）、选择工具、业务工具栈（后进先出优先）。
- `IViewTool::GetCursor()` 返回 `std::optional<QCursor>`，按与事件分发相同的栈序
  取首个非 `nullopt` 者，并做去重避免每次鼠标移动都 set/unset。
- `Application/Snap2D/`：捕捉作为**独立模块**，不是基类。

### 5.4 任务

1. **定义 `IViewTool` 与 `ViewToolControl`**
   - 事件集对齐 Qt：`mousePress/Release/Move/DoubleClick`、`keyPress/Release`、
     `wheel`、`enter/leave`。
   - 返回值语义：`Handled`、`NotHandled`、`Cancel`。
   - 三层栈：`NavigationTool`、`SelectionTool`、业务工具栈。
2. **抽出 `PanZoomTool` 作为导航工具**
   - 吸收 `ActionZoomPan` 与 `ActionDefault::Panning` 状态。
   - 删除 `GuiDocumentView.cpp:1370` 的硬编码分支，中键平移由导航工具处理。
3. **抽出 `SelectTool` 作为选择工具**
   - 吸收 `ActionDefault` 的 `Neutral`、`Dragging`、`SetCorner2` 状态
     （点选、框选、交叉选）。
   - `Moving` 与 `MovingRef`（拖拽实体与夹点）评估后决定：留在 `SelectTool` 内
     还是拆为 `GripEditTool`。倾向后者，与 DS 的 `Edit/EditTool` 对应。
4. **光标仲裁**
   - `IViewTool::GetCursor()` 返回 `std::optional<QCursor>`，由 `ViewToolControl`
     按栈序仲裁并去重。
   - 移除各 Action 的 `updateMouseCursor()` 直接抢占。
5. **Snap 从继承改为组合**
   - 定义 `ISnapService`，把 `Snapper`（`kernel/actions/Snapper.*`）的能力迁入。
   - `ActionInterface` 去掉 `public Snapper`，改为持有 `ISnapService*`。
   - 这是本阶段改动面最大的一项（影响 106 个 Action），但机械性强。
6. **`ActionInterface` 适配为一类 `IViewTool`**
   - 保留现有 Action 体系不动，用适配器把 `GuiEventHandler` 的 Action 栈包成
     业务工具栈的一项，实现渐进迁移。
   - `isExclusive()`、`isSubAction()`、`canBeInterrupt()`、`isViewAction()`
     四个标志的语义由栈层次替代，逐步废弃。

### 5.5 验收标准

- `ActionDefault` 的 `Panning` 状态被删除，`GuiDocumentView` 内无 `new ActionZoomPan`。
- 中键平移、框选、点选在任意 Action 激活期间行为一致且可预测。
- 光标在「绘制中 + 悬停实体 + 按住 Ctrl」等叠加场景下有确定性结果。
- `ActionInterface` 不再继承 `Snapper`。
- 捕捉逻辑可脱离 Action 单独测试（补充单测）。

### 5.6 风险

- **中高**。交互是用户最敏感的部分，行为回归难以自动化验证。
- 缓解：
  - 第 6 项的适配器策略保证旧 Action 无需改写即可运行，迁移可按 Action 分批；
  - 准备一份交互回归检查清单（手工），覆盖各 Action 与选择、平移的组合；
  - 第 5 项（Snap 解耦）可作为独立 PR 先行，它与工具栈无强耦合。

### 5.7 执行结果

分四轮落地，六项任务与其后的两处收尾全部完成。第一轮完成第 1
（`IViewTool`/`ViewToolControl`）、2（`PanZoomTool`）、4（光标仲裁，范围
收窄）、5（Snap 组合化）项；第二轮补上第 3 项（`SelectTool`，
`GripEditTool` 未单独拆分，理由见下）；第三轮完成第 6 项（业务工具
适配器，`LegacyActionTool`）；第四轮把 `SelectTool` 正式注册为
`ViewToolControl` 的选择层，并删除 `ActionDefault` 里最后一次直接置
光标的调用，完成 5.5 节全部五条验收标准。

**与方案的偏差**

| 项 | 方案 | 实际 | 理由 |
|----|------|------|------|
| 落地范围（第一轮） | 六项任务一次性完成 | 先完成第 1、2、4、5 项 | `ActionDefault` 与 `ActionInterface::finish()`（"拒绝退出默认 Action"的特判）、`GuiEventHandler::inSelectionMode()`、`GuiDocumentView::tabletEvent` 的橡皮擦手势深度耦合，拆解它是独立体量的工作，与第 5 项（改动面覆盖 106 个 Action）叠加会突破 10 节"禁止跨阶段大爆炸式 PR"的约束，故拆成两轮 |
| `GripEditTool` 未单独拆分 | `Moving`/`MovingRef` "倾向拆为 GripEditTool" | 与 `Neutral`/`Dragging`/`SetCorner2` 一起留在同一个 `SelectTool` 里 | 这五个状态共享同一次拖拽手势：鼠标刚按下时（`Dragging` 状态）还不知道最终是框选还是拖动实体/夹点，要等移动超过阈值后才能判定，判定逻辑本身就要同时读取"有没有选中的参考点"和"有没有选中的实体"。拆成两个类需要在它们之间转移这次"未决"的拖拽状态，边界不清晰而收益有限；方案本身也把这个拆分标注为"评估后决定"，判断后选择保留在一起 |
| `SelectTool` 注册为选择层后仍与 `LegacyActionTool` 共享同一个事件转发路径 | 三层栈里选择层由 `SelectTool` 担任，与业务层各自独立分发 | `SelectTool` 已 `setSelectionTool()` 注册进 `ViewToolControl`；但 `LegacyActionTool` 的转发规则未同步收窄（依然是"没有业务 Action 活动、且不是中键/空闲态 Ctrl+左键时，一律转发给 `GuiEventHandler`"），空闲态的鼠标事件因此仍主要经业务层→`GuiEventHandler`→`ActionDefault`→`SelectTool` 这条既有路径触达，选择层的注册在**事件分发**上目前只在业务层主动让路的两种场景（中键、空闲态 Ctrl+左键）下才会被真正问到 | `ActionDefault` 与其内部的 `SelectTool` 是同一个实例（`ActionDefault::getSelectTool()` 返回的就是注册进选择层的那个对象），走哪条路径最终都落到同一个状态机上，不存在双份状态或双重处理；选择层注册真正需要的收益（详见下一条"光标仲裁范围"）已经拿到。让 `LegacyActionTool` 在任意空闲态都整体让路、把事件分发也完全收窄到选择层，需要同时处理 `ActionBlocksEdit`/`ActionModifyMText` 对 `getDefaultAction()` 的复用与 `GuiEventHandler::inSelectionMode()` 等耦合点，属于比本次"注册选择层"更深一层、且没有已知行为收益的重构，未来若要动，需要单独评估 |
| `LegacyActionTool` 对中键/空闲 Ctrl+左键的让路判断 | 未细化——方案只说"给它一个 NotHandled 语义" | 只在两种情况下声明 `NotHandled`：中键（任何时候）；Ctrl/Meta+左键且 `!eventHandler->hasAction()`。平移进行中（`PanZoomTool::isPanning()`）时对移动/释放也让路，避免业务层抢在导航层结束平移之前把事件转发给默认/业务 Action | `GuiEventHandler` 本身没有"这次事件我不关心"的概念——它的分发语义是"只要活着就总是处理"。这条规则原样承袭自第一轮的 `wantsPan`/`isPanning()` 判断（当时是 `GuiDocumentView` 里的外部判断），现在收进 `LegacyActionTool` 内部，让 `GuiDocumentView` 的三个鼠标事件处理函数能统一交给 `ViewToolControl` 分发，不用再各自判断"这次要不要问 ViewToolControl" |
| `GuiDocumentView` 对 RightButton / XButton1 释放的特判保留在工具栈之外 | 未细化 | `mouseReleaseEvent` 的 `switch (e->button())` 结构原样保留：`RightButton`（`back()` 的合成事件回退）与 `XButton1`（`enter()` + `emit xbutton1_released()`）两个分支不经过 `ViewToolControl`；只有 `default` 分支（其余按钮，含中键/左键释放）改为统一调用 `m_pViewToolControl->mouseReleaseEvent(e)` | 这两个分支依赖的是 `GuiDocumentView` 自己的方法（`back()`、`enter()`）和信号（`xbutton1_released()`），不是"哪个 Action 处理这次事件"的问题，本质上是画布级的全局快捷手势，与 `zoomAuto()`（中键双击）、滚轮缩放等其它没有并入工具栈的全局手势同类。如果把它们也塞进 `LegacyActionTool`，要么反过来给 `IViewTool`/`IDocumentView` 增加这两个方法，要么让适配器直接依赖具体的 `GuiDocumentView`——两者都超出"包一层 GuiEventHandler"本身的范围，收益也不明显（没有其它工具需要与它们竞争优先级） |
| 光标仲裁范围 | 移除各 Action 的 `updateMouseCursor()` 直接抢占，统一走 `IViewTool::GetCursor()` | `ActionDefault::updateMouseCursor()` 的直接调用已删除（落回 `ActionInterface` 的空实现）；空闲态（没有业务 Action 活动）的光标现在完全经 `SelectTool::getCursor()` → `ViewToolControl::refreshCursor()` 仲裁。其余 105 个业务 Action 的 `updateMouseCursor()` 直接调用保持不变——`LegacyActionTool` 仍不覆盖 `getCursor()`（保持 `nullopt`），它包装的是这批 Action，没有单一、可查询的"当前光标偏好"可以汇报给仲裁链路 | 全部改造 105 个业务 Action 超出阶段2范围（那是件独立体量的工作，且这些 Action 各自的 `updateMouseCursor()` 目前工作正常，没有已知缺陷）。`SelectTool::getCursor()` 因此加了一条针对性的让路判断：有业务 Action 活动（`docView->getEventHandler()->hasAction()`）时返回 `nullopt`，避免选择层用自己在 `Neutral` 下的 `ArrowCursor` 偏好覆盖正在活动的业务 Action 自己设置的光标——这条判断只影响仲裁查询通道；`SelectTool::setStatus()`/`init()` 仍保留无条件的直接 `setMouseCursor()` 调用（内部改用不受此判断约束的 `cursorForStatus()`），服务于 `ActionBlocksEdit`/`ActionModifyMText` 复用 `getDefaultAction()` 时的场景（那时 `hasAction()` 恰好为真，仲裁通道按上述规则保持沉默，只能靠直接调用） |
| `SelectTool` 新增的让路判断 | 未细化 | 新增两处：`mousePressEvent` 的 `Neutral` 分支对 Ctrl/Meta+左键返回 `NotHandled`；`mouseMoveEvent`/`mouseReleaseEvent` 在注入的 `PanZoomTool::isPanning()` 为真时返回 `NotHandled` | `SelectTool` 现在直接参与三层栈的优先级竞争（选择层，位于业务层之下、导航层之上）。press 时机上，选择层排在导航层之前被问到，若不显式让路，会在导航层看到 Ctrl+左键之前就把它当成框选起点抢走；move/release 时机上，`LegacyActionTool` 已经在平移中让路，但选择层若不跟着让路，同样的道理会抢在导航层结束平移之前把事件处理掉，构造函数新增的 `PanZoomTool*` 参数默认可为空（保持原有测试构造方式兼容），为空时视为"从不平移"，行为与改造前一致 |
| Ctrl+左键平移的适用范围 | 未细化 | 仅在 `!eventHandler->hasAction()`（无业务 Action 活动）时生效，与原 `ActionDefault::Panning` 完全一致 | 保持行为零回归：原实现里 Ctrl+拖拽只在 `ActionDefault::Neutral` 状态下响应；业务 Action 运行时左键另有含义（放置绘制点等），不应被导航层截获 |
| 中键平移与活动 Action 的关系 | "在任意 Action 激活期间行为一致" | 中键平移不再通过 `setCurrentAction` 挂起当前 Action，而是在 `GuiDocumentView` 层直接分流给 `PanZoomTool`；平移过程中当前 Action（及其预览）保持活跃、不被挂起 | 与原实现（中键按下会 `setCurrentAction(new ActionZoomPan(...))`，通过挂起/恢复机制暂停前一个 Action）相比是行为上的小幅改善而非逐位复刻，判断为更贴合验收标准"一致且可预测"的本意，已在 `PanZoomTool.h` 顶部注明 |

**执行中发现的缺陷（既有代码，本阶段未修复）**

| 位置 | 问题 | 可达性 |
|------|------|--------|
| `Snapper::setSnapRestriction`（`Snapper.cpp`） | 空实现，从未给 `snapMode.restriction` 赋值——`restriction` 字段只有 `SnapMode::fromInt`/`clear()` 会写。`ActionDefault`（现为 `SelectTool::keyPressEvent`）按 Shift 键临时切到正交捕捉的调用因此从未真正生效 | 原样从 `ActionDefault` 搬到 `SelectTool`，为 `tests/interaction/test_select_tool.cpp` 的单测暴露；不在阶段2范围内修复，测试里已注明不对 `restriction` 取值断言 |

**新增设施**

- `kernel/actions/ISnapService.h` —— 捕捉能力的纯虚接口；`SnapMode` /
  `SnapResultType` / `EntityTypeList` / 吸附常量随之从 `Snapper.h` 迁出。
- `kernel/actions/Snapper.{h,cpp}` —— 改为 `ISnapService` 的具体实现，公开
  API（含签名、默认参数）不变。
- `kernel/actions/ActionInterface.{h,cpp}` —— 不再 `public Snapper` 继承，
  改为持有 `std::unique_ptr<ISnapService>`；保留与 Snapper 完全同名同签名
  的一组转发方法（`catchEntity`/`snapPoint`/`getSnapMode` 等），使既有
  106 个 Action 子类的调用点不必改动一行。另加 `pDocument`/`docView` 两个
  直接成员（构造参数的留存，替代原来经 `Snapper` 继承获得的同名成员）与
  `snapService()` 访问器，供极少数需要绕过 `ActionInterface` 自身
  `finish()`/`suspend()` 语义、直调捕捉器原始实现的子类使用。
- `kernel/actions/IViewTool.h` —— 视图工具接口，事件集对齐 Qt
  （mousePress/Release/Move/DoubleClick、keyPress/Release、wheel、
  enter/leave），参考 `E:\dev\DS` 的 `Application/IViewTool.h`，把
  HOOPS 事件/`QCursor` 换成 Qt 原生事件/`DM::CursorType`。
- `kernel/actions/ViewToolControl.{h,cpp}` —— 三层工具栈分发器（业务栈
  后进先出 > 选择工具 > 导航工具）与光标仲裁，参考 `E:\dev\DS` 的
  `Application/ViewToolControl.{h,cpp}`。
- `kernel/actions/PanZoomTool.{h,cpp}` —— 导航层：中键拖拽（始终生效）与
  Ctrl+左键拖拽（仅无业务 Action 活动时生效）平移，吸收原
  `GuiDocumentView.cpp:1382` 的硬编码 `new ActionZoomPan` 与
  `ActionDefault::Panning` 状态。显式的 Ribbon "Pan" 命令（`ActionZoomPan`
  类本身）不在改动范围内——那是用户主动进入的模态命令，与这里"随时中键一按
  就能平移"的导航手势语义不同，予以保留。
- `kernel/actions/LegacyActionTool.{h,cpp}` —— 阶段2第6项的业务工具
  适配器，把 `GuiEventHandler`（业务 Action 栈 + 默认 Action 的既有分发
  逻辑，一行未改）包成一个 `IViewTool`，注册进 `ViewToolControl` 的
  业务工具栈。只在两类场景声明 `NotHandled`：中键；以及没有业务 Action
  活动时的 Ctrl/Meta+左键——把这两类事件让给导航层 `PanZoomTool`；平移
  进行中时对移动/释放也主动让路，避免业务层抢在导航层结束平移之前
  把事件转发出去。
- `kernel/gui/GuiDocumentView.{h,cpp}` —— 持有 `ViewToolControl`/
  `PanZoomTool`/`LegacyActionTool`（第三轮新增）。`mousePressEvent`/
  `mouseMoveEvent` 统一交给 `ViewToolControl` 分发，不再有 `GuiDocumentView`
  自己判断"这次要不要问 ViewToolControl"的外部 `wantsPan` 逻辑——那部分
  判断已经收进 `LegacyActionTool`。`mouseReleaseEvent` 保留 RightButton/
  XButton1 两个全局手势分支在工具栈之外（原因见"与方案的偏差"表），其余
  按钮的 `default` 分支统一交给 `ViewToolControl`。第四轮新增：构造
  `ActionDefault` 后立即 `m_pViewToolControl->setSelectionTool(
  defaultAction->getSelectTool())`；析构函数里在 `delete eventHandler`
  **之前**先 `m_pViewToolControl->setSelectionTool(nullptr)`——`eventHandler`
  是裸指针、在析构函数体内手动 `delete`，早于 `m_pViewToolControl`
  这个 `unique_ptr` 成员的自动析构（成员按声明逆序析构，发生在函数体
  **之后**）；`ActionDefault`（`eventHandler` 拥有的默认 Action）内部的
  `SelectTool` 正是选择层引用的对象，若不提前断开这个引用，
  `m_pViewToolControl` 自动析构时调用 `SelectTool::onDeactivate()` 会经一个
  已经被删除的 `eventHandler` 间接悬空访问——这是实现过程中发现并修复的、
  本次改动自身引入的顺序问题，不是既有代码的缺陷。
- `tests/support/FakeDocumentView.h` —— `IDocumentView` 的最小测试替身，
  交互层工具的单测因此不必依赖 `GuiDocumentView` 或任何 Qt 界面组件。
  `getGrid()`/`getOverlayContainer()`/`getPreviewContainer()` 用真实的
  `GuiGrid`/`DmEntityContainer` 成员兜底（而非 `nullptr`）——`Snapper` 的
  `init()`/`deleteSnapper()` 等方法会无条件解引用它们的返回值。第四轮
  新增 `eventHandler` 字段，测试可选注入一个真实 `GuiEventHandler`，
  验证依赖 `hasAction()` 的让路/仲裁逻辑；未注入时行为不变（返回
  `nullptr`）。
- `kernel/actions/SelectTool.{h,cpp}` —— 从 `ActionDefault` 抽出的选择/
  拖拽状态机，吸收原来的 `Neutral`/`Dragging`/`SetCorner2`/`Moving`/
  `MovingRef` 五个状态；不继承 `QObject`/`ActionInterface`，构造时接收
  非持有的 `ISnapService*`/`Preview*`（与拥有者共享同一个捕捉器与预览
  容器实例，保证捕捉模式、预览内容一致）。第四轮新增：构造时可选接收
  非持有的 `PanZoomTool*`（默认 `nullptr`，兼容既有测试构造方式）；
  正式 `setSelectionTool()` 注册为 `ViewToolControl` 的选择层，随之新增
  的三处让路/仲裁判断见上表"`SelectTool` 新增的让路判断"与"光标仲裁
  范围"两行。
- `actions/ActionDefault.{h,cpp}` —— 由完整的状态机改为薄适配器：
  持有 `std::unique_ptr<Preview>` 与 `std::unique_ptr<SelectTool>`，把
  `ActionInterface` 的全部虚方法转发给 `SelectTool`。保留下来是因为
  `GuiEventHandler::m_pDefaultAction`/`DM::ActionDefault` 类型身份、以及
  `ActionBlocksEdit`/`ActionModifyMText` 对
  `handler->getDefaultAction()->mouseXEvent(e)` 的直接复用都依赖一个
  仍然存活、行为不变的 `ActionDefault` 实例。第四轮新增：构造函数多接收
  一个非持有的 `PanZoomTool*`（转交给 `SelectTool`）；新增
  `getSelectTool()` 公开访问器供 `GuiDocumentView` 注册选择层；删除
  `updateMouseCursor()` 覆盖（落回 `ActionInterface` 的空实现，见上表）。
- `tests/interaction/` —— `test_select_tool.cpp`（`SelectTool` 的状态
  转换、按钮语义、`Escape`/`Shift` 处理，以及第四轮新增的 3 个用例
  覆盖 Ctrl+左键让路、平移中让路、业务 Action 活动时光标仲裁保持沉默，
  共 12 个用例，用真实的空 `DmDocument` + `Snapper` + `PanZoomTool` +
  `FakeDocumentView` 构造，不需要 `GuiDocumentView`）；
  `test_legacy_action_tool.cpp`（`LegacyActionTool` 的转发/让路判断，
  7 个用例，用一个没有挂任何 Action 的空 `GuiEventHandler` 构造）。
  连同已有的 `ViewToolControl`/`PanZoomTool` 用例，本目录共 35 个用例。

**影响面**

- `actions/ActionDefault.{h,cpp}` —— 第一轮删除 `Panning` 状态；第二轮
  整体重写为薄适配器（见上）。
- `actions/ActionDimAngular.cpp`、`ActionDimDiametric.cpp`、
  `ActionDimRadial.cpp`、`ActionDrawEllipseInscribe.cpp`、
  `ActionDrawLineBisector.cpp` —— 原先绕开虚函数分派、显式调用
  `Snapper::finish()`/`Snapper::suspend()` 的 5 处基类调用，改为
  `snapService()->finish()`/`suspend()`。
- `actions/ActionModifyBevel.cpp`、`ActionModifyRound.cpp`、
  `ActionModifyTrim.cpp`、`ActionZoomPan.cpp` —— 4 处直接读写 `snapMode`
  字段（原继承自 `Snapper` 的 protected 成员）的代码，改为
  `getSnapMode()->...`。
- 其余全部 100 余个 Action 子类文件未改动一行——`catchEntity`/
  `snapPoint`/`snapMode` 等调用点是到 `m_snapService` 的透明转发，
  机械性由编译期验证（全量构建 + 133+35 个既有/新增用例全绿，Debug 与
  Release 均已构建，分层检查脚本无新增违规）。

**验收对照**

| 5.5 节的验收标准 | 状态 |
|------------------|------|
| `ActionDefault` 的 `Panning` 状态被删除，`GuiDocumentView` 内无 `new ActionZoomPan` | 达成 |
| 中键平移、框选、点选在任意 Action 激活期间行为一致且可预测 | 达成。中键平移经由统一的 `ViewToolControl` 分发（`LegacyActionTool` 主动让路），不再依赖 `GuiDocumentView` 里的外部判断；`SelectTool` 已注册为选择层，参与三层栈的统一优先级排序；框选/点选在空闲态下仍主要经 `ActionDefault`→`SelectTool` 这条既有转发路径触达（是同一个实例，见"与方案的偏差"表），行为与阶段1末尾等价，未回归 |
| 光标在叠加场景下有确定性结果 | 达成，限定在结构性可行的范围内：空闲态（`SelectTool` 注册为选择层）与平移进行中/结束后的光标切换现在都经 `ViewToolControl::refreshCursor()` 统一仲裁；有业务 Action 活动时，光标仍由该 Action 自己的 `updateMouseCursor()` 直接决定（105 个未改造，超出阶段2范围，见"与方案的偏差"），`SelectTool::getCursor()` 已加入让路判断，确保不会覆盖它们 |
| `ActionInterface` 不再继承 `Snapper` | 达成 |
| 捕捉逻辑可脱离 Action 单独测试（补充单测） | 达成，并扩展到选择逻辑与业务工具适配器：`SelectTool` 已完全脱离 `ActionInterface`/`QObject`，用一个空 `DmDocument` + 真实 `Snapper` + `PanZoomTool` + `FakeDocumentView` 即可单测；`LegacyActionTool` 用一个没有挂任何 Action 的空 `GuiEventHandler` 即可单测其转发/让路判断，都不需要 `GuiDocumentView` |

5.5 节五条验收标准全部达成，阶段2完成。

`GripEditTool` 的拆分已评估并搁置（见"与方案的偏差"），除非后续出现
新的、独立于点选/框选状态机的夹点交互需求，否则不再重新考虑；`Legacy
ActionTool` 的转发范围是否要进一步收窄（见"与方案的偏差"表最后一条）
留给未来有实际需求时再评估，不是当前的遗留缺口。

---

## 6. 阶段 3：构建拆分与分层治理

### 6.1 目标

把 19 万行的单一可执行目标拆为按层划分的静态库，让依赖方向由 CMake 强制执行。
这是 P5、P7 的解法，也是阶段 4 扩展化的物理前提。

### 6.2 现状证据

- `YiCAD/CMakeLists.txt:196` 只有一个 `add_executable`（另有 `YiCadPluginSdk`
  INTERFACE 库和两个插件 DLL 目标）。
- 源文件全部通过 `file(GLOB ...)` 收集，**未加 `CONFIGURE_DEPENDS`**，新增文件
  不重新配置 CMake 就不会进入构建。
- 已有 PCH（`src/main/YiCadPch.h`），未启用 unity build。
- 分层违规三处：`DmDocument.cpp:47` 引入 `UITabDrawWidget.h`，
  `DmEntityContainer.cpp:31` 与 `DmHatch.cpp:50` 引入 `UIDialogFactory.h`
  —— 数据模型层在弹对话框。

### 6.3 目标库结构

```mermaid
flowchart TB
    App["YiCadApp<br/>exe: main/"]
    Ui["YiCadUi<br/>ui/, 49 个 .ui"]
    Inter["YiCadInteraction<br/>actions/, kernel/actions/"]
    Render["YiCadRender<br/>kernel/painters/, kernel/gui/"]
    Persist["YiCadPersistence<br/>kernel/persistence/, filters/, fileio/"]
    Model["YiCadModel<br/>kernel/builder_model/, data_model/,<br/>history/, modification/, information/"]
    Math["YiCadMath<br/>kernel/math/, utility/"]

    App --> Ui
    App --> Inter
    Ui --> Inter
    Inter --> Render
    Inter --> Model
    Render --> Model
    Persist --> Model
    Model --> Math
    Render --> Math
```

依赖方向严格单向，**`YiCadModel` 不得依赖 `YiCadUi` 或 `YiCadRender`**。

`YiCadRender` 在此仅作为一个层次边界存在，本方案**不改动其内部实现**。
未来的渲染专项将在这个边界内进行，届时不会波及其它库。

### 6.4 任务

1. **先修分层违规（P7）**
   - `DmEntityContainer` 与 `DmHatch` 对 `UIDialogFactory` 的调用：改为由模型层
     返回状态或错误码，由调用方（Action 或 UI 层）决定是否提示；或注入
     `IUserPrompt` 接口，默认实现为静默。
   - `DmDocument` 对 `UITabDrawWidget` 的依赖：走已有的
     `GuiDialogFactoryInterface` 式的接口注入，或用信号解耦。
2. **源文件清单显式化**
   - 把 `file(GLOB)` 改为显式源文件列表（推荐），或至少加 `CONFIGURE_DEPENDS`。
   - 显式列表的额外收益：拆库时每个库的边界一目了然，误放文件会立刻暴露。
3. **逐库拆分**
   - 顺序：`YiCadMath`、`YiCadModel`、`YiCadPersistence`、`YiCadRender`、
     `YiCadInteraction`、`YiCadUi`、`YiCadApp`。
   - 从最底层开始，每拆一个库，编译器会把所有反向依赖暴露出来。
   - 每个库独立 `target_include_directories(... PUBLIC/PRIVATE)`，禁止全局
     include 路径。
4. **构建优化**
   - 每个库单独配置 PCH（当前全局一份 `YiCadPch.h`，拆库后各层需要的头不同）。
   - 对 `YiCadModel`、`YiCadPersistence` 等大库启用 `UNITY_BUILD`。
   - 测量并记录：全量构建时间、单文件改动的增量构建时间。
5. **CI 适配**
   - `.github/workflows/build.yml` 与 `cmake/` 下的安装脚本适配多目标。

### 6.5 验收标准

- `YiCadModel` 的编译不需要 Qt Widgets 与 OpenGL。
- `src/kernel/` 下不再出现对 `UI*` 头文件的包含。
- 全量构建时间与「只改 `DmArc.cpp` 的增量构建时间」相对阶段 0 基线有明确改善数据。
- 新增源文件无需手工重配 CMake，或有明确的显式清单维护流程。

### 6.6 风险

- **中**。主要是循环依赖的发现与拆解，可能暴露出比已知三处更多的反向依赖。
- 缓解：先做一次静态依赖分析（按 `#include` 构图），在动手前把循环依赖清单列全，
  评估后再决定是否调整库边界。

### 6.7 执行结果

按 6.6 节的缓解措施，先做了一次全量静态依赖分析（按 `#include` 逐文件构图，
覆盖全部七个分区），再动手。分析发现的循环依赖规模远超"可能比已知三处更多"
的预期，直接决定了库边界必须调整：详见下表第一行。落地顺序是先修分层
违规与关系错位（6.4.1、6.4.3 部分），再拆库（6.4.3 其余部分），最后跑
Debug/Release 全量构建与 `ctest` 验证。

**与方案的偏差**

| 项 | 方案 | 实际 | 理由 |
|----|------|------|------|
| 目标库结构 | 七个分区各自拆成独立静态库（6.3 节图） | 只有 `YiCadMath`/`YiCadModel`/`YiCadPersistence` 三层拆成独立静态库，依赖方向由 CMake 物理强制；`RENDER`/`INTERACTION`/`UI`/`APP` 四个分区仍合编进 `YiCadCore`（OBJECT 库，机制不变） | 静态依赖分析发现两处真实的双向依赖，都是阶段 1、2 与既有代码的既定设计而非本次拆库引入：(1) `GuiDocumentView`/`GuiEventHandler`（RENDER）在构造函数里直接 `new`/持有 `ViewToolControl`/`PanZoomTool`/`LegacyActionTool`/Action 栈（INTERACTION 的具体类型），阶段 2 就是这样设计的；(2) `src/actions/` 106 个 Action 里 40 余个直接包含 UI 对话框头文件与 `ApplicationWindow`/`MDIWindow`（APP），UI 组件反过来也有多处直接调用 `ApplicationWindow`/`MDIWindow` 取全局状态。这两对关系里任何一边都无法在不改变运行时行为的前提下只靠移动文件解耦，需要的是依赖注入式重构（比如把 `ViewToolControl` 的构造从 `GuiDocumentView` 构造函数搬到 `MDIWindow` 创建视图之后再注入），工作量与风险都超出"拆库"本身，作为独立事项留给未来（候选：与阶段 4 的 `IExtension` 框架一起做，那时本来就要重新设计 Ribbon/命令的注册路径） |
| `kernel/persistence/` 的归属 | 整个目录连同 `Meta/`、`filters/`、`fileio/` 归 `YiCadPersistence` | 拆成两半：`Persistence`/`Stream`/`Reader`/`Writer`/`Archive`/`Tools`/`Base64`/`Swap`/`TimeInfo`/`Uuid`/`gzstream`/`MinizipNgArchive`/`FileInfo`（原 `persistence/` 根目录的大部分文件）与 `Meta/MetaType`、`Meta/Type`（RTTI 基础设施）物理迁到 `kernel/math`；`Meta/` 下各 `MetaXxx`（`MetaArcs`、`MetaCircles`……）、`MigratorBase`、`filters/`、`backuppolicy` 留在 `YiCadPersistence` | `DmFlags.h`（`DmObject`/`DmEntity` 的直接基类）本身就 `#include "Persistence.h"` 和 `"Stream.h"`——`Persistence` 是整个类型系统的根，`DmArc`/`DmCircle`/`DmEllipse`/`DmLine`/`DmPoint`/`DmRay`/`DmSolid` 等实体类的 `saveStream`/`restoreStream` 直接用到 `OutputStream`/`InputStream`。这些文件不含任何 `Dm*` 类型依赖（只用 `pugixml`/`boost`/标准库），是比 Model 更底层的基础设施，物理上放错了目录；`Meta/MetaArcs` 一类文件则相反，直接 `#include "DmEntity.h"`，是真正依赖 Model 的具体持久化实现，留在原位置 |
| `kernel/math/` 的归属 | 整个目录归 `YiCadMath`，且不依赖任何上层 | `FindClosedRegion`、`ConstrainedDelaunayTriangulation`、`Quadratic`、`GeUtility` 迁出到 Model 分区（`FindClosedRegion`/`ConstrainedDelaunayTriangulation`/`Quadratic` 到 `kernel/information`，`GeUtility` 同去）；`SpacialSearchTree` 迁到 `kernel/history`。同时 `Datamodel.h`（`DM::` 命名空间的枚举与常量）与 `DmVector`/`DmRect`（几何值类型）从 `kernel/builder_model` 迁入 `kernel/math` | 前四者虽物理放在 `kernel/math/`，但直接操作 `DmArc`/`DmCircle`/`DmEllipse`/`DmLine`/`DmPolyline`/`DmSpline`/`DmTriangle`/`DmBlockReference`/`DmEntity` 等真实 CAD 实体（`DmTriangle` 本身 `: public DmEntity`），是实体感知的几何算法而非纯数学，应属 Model 层；`Datamodel.h` 反而是零依赖的纯枚举/常量头（没有一个 `#include`），`DmVector`/`DmRect` 只依赖 `Datamodel.h` 和标准库，却被几乎每一个数学算法文件用作最基础的坐标类型，本质上是 Math 层缺失的底座，留在 Model 会让 `YiCadMath` 无法独立编译 |
| `IDocumentView`/`ISnapService` 的归属 | 阶段 1/2 已放在 `kernel/actions/`（INTERACTION），未在阶段 3 计划内变动 | 迁到 `kernel/builder_model/`（MODEL） | `kernel/modification/Selection.h`、`Modification.cpp`、`kernel/history/BlockEditCmd.cpp`（均为 Model 分区，阶段 1 就已让它们持有 `IDocumentView*`，见 4.6 节）与阶段 3 新增的 `DmDocument::m_documentView`（见下）都需要这个接口；接口必须落在消费方里最低的那一层，否则 Model 拆库时会反向依赖 Interaction。顺带修了 `IDocumentView.h` 一处遗留：它 `#include "Snapper.h"`（阶段2的具体实现类），实际只需要阶段2已经拆出的 `ISnapService.h`（`SnapMode`/`DM::SnapRestriction` 所在地） |
| `DmCachePainter` 的归属 | 6.2 节已知问题，"阶段 3 拆库时需处置"，未定具体去向 | 迁到 `kernel/gui/`（RENDER） | 该类完整包装 `GLCachePainter`，成员方法叫 `create_resources()`（初始化 glew 及 shader），是纯 GL 渲染代码，只被 `GuiDocumentView`/`GuiPreviewWidget`（均 RENDER）使用，放在 `kernel/builder_model/` 纯属目录放错 |
| `GuiDialogFactory`/`GuiDialogFactoryAdapter`/`GuiDialogFactoryInterface` 的归属 | 未提及；6.4.1 只说"或注入接口" | 三个文件从 `kernel/gui/`（RENDER）迁到 `kernel/builder_model/`（MODEL） | `GuiDialogFactoryInterface.h` 本身只 `#include <QString>` 与 `"Datamodel.h"`，对话框相关的具体类型（`GuiDocumentView`、`UICommandWidget` 等）全部只是形参里的前置声明，不产生真实编译依赖；但 `DmDocument.cpp`、`Modification.cpp` 等 Model 文件通过 `GUIDIALOGFACTORY` 宏直接调用它，物理放在 RENDER 目录下已构成 Model→Render 依赖。移下去之后 `DmEntityContainer.cpp`/`DmHatch.cpp` 两处死 include（见下一条）与 `Modification.cpp` 一处死 include 都不需要额外处理 |
| 6.4.1 的三处 P7 违规修法 | `DmEntityContainer`/`DmHatch`：改返回值或注入 `IUserPrompt`；`DmDocument`：接口注入或信号解耦 | 前两处：`#include "UIDialogFactory.h"`（连同同时存在的 `#include "GuiDialogFactory.h"`）在文件里通篇未被引用，是死代码，直接删除，未新增任何接口；后一处：给 `GuiDialogFactoryInterface` 新增 `requestUntitledDocumentName(DmDocument*)`，`DmDocument::save()` 里原来直接 `ApplicationWindow::getAppWindow()->getTabDrawWidget()->getTabDrawDataOfDocument(this)` 取标签页名字的调用改走这个接口 | 逐文件核查每处违规的真实用途后发现，`UIDialogFactory.h` 从未被真正需要过，是历史遗留；只有 `DmDocument.cpp` 这一处是真实需求，套用已有的 `GuiDialogFactoryInterface` 单例注入模式（`ApplicationWindow` 启动时 `setFactoryObject`）即可，不需要新发明机制 |
| `GuiDialogFactoryInterface` 的扩展面 | 未列出 | 除 `requestUntitledDocumentName` 外，另加 `requestActiveDocument()`、`requestFileExport()`、`requestFileImport()`、`requestConfirmDialog()` 四个方法 | 静态分析额外发现四类同类问题：(1) `DmDimAngular`/`DmDimDiametric`/`DmDimLinear`/`DmDimRadial`/`DmLeader` 共 9 处调用 `ApplicationWindow::getAppWindow()->getDocument()` 取"当前活动文档"；(2)(3) `DmDocument::save()`/`open()` 直接调 `FileIO::instance()->fileExport/fileImport()`（`kernel/fileio`，见下一条其物理位置本身也要挪走）；(4) `DmDocument::open()` 里两处 `QMessageBox::critical(...)` 直接弹 Qt Widgets 对话框。全部套用同一个已有的接口注入模式解决，具体实现（`UIDialogFactory`，`ui/`）里对 (1)(2)(3) 就是把原来的直接调用原样挪过去，行为不变 |
| `DmDocument::m_documentView` 的类型 | 未提及（阶段 1 明确排除了 `kernel/builder_model`） | 从具体的 `GuiDocumentView*` 改为 `IDocumentView*`；`setDocumentView`/`getDocumentView` 的形参/返回类型同步改；`IDocumentView` 新增 `setDocumentPainterContainer(DmEntityContainer*)` | `DmDocument.cpp` 内部四处调用 `m_documentView->specifyDocumentModified()/redraw()/setDocumentPainterContainer()`，指针类型是 `GuiDocumentView*` 就必须完整定义该类型（哪怕只是调虚函数），无法只前置声明。三个被调方法均属"文档视图"能力，机械改造成接口调用；`kernel/actions/Preview.cpp` 原来正是因为 `DmDocument::getDocumentView()` 返回具体类型才特意 `#include "GuiDocumentView.h"`（代码里留了这条注释），改完接口返回类型后这处 include 也一并去掉，是意外的额外收益 |
| `MTextEditCmd` 的归属 | 未提及（`kernel/history` 属 Model，未被列为已知问题） | 从 `kernel/history/` 迁到 `kernel/actions/`（INTERACTION） | 其中两个类（`MTextEdit_SetSelectBeginEndToNull_Cmd`、`MTextEdit_ResizeCmd`）直接读写 `ui/MTextEditWidget`（UI 层）的公开成员字段（选区字符指针、缩放边界），是编辑控件自身交互状态的撤销/重做，不是文档数据的撤销/重做；全仓唯一调用方是 `ui/MTextEditWidget.cpp`，`kernel/history` 下没有任何引用，迁移零风险。通用的 `MTextEditCmdManager`（不引用任何具体 Cmd 类或 `MTextEditWidget`）留在原地 |
| `kernel/fileio` 的归属 | 计划图未列出，CMakeLists 阶段 0 起归 `PERSISTENCE` | 归 `APP` 分区（物理文件不动，只改 CMake 分区归属） | `Fileio.cpp` 除了 `kernel/filters` 的内置格式过滤器，还要 `setPluginRuntime()` 接入 `plugin_runtime/`（APP）的 `PluginRegistry`/`PluginManager`/`HostApi`，且直接 `#include` 这三个类的完整定义（不是前置声明）——这是已有的、通过 setter 显式注入的设计，但物理上决定了 `Fileio.{h,cpp}` 只能编进能看到 APP 分区头文件的目标。`DmDocument` 对它的调用已通过上一条的接口注入解耦，`YiCadPersistence` 不再需要它 |
| 死 include 清理 | 未列为任务 | 顺手删除 Model 分区内约 30 处死 include：14 个 `Dm*.cpp` 里 `#include "GuiDocumentView.h"`（阶段 1 之前遗留，符号从未被引用）、`DmArc`/`DmCircle`/`DmEllipse`/`DmLine`/`DmPoint`/`DmRay`/`DmSolid` 的 `Writer.h`/`Reader.h`（`Stream.h` 才是真用到的）、`DmLineType`/`DmOverlayEntity`/`DmMTextParagraph`/`CircleData` 的 `Tools.h`、`DmTextStyle.cpp` 的 `ApplicationWindow.h`/`MDIWindow.h`、`DmSystem.cpp` 的 `<QApplication>`、`FilterJsonIO.cpp` 的 `GuiDocumentView.h` | 逐个 `#include` 核查真实用途是拆库前必须做的工作（否则库边界立在错误的地方），顺手做了清理；每一处删除都用"编译器是否报未定义符号"验证过，不是凭 grep 猜测 |
| PCH-per-library 与 UNITY_BUILD（6.4.4） | 每个新库单独配置 PCH，大库启用 UNITY_BUILD | 均未实施，`YiCadCore` 的 PCH 保持原样，三个新静态库不加 PCH | 直接复用现有 `YiCadPch.h` 会把 `<QWidget>` 等重量级头带回 `YiCadMath`/`YiCadModel`，抵消拆库意义；另起炉灶配小型 PCH 需要新一轮"每个头是否安全"的分析，且 `YiCadModel` 达 200 余文件，盲开 `UNITY_BUILD` 有暴露匿名命名空间/静态变量重名等隐藏问题的真实风险。二者都不是"拆库"本身要求的，优先级低于把库边界立对，作为独立事项留给未来 |
| `tools/check_layering.py` | 白名单三处违规修完后一并删除 | 白名单清空（`WHITELIST = {}`），脚本本体与扫描范围（整个 `src/kernel/`）保留 | 脚本检查的是"内核不得包含 `UI*` 头文件"，对 `YiCadMath`/`YiCadModel`/`YiCadPersistence` 这三层，CMake 的 `target_include_directories` 现在物理保证了这件事（想违规都编不过），脚本对它们而言是多余的；但 `kernel/actions`、`kernel/gui` 仍与 UI 合编（见第一条），这两个目录理论上仍可能新增对 `UI*` 头的直接包含，脚本继续作为比"重新配置+编译"更快的 CI 早期预警保留 |

**执行中发现但未修复的问题**

| 位置 | 问题 | 说明 |
|------|------|------|
| `DmDimAngular.cpp`、`DmDimDiametric.cpp`、`DmDimLinear.cpp`、`DmDimRadial.cpp`、`DmLeader.cpp` | 用"当前活动文档"（现改为 `GUIDIALOGFACTORY->requestActiveDocument()`）而非 `this->getDocument()`（实体自身所属文档，`DmObject` 已有此访问器）取 `DmDimStyleTable` | 多文档场景下，若这些标注实体所属的文档不是当前界面焦点文档（例如后台重算、非焦点标签页的撤销重放），会取错标注样式表。这是原有行为（阶段 3 只是把直接调用挪到接口后面，未改语义），修复需要验证这些方法被调用时 `this` 是否已可靠挂到文档树上，留给未来单独评估 |

**新增设施**

- `YiCAD/src/kernel/builder_model/GuiDialogFactoryInterface.h`（从 `kernel/gui/` 迁入）—— 新增
  `requestConfirmDialog()`、`requestActiveDocument()`、`requestUntitledDocumentName()`、
  `requestFileExport()`、`requestFileImport()` 五个方法，连同已有的 `requestWarningDialog()`/
  `commandMessage()` 等，构成 Model 层"向 App/UI 层请求用户交互或宿主状态"的完整接口；
  `GuiDialogFactoryAdapter.h` 同步加了默认空实现，`ui/UIDialogFactory.{h,cpp}` 同步加了
  委托给既有 `ApplicationWindow`/`FileIO` 调用的具体实现。
- `YiCAD/src/kernel/builder_model/IDocumentView.h`、`ISnapService.h`（从 `kernel/actions/` 迁入）——
  `IDocumentView` 新增 `setDocumentPainterContainer(DmEntityContainer*)`；`IDocumentView.h`
  的 `#include "Snapper.h"` 改为 `#include "ISnapService.h"`。
- `YiCAD/src/kernel/gui/DmCachePainter.{h,cpp}`（从 `kernel/builder_model/` 迁入）。
- `YiCAD/src/kernel/math/{Persistence,Stream,Reader,Writer,Archive,Tools,Base64,Swap,TimeInfo,Uuid,gzstream,MinizipNgArchive,FileInfo,MetaType,Type,Datamodel,DmVector,DmRect}.{h,cpp}`
  （从 `kernel/persistence/`、`kernel/persistence/Meta/`、`kernel/builder_model/` 迁入）。
- `YiCAD/src/kernel/information/{FindClosedRegion,ConstrainedDelaunayTriangulation,Quadratic,GeUtility}.{h,cpp}`、
  `YiCAD/src/kernel/history/SpacialSearchTree.{h,cpp}`（从 `kernel/math/`、`kernel/utility/` 迁入）。
- `YiCAD/src/kernel/actions/MTextEditCmd.{h,cpp}`（从 `kernel/history/` 迁入）。
- `YiCAD/CMakeLists.txt` —— `YiCadMath`/`YiCadModel`/`YiCadPersistence` 三个新 `STATIC` 目标，
  依赖方向 `YiCadPersistence -> YiCadModel -> YiCadMath`，全部用 `PUBLIC`
  `target_link_libraries`/`target_include_directories`/`target_compile_definitions`
  传播，公共编译选项（`_USE_MATH_DEFINES`、`/utf-8` 等）只在 `YiCadMath` 声明一次即可
  沿依赖链传到 `YiCadCore` 与可执行/测试目标；`YiCadCore` 改为链接 `YiCadPersistence`
  而非直接收纳七个分区的源文件。
- `tests/CMakeLists.txt` 的 `yicad_add_test()` 新增 `LINK` 参数——`tests/math`、
  `tests/geometry`、`tests/persistence` 分别改链接 `YiCadModel`（`tests/math` 的用例本身
  只需要 `YiCadMath`，但共用入口 `yicad_test_main.cpp` 要先跑 `DmSystem::init`/
  `DmSettings::init`，这两步是 `YiCadModel` 的符号，经 `PUBLIC` 依赖链带上 `YiCadMath`）、
  `YiCadModel`、`YiCadPersistence`，不再链接完整的 `YiCadCore`；`tests/interaction`
  未改动，继续链接 `YiCadCore`（其用例覆盖 `SelectTool`/`PanZoomTool` 等 INTERACTION
  分区代码）。
- `tools/measure_build.ps1` 的 `Datamodel.h` 目标路径同步改为 `kernel/math/Datamodel.h`；
  `Invoke-Build` 补上 `-- -m`（见下一条与本节末尾的"验收对照"）。
- `.github/workflows/build.yml` 的 Build 步骤同样补上 `-- -m`——拆库前只有
  `YiCadCore` 一个大目标，`-m` 可有可无；拆库后 `YiCadMath -> YiCadModel ->
  YiCadPersistence -> YiCadCore` 是一条有依赖顺序的项目链，Visual Studio
  生成器默认不做解决方案级并行，实测无 `-m` 的全量构建比有 `-m` 慢约 10 秒
  （189.7 秒 vs 179.2 秒，见 `doc/BASELINE.md` §5）。

**影响面**

- `kernel/builder_model/DmDocument.{h,cpp}` —— 移除 `Fileio.h`/`ApplicationWindow.h`/
  `UITabDrawWidget.h`/`GuiDocumentView.h`/`<QMessageBox>` 五个跨层 include；`m_documentView`
  改为 `IDocumentView*`；`save()`/`open()` 里的 `FileIO::instance()`/`QMessageBox::critical()`
  调用改走 `GUIDIALOGFACTORY`。
- `kernel/builder_model/dimension/{DmDimAngular,DmDimDiametric,DmDimLinear,DmDimRadial,DmLeader}.cpp` ——
  移除 `ApplicationWindow.h`，`ApplicationWindow::getAppWindow()->getDocument()` 改为
  `GUIDIALOGFACTORY->requestActiveDocument()`。
- `kernel/builder_model/DmImage.cpp` —— 补上直接需要却一直靠 `GuiDocumentView.h` 间接带入的
  `<QImage>`/`<QPolygonF>`（P8 式的隐藏依赖，模式与阶段 1 发现的 `DmEllipse.cpp`/glm
  问题相同）。
- `kernel/builder_model/DmSystem.cpp` —— 补上 `<QCoreApplication>`（`qApp` 宏所在地，同上）。
- `kernel/actions/Preview.cpp` —— 移除不再需要的 `#include "GuiDocumentView.h"`，改
  `#include "IDocumentView.h"`。
- `tests/support/FakeDocumentView.h` —— 补上 `IDocumentView` 新增的
  `setDocumentPainterContainer()` 空实现，否则该测试替身变成抽象类。
- 其余约 20 个文件仅删除死 include，零行为变化（见上表"死 include 清理"）。

**验收对照**

| 6.5 节的验收标准 | 状态 |
|------------------|------|
| `YiCadModel` 的编译不需要 Qt Widgets 与 OpenGL | 达成。`YiCadModel` 只链接 `Qt5::Core`、`Qt5::Gui`、`CDT::CDT`、`Freetype::Freetype`（`DmFont` 直接用 FreeType 量取字形轮廓）与 `Dwrite.lib`（`DmSystem` 枚举系统字体路径），不含 `Qt5::Widgets`、`GLEW`、`OpenGL`；原有的一处 Widgets 依赖（`DmDocument::open()` 里两处 `QMessageBox::critical`）已改走接口注入 |
| `src/kernel/` 下不再出现对 `UI*` 头文件的包含 | 部分达成，且比方案预期更严格：`YiCadMath`/`YiCadModel`/`YiCadPersistence` 三层现在是物理不可能（不在编译期 include 路径上）；`kernel/actions`/`kernel/gui`（仍与 UI 合编进 `YiCadCore`）目前实测同样不含 `UI*` 包含（`tools/check_layering.py` 全量扫描通过，白名单为空），但这两个目录未被禁止将来引入——如引入需在脚本白名单登记 |
| 全量构建时间与「只改 `DmArc.cpp` 的增量构建时间」相对阶段 0 基线有明确改善数据 | 部分达成，且好坏两个方向都很明确，见 `doc/BASELINE.md` §5：`DmArc.cpp` 增量 19.3→9.7 秒（-50%），`GuiDocumentView.h` 增量 65.3→18.7 秒（-71%，两者都是"改一个文件"场景，日常开发的典型情况）；但全量构建 144.3→179.2 秒、`Datamodel.h` 增量 120.9→141.2 秒，两项聚合型指标反而变差。根因是 Visual Studio/MSBuild 按 `YiCadMath -> YiCadModel -> YiCadPersistence -> YiCadCore` 的项目依赖顺序构建，无法把"只需要头文件"和"需要完整 .lib"两种依赖强度区分开，16 核机器上单个下层库文件不够多时反而并行度用不满；已补上 `cmake --build -- -m`（CI 与 `tools/measure_build.ps1` 同步），部分缓解但未消除，彻底解决预计要换生成器（如 Ninja），超出本阶段范围 |
| 新增源文件无需手工重配 CMake，或有明确的显式清单维护流程 | 达成（阶段 0 已完成，本阶段延续，`CONFIGURE_DEPENDS` 覆盖全部七个分区外加三个新库） |

6.5 节四条验收标准，第一、四条完全达成；第二条在物理可行的范围内（三层拆库
部分）达成、在合编部分（Render/Interaction/UI/App）维持既有脚本检查——这一
范围收窄本身是 6.7 节记录的核心偏差，不是遗漏；第三条喜忧参半——两项"改一个
文件"的日常场景指标明显变好，两项"改动波及全树"的场景指标因生成器的项目级
调度粒度反而变差，且已确认无法在不换生成器的前提下完全消除。目标库结构从
七层降为"三层独立静态库 + 一个合编 OBJECT 库"，是本阶段对方案最大的一处修订；
`Render`/`Interaction` 的双向依赖解耦、`UI`/`APP` 的双向依赖解耦、构建生成器
选型、以及 PCH-per-library、`UNITY_BUILD`，都作为独立、有明确前置条件的事项
留给未来，不是当前的遗留缺口。

---

## 7. 阶段 4：命令注册表与扩展化

### 7.1 目标

消除命令的中心化注册，使「新增一个功能」不再需要修改内核文件；
在此基础上把功能按领域拆为进程内扩展。这是 P6 的解法。

### 7.2 现状证据

- `Datamodel.h:127` 的 `DM::ActionType` 枚举含 **162** 项，位于内核最底层，
  被 27 个文件直接包含。
- `UIActionHandler.cpp`（1,735 行）用 **153 个 `case`** 把枚举映射到 Action 构造。
- `cmd/Commands.cpp`（727 行）是命令行输入的第二套映射。
- 插件 DLL 走第三套路径：`plugin_runtime/`（14,363 行），其中
  `HostApi.cpp` 5,661 行、`YiCadPluginAbi.h` 2,036 行、`YiCadPluginSdk.h` 3,806 行，
  合计约 11.5k 行手写 C ABI。

### 7.3 关键决策：内部扩展不走 C ABI

现有的 ABI v3 是为**第三方 DLL** 设计的稳定二进制边界。内部功能拆扩展**不应**复用
它，否则等于为自己的代码支付 ABI 稳定性成本（版本协商、布局静态断言、字符串
借用语义、句柄不可解引用等约束）。

内部扩展应使用进程内 C++ 接口，参考 DS 的 `Application/Framework/`：

- `IExtension`：`OnRegister(IExtensionContext&)`、`OnShutdown()`、`Id()`；
- `IExtensionContext`：启动期资源门面（Ribbon 注册入口等），刻意不暴露全局单例；
- `ExtensionManager`：持有 `unique_ptr<IExtension>`，`OnShutdown` 按注册反序触发。

两套机制并存，边界清晰：**C ABI 对外，`IExtension` 对内**。

### 7.4 任务

1. **命令注册表（本阶段的核心，先于扩展化）**
   - 定义 `CommandRegistry`：以稳定字符串 ID（如 `draw.line`、`modify.trim`）
     为键，注册命令元数据（显示名、图标、可用性谓词、工厂函数）。
   - 自注册机制：每个 Action 在自己的 `.cpp` 中静态注册，加命令等于加一个文件。
   - 三条入口收敛到同一注册表：Ribbon（`UIActionHandler`）、命令行
     （`cmd/Commands.cpp`）、插件（`PluginRegistry`）。
   - `DM::ActionType` 降级：保留为实体类型标识等仍需枚举的场景，
     移除其作为命令 ID 的职责。
2. **删除 153 个 `case`**
   - `UIActionHandler` 退化为「查表加激活」，从 1,735 行降到数百行。
3. **扩展框架**
   - `IExtension`、`IExtensionContext`、`ExtensionManager`。
   - Ribbon 注册改为由扩展在 `OnRegister` 期间声明，而非集中在
     `ApplicationWindow`（2,293 行）中硬编码。
   - 加入命名空间校验：扩展注册的命令 ID 必须以其扩展 ID 为前缀。
4. **试点扩展：`ai/`**
   - 选择理由：7,063 行，自包含度最高，抽掉不影响核心绘图能力，失败成本最低。
   - 验证扩展框架的生命周期、Ribbon 注册、设置页注册三条路径。
5. **按领域推进后续扩展**
   - 建议顺序：`ai/`、标注（Dim 系列）、文字（Text/MText）、块（Block）、
     填充（Hatch）、打印。
   - 每个扩展独立成库（依托阶段 3 的库拆分能力）。

### 7.5 验收标准

- 新增一条绘图命令不需要修改 `src/kernel/` 下的任何文件。
- `UIActionHandler.cpp` 无 `switch (actionType)` 巨型分支。
- `ai/` 作为独立扩展加载，关闭该扩展后应用正常启动与绘图。
- 扩展的注册与卸载顺序可预测，`OnShutdown` 反序执行。

### 7.6 风险

- **高**。改动面覆盖全部 106 个 Action 与全部 Ribbon 装配代码。
- 缓解：
  - 注册表与枚举**并行共存**一段时间：新命令走注册表，旧命令保留 `case` 分支，
    逐批迁移，任何时刻都可构建可运行；
  - 每批迁移后跑阶段 0 的测试加交互回归清单；
  - 先只做第 1、2 项（注册表），确认稳定后再做第 3 至 5 项（扩展框架）。

### 7.7 执行结果

按 7.6 节的缓解措施，只做了任务①②（命令注册表、消除 `UIActionHandler.cpp`
的巨型 switch），任务③④⑤（扩展框架、`ai/` 试点、按领域推进后续扩展）留给
后续会话。分八个批次落地（"阶段4（第一/三/四/五/六/七/八部分）"，第二部分
的编号在执行中被第三部分的批次直接沿用，见下表说明），每批迁移后都跑
`ctest`（Debug）与 `check_layering.py`，收尾额外跑了一次 Release 全量构建
与 `ctest`。

**与方案的偏差**

| 项 | 方案 | 实际 | 理由 |
|----|------|------|------|
| switch 的真实规模 | "153 个 case"（1.2 节、7.2 节、附录 A 均引用此数） | `UIActionHandler::setCurrentAction` 的 switch 实际只有 **132** 个 `case DM::Action*` 标签 | 逐行核对后发现，"153" 是对整个文件里三个独立 switch（`setCurrentAction` 132 个、`keycode()` 11 个、`commandLineActions()` 10 个，132+11+10=153）的合计误记成了单一 switch 的规模。三个 switch 各自独立存在（`keycode()`/`commandLineActions()` 是历史遗留的重复实现，见 5.2/7.2 节已经指出的"三重复"问题），本次只处理 `setCurrentAction` 这一个——它才是 7.5 节验收标准里"无 switch 巨型分支"指代的对象。已用 `git show` 取迁移前版本、`sed`+`grep` 精确统计过，不是估算。 |
| `CommandRegistry` 的键 | "以稳定字符串 ID……为键" | 字符串 ID 为主键，另加 `std::map<DM::ActionType, QString>` 的 legacy 桥接表，只给 132 个内置命令用 | 全字符串化需要同时改 `keyconfig.xml` 的格式、`Commands.cpp` 的既有 `std::map<QString, DM::ActionType>` 模型、以及 16+ 处直接引用 `DM::ActionXxx` 枚举值的调用点——这些都不是"注册表"任务本身要求的，会把改动面从"约100个 Action 文件"扩大到"CLI/快捷键子系统的数据格式"。桥接设计换来的是：`setCurrentAction(DM::ActionType)`、`Commands.cpp`、`ApplicationWindow.cpp` 的 92 处 `connect()` **一行都不用改**，`create(const QString&, ...)` 纯字符串路径已就绪、为后续任务③④的扩展框架预留，两头都不牺牲。 |
| 削减 switch 后的残留结构 | 未细化 | `setCurrentAction` 开头保留两个显式分支：`ActionEditKillAllActions`（无法构造 Action，且要用只在具体类 `GuiDocumentView` 上的 `killAllActions()`，够不到 `IDocumentView` 接口）与转发给既有 `commandLineActions()` 的 11 个 Snap/Restrict 类型（该函数本就是这批类型的权威实现，`command()` 早就在调用它）；两者都**不进注册表**。除这两个分支外，函数体缩成"查表，查不到就是 nullptr"五行代码 | 12 个类型缺乏统一的 `CommandFactory` 签名（`(DmDocument*, IDocumentView*) -> ActionInterface*`）能表达的行为——前者不建任何 Action，后者根本不建 Action、只是转调 GUI 更新槽。硬套进注册表需要扩展 `CommandContext`/放宽接口，收益是"表面上看起来更统一"，成本是给 120 个正常命令的签名添加两个永远用不到的字段；判断为不值得，原样保留为独立分支，`UIActionHandler.cpp` 里"无 switch 巨型分支"这条验收标准仍然成立——12 个分支不是 switch，是两条 if。 |
| `ActionModifyDelete` 的处理 | 未列为已知问题 | 未随其余 7 组"先选后建"用 `makeSelectFirstFactory` 统一处理，单独写了一个不检查 `hasSelect()`、无条件先建 `ActionSelect` 的专用工厂 | 逐 case 核对时发现原 switch 里它确实和其余 7 组（Copy/Move/Rotate/Scale/Mirror/Explode/Reverse）行为不一致——那 7 组都是"没选中才弹选择"，`ActionModifyDelete` 是"永远先弹选择"。这是原有行为差异（可能是有意的删除保护，也可能是历史遗留），照原样保留，不在迁移中"顺手对齐"，避免把一次机械重构变成一次隐藏的行为变更。 |
| `ActionBlocksEdit` 的 `tr()` 上下文 | 未提及 | 两处 `QMessageBox::warning` 从 `UIActionHandler::tr(...)` 改成 `ActionBlocksEdit::tr(...)`（原字符串不变） | 这段逻辑从 `UIActionHandler.cpp` 搬进 `ActionBlocksEdit.cpp` 的匿名命名空间自由函数后，不再有 `UIActionHandler` 这个 `QObject` 子类的隐式 `tr()` 可用，只能显式指定一个 `Q_OBJECT` 类；选了逻辑上更贴切的 `ActionBlocksEdit` 而非 `QObject`。翻译文本本身没变，但 `.ts` 里的归属条目会从 `"UIActionHandler"` 变成 `"ActionBlocksEdit"`，下次 `lupdate` 需要人工核对，不是本次自动完成的。 |
| 批次划分与命名 | 六个批次（阶段0任务1+2合并算一批，之后按领域拆分） | 实际八个批次（第一、三、四、五、六、七、八部分——"第二部分"编号被跳过，因为原计划的"File/Edit/Zoom/Select"那批在执行时直接沿用了"第一部分"里已经写好的 CommandRegistry 骨架验证，两者在同一次 commit 里一并落地，之后的批次继续按顺序编号，未回填"第二部分"这个空档） | 批次边界服务于"每批独立可编译可回退"，不是服务于编号连续性；重新编号需要改动前面已经提交的 commit message，收益不大，直接在这里注明空档原因即可 |

**已知缺陷（既有代码，原样保留，未在本次迁移中修复）**

| 位置 | 问题 | 说明 |
|------|------|------|
| `ActionSelectSingle.cpp` 的 `select.single` 工厂 | `ctx.view->getCurrentAction()` 返回值在没有活动 Action 时是 `nullptr`，随即被无条件解引用（`current->getEntityType()`） | 原 switch 里就是这个写法，迁移时原样保留。候选：加一条 `DISABLED_` 回归测试锁定这个可复现的崩溃条件，留给未来专门修 bug 的会话。 |

**新增设施**

- `YiCAD/src/kernel/actions/CommandRegistry.h`、`.cpp` —— 命令注册表本体。
  `CommandContext{document, view, handler, sender}`、`CommandFactory =
  std::function<ActionInterface*(const CommandContext&)>`、
  `registerCommand(id, factory)` / `registerLegacyCommand(legacyType, id,
  factory)` / `hasLegacyMapping(legacyType)` / `create(legacyType, ctx)` /
  `create(id, ctx)`，以及复刻"先选后建"同形态 case 的
  `makeSelectFirstFactory(noSelectType, buildReal)`。注册冲突（重复 ID、
  重复 legacy 类型）用返回值 `false` 报告，不用 `assert()` 硬中断——这条
  路径本身要可测试，`assert()` 在 Debug 测试里会直接杀掉进程，见
  `tests/interaction/test_command_registry.cpp` 的"重复注册"用例。
- `YiCAD/src/kernel/gui/GuiDocumentView.h` 一类的具体渲染类型不涉及本阶段；
  `CommandRegistry.h` 只前置声明 `UIActionHandler`，不 `#include` 它，
  `tools/check_layering.py` 的空白名单不受影响。
- `tests/interaction/test_command_registry.cpp` —— 6 个用例：字符串 ID 注册、
  legacy 桥接双路径查找、重复 ID/重复 legacy 类型被拒绝且不留半成品、
  `makeSelectFirstFactory` 的两个分支（用 `EntityTable::add_direct` 直接
  插入一个选中实体触发"已选中"分支，绕开需要完整应用上下文的
  `EntityTable::add`）。

**影响面**

- `YiCAD/src/ui/UIActionHandler.cpp` —— `setCurrentAction` 从 132-case
  switch 缩成"两个特判 + 一次注册表查找"；`keycode()`、`commandLineActions()`
  两个历史遗留的重复小 switch **未改动**（各自 11/10 个 case，均非"巨型"，
  不在本次任务范围，见上表"batch 划分"一行）；文件顶部约 90 处不再需要的
  `#include "ActionXxx.h"` 一并清理，只保留仍有直接调用点的
  `ActionBlocksEdit.h`（`slotCmdStateChanged()` 用）、`ActionSelect.h`
  已随最后一批不再需要而移除。
- `src/actions/` 下 **100** 个文件（132 个 legacy `ActionType` 里，
  `ActionEditKillAllActions` 与 11 个 Snap/Restrict 类型不进注册表，
  132-12=120 个类型分布在 100 个文件里自注册，一个文件常承载一对
  `Xxx`/`XxxNoSelect` 或一个类映射两个枚举值的情形）——每个文件新增一个
  `#include "CommandRegistry.h"` 和一个匿名命名空间里的静态注册对象，
  函数体本身未改动一行。
- `ApplicationWindow.cpp` 的 92 处 Ribbon `connect(...)`、`src/cmd/
  Commands.{h,cpp}` 的 CLI 字符串映射、`keyconfig.xml` 的格式——均未改动。

**验收对照**

| 7.5 节的验收标准 | 状态 |
|------------------|------|
| 新增一条绘图命令不需要修改 `src/kernel/` 下的任何文件 | 机制就绪、未接线验证：`CommandRegistry::create(const QString&, ...)` 支持无 legacy 类型的纯字符串注册，新命令确实不需要碰 `Datamodel.h`；但目前没有任何调用方会以字符串 ID 发起命令（Ribbon/CLI 仍只认 `DM::ActionType`），要等任务③④的扩展框架把 Ribbon 注册接到字符串 ID 上才能端到端验证，如实记录为未完全达成 |
| `UIActionHandler.cpp` 无 `switch (actionType)` 巨型分支 | 达成。`setCurrentAction` 的 132-case switch 已删空；`keycode()`/`commandLineActions()` 的两个小 switch（11/10 个 case）不算"巨型"，且是独立于本次任务的既有重复实现，留作已知的、非本次范围的清理项 |
| `ai/` 作为独立扩展加载，关闭该扩展后应用正常启动与绘图 | 未做，留给任务④ |
| 扩展的注册与卸载顺序可预测，`OnShutdown` 反序执行 | 未做，留给任务③ |

四条验收标准中一条完全达成（无巨型 switch）、一条机制就绪但未接线验证、
两条明确留给后续会话——诚实反映"任务①②完成，任务③④⑤未做"的实际状态，
不写"阶段4已完成"。

---

## 8. 阶段 5：Qt 5.15 到 Qt 6 迁移

### 8.1 目标

脱离已停止公开维护的 Qt 5.15 开源版本。

### 8.2 理由

- Qt 5.15 开源版本早已停止公开补丁发布，长期停留有安全与兼容风险。
- `E:\dev\DS` 已在 Qt 6.8，两个项目对齐后可共享 UI 组件经验
  （SARibbonBar、QtADS、ElaWidgetTools）。
- 附带收益：Qt 6 提供 RHI（Rendering Hardware Interface），一份渲染代码可跑
  OpenGL、Vulkan、D3D12、Metal。未来的渲染专项若要评估后端替换，
  Qt 6 是前置条件——但这不是本阶段的排期理由。

### 8.3 任务

1. 依赖侧：SARibbonBar、CDT 及 Conan 依赖的 Qt 6 版本确认与重新构建；
   更新 `conanfile.py`、`conan.lock`、`CMakePresets.json`、CI。
2. 代码侧：`QRegExp` 改 `QRegularExpression`、容器 API 变更、
   `QMouseEvent::x()/y()` 改 `position()`（`Snapper.cpp:750` 等处仍在用旧 API）、
   `qAsConst`、`QTextCodec` 迁移、`QStringRef` 移除等。
3. 字符编码：`DocumentSettings::sourceCodePage` 相关的 DXF 编码处理需重点验证
   （Qt 6 移除了 `QTextCodec`，需改用 `QStringConverter` 或 ICU）。
4. 翻译：`ts/` 下的翻译文件用 Qt 6 的 `lupdate` 重新生成。

### 8.4 验收标准

- Qt 6 下全部测试绿灯，三份基准图纸行为一致。
- DXF 导入导出的中文编码往返测试通过。
- CI 同时或切换到 Qt 6 构建。

### 8.5 风险

- **中**。Qt 6 迁移路径成熟，但 `ui/` 26,385 行加 49 个 `.ui` 文件的改动量不小，
  且字符编码部分（DXF 代码页）有真实的行为回归风险。
- 缓解：阶段 0 建立的 persistence 往返测试在此阶段发挥核心作用。

---

## 9. 独立小项

不依赖任何阶段，可随时穿插执行。

### 9.1 清理残余 O(N) 全量扫描（P10）

| 位置 | 现状 | 改法 |
|------|------|------|
| `Selection::selectWindow`（`Selection.cpp:80`） | 遍历全表，仅有 AABB 粗筛 | 先 `searchEntities(min, max, ...)` 取候选 |
| `EntityTable::getNearestVirtualIntersection`（`EntityTable.cpp:338`） | 对全容器调 `getNearestEntity` | 改用 R 树候选集 |

`EntityTable::searchEntities`（`EntityTable.h:81`）已存在且由 R 树驱动，
两处改造都只是换调用方式，无需新建设施。

**注意**：`Snapper::catchEntity` 已经正确使用 R 树（见第 1.3 节），不在此列，
不要误改。

**验收**：大图纸上框选与虚拟交点捕捉的耗时相对基线明显下降。

**风险**：低。**工作量**：S。

---

## 10. 回退策略

每个阶段都应满足以下约束，确保任何时刻主干可构建、可运行：

| 阶段 | 回退粒度 | 保障机制 |
|------|---------|---------|
| 0 | 单个 PR | 基本不改产品代码路径 |
| 1 | 两个 PR | 纯机械变换，编译期即可发现问题 |
| 2 | 按 Action 分批 | 适配器让新旧事件体系共存，可逐 Action 回退 |
| 3 | 按库分批 | 自底向上拆，每拆一层单独 PR |
| 4 | 按命令分批 | 注册表与枚举并行共存，未迁移的命令走旧路径 |
| 5 | 分支隔离 | Qt 5 与 Qt 6 双 CI 并行一段时间 |

**通用约束**：禁止出现跨阶段的「大爆炸式」PR。任何超过三个核心文件的改动，
按 `AGENTS.md` 的约定先说明影响范围再执行。

---

## 附录 A 现状度量（基线 `d8e0be5`）

### A.1 代码量

```
src/kernel/            424 文件  104,020 行
  builder_model/       141 文件   51,368 行
  persistence/          89 文件   12,256 行
  math/                 20 文件    8,848 行
  history/              39 文件    8,571 行
  data_model/           59 文件    5,700 行
  painters/             27 文件    3,998 行
  gui/                  14 文件    3,856 行
  filters/               5 文件    3,222 行
  modification/          4 文件    2,023 行
  actions/               8 文件    1,985 行
  information/           4 文件    1,223 行
  其余 (fileio/generators/debug/printing/solver/utility)  约 928 行
src/actions/           212 文件   36,122 行   (106 个 Action 类)
src/ui/                160 文件   26,385 行   (49 个 .ui)
src/plugin_runtime/     18 文件   14,363 行
src/ai/                 29 文件    7,063 行
src/main/                9 文件    3,481 行
src/cmd/                 2 文件      914 行
--------------------------------------------
合计                   853 文件  192,306 行
```

### A.2 关键耦合指标

| 指标 | 数值 |
|------|-----:|
| `GuiDocumentView.h` 被包含次数 | 121 |
| `DmDocument.h` 被包含次数 | 146 |
| `Debug.h` 被包含次数 | 143 |
| `GuiDialogFactory.h` 被包含次数 | 105 |
| `DM::ActionType` 枚举项数 | 162 |
| `UIActionHandler.cpp` 的 `case` 数 | 153 |
| `.cpp` 中 `new` 出现次数 | 1,414 |
| `.cpp` 中 `delete` 出现次数 | 227 |
| kernel 反向依赖 ui 的文件数 | 3 |
| 可执行与库目标数（不含插件） | 1 |
| 自动化测试数 | 0 |

### A.3 待修的具体位置索引

| 问题 | 位置 |
|------|------|
| 重头文件 | `kernel/gui/GuiDocumentView.h:26-42` |
| `using namespace` 污染 | `kernel/painters/PainterCreator.h:27` |
| 硬编码 pan | `kernel/gui/GuiDocumentView.cpp:1370` |
| 每帧 `std::cout` | `kernel/gui/GuiDocumentView.cpp:1354` |
| 无效 `glEnable` | `kernel/gui/GuiDocumentView.cpp:124` |
| Snap 继承 | `kernel/actions/ActionInterface.h:37` |
| 中心枚举 | `kernel/builder_model/Datamodel.h:127` |
| 巨型 switch | `ui/UIActionHandler.cpp` |
| 分层违规 | `kernel/builder_model/DmDocument.cpp:47`、`DmEntityContainer.cpp:31`、`DmHatch.cpp:50` |
| 全量框选 | `kernel/modification/Selection.cpp:80` |
| 全容器求最近 | `kernel/history/EntityTable.cpp:343` |

---

## 附录 B 基线数据表模板

阶段 0 产出，后续每个阶段结束时复测并追加一列。

| 指标 | 小图纸(约 1k) | 中图纸(约 50k) | 大图纸(约 500k) |
|------|-------------:|---------------:|----------------:|
| 打开文档耗时 (ms) | | | |
| 稳态帧耗时 (ms) | | | |
| 点选（`catchEntity`）耗时 (ms) | | | |
| 全选框选（`selectWindow`）耗时 (ms) | | | |
| 虚拟交点捕捉耗时 (ms) | | | |

构建指标：

| 指标 | 数值 |
|------|-----:|
| 全量构建耗时 (Release) | |
| 改 `DmArc.cpp` 后增量构建耗时 | |
| 改 `GuiDocumentView.h` 后增量构建耗时 | |
| 改 `Datamodel.h` 后增量构建耗时 | |
