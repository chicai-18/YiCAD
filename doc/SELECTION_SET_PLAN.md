# YiCAD 选择集移出 Model 方案

本文档给出把选择集从数据模型（实体上的标志位）移到 Application 层的执行方案。它展开的是
`LAYER_RESTRUCTURE_PLAN.md` 11.2 节"选择集移出 Model"一项：该方案的 D2 把 `Selection` 留在
`model/edit/`，只去掉它对视图的依赖，并注明"选择集连同状态移到 Application 是另一件事"。

> 本方案于 2026-09-27 提出，文中的行号与数量基于 `6378f1b` 实测。引用 `LAYER_RESTRUCTURE_PLAN.md`
> 的章节时写作"分层方案 x.y 节"。状态：第 1、2 步已完成（2026-09-27，见第 9 节）；第 7 节的决策全部已定（2026-09-27）。

---

## 1. 目标

选择集是编辑会话的状态，不是图纸数据。常规 CAD 程序都把它放在编辑器一侧：AutoCAD 的选择集属于
AcEd（编辑器），不在 AcDb（数据库）；FreeCAD 的 `Gui::Selection` 在 Gui 层，`App::Document` 不知道
哪些对象被选中。

完成后：

- Model 不再保存选中状态：实体没有选中接口，`FlagSelected` 不再置位；Model 的编辑操作接收显式的实体列表。
- 选中状态由 Application 层的 `SelectionSet` 保存，每份文档一个。
- Render 经只读接口判断实体是否选中，不认识 `SelectionSet`。
- 窗选、交叉选里的几何判断留在 Model，成为只读查询；`test_geometry` 仍只链接 `YiCadModel`。

---

## 2. 现状

### 2.1 状态存放

- 状态是实体的标志位 `FlagSelected`（`Datamodel.h:63`）。`DmEntity::setSelected` 拒绝选中锁定图层上的
  实体（`DmEntity.cpp:105`）；`isSelected` 同时要求实体可见（`DmEntity.cpp:133`），所以关掉图层后实体
  "不再选中"，但标志位还在。
- 11 个类重写 `setSelected`，把状态传给子实体：`DmEntityContainer`、`DmPolyline`、`DmHatch`、
  `DmBlockReference`、`DmText`、`DmMText`、`DmMTextParagraph`、`DmMTextLine`、`DmChar`、`DmDimension`、
  `DmLeader`。`DmBlockReference` 重建子实体时还把自己的选中状态抄给新子实体（`DmBlockReference.cpp:212`）。
- 子实体上的位只有两个读者：`EntityTable::getNearestSelectedRef` 用 `isParentSelected` 排除子实体
  （`EntityTable.cpp:325`）；`DmAtomicEntity::moveSelectedRef` 检查自身的位，但它连同
  `DmEntityContainer::moveSelectedRef` 没有外部调用者，是死代码。Render 按顶层实体分组
  （`DmCachePainter.cpp:217`），不读子实体的位。

### 2.2 读写点

| 层 | 位置 | 用法 |
|----|------|------|
| Model | `Selection` 的四个选择函数 | 置位，然后 `notifyDocumentModified()` 与 `requestRedraw()` |
| Model | `Modification` 的 `remove`、`copy`（含 `copyEntity`）、`move`、`moveRef` | 按"当前选中"操作，操作中取消或重新置位；`offset`、`deselectOriginals` 也读写，但没有调用者 |
| Model | `EntityTable::hasSelect`、`countSelect`、`getNearestSelectedRef`（`EntityTable.cpp:281`、`:292`、`:316`） | 查询 |
| Model | 撤销命令构造时取消选中（`EntityTableCmd.cpp:56`、`:137`、`:184`） | 保证撤销后未选中 |
| Model | 16 个实体的 `clone()`、`Modification::pasteEntity`，以及 `DmEntity::restoreStream`（`DmEntity.cpp:827`） | 副本、读回的实体设为未选中 |
| Render | `DmCachePainter::addGroupEntity` 分出选中组（`DmCachePainter.cpp:217`），`cacheSelectedPoints` 取选中实体的夹点（`:515`） | 绘制 |
| Application | `SelectTool`（构造 `Selection` 4 处）、`SelectFirstCommand`（`hasSelect`）、`EditTool`（最近夹点、计数）、`Preview::addSelectionFromDocument`（`Preview.cpp:72`）、`UIView`（橡皮擦，`UIView.cpp:423`） | 选择、查询 |
| Ui、Shell | `UICurrentActivePen`、`ApplicationWindow`、`UIActionHandler` | 查询、计数 |
| 扩展 | 约 30 个文件：修改、编辑、块、填充、图层、文字、测量、AI 上下文；其中约 20 处 `updateSelectionWidget(countSelect())` | 遍历选中实体、取消选中、计数 |
| 测试 | 10 个文件；`test_geometry_spatial_query` 有 5 个用例经 `Selection` 框选，`test_select_first_commands` 用得最多 | — |

