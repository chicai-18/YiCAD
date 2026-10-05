# YiCAD 性能基线

本文件是 `doc/ARCHITECTURE_EVOLUTION_PLAN.md` 附录 B 的落地版本：说明怎么复现
基准图纸、怎么采集数据，以及历次测量的结果。

每个阶段结束时重测一次，在表里追加一列。**不要覆盖旧列**——趋势比绝对值有用，
而且绝对值只在同一台机器上可比。

---

## 1. 准备基准图纸

三份图纸不入库（大图纸 88 MB），用脚本按需重现：

```bash
python tools/gen_benchmark_drawings.py
```

默认输出到 `build/benchmarks/`（已被 `.gitignore` 覆盖）：

| 文件 | 实体数 | 大小 |
|------|-------:|-----:|
| `benchmark_small.dxf` | 1,000 | 184 KB |
| `benchmark_medium.dxf` | 50,000 | 8.7 MB |
| `benchmark_large.dxf` | 500,000 | 88 MB |

生成是确定性的：同样的 `--seed`（默认 20260922）与实体数一定产出逐字节相同的
文件，所以不同机器、不同时间采集的数据可比。**改 seed 或改实体混合比例就等于
换了一套基准，旧数据作废。**

实体混合比例（见脚本里的 `ENTITY_MIX`）：

| 类型 | 权重 | 说明 |
|------|-----:|------|
| LINE | 40 | 真实图纸里的大头 |
| ARC | 16 | |
| CIRCLE | 12 | |
| LWPOLYLINE | 12 | |
| INSERT | 6 | 块引用，验证块展开路径 |
| TEXT | 5 | 单行文字 |
| SPLINE | 4 | 绘制成本高，占比压低 |
| HATCH | 3 | 填充，绘制成本最高 |
| MTEXT | 2 | 多行文字 |

校验生成结果（需要 `pip install ezdxf`，脚本本身不依赖它）：

```bash
python tools/gen_benchmark_drawings.py --validate
```

---

## 2. 采集运行期数据

埋点默认关闭，靠环境变量打开：

```bash
YICAD_PROFILE=1
```

Windows PowerShell：

```powershell
$env:YICAD_PROFILE = "1"
$env:YICAD_LOG = "render:info"
.\build\Release\bin\YiCAD.exe
```

埋点位置（`YiCAD/src/base/debug/ScopedTimer.h` 里的 `counters` 命名空间）：

| 计数器 | 位置 | 对应的指标 |
|--------|------|----------------|
| `render.paintGL` | `GuiDocumentView::paintGL` | 稳态帧耗时 |
| `render.regen` | `GuiDocumentView::updateDocumentCache`（文档画笔整图重建） | 整图重建的次数与耗时（渲染方案 0.1 步） |
| `render.regenSelection` | 同上，选择集变化后只重建选中组、夹点与高亮组 | 局部重建的次数与耗时（渲染方案阶段 1） |
| `render.regenHighlight` | 同上，高亮集变化后只重建高亮组 | 局部重建的次数与耗时（渲染方案阶段 1） |
| `render.scene` | `GuiDocumentView::drawScene`（场景底图重画：背景、网格、文档的普通组与选中组） | 场景底图的重画次数与耗时（渲染方案阶段 1） |
| `render.frameAfterHighlight` | `GuiDocumentView::paintGL`，高亮集变化后由 `UIView` 标记下一帧 | 悬停高亮变化后的首帧耗时 |
| `render.frameAfterSelection` | 同上，选择集变化后 | 点选、全选后的首帧耗时 |
| `render.uploadBytes`（数量） | `opengl::GLFrameStats`：`glBufferData`、图片的 `glTexImage2D`，每帧一个采样 | 每帧上传字节数 |
| `render.drawCalls`（数量） | `opengl::GLFrameStats`：`glDrawArrays`、`glMultiDrawArrays`，每帧一个采样 | 每帧绘制调用数 |
| `snap.catchEntity` | `Snapper::catchEntity` | 点选耗时 |
| `selection.selectWindow` | `SelectionSet::selectWindow` | 全选框选耗时 |
| `snap.nearestVirtualIntersection` | `EntityTable::getNearestVirtualIntersection` | 虚拟交点捕捉耗时 |
| `document.open` | `DmDocument::readFile`（读入文件，不含建窗口与首帧） | 打开文档耗时 |

耗时计数器累计次数、总耗时、最小、最大；数量计数器（`yicad::ValueCounter`）累计
次数、合计、最小、最大。`yicad::Profiler::report()` 把两类一次性汇总到 `render`
日志分类（Info 级别）；埋点开启时程序退出前会调用它，输出经 qDebug，在调试器的
输出窗口里看。**不要逐帧打印**——那正是这套设施要替代的问题（P11）。

### 自动采集（渲染方案阶段 0 起）

第 4、8 节的数据由 `test_interaction` 的 `BaselineRuntimeTest` 采集，不再手工操作：

```powershell
python tools/gen_benchmark_drawings.py
$env:YICAD_BENCHMARK_DIR = "$PWD\build\benchmarks"   # 在仓库根执行
.\build\Release\bin\test_interaction.exe --gtest_filter="BaselineRuntime*"
```

没有设 `YICAD_BENCHMARK_DIR` 时它跳过（CI 不设）。用例在本机显卡上打开一个真实的
`UIView` 窗口（1600×900），对每份图纸用代码依次做下面的操作，最后打印一张 Markdown
表，可以直接贴进来：

1. 经 `DmDocument::readFile()` 打开（`document.open`）；渲染方案阶段 2 起另记显示后首帧的整图重建（`render.regen`）；
2. 按实体表范围缩放到全图，热身 5 帧后连续画 30 帧（`render.paintGL`、每帧绘制调用与上传字节；
   渲染方案阶段 1 起另用 `GL_TIME_ELAPSED` 查询量 `paintGL` 的 GPU 耗时）；
   渲染方案阶段 2 起接着通知画布文档已修改（`DmDocumentListener::documentModified`，改动任何实体都会这样）3 次，
   各画一帧，取整图重建的耗时（`render.regen`）；渲染方案阶段 4 起改为要求整图重建（`DmDocument::requestFullRebuild()` 后通知修改，即 REGEN），
   并另记：每帧平移 1 像素的帧（场景整幅重画，CPU 与 GPU 耗时）、打开图纸前后可用显存之差（`GL_NVX_gpu_memory_info`，只有 NVIDIA 驱动有）、
   在事务里移动 20 条直线各画一帧（图形系统只处理变更集：`render.gsChanges`、`render.gsCompile`）；
   渲染方案阶段 6 起画布每帧画完整幅场景（`GuiDocumentView::setSceneBudget` 不限），上面各行与以前可比；另记按预算渐进绘制的平移
   （预算按实测的 GPU 耗时定，一帧只画预算内的部分，CPU 与 GPU 耗时）与停下后画完整幅用的帧数；
3. 换 20 次高亮的实体，每次画一帧（`render.frameAfterHighlight`，其中 `render.regen` 或 `render.regenHighlight`，
   以及 `render.scene`），相当于命令里光标从一个候选实体移到另一个上；
4. 在 20 条直线的中点点选（`snap.catchEntity`），选中后画一帧（`render.frameAfterSelection`，其中 `render.regen` 或 `render.regenSelection`）；
5. 框选盖住全部实体（`selection.selectWindow`），画一帧（全选后首帧）；
6. 从 20 条直线的端点沿直线方向求虚拟交点（`snap.nearestVirtualIntersection`）。

与手工操作的差别：不经过鼠标事件与事件分发；每帧用 `update()` 后处理事件等 `paintGL`
执行（Qt 6 会把一个刷新周期内的多次 `repaint()` 合并）。耗时都是 CPU 侧的提交耗时，
与程序里的埋点相同，不等 GPU 完成。采集期间不要在别的会话里构建。

### 手工操作步骤（架构演进方案阶段 0 的原始做法，保留备查）

对每份图纸各做一遍：

1. 打开图纸，记录从点「打开」到可交互的墙上时间（打开文档耗时）。
2. 缩放到全图，静置几秒让帧耗时稳定，读 `render.paintGL` 的平均值（稳态帧耗时）。
3. 在实体密集处点选 20 次，读 `snap.catchEntity` 的平均值（点选耗时）。
4. 从图纸一角拖框到对角，覆盖全部实体，读 `selection.selectWindow`（框选耗时）。
5. 在两条线的延长线交点附近悬停触发虚拟交点捕捉，读
   `snap.nearestVirtualIntersection`（虚拟交点捕捉耗时）。

