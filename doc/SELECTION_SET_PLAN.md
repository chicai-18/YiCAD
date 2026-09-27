# YiCAD 选择集移出 Model 方案

本文档给出把选择集从数据模型（实体上的标志位）移到 Application 层的执行方案。它展开的是
`LAYER_RESTRUCTURE_PLAN.md` 11.2 节"选择集移出 Model"一项：该方案的 D2 把 `Selection` 留在
`model/edit/`，只去掉它对视图的依赖，并注明"选择集连同状态移到 Application 是另一件事"。

> 本方案于 2026-09-27 提出，文中的行号与数量基于 `6378f1b` 实测。引用 `LAYER_RESTRUCTURE_PLAN.md`
> 的章节时写作"分层方案 x.y 节"。状态：全部完成（2026-09-27，第 1 至 5 步，见第 9 节）；第 7 节的决策全部已定（2026-09-27）。

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
  `delFlag(DM::FlagSelected)`，旧文件里可能带着这一位。（后改：程序尚未发布，没有要兼容的旧文件，枚举值连同读回清除
  一并删除，见 9.5 节末。）
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
  `m_pPreviewPainter`。（第 4 步去掉了预览的那一个：预览里是临时实体，没有选中状态，见 9.4 节第 1 条。）
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

> 第 4 步没有照这一段做：引导线借选中色是唯一的用法，改为去掉引导线，`Preview` 不涉及选中，
> 不实现 `ISelectionSource`，见 9.4 节第 1 条。

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
| 删除后撤销 | 实体回来是未选中 | 不变（文档通知修改时剔除已删除的实体，9.4 节第 2 条） |
| 选中后撤销添加再重做 | 实体回来仍是选中 | 实体回来是未选中（撤销时已剔除） |
| 进出块编辑 | 只取消被编辑的块参照；模型空间里其他实体退出后仍显示为选中 | 进、出都清空选择集 |
| 取消全部（Esc，复制、移动之后等） | 只取消可见实体：关掉图层上的选中实体，重新打开后仍显示为选中 | 连不可见的一起取消（9.4 节第 3 条） |
| 按住 Shift 拖夹点、移动、复制 | 另画一条参考点到吸附点的连线，借选中色加粗 | 不再画这条线，吸附照旧（9.4 节第 1 条） |
| 复制命令的预览 | 文字、标注、块参照的克隆带着原实体的选中位，画成选中色 | 按普通颜色画（预览没有选中状态） |
| 选择改变 | 触发 `DmDocumentListener::documentModified` | 不再触发；视图经 `SelectionSet::changed()` 重建缓存 |
| 保存文件 | 标志字里可能带选中位 | 选中位恒为 0 |
| 大图纸全部选中 | 查询读标志位 | 按 `DmId` 查找，50 万实体全选时各项操作多约 0.3 s（9.4 节第 7 条） |

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

2026-09-27 完成，基线 `4ced7bc`，一个提交（`2cc3d30`）。行为不变。

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

2026-09-27 完成，基线 `2cc3d30`，一个提交（`b7c5260`）。行为不变。

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

### 9.3 第 3 步：引入 `SelectionSet` 与 `ISelectionSource`，迁移全部调用方

2026-09-27 完成，基线 `b7c5260`，一个提交（`96bca76`）。行为不变：选中状态仍是实体的 `FlagSelected` 位，`SelectionSet` 读写它、
修改后仍经文档通知监听者，只换入口。

**方案未写、执行时定的**：

1. **`UIView` 接收 `AppDocument*`**：构造函数的文档参数由 `DmDocument*` 改为 `AppDocument*`，画布照旧拿其中的文档，
   选择层、夹点工具、预览与命令拿其中的选择集。没有另加一个选择集参数，免得"文档与选择集同为空或同不为空"要靠调用方保证。
2. **入口的形式**：`ICommandHost::selection()`、`BaseExclusiveCommand::selection()` 返回指针，与同类的 `document()` 一致
   （不活动时为空）。`BaseExclusiveCommand::selection()` 是公开的：放置工具里有 7 处读选择集（倒角、圆角、两点打断、多段线加点与
   删点刷新计数，延伸取边界，修改实体属性选中被点的实体），工具经 `BasePlaceTool::command()` 取用；工具的文档与视图在构造时
   传入，给几十个工具的构造函数再加一个参数不划算。3.4 节表里的 `selection().count()` 因此写作 `selection()->count()`。