合计约 90 个文件。

### 2.3 文件与撤销

`DmFlags::saveStream` 把整个标志字写出（`DmFlags.cpp:32`），选中位随之进了文件；读回时由
`DmEntity::restoreStream` 清掉（`DmEntity.cpp:827`，经 `isSelected()` 判断，不可见实体的位清不掉）。
修改操作的撤销快照也用这对函数（`EntityTableCmd.cpp:184` 起），所以撤销后被修改的实体总是未选中。

选中位在文件里没有意义，但确实写进去了。迁移后新文件这一位恒为 0，读旧文件时仍要清掉。

### 2.4 容易漏的用法

- **预览引导线**：`EditTool.cpp:253`、`ModifyMoveCommand.cpp:176`、`ModifyCopyCommand.cpp:245` 把预览里
  新建的引导线设为选中，借选中色绘制。它们不在文档里，文档的选择集管不到。
- **块编辑**：编辑块时 `DmDocument::getEntityTable()` 返回块的实体表（`DmDocument.h:78`）。现在"遍历当前表
  再查标志位"自然把模型空间与块内的选中隔开。进入块编辑只取消了被编辑的块参照（`BlockEditTool.cpp:106`），
  模型空间里其他实体的位留着，退出后仍显示为选中。
- **即时命令**经 `CommandContext`（`CommandRegistry.h:69`）拿文档与视图，不经 `ICommandHost`，要另给入口。
- **重绘通知**：选择改变现在借 `DmDocument::notifyDocumentModified()` 让视图重建缓存；
  `DmDocumentListener::documentModified` 的注释写着"实体、选中状态、画笔"（`DmDocumentListener.h:39`）。
  实现这个监听接口的只有 `GuiDocumentView`。

---

## 3. 目标结构

### 3.1 Model

- **删除**：
  - `DmEntity::setSelected`、`toggleSelected`、`isSelected`、`isParentSelected`，以及 2.1 节的 11 个重写和
    `DmBlockReference.cpp:212` 的抄写；
  - `getNearestSelectedRef`（`DmEntity` 与 `DmDimension`、`DmHatch`、`DmPolyline`、`DmSpline`、`DmText`
    的 5 个重写）、`moveSelectedRef`（死代码）；
  - 16 个 `clone()` 与 `pasteEntity` 里的重置；
  - `EntityTable` 的三个选中查询；撤销命令里的 3 处取消选中；
  - `Modification::offset`、`deselectOriginals`（没有调用者）。
- **`FlagSelected`**：编号保留、不再复用，注释写明已废弃。`DmEntity::restoreStream` 改为无条件
  `delFlag(DM::FlagSelected)`，旧文件里可能带着这一位。
- **几何查询**：`Selection` 里的几何判断移进 `EntityTable`，与已有的空间查询 `searchEntities`、
  `getNearestVirtualIntersection` 放在一起，返回命中的顶层实体，不改任何状态：
  - `entitiesInsideRect(corner1, corner2, types)`：完全落在矩形内的实体（窗选）；
  - `entitiesCrossingRect(corner1, corner2, types)`：落在矩形内或与矩形边界相交的实体（交叉选）。

  两者保留现在的候选取法（矩形盖住全部实体时顺序遍历，否则走空间树）、类型过滤、可见与已删除过滤、
  包围盒粗筛；交叉判断 `crossesWindow` 随之移进 `EntityTable.cpp`。`EntityTable` 本来就经
  `getNearestVirtualIntersection` 调用 `Information::getIntersection`，依赖不增加。