第 4、5 两项在方案第 9.1 节改造前走全量扫描（P10），改造后改走空间搜索树；
其中第 4 步的窗口盖住全部实体，框选仍按原方式顺序遍历，耗时应与改造前持平。
阶段 0 的运行期数据一直未采集，因此没有改造前的 GUI 实测对照；改造前后的
无头对比见方案 9.1 节的执行结果。

---

## 3. 采集构建指标

```powershell
powershell -ExecutionPolicy Bypass -File tools/measure_build.ps1
```

脚本测四项：全量构建，以及分别改 `DmArc.cpp`（叶子 .cpp）、
`GuiDocumentView.h`（被 121 个文件包含）、`Datamodel.h`（中心枚举，被 27 个文件
直接包含）之后的增量构建。改法是追加一行注释再原样写回，不改变任何语义。

这三个增量指标分别对应后续阶段的收益：

- `DmArc.cpp` —— 单文件编译加链接的固定成本，阶段 3 拆库后应当明显下降
- `GuiDocumentView.h` —— 阶段 1 头文件瘦身的直接度量
- `Datamodel.h` —— 阶段 4 去中心化的直接度量

---

## 4. 运行期数据

单位毫秒。`render.paintGL`、`snap.catchEntity` 等取平均值。

### 阶段 0（基线）

采集环境：___________（CPU / 内存 / 显卡 / 驱动版本）
采集日期：___________
提交：___________

| 指标 | 小图纸 (1k) | 中图纸 (50k) | 大图纸 (500k) |
|------|-----------:|-------------:|--------------:|
| 打开文档耗时 (ms) | | | |
| 稳态帧耗时 (ms) | | | |
| 点选 `catchEntity` 耗时 (ms) | | | |
| 全选框选 `selectWindow` 耗时 (ms) | | | |
| 虚拟交点捕捉耗时 (ms) | | | |

> 架构演进方案阶段 0 时没有采集（需要手工操作），这张表不再补：那之后代码已经大变，
> 补上的数字也不是"改造前"。运行期基线从渲染层重构方案阶段 0 起由自动采集用例测得，见第 8 节。

---

## 5. 构建指标

采集环境：Windows 11 Pro 22621，MSVC 2022 (v194)，Visual Studio 17 2022 生成器，
`/MP` 并行编译，Release 配置
采集日期：2026-09-22（阶段 0）、2026-09-23（阶段 1、阶段 3）
提交：阶段 0 完成时；阶段 1 完成时（`IDocumentView` 抽取落地后）；
阶段 3 完成时（三库拆分落地后，`cmake --build` 加 `-- -m` 后测得，理由见下）

| 指标 | 阶段 0 | 阶段 1 | 阶段 3 |
|------|-------:|-------:|-------:|
| 全量构建耗时 (Release, 秒) | 144.3 | 129.5 | 179.2 |
| 改 `DmArc.cpp` 后增量 (秒) | 19.3 | 11.2 | 9.7 |
| 改 `GuiDocumentView.h` 后增量 (秒) | 65.3 | 25.4 | 18.7 |
| 改 `Datamodel.h` 后增量 (秒) | 120.9 | 90.3 | 141.2 |

几点值得注意：

- **改一个中心头文件几乎等于全量重建。** `Datamodel.h` 的增量是 120.9 秒，
  占全量 144.3 秒的 **84%**。它只被 27 个文件直接包含，但经 `DmVector.h`
  间接波及到几乎整棵树。这是阶段 4 去掉 162 项中心枚举的收益上限。
- **`GuiDocumentView.h` 的 65.3 秒占全量的 45%。** 它被 121 个文件包含，
  且拖着 `GL/glew.h`、`GL/gl.h`、`QOpenGLWidget` 等一整套重头文件。
  这是阶段 1 头文件瘦身的收益上限。
- **改一个叶子 .cpp 要 19.3 秒。** 单文件编译只占其中一小部分，其余是
  YiCAD.exe 与三个测试二进制的链接。阶段 3 拆库后，改 `DmArc.cpp` 只需
  重链 `YiCadModel`，这个数字应当明显下降。
- **阶段 1 落地后，`GuiDocumentView.h` 的增量从 65.3 秒降到 25.4 秒
  （-61%）。** 这是本阶段头文件瘦身 + `IDocumentView` 抽取的直接收益：
  106 个 Action 不再因为这一个头文件的改动而触发对 GL/Qt-OpenGL 重头文件的
  重新解析。全量构建也从 144.3 秒降到 129.5 秒（-10%），`DmArc.cpp` 增量从
  19.3 秒降到 11.2 秒（-42%，同一台机器上重复测量的噪声，不是阶段 1 的
  改动对象，仅供参考）。`Datamodel.h` 增量从 120.9 秒降到 90.3 秒
  （-25%）算是意外收获——它间接包含 `GuiDocumentView.h` 的路径也变轻了，
  但这不是阶段 1 的目标，阶段 4 去中心化之后还会有更大空间。
- **阶段 3 拆库后，两项"改一个文件"的增量指标按预期下降，但两项聚合型
  指标反而上升，根因是同一件事。** `DmArc.cpp`（叶子 .cpp，现属
  `YiCadModel`）增量从阶段 1 的 11.2 秒降到 9.7 秒，`GuiDocumentView.h`
  （现属仍与 UI/APP 合编的 `YiCadCore`）从 25.4 秒降到 18.7 秒——这两个
  文件改动后只需重编该文件所在的那一个目标再重链，不再像从前一样让
  `YiCadCore` 这一个大 OBJECT 库整体过一遍增量检查。但全量构建从 129.5 秒
  升到 179.2 秒，`Datamodel.h`（现属最底层的 `YiCadMath`，牵动
  `YiCadModel`/`YiCadPersistence`/`YiCadCore` 全部四层）的增量从 90.3 秒
  升到 141.2 秒，双双劣于阶段 1、逼近或超过阶段 0。
  原因是 Visual Studio/MSBuild 生成器按 `ProjectReference` 的声明顺序构建
  项目：`YiCadModel` 要等 `YiCadMath` 的 Lib 步骤完全结束才开始，即便
  `YiCadModel` 自己的编译阶段其实只需要 `YiCadMath` 的头文件、并不需要它
  已经归档成 `.lib`。16 核的机器上，`YiCadMath`（约几十个文件）不够填满
  所有核心时，`YiCadModel`/`YiCadPersistence` 却因为这条强制顺序而不能提前
  插空编译——这是把一个足够大的 OBJECT 库拆成一条有依赖顺序的静态库链后，
  在这一个生成器上必然出现的代价，不是配置疏漏。
  已确认并处理的一半：脚本原先调用 `cmake --build` 时没有 `-- -m`
  （solution 级并行），阶段 0/1 因为当时只有一个大目标，这个参数可有可无；
  拆库之后补上后，全量构建从未加时的 189.7 秒降到本表的 179.2 秒，
  `tools/measure_build.ps1` 与 CI 的构建步骤都已同步加上。
  剩下这部分差距（129.5 → 179.2、90.3 → 141.2）目前判断只能通过换生成器
  （比如 Ninja，能做到指令级而非项目级的调度）解决，超出本阶段范围，留给
  未来评估；日常开发里更常触发的是 `DmArc.cpp`/`GuiDocumentView.h` 这一类
  改动而不是改 `Datamodel.h` 或者从空构建目录整个重建，所以两项确有改善的
  指标更能代表典型的开发体验。

---

## 6. 代码规模与耦合指标

这一组不需要跑程序，随时可复现，用来衡量阶段 1、3、4 的结构性收益。

```bash
# 源文件数与分区
python tools/regen_source_lists.py --check

# 分层违规
python tools/check_layering.py
```