3. **`CommandContext::selection` 放在 `view` 之后**：带 `sender`、`entity` 的构造不补上就编译不过。源码里的构造是 10 处，
   不是 2.2 节说的 9 处（`UIActionHandler::activateCommand` 写作 `const CommandContext ctx{...}`）。读它的即时命令：图层的
   激活、锁定与全部锁定，选中实体信息，删除选择集（`delete_no_select`），文字的选择变化。`ModifyDeleteCommand::deleteSelection`
   与 `InfoSelectedCommand::run` 的参数随之换成（或加上）选择集。
4. **Shell 的取法**：`MDIWindow` 加 `getSelection()`（第 2 步说的"需要时再加访问器"）；`UIActionHandler` 加 `set_selection()`，
   在 `UITabDrawWidget` 设文档的三处一并设置；`ApplicationWindow` 本来就经当前图纸窗口取文档，选择集也经它取；
   `ApplicationWindowDocumentManager::selection()` 按文档找标签页的图纸窗口。
5. **AI 上下文**：`AIExtension` 在按钮回调里经 `documentManager()->selection(doc)` 取选择集，与文档一起经 `AIAssistant::show`、
   `AIPipeline` 传给 `ContextResolver`，与原先传文档指针的做法相同。
6. **每次修改都通知**：`SelectionSet` 的每个修改函数都经文档通知监听者（`notifyDocumentModified()` 与 `requestRedraw()`），
   包括 `add`、`remove`。原先直接调 `setSelected` 的地方（旋转、镜像、创建块、复制到图层、锁定图层、夹点落位后的恢复等）
   因此多了通知。监听者只有画布，收到后只把缓存标为待重建、请求一次重绘，这些地方随后本来就提交事务或重绘，界面上看不出
   差别。第 4 步改为每次修改发 `changed()`，调用处不用再动。`toggle` 沿用 `Selection::selectSingle`：锁定图层上的实体不变，
   也不通知。
7. **`SelectionSet` 的接口**：`contains` 接收 `const DmEntity*`，空指针为假；`selectWindow`、`selectLayer` 保留 `Selection` 的
   `select` 参数；`selectAll()` 不带参数，取消全部用 `clear()`；没有调用者的 `deselectLayer` 不再保留。`selectAll()` 自 `init`
   起就没有调用者，`selectLayer` 只有测试调用，3.3 节列了它们，照列保留。`count`、`isEmpty`、`nearestRef` 暂时委托给
   `EntityTable` 的三个选中查询，`entities()` 遍历当前实体表取 `isSelected()`；第 4 步改为按 `DmId` 实现、删除那三个查询。
8. **`Preview`**：构造参数由文档改为选择集（原先只有 `addSelectionFromDocument` 用文档），`CommandPreview` 同样，7 处
   `make_unique<CommandPreview>(document(), view())` 改为传 `selection()`。`Preview` 实现 `ISelectionSource`，暂时读实体的
   选中位；`UIView` 把自己的预览（选择层与夹点工具共用的那个）交给预览画笔。
9. **画布**：画笔在 `initializeGL` 里才创建，晚于 `UIView` 的构造，所以 `GuiDocumentView` 把两个来源存为成员，建画笔时交给它们；
   已有画笔时立即交给。`DmCachePainter::setSelectionSource` 同时把缓存标为待重建。`UIView` 析构时先把预览画笔的来源置空
   （预览随成员先于基类释放）。`GuiPreviewWidget`（块预览、多行文字编辑器）的画笔不设来源，没有实体按选中绘制：它们画的都是
   克隆，克隆出来的实体原先也是未选中。
10. **留到第 4 步的**：克隆后的 `setSelected(false)`（`Preview` 两处、复制到图层、嵌套块选择对话框）暂不删除：36 个 `clone()`
    里只有 2.2 节说的 16 个清掉选中位，文字、标注、块参照等的克隆带着原实体的位，现在删掉，选中的多行文字在移动预览里会画成
    选中色。预览引导线的三处 `setSelected(true)` 与 `Preview::isSelected` 读位也留着，第 4 步随"`Preview` 改用自己的集合"一起改。