- **命名**：矩形是世界坐标里的两个角点，与界面窗口无关，所以按几何命名为 Rect。CAD 惯称的 window
  在 Qt 代码库里容易被读成界面窗口。实体上已有的 `DmEntity::isInWindow`、`DmTriangle::isInCrossWindow`、
  `DmSolid::isInCrossWindow` 用的也是 CAD 意义的 window，不在本方案改名。
- **不进 Model 的部分**：按图层取实体与几何无关，由 `SelectionSet::selectLayer` 自己遍历实体表；
  锁定图层的规则也不放在查询里，由选择集执行（3.3 节）。
- **`Modification`**：`remove`、`copy`、`move`、`moveRef` 改为接收实体列表，不再读写选中状态。操作后
  是否取消选中由调用方决定，保持现在的效果：`copy`、剪切、`move` 之后取消，`moveRef` 之后保持选中。
  这与 AcDb 的分工一致：数据库操作接收对象列表，选择集在编辑器一侧。

### 3.2 Render

- 新增只读接口 `ISelectionSource`（`render/view/`）：`virtual bool isSelected(const DmEntity& entity) const = 0;`。
- `DmCachePainter` 持有一个可为空的来源指针，`addGroupEntity`、`cacheSelectedPoints` 改为查它；为空时
  没有实体被选中。
- `GuiDocumentView` 增加两个设置函数，把文档的来源交给 `m_pDocumentPainter`，把预览的来源交给
  `m_pPreviewPainter`。
- Render 不认识 `SelectionSet`。选择改变后由 Application 调用视图已有的 `specifyDocumentModified()` 与 `redraw()`。

### 3.3 Application

**`SelectionSet`**（`application/SelectionSet.h`），每份文档一个：

- **构造**：`SelectionSet(DmDocument&)`，登记为文档监听者，`paintContainerChanged`（进出块编辑）时清空。
- **存储**：`DmId` 的集合。不存指针：`EntityTable::remove_direct`、`clear_direct`（`EntityTable.cpp:185`、
  `:197`）会真正 delete 实体，存指针会悬空。
- **修改**：`add`、`remove`、`toggle`、`clear`，以及 `selectWindow`、`selectLayer`、`selectAll`。后三个
  沿用 `Selection` 的函数名，这里的 window 指 CAD 的窗选模式：`selectWindow` 调用 `EntityTable` 的两个
  矩形查询，`selectLayer`、`selectAll` 自己遍历实体表。加入时拒绝锁定图层上的实体，沿用
  `DmEntity::setSelected` 的规则。
- **查询**：`contains`、`count`、`isEmpty`、`entities()`、`nearestRef`。`entities()` 按当前实体表的顺序
  返回，过滤已删除与不可见的实体，与现在 `isSelected()` 的语义和遍历顺序一致；`nearestRef` 取代
  `EntityTable::getNearestSelectedRef`。
- **通知**：`SelectionSet` 是 `QObject`，每次修改（一次框选算一次）后发 `changed()`；`UIView` 订阅它，
  重建缓存并重绘。
- 实现 `ISelectionSource`。
- 原 `Selection` 并入 `SelectionSet`，`model/edit/Selection.*` 在第 3 步删除（D5）。

**访问入口**：

| 调用方 | 入口 |
|--------|------|
| 交互命令 | `ICommandHost::selection()`（`UIView` 与 `tests/support/TestCommandHost.h` 实现），`BaseExclusiveCommand::selection()` |
| 即时命令 | `CommandContext` 增加 `SelectionSet* selection`（现有 9 处 `CommandContext{...}` 构造要补上） |
| 控件、扩展、AI 上下文 | `IDocumentManager::selection(const DmDocument*)`；`UICurrentActivePen` 已注入 `IDocumentManager` |
| `SelectTool`、`EditTool` | `UIView` 构造它们时传入 |