| 指标 | 阶段 0 基线 | 目标阶段 | 阶段 3 实际 |
|------|------------:|---------|------------:|
| 源文件总数 | 857 | — | — |
| `GuiDocumentView.h` 被引用的文件数 | 122 | 阶段 1 降低 | — |
| `DM::ActionType` 枚举项数 | 162 | 阶段 4 清零其命令 ID 职责 | — |
| `UIActionHandler.cpp` 的 `case` 数 | 153 | 阶段 4 降到 0 | — |
| kernel 反向依赖 ui 的文件数 | 3 | 阶段 3 降到 0 | 0（`tools/check_layering.py` 白名单已清空） |
| 库与可执行目标数（不含插件） | 2（YiCadCore + YiCAD） | 阶段 3 升到 8 | 5（`YiCadMath`/`YiCadModel`/`YiCadPersistence`/`YiCadCore`/`YiCAD`）。未达 8：`Render`/`Interaction`/`UI`/`APP` 四个分区存在真实双向依赖，无法各自拆成独立库，详见 `doc/ARCHITECTURE_EVOLUTION_PLAN.md` 6.7 节 |
| 自动化测试用例数 | 133（130 启用 + 3 DISABLED） | 持续增加 | 168（133 + `tests/interaction` 35 个，阶段 2 已引入，阶段 3 未新增用例） |

几点口径说明：

- 源文件总数 857 = 方案附录 A 基线 `d8e0be5` 的 853，加上阶段 0 新增的
  `YiCadLog.{h,cpp}`、`ScopedTimer.{h,cpp}`。以
  `python tools/regen_source_lists.py --check` 的输出为准。
- `GuiDocumentView.h` 这一行是 **122** 而非方案附录 A 的 121：下面的命令数的是
  「提到该头文件的文件数」，把 `GuiDocumentView.cpp` 自己也算进去了。口径不同，
  趋势可比，重测时用同一条命令即可。
- 3 个 DISABLED 用例对应方案 3.5 节记录的 B1、B3、B4 三个缺陷，
  修复后去掉 `DISABLED_` 前缀即为验收。

复现耦合指标的命令（在仓库根执行）：

```bash
grep -rl 'GuiDocumentView\.h' --include=*.h --include=*.cpp YiCAD/src | wc -l
grep -c 'case ' YiCAD/src/shell/UIActionHandler.cpp   # 分层重组 S5 之前在 YiCAD/src/ui/
```

---

## 7. 分层重组方案（`LAYER_RESTRUCTURE_PLAN.md`）

S0 记录起点，S2（目录重组、`YiCadPersistence` 并入 `YiCadModel`）与 S6（`YiCadCore` 拆成四个库）之后各追加一列。
同样不要覆盖旧列。

### 7.1 构建指标

采集环境：Windows 11 Pro 22621，16 逻辑核，MSVC 19.38.33139（v143 工具集），Visual Studio 17 2022 生成器，
`/MP` 加 `cmake --build ... -- -m`，Release 配置，CMake 4.1.2（CLion 自带）
采集日期：2026-09-26（S0、S2），2026-09-27（S6）
提交：S0 为 `17aaeb5` 加 S0 的测试（产品代码与 `17aaeb5` 相同）；S2 为 `5e5a44b`；S6 为 S6 的提交

| 指标 | S0（起点） | S2 | S6 | S6，Render/Application 加预编译头 | S6，Ninja 生成器 |
|------|----------:|---:|---:|---:|---:|
| 全量构建耗时 (Release, 秒) | 203.2 | 191.7 | 218.7 | 205.6 | 182.3 |
| 改 `DmArc.cpp` 后增量 (秒) | 10.9 | 10.9 | 13.8 | 11.2 | 8.1 |
| 改 `GuiDocumentView.h` 后增量 (秒) | 21.5 | 21.3 | 33.6 | 27.7 | 30.3 |
| 改 `Datamodel.h` 后增量 (秒) | 163.4 | 141.1 | 180.4 | 160.9 | 149.0 |

测法与第 5 节不同的地方：

- 在单独的构建目录 `build/measure-s0` 里测，没有用默认的 `build/Release`：
  `tools/measure_build.ps1` 测全量构建时会删掉整个构建目录再按缓存重新配置，而它重放的变量里没有
  `CMAKE_PREFIX_PATH`（本机的 Qt 6 靠它找到），也不指定 CMake 可执行文件。测之前先用与 `build/Release` 相同的
  生成器、工具链与变量配置好 `build/measure-s0`，再在同一个 PowerShell 里把 CLion 的 CMake 放到 `PATH` 最前、
  设好环境变量 `CMAKE_PREFIX_PATH`，然后运行：

  ```powershell
  powershell -ExecutionPolicy Bypass -File tools/measure_build.ps1 -BuildDir build/measure-s0
  ```

  S2、S6 复测照此办理，否则数字不可比。
- 与第 5 节阶段 3 的数字（179.2 / 9.7 / 18.7 / 141.2）相比全面变慢，但两者之间代码已经大变：阶段 4 把命令拆进
  13 个扩展（各自一个 OBJECT 库）、阶段 5 切到 Qt 6，`test_interaction` 从 35 个用例涨到 282 个。这一列只作为本方案的起点，
  不用来评价阶段 3 之后的改动。
- S2 在 `build/measure-s2` 里照上面的做法测，三个目标文件按新路径（`model/entity/`、`render/view/`、`base/core/`）。
  全量构建快了约 6%，改 `Datamodel.h` 后的增量快了约 14%，另两项持平。S2 只搬目录、没有改代码，差异应来自项目链少了一级
  （`YiCadPersistence` 并入 `YiCadModel`，Visual Studio 生成器少串行一个项目）；各项只测了一次，没有重复取均值。
- S6 测了三种构建，各在自己的构建目录里，照上面的做法（`measure_build.ps1` 此后支持 Ninja 构建目录，并在全量构建时重放
  缓存里的 `CMAKE_PREFIX_PATH`，不必再设环境变量）：
  - "S6"：`build/measure-s6`，S6 提交的库划分原样，Visual Studio 生成器。
  - "Render/Application 加预编译头"：`build/measure-s6-pch`，测量期间临时给 `YiCadRender`、`YiCadApplication` 各加一句
    `target_precompile_headers(... PRIVATE YiCadPch.h)`，测完恢复，未提交。
  - "Ninja 生成器"：`build/measure-s6-ninja`，S6 提交的库划分原样。conan 按 Visual Studio 生成的 toolchain 无条件设置
    `CMAKE_GENERATOR_PLATFORM`，Ninja 不接受，所以另跑一次
    `conan install ... --output-folder=build/conan-release-ninja -c tools.cmake.cmaketoolchain:generator=Ninja -c "tools.cmake.cmaketoolchain:user_presets="`
    （`--build=never`，只换 toolchain，不改包）；在 `vcvars64.bat -vcvars_ver=14.38` 的环境里配置与测量，编译器与另两种相同
    （MSVC 19.38.33139），ninja 用 CLion 自带的 1.12.1。
- S6 的第一次测量与另一个 worktree 里的全量构建重叠，Visual Studio 那组作废；表中是三种构建连续重测的结果，期间其他构建目录
  没有写入。第一次测量中没有重叠的两组与重测相差不到 3.5%（Ninja：180.9 / 8.1 / 30.1 / 148.0；加预编译头：212.8 / 11.3 / 27.5 / 160.1），
  可以当作重复性的参考。
- S6 比 S2 四项全部变慢：全量 +14%，`DmArc.cpp` +27%，`GuiDocumentView.h` +58%，`Datamodel.h` +28%。原因有二：Render、Application
  原先在 `YiCadCore` 里带着预编译头编译，拆出来后没有；Visual Studio 生成器按项目依赖串行构建，库链从三级变成六级。给 Render、
  Application 加回预编译头后四项都变快（-6%、-19%、-18%、-11%），但仍比 S2 慢。Ninja 按文件而不是按项目排队，同样不加预编译头时
  比 Visual Studio 生成器四项都快（-17%、-41%、-10%、-17%），全量与 `DmArc.cpp` 比 S2 还快（-5%、-26%），`Datamodel.h` 略慢（+6%），
  `GuiDocumentView.h` 慢 42%，主要慢在 Render、Application 没有预编译头（Ninja 加预编译头的组合没有测）。结论与决定见 `LAYER_RESTRUCTURE_PLAN.md` 10.5 节。

### 7.2 自动化测试用例数

`<二进制> --gtest_list_tests` 的条目数，含 `DISABLED_`。