11. **第 4 步要先定的一点**：一个视图有多个 `Preview` 对象（`UIView` 的一个，加上每个命令的 `CommandPreview` 各一个），共用
    视图的预览容器与唯一的预览画笔，而引导线除了夹点工具，还由移动、复制命令经自己的 `CommandPreview` 加入。现在 `Preview`
    读实体的位，交给画笔的是哪一个都一样；第 4 步各自维护集合后，画笔只认识 `UIView` 的那一个，移动、复制的引导线会失去选中色。
    3.3 节"`Preview` 实现 `ISelectionSource`，交给预览画笔"要补上多个预览怎么共用这一个来源。
12. 另一处第 4 步要注意的：`test_text_extension` 的"属性面板不可打断单击取消选中并结束"选中的是一个不在实体表里的多行文字，
    现在按位判断能通过；改存 `DmId`、`contains` 按当前实体表过滤后，要么把文字放进表，要么说明这种用法。
13. 只为选中查询包含的 `EntityTable.h` 随之去掉（`UIView`、`EditTool`、`SelectFirstCommand`、选中实体信息、编辑块、文字扩展）。

**改动**：

| 位置 | 改法 |
|------|------|
| `render/view/ISelectionSource.h`（新增） | 只读接口 `isSelected(const DmEntity&)` |
| `DmCachePainter` | 持有可为空的来源，`addGroupEntity`、`cacheSelectedPoints` 改为查它，为空时没有实体选中 |
| `GuiDocumentView` | `setDocumentSelectionSource`、`setPreviewSelectionSource`，见上第 9 条 |
| `application/SelectionSet.*`（新增） | 并入 `Selection`，见上第 6、7 条；框选耗时埋点 `selection.selectWindow` 随之进 `SelectionSet::selectWindow`（`ScopedTimer.h`、`BASELINE.md` 改名） |
| `model/edit/Selection.*` | 删除 |
| `AppDocument` | 持有 `SelectionSet`，析构时先于文档释放 |
| `ICommandHost`、`BaseExclusiveCommand`、`CommandContext`、`IDocumentManager` | 各加选择集入口，见上第 2、3 条 |
| `UIView`、`SelectTool`、`EditTool`、`SelectFirstCommand`、`Preview`、`CommandPreview`、`PlaceCommand` | 见上第 1、8 条；选择层与夹点工具的构造函数在文档之后加选择集 |
| `UICurrentActivePen`、`ApplicationWindow`、`MDIWindow`、`UIActionHandler`、`UITabDrawWidget` | 见上第 4 条 |
| 扩展（`ai`、`block`、`draw`、`edit`、`hatch`、`layer`、`measure`、`modify`、`text`，37 个文件） | 按 3.4 节机械替换 |
| `AGENTS.md`、`YiCAD/CMakeLists.txt`、`tests/interaction/CMakeLists.txt` | 说明里加上 `SelectionSet`、`ISelectionSource` |

**测试**：

- 夹具：`CommandTestFixture.h` 与自己装配的夹具（`test_edit_tool`、`test_exclusive_command_bus`、`test_select_first_commands`、
  `test_select_tool`）加一个 `SelectionSet`，交给预览、选择层、夹点工具与 `TestCommandHost`；`TestCommandHost` 实现
  `selection()`；`FakeDocumentManager` 按文档返回预设的选择集。
- 用例里对实体的 `setSelected`、`isSelected` 换成选择集的 `add`、`remove`、`contains`（8 个文件），带文档的 `CommandContext`
  补上选择集。
- `test_document_listener`：`Selection` 用例改为"选择集的各种修改都通知修改并重绘"，补上 `add`、`remove`、`selectAll`；复制用例
  经选择集选中后把计数清零，只数复制的通知。
- `test_geometry_spatial_query` 的"框选可反选"移到新增的 `test_selection_set.cpp`（`test_interaction`），同文件另加三例锁住现在的
  语义，第 4 步改存储后应照样通过：锁定图层上的实体选不中（`add`、`toggle`、`selectAll`、框选），不可见的实体不算选中、重新可见后
  仍选中，`entities()` 按实体表的顺序。
