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

埋点位置（`YiCAD/src/kernel/debug/ScopedTimer.h` 里的 `counters` 命名空间）：

| 计数器 | 位置 | 对应附录 B 的行 |
|--------|------|----------------|
| `render.paintGL` | `GuiDocumentView::paintGL` | 稳态帧耗时 |
| `snap.catchEntity` | `Snapper::catchEntity` | 点选耗时 |
| `selection.selectWindow` | `SelectionSet::selectWindow` | 全选框选耗时 |
| `snap.nearestVirtualIntersection` | `EntityTable::getNearestVirtualIntersection` | 虚拟交点捕捉耗时 |
| `document.open` | 尚未接入 | 打开文档耗时 |

计数器累计次数、总耗时、最小、最大，由 `yicad::Profiler::report()` 一次性汇总
到 `render` 日志分类（Info 级别）。**不要逐帧打印**——那正是这套设施要替代的
问题（P11）。

### 操作步骤

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

> 尚未采集。需要在装有 GPU 与显示环境的开发机上按第 2 节的步骤手工完成——
> 帧耗时与框选耗时都依赖真实的交互，没法在无头环境里测。

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