| 测试二进制 | S0 之前（`17aaeb5`） | S0 | D8 修复步 | S1 | S2 | S3 | S4a | S4b | S4c | S4d | R4 修复 | 跨文档粘贴 | S5 | S6 |
|------------|--------------------:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| `test_math` | 72（1 DISABLED） | 72（1 DISABLED） | 83（1 DISABLED） | 83（1 DISABLED） | 83（1 DISABLED） | 83（1 DISABLED） | 83（1 DISABLED） | 83（1 DISABLED） | 83（1 DISABLED） | 83（1 DISABLED） | 83（1 DISABLED） | 83（1 DISABLED） | 83（1 DISABLED） | 83（1 DISABLED） |
| `test_geometry` | 44（1 DISABLED） | 44（1 DISABLED） | 44（1 DISABLED） | 44（1 DISABLED） | 44（1 DISABLED） | 44（1 DISABLED） | 46（1 DISABLED） | 46（1 DISABLED） | 46（1 DISABLED） | 46（1 DISABLED） | 46（1 DISABLED） | 50（1 DISABLED） | 50（1 DISABLED） | 50（1 DISABLED） |
| `test_persistence` | 27（1 DISABLED） | 58（20 DISABLED） | 64（5 DISABLED） | 63（4 DISABLED） | 63（4 DISABLED） | 63（4 DISABLED） | 63（4 DISABLED） | 68（4 DISABLED） | 65（2 DISABLED） | 65（2 DISABLED） | 68 | 68 | 68 | 68 |
| `test_interaction` | 282 | 282 | 282 | 282 | 282 | 291 | 292 | 293 | 305 | 305 | 305 | 306 | 308 | 308 |
| 合计 | 425（422 启用 + 3 DISABLED） | 456（434 启用 + 22 DISABLED） | 473（466 启用 + 7 DISABLED） | 472（466 启用 + 6 DISABLED） | 472（466 启用 + 6 DISABLED） | 481（475 启用 + 6 DISABLED） | 484（478 启用 + 6 DISABLED） | 490（484 启用 + 6 DISABLED） | 499（495 启用 + 4 DISABLED） | 499（495 启用 + 4 DISABLED） | 502（500 启用 + 2 DISABLED） | 507（505 启用 + 2 DISABLED） | 509（507 启用 + 2 DISABLED） | 509（507 启用 + 2 DISABLED） |

S0 新增的 19 个 `DISABLED_` 对应读回路径的缺陷 R1–R9（`LAYER_RESTRUCTURE_PLAN.md` 4.5 节，
`tests/persistence/test_persistence_document.cpp` 文件头部），修复后去掉前缀即为验收。
D8 修复步（同文档 4.6 节）修好 R1–R3、R5、R6、R9，启用其中 15 个，另新增 `XMLReader` 的 11 个、
按修订号读回的 5 个与当前线型的 1 个；剩下的 4 个依赖 R4、R7、R8。
S1 删除 `Persistence::dumpToStream`/`restoreFromStream` 时连同 `DISABLED_压缩流往返` 一起删掉，启用数不变。
S2 只搬目录、改构建脚本，用例不变；`test_persistence` 改链 `YiCadModel`（原 `YiCadPersistence` 已并入）。
S3 新增 `test_interaction` 的 `test_document_listener.cpp`（9 个）：文档经监听接口通知画布。
S4a 新增 `test_geometry` 的 `test_geometry_dimension.cpp`（2 个）：标注与引线从自己的文档取箭头块；
`test_interaction` 的 `test_modify_commands.cpp` 新增 1 个：粘贴别的图纸的标注时标注改归本文档。
S4b 新增 `test_persistence` 的 `test_persistence_filter_registry.cpp`（5 个）：格式注册表；`test_dxf_encoding.cpp`
新增 1 个：插件格式登记进格式注册表、卸载前注销。
S4c 把存盘策略的 7 个用例从 `test_persistence` 搬到 `test_interaction` 的 `test_document_file_service.cpp`，其中依赖 R7、R8 的
2 个启用，另加 5 个；`test_persistence` 另加 4 个（异常路径 1 个、只链接 Model 的读写 3 个）。
S4d 把 `test_geometry_dimension.cpp` 里依赖宿主当前文档的写法改为只涉及文档，用例数不变。
R4 修复（`LAYER_RESTRUCTURE_PLAN.md` 4.7 节）启用 `test_persistence_document.cpp` 里依赖 R4 的 2 个，另加 3 个：默认条目按文件里的属性读回、
文件缺少的默认条目读完补上、读进已有内容的文档时以文件为准。剩下的 2 个 `DISABLED_` 是 `test_math`、`test_geometry` 的既有数值缺陷，与读写无关。
跨文档粘贴修复（`LAYER_RESTRUCTURE_PLAN.md` 8.9 节）新增 `test_geometry` 的 `test_geometry_document_transfer.cpp`（4 个）：复制到剪贴板后与来源图纸无关、
粘贴时同名条目用目标文档的缺的才复制、粘贴预览不改动目标文档、复制进来的条目随事务撤销；`test_interaction` 的 `test_modify_commands.cpp` 新增 1 个：
粘贴提交只复制用到的图层并随撤销移除，原有的"粘贴别的图纸复制来的标注"改为来源图纸先关闭、图层与标注样式取本文档的。
S5（`LAYER_RESTRUCTURE_PLAN.md` 9.5 节）新增 `test_interaction` 2 个：文件命令经宿主管理的打开图纸（`IDocumentManager`）执行，没有打开的图纸时
未命名文档的自动保存副本名为空；未命名文档的两个自动保存用例改由假的 `IDocumentManager` 给名字，用例数不变。
S6（同文档 10.5 节）只拆库、改构建脚本，用例不变；`test_interaction` 改链 `YiCadShell`（带上 `YiCadUi` 及以下）与全部扩展库，
另外三个仍只链 `YiCadModel`。

---

## 8. 渲染层重构方案（`RENDER_PLAN.md`）

阶段 0 记录起点，之后每个阶段结束时追加一列，不要覆盖旧列。

### 8.1 运行期数据

由第 2 节的自动采集用例测得（`test_interaction` 的 `BaselineRuntimeTest`），单位毫秒，另有注明的除外；
除注明 GPU 的一行（阶段 1 起）外，耗时都是 CPU 侧的提交耗时，不等 GPU 完成。

采集环境：Windows 11 Pro 22621，AMD Ryzen 9 6900HX，32 GB 内存，NVIDIA GeForce RTX 3070 Ti Laptop GPU（驱动 32.0.15.6607；
用例打印的 GL_RENDERER 确认用的是这块独显），画布 1600×900，Release 配置
采集日期：2026-09-27
提交：`99c076a` 加渲染方案阶段 0 的改动（计数器、采集用例；旧渲染器只多了 `common.inl` 里的两句 `discard`，不影响这些数字）

| 指标 | 小图纸 (1k) | 中图纸 (50k) | 大图纸 (500k) |
|------|-----------:|-------------:|--------------:|
| 打开文档 `document.open` | 250.1 | 13,237.7 | 143,337.7 |
| 稳态帧 `render.paintGL` | 3.74 | 0.95 | 1.43 |
| 稳态帧绘制调用（次/帧） | 119 | 119 | 119 |
| 稳态帧上传（字节/帧） | 2,976 | 2,976 | 2,976 |
| 换高亮后首帧 `render.frameAfterHighlight` | 9.81 | 339.96 | 3,764.95 |
| 其中整图重建 `render.regen` | 5.99 | 338.77 | 3,763.48 |
| 换高亮后首帧上传（字节） | 1,200,960 | 61,535,716 | 624,925,328 |
| 点选 `snap.catchEntity` | 0.026 | 0.136 | 0.834 |
| 点选后首帧 `render.frameAfterSelection` | 7.36 | 369.55 | 4,029.69 |
| 全选框选 `selection.selectWindow` | 0.51 | 44.46 | 517.89 |
| 框选选中数 | 968 | 48,506 | 484,876 |
| 全选后首帧 `render.frameAfterSelection` | 13.04 | 607.28 | 6,818.60 |
| 虚拟交点 `snap.nearestVirtualIntersection` | 0.006 | 0.019 | 0.103 |

几点说明：

- **P1 的代价**：换高亮、点选一个实体、全选，都会让文档画笔整图重建（删除全部缓存、重新分组、重新上传）。大图纸上一次约 3.8～4 秒，
  全选后首帧 6.8 秒，每次重建上传约 625 MB。首帧耗时几乎全是 `render.regen`。这是渲染方案阶段 1（止血）与阶段 4 的主要改进对象，
  `RENDER_PLAN.md` 第 8 节性能标准里"点选一个实体、悬停高亮变化：不触发任何几何重建；帧耗时小于 5 ms"就是对着这几行定的。