**持有者**：`AppDocument`，每份文档一个（D1，6.3 节）。

**`Preview`**：自己维护一个"按选中色绘制"的实体集合，引导线加入后登记进去。集合存指针即可：
预览实体归预览容器所有，随清空一起释放。`Preview` 实现 `ISelectionSource`，交给预览画笔；
`CommandPreview::entities()` 返回的也是它。`addSelectionFromDocument` 改为从 `SelectionSet` 取实体。

### 3.4 Ui、Shell、扩展

机械替换：

| 现在 | 改为 |
|------|------|
| `getEntityTable()->countSelect()` | `selection().count()` |
| `getEntityTable()->hasSelect()` | `!selection().isEmpty()` |
| 遍历实体表再查 `isSelected()` | 遍历 `selection().entities()` |
| 对文档实体 `setSelected(true/false)` | `selection().add(e)` / `selection().remove(e)` |
| 克隆后 `setSelected(false)` | 删除 |

---

## 4. 实施步骤

每步单独提交。每步都要求 `cmake --build --preset Release`、`ctest` 通过，`python tools/check_layering.py`
通过，并运行安装后的程序做一遍点选、框选、删除、移动、夹点编辑、块编辑进出。

| 步 | 内容 | 行为 |
|----|------|------|
| 0 | 定下第 7 节的决策（全部已定）；分层方案 11.2 节已指向本文档，并按 D3 记下其他会话状态（2026-09-27） | — |
| 1 | **Model 先改接口，状态仍在标志位**：`EntityTable` 新增 `entitiesInsideRect`、`entitiesCrossingRect`，`Selection::selectWindow` 暂时改为调用它们再置位；`Modification` 四个操作改为接收实体列表，调用方从标志位收集列表传入，并按 3.1 节自行取消或保持选中；删除 `offset`、`deselectOriginals`；`test_geometry_spatial_query` 的 5 个用例改为断言两个矩形查询的结果（"可反选"一半暂时仍经 `Selection`，第 3 步随它移到 `test_interaction`） | 不变 |
| 2 | **提取每文档对象 `AppDocument`**（D1）：新增 `application/AppDocument.*`，持有 `DmDocument` 与 `DocumentFileService`；`MDIWindow` 改为持有 `AppDocument`，其余职责不变。析构顺序照旧：`MDIWindow` 先删视图，再释放 `AppDocument`；`AppDocument` 内先释放文档服务（第 3 步起还有 `SelectionSet`），最后释放文档。见 6.3 节 | 不变 |
| 3 | **引入 `SelectionSet` 与 `ISelectionSource`，迁移全部调用方**：`Selection` 并入 `SelectionSet`，删除 `model/edit/Selection.*`；`SelectionSet` 内部暂时仍读写标志位、仍经文档通知重绘，只换入口，便于逐文件核对；`test_document_listener` 的 Selection 用例改用 `SelectionSet`，`test_geometry_spatial_query` 的"可反选"断言移到 `test_interaction`。改文件最多的一步 | 不变 |
| 4 | **换存储，删除实体上的选中接口**：`SelectionSet` 改存 `DmId`；按 3.1 节删除；`Preview` 改用自己的集合；`restoreStream` 无条件清位；选择改变改为经 `SelectionSet::changed()` 通知视图，`DmDocumentListener` 的注释去掉"选中状态"；改写测试：`test_document_listener` 的选择集用例改为断言 `changed()`、不再有文档通知，新增 `SelectionSet` 用例（锁定图层拒绝、不可见过滤、已删除掉出、实体被 delete 后不悬空、进出块编辑清空） | 见第 5 节 |
| 5 | 回填第 9 节执行记录；grep 确认 `FlagSelected` 只剩枚举定义与读回清除两处 | — |

---

## 5. 行为差异

第 1 至 3 步行为不变。第 4 步有以下差异：