- `test_app_document` 加一例：选择集选的是 `AppDocument` 持有的文档里的实体。

**验证**：`cmake --build --preset Release`（没有新增警告）、`ctest`（4 个测试二进制全部通过，`test_interaction` 318 例）、
`python tools/check_layering.py` 通过；在 `DmCachePainter.cpp` 临时包含 `SelectionSet.h`，构建 `YiCadRender` 报 C1083，恢复后
通过。安装后启动程序（新建第一张图纸即构造 `AppDocument`、`SelectionSet`，并把选择来源交给两个画笔），能正常响应，关闭后退出码
为 0。界面走查（点选、框选、删除、移动、夹点编辑、块编辑进出）交由用户进行，用户确认后提交。

### 9.4 第 4 步：换存储，删除实体上的选中接口

2026-09-27 完成，基线 `96bca76`，一个提交（`b96b12d`）。行为差异见第 5 节。

**方案未写或与方案不同、执行时定的**（第 1 至 3、7 条与用户讨论后定）：

1. **预览不涉及选中，去掉引导线**：3.2、3.3 节原写"`Preview` 维护按选中色绘制的集合、实现 `ISelectionSource`、交给预览画笔"。
   这个集合只为一种用法：按住 Shift 拖夹点、移动、复制时，预览里另画一条参考点到吸附点的连线（代码里叫引导线），
   加入后 `setSelected(true)`，借选中色加粗绘制。9.3 节第 11 条记下的问题（一个视图有多个 `Preview` 共用一个预览容器与画笔，
   各自的集合只有一个能交给画笔）也由它而来。用户的判断是：预览是命令的临时实体，没有选中状态，只有文档里的实体有；
   这条线也没有必要显示。于是删掉三处引导线（`EditTool::updatePreview` 的 Shift 分支，`ModifyMoveCommand::previewMove`、
   `ModifyCopyCommand::previewCopy` 去掉 `showGuide` 参数），吸附照旧；`Preview` 不再实现 `ISelectionSource`，
   `GuiDocumentView::setPreviewSelectionSource` 删除，预览画笔不设来源（`DmCachePainter` 为空时没有实体按选中绘制）。
   `Preview`、`CommandPreview` 仍是每个命令一个，不需要共用。
2. **已删除的实体怎样掉出**：只在查询时过滤的话，删除后撤销，实体回来仍是选中。`SelectionSet` 作为文档监听者
   （私有继承 `DmDocumentListener`），在 `documentModified`（事务提交、撤销、重做都会发）时剔除已删除或已不在当前实体表里的 id，
   剔除了就发 `changed()`；查询时照样过滤已删除、不可见的实体。删除后撤销与改前一样是未选中；选中后撤销添加再重做，
   实体回来是未选中（改前是选中）。
3. **`clear()` 清空整个集合**：原 `selectAll(false)` 只取消可见实体，改为连不可见的一起取消。
4. **只记当前实体表里的实体**：`add`、`toggle` 拒绝 id 无效或 `EntityTable::find(id)` 不是它本身的实体。预览实体、刚克隆出来的
   实体 id 都是 `"0"`，不拒绝会彼此算作选中。`test_text_extension` 那一例（9.3 节第 12 条）改为把文字放进实体表。
5. **通知**：`SelectionSet` 是 `QObject`，每个修改函数调用后发一次 `changed()`（一次框选算一次；`toggle` 遇到锁定图层上的实体
   不变、也不发），时机与第 3 步经文档通知时相同，调用处不用动。`UIView` 构造时订阅它：`specifyDocumentModified()` 再 `redraw()`，
   与原先文档通知画布时做的一样。进出块编辑（`paintContainerChanged`）清空后也发。
6. **查询的实现**：`contains` 先判断集合是否为空（常态，画布重建缓存时对每个顶层实体都问一次），再看可见、未删除、id 是否在集合里；
   `count`、`isEmpty` 对集合里的每个 id 在当前实体表里查；`entities`、`nearestRef` 按实体表的顺序遍历，`nearestRef` 对选中实体取
   `getNearestRef`，距离相等时取在前的，与原 `EntityTable::getNearestSelectedRef` 相同。
