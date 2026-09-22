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
| `selection.selectWindow` | `Selection::selectWindow` | 全选框选耗时 |
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

第 4、5 两项目前走的是全量扫描（P10），是第 9.1 节改造的直接对照组。

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
采集日期：2026-09-22
提交：阶段 0 完成时

| 指标 | 阶段 0 | 阶段 1 | 阶段 3 |
|------|-------:|-------:|-------:|
| 全量构建耗时 (Release, 秒) | 144.3 | | |
| 改 `DmArc.cpp` 后增量 (秒) | 19.3 | | |
| 改 `GuiDocumentView.h` 后增量 (秒) | 65.3 | | |
| 改 `Datamodel.h` 后增量 (秒) | 120.9 | | |

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

---

## 6. 代码规模与耦合指标

这一组不需要跑程序，随时可复现，用来衡量阶段 1、3、4 的结构性收益。

```bash
# 源文件数与分区
python tools/regen_source_lists.py --check

# 分层违规
python tools/check_layering.py
```

| 指标 | 阶段 0 基线 | 目标阶段 |
|------|------------:|---------|
| 源文件总数 | 857 | — |
| `GuiDocumentView.h` 被引用的文件数 | 122 | 阶段 1 降低 |
| `DM::ActionType` 枚举项数 | 162 | 阶段 4 清零其命令 ID 职责 |
| `UIActionHandler.cpp` 的 `case` 数 | 153 | 阶段 4 降到 0 |
| kernel 反向依赖 ui 的文件数 | 3 | 阶段 3 降到 0 |
| 库与可执行目标数（不含插件） | 2（YiCadCore + YiCAD） | 阶段 3 升到 8 |
| 自动化测试用例数 | 133（130 启用 + 3 DISABLED） | 持续增加 |

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
grep -c 'case ' YiCAD/src/ui/UIActionHandler.cpp
```