| 场景 | 现在 | 之后 |
|------|------|------|
| 撤销、重做 | 被撤销命令触及的实体失去选中（2.3 节） | 已删除的实体自动掉出选择集；被撤销修改、仍然存在的实体留在选择集里（D2） |
| 进出块编辑 | 只取消被编辑的块参照；模型空间里其他实体退出后仍显示为选中 | 进、出都清空选择集 |
| 选择改变 | 触发 `DmDocumentListener::documentModified` | 不再触发；视图经 `SelectionSet::changed()` 重建缓存 |
| 保存文件 | 标志字里可能带选中位 | 选中位恒为 0 |

关掉图层后重新打开的效果不变：两种做法都保留选中记录、只在查询时过滤不可见实体，所以重新打开后
实体仍显示为选中。锁定图层时取消选中由图层扩展显式完成（`LayerExtension.cpp:221`、`:272`），
改为调用 `selection().remove()` 后效果也不变。

---

## 6. 选择集的持有者：MDIWindow 与 UIView

### 6.1 两者现在做什么

**`MDIWindow`**（`shell/`，`QMdiSubWindow` 的子类），每个图纸标签页一个，唯一的创建点是
`UITabDrawWidget.cpp:465`。它做四件事：

1. 创建并持有 `DmDocument`（构造函数虽然接受外部文档，但唯一的调用方传空，所以总是自己持有）；
2. 持有 `DocumentFileService`（`MDIWindow.cpp:66`）；
3. 创建 `UIView` 作为自己的内容控件，并安排析构顺序：视图先于文档服务，文档服务先于文档；
4. 打开、保存、另存为的槽函数，包括文件对话框与等待光标。

此外还有从 LibreCAD 的 `QC_MDIWindow` 带来的遗留：父子窗口列表、窗口编号、`operator<<`。
`addChildWindow` 在本文件之外没有调用者，子窗口列表恒为空。名字里的 MDI 也只剩形式：主窗口创建了
`QMdiArea`（`ApplicationWindow.cpp:381`），但没有任何 `addSubWindow`，标签页是 `UITabDrawWidget` 自己实现的。

**`UIView`**（`application/view/`），继承渲染层的画布 `GuiDocumentView` 并实现 `ICommandHost`。它装配
一个视口的交互：`ViewToolControl` 与导航、选择、夹点编辑三层工具，捕捉器、预览、命令总线；接收画布的
Qt 输入事件并分发。它不管文件，也不管文档的生死。

### 6.2 对照常规 CAD 程序

| 职责 | AutoCAD（ObjectARX） | FreeCAD | YiCAD 现在 |
|------|---------------------|---------|-----------|
| 图纸数据 | `AcDbDatabase` | `App::Document` | `DmDocument`（Model） |
| 一份打开的图纸（非控件，每文档一个） | `AcApDocument`，由 `AcApDocManager` 管理 | `Gui::Document` | 没有独立的类，职责在 `MDIWindow` 里 |
| 图纸窗口 | MDI 子框架窗口 | `Gui::MDIView` | `MDIWindow` |
| 视口的绘制与交互 | 图形视图，加上编辑器（AcEd）的命令、取点、选择 | `View3DInventor` 与其中的 `View3DInventorViewer` | `GuiDocumentView`（Render）加 `UIView`（Application） |
| 选择集 | 编辑器一侧，按文档 | `Gui::Selection`（全局单例，按文档名与对象名记录） | 实体标志位（本方案要改的） |

按这个对照，`UIView` 属于 Application 层的视口交互，位置是对的。`MDIWindow` 属于 Shell 层的窗口，
但它身上的"持有文档、持有文档服务"是"一份打开的图纸"的职责，常规 CAD 程序把这部分放在一个
不是控件的每文档对象里（`AcApDocument`、`Gui::Document`），窗口只负责显示。

### 6.3 要不要合并

不合并 `MDIWindow` 与 `UIView`：

- **数量关系不同**：一份文档对多个视口是常规 CAD 的常态（AutoCAD 的平铺视口与布局视口，FreeCAD 可以
  对同一文档开多个 3D 视图），它们共享同一个选择集。现在是一对一，合并会把一对一固化下来。