- **稳态帧**只量 `paintGL` 在 CPU 上提交命令的时间：顶点早已在显存里，每帧只按画笔与类型发 119 次绘制调用，与图纸大小无关，
  所以大图纸的稳态帧反而不慢；上传的 2,976 字节是背景、网格、原点标记这些每帧重新上传的立即模式图元。GPU 侧的耗时没有量。
  小图纸的稳态帧比中、大图纸慢，原因没有追查（各只测了一次）。
- 打开文档包含 DXF 插件的解析与建实体，不含建窗口与首帧。
- 框选选中数比顶层实体数少约 3%：框取的是实体表范围外扩 1%，仍有实体没有完全落在框内（窗选只选完全落在框内的），原因没有追查。
- 三份基准图纸里的填充都以多段线为边界，现在画不出来（`RENDER_PLAN.md` 第 10 节阶段 0 的记录），所以这里没有测到填充的绘制开销。
- 同一台机器上第一次采集的结果与表中相差都在几个百分点以内（例如大图纸换高亮 3,793.85 与 3,764.95）。

#### 阶段 1（止血）

采集日期：2026-09-28，环境同上；代码：`0e5eee4` 加渲染方案阶段 1 的改动。

阶段 1 起采集用例多了稳态帧的 GPU 耗时（`GL_TIME_ELAPSED` 查询包住 `paintGL`，不含 Qt 之后的解析与合成）与局部重建、
场景底图的几行。为了对照，先用同一个用例在阶段 1 改动之前的代码（`0e5eee4`）上补测了一次：右列的 GPU 耗时取自那次
（小、中图纸为 0.26、0.76），其余各行与上表相差都在几个百分点以内（例如大图纸换高亮 3,781.19、点选后首帧 4,101.56），右列仍抄上表。
打开文档、点选、框选、虚拟交点与渲染无关，这次与上表相差也在几个百分点以内，不再列出。

| 指标 | 小图纸 (1k) | 中图纸 (50k) | 大图纸 (500k) | 大图纸，阶段 0 |
|------|-----------:|-------------:|--------------:|---------------:|
| 稳态帧 `render.paintGL` | 1.20 | 0.39 | 0.38 | 1.43 |
| 稳态帧 GPU 耗时（`GL_TIME_ELAPSED`） | 0.29 | 0.24 | 0.23 | 6.03 |
| 稳态帧绘制调用（次/帧） | 23 | 23 | 23 | 119 |
| 稳态帧上传（字节/帧） | 736 | 736 | 736 | 2,976 |
| 换高亮后首帧 `render.frameAfterHighlight` | 1.45 | 0.45 | 0.48 | 3,764.95 |
| 换高亮 20 次的整图重建次数 | 0 | 0 | 0 | 20 |
| 其中局部重建 `render.regenHighlight` | 0.170 | 0.041 | 0.040 | — |
| 换高亮 20 次的场景底图重画次数 `render.scene` | 0 | 0 | 0 | — |
| 换高亮后首帧上传（字节） | 776 | 776 | 776 | 624,925,328 |
| 点选后首帧 `render.frameAfterSelection` | 1.22 | 1.08 | 1.51 | 4,029.69 |
| 点选 20 次的整图重建次数 | 0 | 0 | 0 | 20 |
| 其中局部重建 `render.regenSelection` | 0.048 | 0.050 | 0.046 | — |
| 全选后首帧 `render.frameAfterSelection` | 9.32 | 390.72 | 5,100.01 | 6,818.60 |
| 其中局部重建 `render.regenSelection` | 7.04 | 388.56 | 5,094.74 | — |

几点说明：

- **稳态帧**（只移动光标的帧也是这样）：场景底图不重画，GPU 上只有一次多重采样拷贝加叠加层，三份图纸都在 0.2～0.3 ms，
  与图纸大小无关；阶段 0 每帧重画全部实体，大图纸 6 ms。背景与网格只在场景重画时上传，每帧上传的 736 字节是前景的立即模式图元。
- **换高亮**只重建高亮组（一个实体），不重画场景，首帧 0.5 ms 以内，与图纸大小无关。
- **点选**只重建选中组与夹点（一个实体），但选中组在场景底图里，场景要重画一次：CPU 提交 1.5 ms，GPU 上的重画没有单独量，
  开销与阶段 0 的稳态帧相当（大图纸约 6 ms）。
- **全选**仍要把 48 万个实体的顶点重新组成选中组并上传，开销随选中数，大图纸 5.1 秒（阶段 0 为 6.8 秒，省下的是普通组的重建）。
  要等阶段 4 的对象状态缓冲（`RENDER_PLAN.md` 第 4.3.7 节）才只改状态。平移、缩放同样每帧重画整个场景，GPU 开销随图纸大小，由阶段 6 的 LOD 处理。

#### 阶段 2（GI 边界）

采集日期：2026-09-28，环境同上；代码：`de671ca` 加渲染方案阶段 2 的改动。

阶段 2 起采集用例多了两行整图重建：显示后首帧，与文档修改后（改动任何实体都会整图重建）。阶段 1 之后换高亮、点选都不再整图重建，
表里原先唯一的整图重建一行（换高亮的 `render.regen`）从阶段 1 起是 0。右列是对照：打开文档与"文档修改后的整图重建"取阶段 0
（后者取阶段 0 换高亮的 `render.regen`，同是整图重建，那时实体里缓存着顶点；阶段 1 没有改整图重建），其余取阶段 1。

| 指标 | 小图纸 (1k) | 中图纸 (50k) | 大图纸 (500k) | 大图纸，对照 |
|------|-----------:|-------------:|--------------:|-------------:|
| 打开文档 `document.open` | 227.5 | 12,192.5 | 129,357.7 | 143,337.7 |
| 显示后首帧的整图重建 `render.regen` | 21.11 | 823.75 | 8,413.65 | — |
| 文档修改后的整图重建 `render.regen` | 5.26 | 242.23 | 2,526.87 | 3,763.48 |
| 稳态帧 `render.paintGL` | 1.23 | 0.39 | 0.37 | 0.38 |
| 稳态帧 GPU 耗时（`GL_TIME_ELAPSED`） | 0.32 | 0.23 | 0.22 | 0.23 |
| 换高亮后首帧 `render.frameAfterHighlight` | 1.19 | 0.44 | 0.42 | 0.48 |
| 点选后首帧 `render.frameAfterSelection` | 0.96 | 1.02 | 1.56 | 1.51 |
| 全选后首帧 `render.frameAfterSelection` | 8.37 | 279.88 | 3,262.61 | 5,100.01 |
| 其中局部重建 `render.regenSelection` | 6.32 | 277.54 | 3,259.22 | 5,094.74 |

几点说明：

- **实体里不再缓存顶点**（`RENDER_PLAN.md` 2.4 步）：每次整图重建都经 worldDraw 与适配器 `GLCacheWorldDraw` 重新生成全部顶点。
  没有缓存的情况下，文档修改后的整图重建反而比原先快约三分之一（大图纸 2.5 秒，原先 3.8 秒）：原先的开销主要在
  `getSubEntities()` 展平时为每个实体建链表、按画笔分组，而不在顶点本身。
- **样条例外**：离散一条样条要递归求 B 样条基函数，中图纸 2 千条样条约 0.6 秒，比其余 4.8 万个实体加起来还多；原先离散结果缓存在
  `DmSpline` 里（读图时算好）。适配器按曲线内容缓存离散结果（`GLCacheWorldDraw::NurbsSamples`，整图重建时标记—清除），
  所以只有显示后首帧要离散一遍，表里"显示后首帧"比"文档修改后"多出的主要就是这一项。原先这笔开销含在打开文档里，
  打开文档因此少了约 10%（大图纸 129 秒，原先 143 秒；另一部分是读图时不再算圆弧、圆、椭圆的顶点）。
- 全选后首帧同样少了展平与分组的开销，大图纸 3.3 秒（阶段 1 为 5.1 秒）。
- 稳态帧、换高亮、点选与阶段 1 相同。只有样条缓存之前测过一次，那次文档修改后的整图重建大图纸为 8.2 秒，全选后首帧 8.7 秒。