7. **性能**：无头测 50 万条直线全部选中（Release）：逐个 `contains` 约 300 ms（读标志位约 10 ms），`count` 约 320 ms，全选约 450 ms，
   覆盖全图的框选约 360 ms，文档通知修改时的剔除约 290 ms；集合为空时逐个 `contains` 约 2 ms。开销主要在以 36 字符字符串为键的
   哈希查找（缓存未命中与字符串比较）。试过两种改法：`DmObject::getId()`、`DmId::asString()` 改为返回常量引用，只快约 10%；以实体指针为键、
   命中后比对 id，快约 40%，但离读标志位仍差一个数量级，约定也更绕。用户选定照方案存 `DmId`，两种改法都没有采用；GUI 实测觉得慢时
   再单独优化（比如 `DmId` 改为二进制 UUID）。
8. **块编辑**：`BlockEditTool::beginEditing` 原先先取消被编辑块参照的选中，进出块编辑时选择集自己清空，这一句删除，参数随之去掉。
9. `EditTool::commit` 夹点落位后重新选中的那段删除（9.1 节第 3 条预告的）：撤销命令不再取消选中，选择集按 id 记录，修改实体不影响它。
10. `Modification.cpp` 里注释掉的旧代码中还有几处 `setSelected`，是注释，未动。

**改动**：

| 位置 | 改法 |
|------|------|
| `DmEntity` | 删除 `setSelected`、`toggleSelected`、`isSelected`、`isParentSelected`、`getNearestSelectedRef`、`moveSelectedRef`；`restoreStream` 无条件清掉 `FlagSelected` |
| 11 个 `setSelected` 重写、5 个 `getNearestSelectedRef` 重写、`DmEntityContainer::toggleSelected`、两个 `moveSelectedRef` | 删除 |
| 16 个 `clone()`、`Modification::pasteEntity`、`DmBlockReference` 重建子实体时的抄写 | 删除对选中位的读写 |
| `EntityTable` | 删除 `hasSelect`、`countSelect`、`getNearestSelectedRef` |
| `EntityTableCmd` | 删除 3 处取消选中 |
| `Datamodel.h`、`DmDocumentListener.h` | `FlagSelected` 注明已废弃、编号保留；`documentModified` 的注释去掉"选中状态" |
| `SelectionSet` | 改存 `std::unordered_set<DmId>`，是 `QObject` 并私有继承 `DmDocumentListener`，见上第 2 至 6 条 |
| `UIView` | 订阅 `SelectionSet::changed()`；不再给预览画笔设来源 |
| `Preview`、`GuiDocumentView`、`ISelectionSource.h` | 见上第 1 条；`Preview` 两处克隆后的取消选中删除 |
| `EditTool`、`ModifyMoveCommand`、`ModifyCopyCommand` | 删除引导线（第 1 条）；`EditTool` 见第 9 条 |
| `CopyToLayerCommand`、`UINestedBlockSelectDialog` | 克隆后的取消选中删除 |
| `BlockEditTool`、`BlocksEditCommand` | 见上第 8 条 |

**测试**：

- `test_selection_set`：原有四例照样通过；新增六例：只记当前实体表里的实体（不在表里的、克隆的都不算），取消全部连不可见的一起清掉，
  已删除的实体掉出、删除后撤销回来是未选中（走事务与 `DmDocument::undo`，并断言剔除时发一次 `changed()`），被撤销修改的实体仍然选中
  （撤销、重做后都在），实体被 `remove_direct` 释放后查询不出错、同处新加的实体不算选中，进出块编辑时清空（块编辑时选的是块里的实体）。
- `test_document_listener`：选择集用例改为"各种修改发出 changed 而不通知文档"，逐个断言 `changed()` 的次数，文档监听者收不到通知；
  复制用例不再需要把选中产生的通知清零。
- `test_modify_commands`："复制到剪贴板后取消选中并通知视图"原断言文档发出修改通知（第 1 步时取消选中经文档通知），改为断言选择集
  发出 `changed()`、文档不再通知。
- `test_edit_tool`："按住Shift移动时多一条引导线"改为"只做角度吸附不另画引导线"，预览里只有被拖的实体。
- `test_text_extension`：见上第 4 条，并先断言文字已选中，免得用例空过。
- 测性能用的临时用例测完即删，没有提交。