- **层不同**：`UIView` 在 Application 层，不应该管文件对话框和文档的生死；`MDIWindow` 在 Shell 层，
  把交互塞进去会让 Shell 之外的代码（命令、测试）拿不到交互入口。

该做的是把 `MDIWindow` 拆开：它身上属于"一份打开的图纸"的部分，提成 Application 层的每文档对象
`AppDocument`（`application/AppDocument.h`），持有 `DmDocument`、`DocumentFileService` 与本方案的
`SelectionSet`；`MDIWindow` 只剩窗口：持有 `AppDocument`、承载 `UIView`、处理文件对话框。
`IDocumentManager` 通过 `AppDocument` 提供选择集。父子窗口列表等遗留代码可以在这一步一并删除。

命名取自 AutoCAD 的 `AcApDocument`（Ap 即 Application），职责相同：一份打开的图纸，持有数据库。
与 `DmDocument` 按层一一对应：`DmDocument` 是 Model 层的图纸数据，`AppDocument` 是 Application 层的
打开的图纸。注意 FreeCAD 的 `App::Document` 是数据文档，对应的是这里的 `DmDocument`，不是 `AppDocument`。

---

## 7. 待确认决策

| 编号 | 决策 | 建议 | 状态 |
|------|------|------|------|
| D1 | `SelectionSet` 由谁持有 | 三种做法：A. `MDIWindow` 直接持有，与 `DocumentFileService` 并列，改动最少，但把 Application 层的每文档状态继续放在 Shell 的控件里，以后提取时要再搬；B. 先按 6.3 节提取每文档对象 `AppDocument`（第 2 步，约 5 个文件），选择集放在那里；C. 作为 `UIView` 的成员，最省事，但选择集变成按视口而不是按文档，与第 1 节的目标相悖。定为 B | 已定（2026-09-27） |
| D2 | 撤销、重做后选择集怎么办 | A. 接受第 5 节的差异，只让已删除的实体掉出，改动最小；B. 撤销、重做时清空整个选择集，但这会让"选中 X 后撤销与 X 无关的操作"也丢掉 X，而现在不会。定为 A | 已定（2026-09-27） |
| D3 | 范围 | 悬停高亮 `FlagHighlighted`（Render 同样读它，`DmCachePainter.cpp:221`）与块列表的 `DmBlock::selectedInBlockList` 也是会话状态。这次不动，已记入分层方案 11.2 节 | 已定（2026-09-27） |
| D4 | 约 20 处散落的 `GUIDIALOGFACTORY->updateSelectionWidget(...)` | 这次只换数据来源；改成订阅 `SelectionSet::changed()` 另做，因为空闲态与选择阶段的通知时机不同（`SelectTool.h:183`），要单独核对 | 已定（2026-09-27） |
| D5 | 命名与合并 | 每文档对象 `AppDocument`（理由见 6.3 节）；选择集 `SelectionSet`；Render 接口 `ISelectionSource`；几何查询为 `EntityTable::entitiesInsideRect`、`entitiesCrossingRect`，不设按图层的 Model 查询（理由见 3.1 节；曾暂名 `SelectionQuery`，因为把"选择"带回 Model、`Window` 易被读成界面窗口而放弃）。`Selection` 并入 `SelectionSet`：这是本文的选择，分层方案 11.2 节原文只说"`Selection` 随状态进 Application" | 已定（2026-09-27） |

---

## 8. 验证

```powershell
cmake --build --preset Release
ctest --test-dir build/Release -C Release --output-on-failure
cmake --install build/Release --config Release
python tools/check_layering.py
```

编译期保证：第 4 步之后，Model、Render 里调用 `isSelected`、`setSelected` 都会编译失败（接口已删除）；
在 Render 的文件里临时包含 `SelectionSet.h`，构建 `YiCadRender` 应报 C1083（找不到头文件）。

---

## 9. 执行记录

### 9.1 第 1 步：Model 先改接口

2026-09-27 完成，基线 `4ced7bc`，一个提交。行为不变。