#### 阶段 4（GS 主体）

采集日期：2026-10-03，环境同上。旧渲染器一列是删除它之前同一份采集用例在旧渲染器下跑的（`YICAD_RENDERER=legacy`，新旧并存期间），
图形系统一列是删除旧渲染器、做完下面的优化之后的最终代码。阶段 4 起采集用例多了几行：

- **平移一帧**：每帧平移 1 像素，场景整幅重画（平移、缩放时每帧都这样），CPU 与 GPU 耗时。旧渲染器在阶段 1 之后不再每帧重画场景，
  这一行没有在旧渲染器下采集；对照取阶段 0 的稳态帧（那时每帧重画全部实体，CPU 3.74 / 0.95 / 1.43，GPU 中、大图纸为 0.76、6.03，小图纸 0.26，见阶段 1 一节）；
- **显存占用**：打开图纸、画过稳态帧之后可用显存（`GL_NVX_gpu_memory_info`）比之前少了多少，含窗口的帧缓冲，只用来新旧对比；
- **要求整图重建**：原先是"文档修改后的整图重建"（`documentModified`，旧渲染器改任何实体都整图重建）；图形系统按变更集更新，
  改为 `DmDocument::requestFullRebuild()` 后通知修改（REGEN），两种渲染器都整图重建；
- **移动一条直线**：在事务里移动一条直线、提交后画一帧（旧渲染器整图重建，图形系统只处理变更集并重编所在分块）。

`render.regen` 在图形系统里计文档图形模型的整次重建（建节点、编译、上传），预览等容器模型的重建不计入；
`render.regenSelection` 是改对象状态里的选中标记，`render.regenHighlight` 是重新收集叠加通道的高亮对象。

| 指标 | 小图纸，旧 | 小图纸，GS | 中图纸，旧 | 中图纸，GS | 大图纸，旧 | 大图纸，GS |
|------|-----:|-----:|-----:|-----:|-----:|-----:|
| 打开文档 `document.open` | 228.0 | 215.9 | 12,175.0 | 11,644.0 | 130,014.5 | 131,116.6 |
| 显示后首帧的整图重建 `render.regen` | 21.72 | 11.06 | 831.70 | 558.47 | 8,598.19 | 5,969.72 |
| 稳态帧 `render.paintGL` | 1.36 | 0.39 | 0.38 | 0.45 | 0.36 | 0.63 |
| 稳态帧 GPU 耗时 | 0.83 | 0.08 | 0.24 | 0.65 | 0.22 | 0.30 |
| 稳态帧绘制调用（次/帧） | 23 | 2 | 23 | 2 | 23 | 2 |
| 稳态帧上传（字节/帧） | 736 | 2,656 | 736 | 2,656 | 736 | 2,656 |
| 平移一帧 `render.paintGL` | — | 0.56 | — | 0.71 | — | 2.27 |
| 平移一帧 GPU 耗时 | — | 0.27 | — | 1.07 | — | 7.27 |
| 显存占用（MB） | 308.1 | 300.7 | 346.4 | 315.3 | 905.1 | 446.7 |
| 要求整图重建后的整图重建 `render.regen` | 5.26 | 3.69 | 239.02 | 252.32 | 2,616.91 | 4,141.73 |
| 移动一条直线后首帧 `render.paintGL` | 7.75 | 0.99 | 237.74 | 1.15 | 2,517.63 | 3.11 |
| 移动 20 次的整图重建次数 | 20 | 0 | 20 | 0 | 20 | 0 |
| 其中处理变更集 `render.gsChanges` | — | 0.010 | — | 0.015 | — | 0.017 |
| 其中编译分块 `render.gsCompile` | — | 0.391 | — | 0.577 | — | 0.761 |
| 换高亮后首帧 `render.frameAfterHighlight` | 0.41 | 0.57 | 0.41 | 0.41 | 0.42 | 1.34 |
| 换高亮 20 次的整图重建次数 | 0 | 0 | 0 | 0 | 0 | 0 |
| 换高亮 20 次的场景底图重画次数 `render.scene` | 0 | 0 | 0 | 0 | 0 | 0 |
| 点选 `snap.catchEntity` | 0.025 | 0.024 | 0.105 | 0.113 | 0.739 | 0.875 |
| 点选后首帧 `render.frameAfterSelection` | 0.95 | 0.64 | 1.00 | 0.56 | 2.95 | 2.68 |
| 点选 20 次的整图重建次数 | 0 | 0 | 0 | 0 | 0 | 0 |
| 其中局部更新 `render.regenSelection` | 0.038 | 0.002 | 0.043 | 0.017 | 0.041 | 0.570 |
| 全选框选 `selection.selectWindow` | 0.55 | 0.40 | 42.33 | 35.50 | 541.30 | 553.07 |
| 全选后首帧 `render.frameAfterSelection` | 8.58 | 1.20 | 275.73 | 24.31 | 3,216.06 | 277.41 |
| 其中局部更新 `render.regenSelection` | 5.69 | 0.32 | 273.58 | 23.37 | 3,213.03 | 273.60 |
| 虚拟交点 `snap.nearestVirtualIntersection` | 0.006 | 0.006 | 0.009 | 0.008 | 0.029 | 0.029 |

对照第 8 节的门槛（大图纸，用户定的减项版，见 `RENDER_PLAN.md` 第 10 节阶段 4）：

| 门槛 | 结果 |
|------|------|
| 只移动光标的帧 CPU 小于 1 ms，与图纸大小无关 | 达到：稳态帧 0.39 / 0.45 / 0.63 ms |
| 点选、悬停不触发几何重建，帧耗时小于 5 ms | 达到：整图重建 0 次，悬停 1.34 ms、点选 2.68 ms |
| 修改一个实体：重建与上传小于 5 ms | 达到：移动一条直线后首帧 3.11 ms（含场景重画） |
| 全选：状态更新小于 50 ms | **没有达到**：273.6 ms（旧渲染器 3,213 ms）。几乎全花在逐个问选择集"是否选中"上，见下 |
| 显存不高于现状 | 达到：446.7 MB，旧渲染器 905.1 MB |
| 平移、缩放不比现状差 | **没有完全达到**：大图纸平移一帧 GPU 7.27 ms、CPU 2.27 ms，旧渲染器整幅重画为 6.03 ms、1.43 ms；中图纸 GPU 1.07 ms 对 0.76 ms；小图纸持平 |

几点说明：

- **修改一个实体**原先要整图重建（大图纸 2.5 秒），现在只重新记录这个实体的 GI 流、重编它所在的一个分块（0.76 ms）并重画场景。
  为此分块的预算从开发时的 512 KB 降到 64 KB，并且四叉树节点分裂之后留在本层的大实体超过预算时另开"溢出分块"（不然上层分块会无限长，
  改其中一个实体要重编几千个节点）。分块变小后命令变多（大图纸约 5 万条间接绘制命令、3 千多个分块），每个分块的命令改在编译时备好，收集时整段复制。
- **全选**：图形系统这边只是改 48 万个对象状态的选中位、上传 8 MB；耗时在问选择集上——`SelectionSet` 按 UUID 字符串存 ID（`unordered_set<DmId>`），
  对 50 万个对象各查一次哈希表、访问一次实体，每个约 0.5 µs。框选本身（`selection.selectWindow`，往同一个哈希表里插 48 万个字符串）也要 553 ms。
  要降到 50 ms 以内得改选择集的存法（按实体或槽位，或者把变化的部分通知出来），不在本阶段。本阶段去掉了 `std::hash<DmId>` 每次复制字符串（`DmId::asString()` 改返回引用）、
  选择集判断时复制 ID（`DmObject::getIdRef()`），选中多时遍历全部对象问"是否选中"、少时逐个找槽位，集合求差改用状态里的标记位。
- **平移一帧**：场景整幅重画，GPU 开销随图纸大小。开发中途大图纸是 35 ms，两处改动降下来：一是样条离散过密（每段转角超过 3° 时整段分成 15 份再递归，
  中图纸 2 千条样条离散出 70 万个点；`GiNurbs::sampleRecursive` 改为二分，仍保证每段转角不超过 3°，点数降到约 1/3），
  二是不显示线宽时场景里的线段按 GPU 的线图元画（每段 2 个顶点，片段只判划线），选中的线另按四边形加宽画在上面。
  剩下的差距主要是圆弧（解析的环带，每条 48 个顶点、片段逐采样点算覆盖）与字形实例；大图纸这类图元的数量与远看时的大小，要靠第 6 阶段的 LOD。