**验证**：`cmake --build --preset Release`（改过的文件没有新增警告）、`ctest`（4 个测试二进制全部通过，`test_interaction` 325 例）、
`python tools/check_layering.py` 通过；在 `DmCachePainter.cpp` 临时包含 `SelectionSet.h`，构建 `YiCadRender` 报 C1083，恢复后通过。
grep 确认 `FlagSelected` 只剩枚举定义与 `DmEntity::restoreStream` 里的清除两处（第 5 步的检查项）。安装后启动程序能正常响应，
关闭后退出码为 0。界面走查（点选、框选、删除后撤销、移动、夹点编辑、块编辑进出，Shift 吸附）交由用户进行，用户确认后提交。

### 9.5 第 5 步：收尾

2026-09-27 完成，基线 `b96b12d`，一个提交。只改文档与一处测试注释，行为不变。

**检查**：

1. `grep -rnw FlagSelected YiCAD/src tests` 只剩两处：枚举定义（`Datamodel.h:63`，注明已废弃）与 `DmEntity::restoreStream` 里的
   无条件清除（`DmEntity.cpp:765`）。`FlagSelected1`、`FlagSelected2` 是另外两个位，见下"发现、未处理"。
2. 3.1 节删除的接口（`setSelected`、`toggleSelected`、`isSelected`、`isParentSelected`、`getNearestSelectedRef`、`moveSelectedRef`，
   `EntityTable` 的 `hasSelect`、`countSelect`）在源码与测试里没有调用，只剩注释掉的旧代码（`Modification.cpp`、`DmSpline`，9.4 节第
   10 条）。同名的都与实体无关：`ISelectionSource::isSelected` 及其实现 `SelectionSet`、`DmCachePainter` 查来源的私有函数、
   OpenGL 画笔的着色器参数、多行文字编辑器里表示"有文字被选中"的局部变量 `hasSelect`。
3. `Selection` 类在代码里只剩说明来历的注释（`SelectionSet.cpp`、`test_geometry_spatial_query.cpp`、`test_math_rtree.cpp`）。
   `ARCHITECTURE_EVOLUTION_PLAN.md` 的性能记录与分层方案 S2 至 S4 的执行结果里提到它，是当时的记录，不改。

**回填**：

1. 9.1 至 9.4 节补上各步的提交号；文首状态改为全部完成。
2. 分层方案：2.2 节目录去掉 `model/edit/` 的 `Selection`，在 `application/` 根目录加上 `AppDocument`、`SelectionSet`，在
   `render/view/` 加上 `ISelectionSource`，目录下注明改动的来由（`AGENTS.md` 说源码布局见该节）；11.2 节"选择集移出 Model"
   注明已完成；12 节 D2 注明后来的去向，决策本身照旧。
3. `test_math_rtree.cpp` 的文件说明提到 `Selection::selectWindow`，补上它现在是 `EntityTable` 的两个矩形查询。

**发现、未处理**：`DmAtomicEntity` 的端点选中（`setStartpointSelected`、`setEndpointSelected`、`isStartpointSelected`、
`isEndpointSelected`，读写 `FlagSelected1`、`FlagSelected2`）在定义之外没有调用者，是死代码。它不是选择集，本方案只管
`FlagSelected`，D3 所列的其他会话状态里也没有它，这次不删，另行处理。

> 后续（2026-09-27，另一个提交）：四个函数已删除，查 git 历史，它们从首次提交起就没有调用者。程序尚未发布，没有要兼容
> 的旧文件，所以 `FlagSelected`、`FlagSelected1`、`FlagSelected2` 三个枚举值也删除，编号不再保留；`DmEntity::restoreStream`
> 里对 `FlagSelected` 的无条件清除随之删除（3.1 节的做法作废）。

**验证**：`cmake --build --preset Release`（只重新编译了 `test_math_rtree.cpp`，构建目录与 `b96b12d` 一致）、`ctest`（4 个测试
二进制全部通过，`test_interaction` 325 例）、`python tools/check_layering.py` 通过。程序代码没有改动，没有重新安装走查。