**方案未写、执行时定的**：

1. **调用方怎么收集列表**：四处调用方（`EditTool::commit`、`EditCopyCommand::commitCopy`、`ModifyDeleteCommand::deleteSelection`、
   `ModifyMoveCommand::commitMove`）就地遍历实体表、取 `isSelected()` 的实体，与旋转、缩放等命令的写法相同。没有在 Model 或
   `Selection` 上加"取选中实体"的函数，第 3 步这几处换成 `selection().entities()`。
2. **复制、剪切、移动之后取消选中**：调用方调 `Selection(doc).selectAll(false)`。列表就是全部选中实体，与逐个取消等价
   （`selectAll(false)` 只动可见实体，`isSelected()` 本来就要求可见）。复制不经事务，原先靠 `copyEntity` 里的
   `notifyDocumentModified()` 让视图重建缓存，`selectAll` 同样经文档通知。第 3 步换成 `selection().clear()`。
3. **夹点落位之后保持选中**：`startModify` 构造的 `EntityTableModifyCmd` 会取消选中（2.2 节"撤销命令构造时取消选中"，第 4 步删除），
   原先由 `moveRef` 在提交前重新置位，现在由 `EditTool::commit` 在提交后对列表重新置位。`Transaction::commit` 调
   `regenerate()` 只把视图的缓存标为待重建，重建在下一次绘制时进行，所以显示的是恢复后的状态。重做快照（提交时存下的
   `m_newData`）原先带选中位，现在不带；`DmEntity::restoreStream` 对可见实体本来就清掉这一位，重做后的结果相同。
   第 4 步选择集按 `DmId` 保存、撤销命令不再取消选中，这段恢复随之删除。
4. `OffsetData` 只有 `Modification::offset` 用，一并删除。
5. 框选耗时埋点 `selection.selectWindow` 留在 `Selection::selectWindow`，量的是查询加置位，与 `BASELINE.md` 的定义一致；
   第 3 步随它进 `SelectionSet::selectWindow`。

**改动**：

| 位置 | 改法 |
|------|------|
| `EntityTable` | 新增 `entitiesInsideRect`、`entitiesCrossingRect`，共用私有的 `entitiesInRect`；候选取法、类型过滤、可见与已删除过滤、包围盒粗筛照搬 `Selection::selectWindow`，`crossesWindow` 移进 `EntityTable.cpp` 的匿名命名空间 |
| `Selection::selectWindow` | 调用两个矩形查询，对结果置位后照旧经文档通知 |
| `Modification` | `remove`、`copy`、`move`、`moveRef` 第一个参数改为 `const std::vector<DmEntity*>&`，不再读写选中位；`copyEntity` 去掉选中检查、取消选中与文档通知；删除 `offset`、`deselectOriginals`、`OffsetData` |
| 四处调用方 | 见上 1 至 3 条 |

**测试**：

- `test_geometry_spatial_query`：5 个用例改为断言矩形查询的结果，结果转集合时检查没有重复。"框选跳过不可见实体且可反选"拆成
  "矩形查询跳过不可见实体"与"框选可反选"，后者仍经 `Selection`，第 3 步移到 `test_interaction`。新增"矩形查询按类型过滤并跳过
  已删除实体"，两条取候选的路径各走一遍（已删除实体只打标记、留在空间搜索树里）。
- `test_document_listener`：方案没有提到的一例。"Modification复制选中实体时通知修改并重绘"断言的正是移走的行为，改为
  "Modification复制不改选中状态也不通知"。
- `test_geometry_document_transfer`：复制改为传入全部实体，不再为了能选中而先解冻、解锁隐藏线图层。
- 新增提交路径的用例，锁定提交后的选中状态与改前一致：`test_modify_commands` 的删除（撤销后恢复为未选中）、移动（取消选中）、
  复制到剪贴板与剪切（取消选中，复制时文档发出修改通知），`test_edit_tool` 的夹点落位（保持选中）。`test_edit_tool` 与
  `CommandTestFixture.h` 原说明"默认构造的 DmDocument 走事务会崩溃"不准确：崩溃的是不开事务直接调 `add()`，
  `test_modify_commands` 的粘贴用例早已走过事务，两处说明一并改正。