- **要求整图重建（REGEN）**大图纸比旧渲染器慢（4.1 秒对 2.6 秒），显示后首帧却快（6.0 秒对 8.6 秒）：首帧两边都要离散样条，REGEN 时旧渲染器的样条离散缓存还在，
  图形系统按变更集更新以后，整图重建只在读盘、REGEN、进出块编辑时发生。开发中途 REGEN 每次都清空样条缓存（大图纸 9.6 秒），已改为标记—清除。
- 稳态帧的绘制调用是 2（贴场景底图与叠加层各一次）；上传的 2,656 字节是每帧常量与叠加层顶点。中图纸稳态帧 GPU 0.65 ms 是这一次的波动（其他几次 0.08～0.09）。

#### 阶段 5（线型与显示语义）

采集日期：2026-10-04，环境同上；代码：`c889dba` 加渲染方案阶段 5 的改动。只采集了大图纸。左列抄阶段 4 的图形系统一列。

| 指标 | 大图纸，阶段 4 | 大图纸，阶段 5 |
|------|-----:|-----:|
| 打开文档 `document.open` | 131,116.6 | 134,989.3 |
| 显示后首帧的整图重建 `render.regen` | 5,969.72 | 6,132.45 |
| 稳态帧 `render.paintGL` | 0.63 | 1.10 |
| 稳态帧 GPU 耗时 | 0.30 | 0.10 |
| 稳态帧绘制调用（次/帧） | 2 | 2 |
| 稳态帧上传（字节/帧） | 2,656 | 2,656 |
| 平移一帧 `render.paintGL` | 2.27 | 2.93 |
| 平移一帧 GPU 耗时 | 7.27 | 8.42 |
| 显存占用（MB） | 446.7 | 552.5 |
| 要求整图重建后的整图重建 `render.regen` | 4,141.73 | 4,879.10 |
| 移动一条直线后首帧 `render.paintGL` | 3.11 | 3.16 |
| 移动 20 次的整图重建次数 | 0 | 0 |
| 其中处理变更集 `render.gsChanges` | 0.017 | 0.017 |
| 其中编译分块 `render.gsCompile` | 0.761 | 0.812 |
| 换高亮后首帧 `render.frameAfterHighlight` | 1.34 | 1.02 |
| 换高亮 20 次的整图重建次数 | 0 | 0 |
| 换高亮 20 次的场景底图重画次数 `render.scene` | 0 | 0 |
| 点选 `snap.catchEntity` | 0.875 | 0.850 |
| 点选后首帧 `render.frameAfterSelection` | 2.68 | 2.78 |
| 点选 20 次的整图重建次数 | 0 | 0 |
| 其中局部更新 `render.regenSelection` | 0.570 | 0.813 |
| 全选框选 `selection.selectWindow` | 553.07 | 635.64 |
| 全选后首帧 `render.frameAfterSelection` | 277.41 | 278.91 |
| 其中局部更新 `render.regenSelection` | 273.60 | 274.65 |
| 虚拟交点 `snap.nearestVirtualIntersection` | 0.029 | 0.029 |

几点说明：

- 三份基准图纸全是连续线与实心填充（`tools/gen_benchmark_drawings.py`），这里量的是线型的改动给不用线型的图纸添了多少开销，不是虚线本身的开销。
- **平移一帧** GPU 多 1.1 ms（约 15%），**REGEN** 多约 18%。同一天采集了两次：第一次顶点着色器对每个顶点都取图元记录的第三个纹素（分段参数）、
  都取线型表头，平移 GPU 8.41 ms、REGEN 4,690 ms；改为只在用得到时才取之后（表中这一次），平移 GPU 不变，所以差距不在这两次取数上。
  可能的来源有：图元记录由 32 字节扩到 48 字节（每条仍读两个纹素，但步长变大）；顶点到片段的平直插值变量由 2 个分量增加到 5 个；
  每个顶点都要算一遍画法（`strokeOf`，圆弧每条 48 个顶点）；REGEN 多出的是编译时为每条线定画法、给分块汇总各线型的最长线长。
  没有逐项拆开测量，用户认为差距不大，不再追查。阶段 4 没有完全达到的"平移、缩放不比现状差"一项因此又差了一些。
- **显存**两次采集分别为 474.1 与 552.5 MB，两次的数据布局相同、只差着色器，`GL_NVX_gpu_memory_info` 给的是整块显卡的可用显存，
  这一行在本机上波动大，判断不出阶段 5 增加了多少（图元记录每条多 16 字节，其余缓冲的布局没有变）。
- 稳态帧 CPU 两次分别为 0.81 与 1.10 ms，全选框选与点选的局部更新也在几次采集之间上下浮动，与渲染的改动无关（框选不经过渲染）。
- 曾想在同一天用阶段 4 的代码重测一次作对照，但那次临时构建没有编 DXF 插件，读图用的是另一个旧的 Debug 插件，
  读进来的实体不同（打开文档 71.5 秒、框选选中数 475,003），数字作废，没有列入。

#### 阶段 6（LOD 与大图纸）

采集日期：2026-10-05，环境同上；代码：`dc84d8b` 加渲染方案阶段 6 的改动。三份图纸都采集了，最右一列抄阶段 5 的大图纸。

阶段 6 起采集用例多了三行（第 2 节）：按预算渐进绘制（`RENDER_PLAN.md` 第 4.3.10 节）时平移一帧的 CPU、GPU 耗时，与停下以后
画完整幅用的帧数；其余各行画布都不限预算，每帧画完整幅，与以前各阶段可比。

| 指标 | 小图纸，阶段 6 | 中图纸，阶段 6 | 大图纸，阶段 6 | 大图纸，阶段 5 |
|------|-----:|-----:|-----:|-----:|
| 打开文档 `document.open` | 219.6 | 11,891.0 | 137,422.2 | 134,989.3 |
| 显示后首帧的整图重建 `render.regen` | 5.79 | 123.21 | 1,940.85 | 6,132.45 |
| 稳态帧 `render.paintGL` | 0.44 | 0.27 | 0.71 | 1.10 |
| 稳态帧 GPU 耗时 | 0.09 | 0.45 | 0.09 | 0.10 |
| 稳态帧绘制调用（次/帧） | 2 | 2 | 2 | 2 |
| 稳态帧上传（字节/帧） | 3,680 | 3,680 | 3,680 | 2,656 |
| 平移一帧 `render.paintGL` | 0.56 | 0.51 | 1.88 | 2.93 |
| 平移一帧 GPU 耗时 | 0.29 | 0.54 | 3.20 | 8.42 |
| 按预算平移一帧 `render.paintGL` | 0.41 | 0.45 | 1.76 | — |
| 按预算平移一帧 GPU 耗时 | 0.24 | 0.57 | 2.66 | — |
| 按预算停下后画完整幅的帧数 | 1 | 1 | 1 | — |
| 显存占用（MB） | 310.3 | 320.0 | 455.0 | 552.5 |
| 要求整图重建后的整图重建 `render.regen` | 4.15 | 143.82 | 2,162.73 | 4,879.10 |
| 移动一条直线后首帧 `render.paintGL` | 1.01 | 1.22 | 3.42 | 3.16 |
| 移动 20 次的整图重建次数 | 0 | 0 | 0 | 0 |
| 其中处理变更集 `render.gsChanges` | 0.010 | 0.014 | 0.017 | 0.017 |
| 其中编译分块 `render.gsCompile` | 0.502 | 0.689 | 0.873 | 0.812 |
| 换高亮后首帧 `render.frameAfterHighlight` | 0.40 | 0.40 | 1.39 | 1.02 |
| 换高亮 20 次的整图重建次数 | 0 | 0 | 0 | 0 |
| 换高亮 20 次的场景底图重画次数 `render.scene` | 0 | 0 | 0 | 0 |
| 点选 `snap.catchEntity` | 0.025 | 0.108 | 0.729 | 0.850 |
| 点选后首帧 `render.frameAfterSelection` | 0.51 | 0.59 | 2.93 | 2.78 |
| 点选 20 次的整图重建次数 | 0 | 0 | 0 | 0 |
| 其中局部更新 `render.regenSelection` | 0.003 | 0.016 | 0.710 | 0.813 |
| 全选框选 `selection.selectWindow` | 0.41 | 34.57 | 486.61 | 635.64 |
| 全选后首帧 `render.frameAfterSelection` | 1.06 | 25.20 | 285.78 | 278.91 |
| 其中局部更新 `render.regenSelection` | 0.30 | 24.22 | 280.99 | 274.65 |
| 虚拟交点 `snap.nearestVirtualIntersection` | 0.006 | 0.009 | 0.027 | 0.029 |