**验证**：`cmake --build --preset Release`、`ctest`（4 个测试二进制全部通过）、`python tools/check_layering.py` 通过；安装后启动程序，
能正常响应并正常退出。界面走查（点选、框选、删除、移动、夹点编辑、块编辑进出）由用户完成，没有发现问题。

### 9.2 第 2 步：提取每文档对象 `AppDocument`

2026-09-27 完成，基线 `2cc3d30`，一个提交。行为不变。

**方案未写、执行时定的**：

1. **`MDIWindow` 不再接受外部文档**：构造函数去掉 `DmDocument* doc` 参数与 `owner` 标志。唯一的调用方
   `UITabDrawWidget::createMdiWindow` 本来就传空（6.1 节），`AppDocument` 总是自己新建并持有文档。
2. **遗留代码一并删除**：按 6.3 节"父子窗口列表等遗留代码可以在这一步一并删除"，删掉 6.1 节列出的三项：父子窗口列表
   （`addChildWindow`、`removeChildWindow`、`getChildWindows`、`setParentWindow`、`getParentWindow`、`has_children`）、
   窗口编号（`id`、`idCounter`、`getId`）与 `operator<<`；`UITabDrawWidget` 里读它们的四处（关闭按钮与全部关闭时先关子窗口、
   关闭时的 `hasParent` 判断、`doClose` 的递归）随之删除。子窗口列表恒为空，`hasParent` 恒为假，所以行为不变。
   `doClose` 的 `activateNext` 参数原本只有递归调用传 `false`，现在没有读者，签名未改。没有连接的 `signalClosing` 与
   没有调用者的 `slotZoomAuto` 不在 6.1 节所列之内，未动。
3. **析构顺序**：`AppDocument` 的析构函数显式先释放文档服务、再释放文档，不依赖成员的声明顺序；`MDIWindow` 的析构函数
   照旧先删视图，再释放 `AppDocument`。
4. **`MDIWindow` 暂不暴露 `AppDocument`**：这一步没有使用者，`getDocument()` 改为返回 `AppDocument` 持有的文档，其余
   调用方不变。第 3 步 `IDocumentManager::selection` 需要时再加访问器。
5. 打开、保存、另存为三个槽函数去掉对文档的判空（文档不会为空），头文件去掉不再需要的 `Datamodel.h` 与前置声明。

**改动**：

| 位置 | 改法 |
|------|------|
| `application/AppDocument.*`（新增） | 构造时新建文档、`initDoc()`，再以宿主的 `IDocumentManager` 构造 `DocumentFileService`；提供 `document()`、`fileService()`；不可复制 |
| `MDIWindow` | 持有 `std::unique_ptr<AppDocument>`，视图、文件槽函数经它取文档与文档服务；删除第 1、2 条所列 |
| `UITabDrawWidget` | 构造 `MDIWindow` 时不再传空文档；删除第 2 条所列的四处 |
| `AGENTS.md`、`YiCAD/CMakeLists.txt`、`tests/interaction/CMakeLists.txt` | 说明里加上 `AppDocument` |

**测试**：新增 `test_app_document.cpp`（`test_interaction`）两例：文档服务管理的是 `AppDocument` 持有的文档、`AppDocument`
释放后按这份文档找不到服务；宿主给的未命名文档名字经 `AppDocument` 传到文档服务（自动保存副本以它命名）。存盘策略本身的
用例仍在 `test_document_file_service.cpp`，未改。

**验证**：`cmake --build --preset Release`（没有新增警告）、`ctest`（4 个测试二进制全部通过）、`python tools/check_layering.py`
通过；安装后启动程序（新建第一张图纸即构造 `AppDocument`），能正常响应，关闭后退出码为 0。界面走查（点选、框选、删除、移动、
夹点编辑、块编辑进出，以及新建、打开、保存、关闭图纸）交由用户进行，用户确认后提交。