几点说明：

- **平移一帧** GPU 大图纸 8.42 → 3.20 ms、中图纸 0.54 ms，都低于阶段 4 门槛对照的旧渲染器整幅重画（6.03、0.76 ms）；
  大图纸的 CPU 1.88 ms 仍比旧渲染器的 1.43 ms 慢约 0.4 ms。降下来的是字形（字高不到 2 像素的整组不发出）与圆弧
  （屏幕半径不到 4 像素的整段改用每条 6 个顶点的程序），顶层细线不受 LOD 影响。分步与原因见 `RENDER_PLAN.md` 第 10 节阶段 6。
- **显示后首帧的整图重建** 6.1 → 1.9 秒、**REGEN** 4.9 → 2.2 秒，来自并行记录 GI 流与并行编译分块（本机 16 个硬件线程）。
  REGEN 仍比首帧慢，可能是先在当前线程上逐个释放 50 万个节点，没有查。
- **按预算**的两行与整幅重画的两行画的内容相同：本机独显上三份图纸的场景都在 10 毫秒的预算内，一帧画完（停下后的帧数都是 1），
  两者的差是波动。分批画的路径要在更慢的显卡上才起作用，集成显卡本机没有，没有测。
- **稳态帧上传**多 1,024 字节：每帧常量加了 16 字节的 LOD 阈值（256 → 272 字节），按 256 对齐后四个通道各占 512 字节。
- 三份基准图纸里的实心填充都以多段线为边界，阶段 0 起一直画不出来（第 8.1 节阶段 0 的说明），阶段 6 改用 `DmRegion::getLoops`
  之后画出来了，本阶段各行的数字含它的开销。
- 中图纸稳态帧 GPU 0.45 ms 是这一次的波动（同一天的第一次采集 0.08 ms）。同一天采集了两次，第一次在最后一处只影响编译的改动
  （字形串里每个字形只查一次）之前：大图纸平移 GPU 3.19 与 3.20 ms、显示后首帧的整图重建 1,969 与 1,941 ms、REGEN 2,618 与 2,163 ms。
- 移动一条直线、换高亮、点选、框选各行与阶段 5 的差在几次采集的波动以内。

### 8.2 自动化测试用例数

`<二进制> --gtest_list_tests` 的条目数，含 `DISABLED_`。

| 测试二进制 | 阶段 0 之前（`99c076a`） | 阶段 0 | 阶段 1 | 阶段 2 | 阶段 3 | 阶段 4 | 阶段 5 | 阶段 6 |
|------------|------:|------:|------:|------:|------:|------:|------:|------:|
| `test_math` | 83（1 DISABLED） | 87（1 DISABLED） | 88（1 DISABLED） | 88（1 DISABLED） | 88（1 DISABLED） | 89（1 DISABLED） | 89（1 DISABLED） | 89（1 DISABLED） |
| `test_geometry` | 51（1 DISABLED） | 51（1 DISABLED） | 51（1 DISABLED） | 51（1 DISABLED） | 51（1 DISABLED） | 51（1 DISABLED） | 51（1 DISABLED） | 51（1 DISABLED） |
| `test_graphics` | — | — | — | 38（1 DISABLED） | 38（1 DISABLED） | 38（1 DISABLED） | 42（1 DISABLED） | 43 |
| `test_persistence` | 68 | 68 | 68 | 68 | 68 | 68 | 73 | 73 |
| `test_interaction` | 335 | 336 | 336 | 336 | 336 | 349 | 354 | 354 |
| `test_render` | — | 15 | 23 | 23 | 43 | 48 | 57 | 67 |

阶段 0 新增：`test_math` 的数量计数器与新计数器 4 个；`test_interaction` 的基线采集 1 个（不设 `YICAD_BENCHMARK_DIR` 时跳过）；
`test_render`（新）的环境检查 2 个与参考图纸出图比对 13 个（缺 SHX 字体时 `text_shx` 跳过，CI 上就是这样）。

阶段 1 新增：`test_math` 的新计数器 1 个；`test_render` 的增量更新 7 个（选择集、高亮集的局部重建与整图重建的图比对，
场景底图什么时候重画，整图重建复用图片纹理）与图片纹理缓存 1 个。

阶段 2 新增：`test_graphics`（新）的仿射变换 5 个、样条离散 3 个、GI 流的记录重放与序列化 6 个、各实体的 worldDraw 20 个
（其中以多段线为边界的实心填充 1 个是 `DISABLED_`，等 `Edge::getPoints` 修好后启用）、文字的字形串 4 个。
`test_render` 的用例数不变，`entities`、`entities_grid`、`entities_selected_highlighted`、`blocks` 四张基准图像更新（`RENDER_PLAN.md` 第 10 节阶段 2）。

阶段 3 新增：`test_render` 的 RHI 一致性 18 个（9 个用例各在两条上传路径上跑）、调试输出 1 个、环境检查 1 个；另有 CTest 项 `shader_toolchain`（8 个用例，不计入上表）。

阶段 4 新增：`test_math` 的图形系统计数器 1 个；`test_interaction` 的变更集 11 个与预览变换 2 个（修剪、延伸的两个用例改为检查临时隐藏集）；
`test_render` 的增量更新改测图形系统（原 7 个与图片纹理缓存 1 个，换成 13 个：选择集、高亮集、临时隐藏、预览变换、修改实体、大高亮集、修订号抽查、网格线等），
参考图纸 13 个改为只画图形系统，13 张基准图像全部重新生成（`RENDER_PLAN.md` 第 10 节阶段 4）。

阶段 5 新增：`test_graphics` 的实体线型比例 1 个、块参照不把线型比例交给块的内容 1 个、填充图案线 3 个（实线、虚线的相位与移动缩放、圆环），原"图案填充的线段在以填充为父实体的容器里"按新做法改写为其中的实线一个，净增 4 个；
`test_persistence` 的线型表保留记录 1 个、随层随块往返 2 个、实体线型比例与文档变量往返 1 个、实体线型比例的流往返 1 个；
`test_interaction` 的 DXF 随层随块 3 个、实体线型比例经组码 48 导出读回 1 个、改文档变量的命令登记变更 1 个；
`test_render` 的线型 8 个（`test_render_linetype.cpp`：随层随块记录 2 个、LTSCALE 只改每帧常量 1 个、超长线分段 1 个、编译器的分段参数 1 个、闭合曲线的整周期 1 个、填充图案线与 Model 切好的划线一致 2 个）
与参考图纸 `linetype_scale` 1 个，`linetypes`、`autocad_linetype` 两张基准图像更新；用 AutoCAD 核对后改正闭合曲线与块参照线型比例，`linetype_scale` 的基准图像重新生成（`RENDER_PLAN.md` 第 10 节阶段 5）。

阶段 6 新增：`test_graphics` 的实心填充的圆与圆弧边界按凸度交出 1 个；阶段 2 的以多段线为边界的实心填充（`DISABLED_`）启用；填充改交图案定义与边界后，原"图案填充的实线图案线按连续线逐条画"改写为"图案填充交出图案线的定义与边界"，虚线、圆环两个改为检查 Model 用共用的切线算法切出的划线，净增 1 个；`test_render` 的 LOD 8 个（`test_render_lod.cpp`：小字画细条、亚像素对象画点、小圆弧只画一个四边形、密填充按覆盖率画实心、椭圆重新离散、样条在后台重新离散、多线程与单线程编译一致、渐进绘制分几帧画完与一次画完一致）与 RHI 时间戳查询 2 个（1 个用例在两条上传路径上跑）；`entities`、`entities_grid`、`entities_selected_highlighted` 三张基准图像更新，只多了以多段线为边界的实心填充（`RENDER_PLAN.md` 第 10 节阶段 6）。
