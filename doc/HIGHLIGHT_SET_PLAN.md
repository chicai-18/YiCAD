# YiCAD 高亮移出 Model 方案

本文档给出把临时高亮从数据模型（实体上的标志位）移到 Application 层的执行方案。它展开的是
`LAYER_RESTRUCTURE_PLAN.md` 11.2 节"其他会话状态移出 Model"里的高亮一项（第 0 步起单列为"高亮移出 Model"）：
`SELECTION_SET_PLAN.md` 的 D3 当时把它定为不动、留到之后。同一条里的块列表选中 `DmBlock::selectedInBlockList`
不在本方案（D5）。

> 本方案于 2026-09-27 提出，文中的行号与数量基于 `0834829` 实测。引用 `SELECTION_SET_PLAN.md` 的章节时写作
> "选择集方案 x.y 节"，引用 `LAYER_RESTRUCTURE_PLAN.md` 时写作"分层方案 x.y 节"。状态：第 0、1、2 步已完成（第 2 步的
> 界面走查待做，见 8.2 节），第 6 节的决策全部已定（2026-09-27）；第 3 步起尚未开始。

---

## 1. 目标

高亮与选择集一样是编辑会话的状态，而且更短命：它是命令进行中给用户看的反馈，命令结束就没有意义。
常规 CAD 程序都不把它放进图纸数据：

| 程序 | 做法 |
|------|------|
| AutoCAD（ObjectARX） | 入口在实体上（`AcDbEntity::highlight()`、`unhighlight()`），但它们是 const 函数，对象以只读方式打开即可调用；状态记在图形系统 AcGs 里，不进数据库，不存盘，不进撤销。空闲时的悬停预选（selection preview）归编辑器 AcEd |
| ODA | `OdDbEntity::highlight()` 同样是 const，转给 `OdGsModel::highlight()`，状态记在图形节点上；按路径标识对象，能指到块参照里的嵌套对象或子实体，可以指定高亮样式 |
| Open CASCADE | `AIS_InteractiveContext` 同时管悬停检测到的对象（动态高亮）与选中的对象，两种样式分开设置；动态高亮走即时重绘，在已画好的场景上叠一层 |
| FreeCAD | `Gui::Selection` 同时管预选（悬停）与选中，`App::Document` 不知道 |
| DS（`DimX/Source`） | 选择的权威状态在 `SelectionService`，由 `SelectionViewSync` 投影到各视图 HOOPS 的高亮集；操作器只管"命令局部的临时高亮"（`SelectionViewSync.h` 的类说明）；视图另有叠加用的 `HQWidget::highlightContainer()` |

共同点：状态在交互层或图形层；按用途分种类与样式；命令里的临时高亮随命令结束清掉；绘制是在场景上叠加。

完成后：

- Model 没有高亮：`FlagHighlighted`、`DmEntity::setHighlighted`、`isHighlighted` 与 9 个重写删除，克隆、撤销命令、
  读回里的清位随之删除。
- 高亮由 Application 层的 `HighlightSet` 保存，每个视图一个（D1），命令结束时由视图清空（D2）。
- Render 经只读接口 `IHighlightSource` 取要高亮的实体，不认识 `HighlightSet`。
- 高亮改变经 `HighlightSet::changed()` 通知视图，命令不再自己调 `specifyDocumentModified()`、`redraw()`。
- 预览不涉及高亮（D6），与选择集方案 9.4 节第 1 条"预览没有选中状态"一致。

---

## 2. 现状

### 2.1 状态存放

- 状态是实体的标志位 `FlagHighlighted = 16384`（`Datamodel.h:66`）。编号（1<<14）与注释"Entity is highlighted
  temporarily (as a user action feedback)"和 LibreCAD 的 `RS2::FlagHighlighted` 相同，是沿用下来的做法。
- 接口是 `DmEntity::setHighlighted`、`isHighlighted`（`DmEntity.h:134`、`:135`）。9 个类重写 `setHighlighted`，把状态传给
  可见的子实体：`DmEntityContainer`（`DmEntityContainer.cpp:155`）、`DmPolyline`（`DmPolyline.cpp:286`）、`DmText`
  （`DmText.cpp:325`）、`DmMText`（`DmMText.cpp:2178`）、`DmMTextParagraph`（`DmMTextParagraph.cpp:928`）、`DmMTextLine`
  （`DmMTextLine.cpp:527`）、`DmChar`（`DmChar.cpp:434`）、`DmDimension`（`DmDimension.cpp:348`）、`DmLeader`
  （`DmLeader.cpp:339`）；`DmSpline` 另有一段注释掉的重写（`DmSpline.cpp:1626`、`DmSpline.h:144`）。
- 子实体上的位没有读者：Render 只看顶层实体（`DmCachePainter.cpp:233`）。`isHighlighted` 其余的调用是
  `DmEntity::restoreStream` 自己清位（`DmEntity.cpp:761`）和角平分线命令清位前的判断（2.3 节）。

### 2.2 读写点

| 层 | 位置 | 用法 |
|----|------|------|
| Model | `DmFlags::saveStream` 写出整个标志字（`DmFlags.cpp:32`） | 高亮位随之进文件、进撤销快照 |
| Model | `DmEntity::restoreStream`（`DmEntity.cpp:761`，注释"导入实体不允许高亮"） | 读回（打开文件、撤销重做恢复快照）时清位 |
| Model | `EntityTableAddCmd::execute`（`EntityTableCmd.cpp:56`）、`EntityTableRemoveCmd::execute`（`:136`）、`EntityTableModifyCmd` 的构造（`:182`） | 清位；修改命令在存快照之前清，免得撤销后变成高亮 |
| Model | `Modification::trim`（`Modification.cpp:462`） | 同上，注释"先取消高亮，undo才不会变成高亮"；紧接着的 `startModify` 构造修改命令时已经会清，这一句重复 |
| Model | 16 个 `clone()`（`DmLine`、`DmArc`、`DmPolyline`、`DmHatch`、`DmImage` 等） | 副本清位；文字、标注、块参照、容器的 `clone()` 不清，副本带着原实体的位 |
| Render | `DmCachePainter::addGroupEntity`（`DmCachePainter.cpp:222`） | 可见的顶层实体未选中而高亮时进高亮组；高亮组画在普通组之上，用高亮色加宽（着色器的 `u_isHighlighted`）。文档画笔与预览画笔都这样分组 |
| Application | 无 | 选择层、夹点编辑工具都不用高亮 |
| 扩展 | 14 个文件里的 15 个放置工具，见 2.3 节 | 命令中的拾取反馈 |
| 扩展 | `UINestedBlockSelectDialog.cpp:48` | 克隆后清位 |
| 测试 | `test_modify_commands` 5 例（`:189` 单个偏移、`:230` 修剪、`:318` 多段线加点删点、`:356` 查询角度、`:394` 打断），`test_draw_line_commands` 1 例（`:356` 两圆公切线） | 断言实体的 `isHighlighted()` |

### 2.3 命令里的用法

分层方案 11.2 节（第 0 步之前）与选择集方案 D3 称它"悬停高亮"，不全面。空闲时没有悬停高亮，选择层、夹点编辑工具都不写它。
写它的是 15 个命令放置工具，用途是命令进行中的拾取反馈，分两种：

- **悬停**：当前这一步可选的、光标下的实体，随鼠标移动更换；
- **已选定**：前几步已经选中的对象，留到提交、退回或命令结束。

| 命令（工具） | 悬停 | 已选定 | 高亮改变后 |
|--------------|------|--------|-----------|
| 修剪 `ModifyTrimTool` | 选边界时光标下的实体 | 边界（可以多条） | 重建缓存 |
| 倒圆 `ModifyRoundTool`、倒角 `ModifyBevelTool` | 光标下的实体 | 第一个对象 | 重建缓存 |
| 查询角度 `InfoAngleTool` | 光标下的线 | 第一条线 | 重建缓存 |
| 单个偏移 `ModifySingleOffsetTool` | 光标下的实体 | 源实体 | 重建缓存 |
| 角平分线 `DrawLineBisectorTool` | 光标下的直线 | 第一、二条线 | 重建缓存 |
| 过点切线 `DrawLineTangent1Tool` | 光标下的圆 | — | 重建缓存 |
| 两圆公切线 `DrawLineTangent2Tool` | 光标下的第二个圆 | 第一个圆 | 重建缓存 |
| 垂直切线 `DrawLineOrthTanTool` | 光标下的圆 | 直线 | 重建缓存 |
| 多段线加点 `PolylineAddTool`、删点 `PolylineDelTool` | — | 多段线 | 重建缓存 |
| 打断 `ModifyCutTool` | — | 被打断的实体 | 只重绘 |
| 两切圆 `DrawCircleTan2Tool`、三切圆 `DrawCircleTan3Tool` | — | 已选的圆 | 只重绘 |
| 内切椭圆 `DrawEllipseInscribeTool` | 最后一条线（同时预览椭圆） | 前几条线：克隆放进预览、给克隆置位（`DrawEllipseInscribeCommand.cpp:250`） | 只重绘 |

几点要注意：

1. **刷新靠各命令自己**：置位后调 `view()->specifyDocumentModified()` 与 `redraw()`。前者让文档画笔下一帧执行
   `cacheAll()`（`DmCachePainter.cpp:98`）：删掉全部缓存、重新分组、重新生成整张图的 GL 数据。修剪、倒圆等悬停时，
   光标每换一个实体就重建一次整张图。
2. **只重绘的，高亮不会立即出现或消失**：`redraw()` 只是 `update()`（`GuiDocumentView.cpp:252`），缓存没有标为待重建，
   `DmCachePainter::draw` 照旧画旧缓存。打断、两切圆、三切圆选中对象后，被选的实体不变色，要等别的原因重建缓存才变；
   内切椭圆悬停同样如此，它把已选的线克隆进预览再置位，可能就是为了绕开这一点（预览每次都会重建）。结束时只清位、
   不重建缓存的，高亮留在屏幕上直到下一次重建：过点切线、角平分线（`onFinish` 只清位），两切圆、三切圆、内切椭圆
   （只重绘），打断（`onFinish` 不重绘）。以上按代码推断，第 2 步走查时实测（第 5 节）。
3. **清理靠各命令自己，有遗漏**：每个工具在 `onFinish`（`BasePlaceTool.h:128` 的说明就是"取消高亮等收尾"）、右键退回、
   提交后分别清自己记下的实体。有的依赖 Model 顺手清：倒圆、倒角提交后，第一个对象的高亮是 `startModify` 构造修改命令时
   清掉的（`EntityTableCmd.cpp:182`），工具只清自己最后记下的那个；修剪时把某条边界本身修剪掉，`Modification::trim` 把它
   移出边界列表（`Modification.cpp:478`），高亮同样由修改命令清掉。查询角度不改实体，没有这层兜底：先把光标移到第二条线上
   再点，量完回到第一步时第一条线仍是高亮，结束命令时 `onFinish` 也只清最后记下的那条，第一条线的位一直留着，存盘会写进
   文件（按代码推断）。
4. **子实体**：不少命令以 `DM::ResolveAll` 拾取（`Snapper.cpp:663`），拾到的可能是块参照、标注、文字里的子实体。置位后
   画笔只看顶层实体，所以不显示。
5. 两点打断 `ModifyCut2PTool` 从不置位，却在提交时清位（`ModifyCutCommands.cpp:182`）。

---

## 3. 目标结构

### 3.1 Model

删除：

- `FlagHighlighted`。程序尚未发布，没有要兼容的旧文件，枚举值直接删除，编号不保留，与选择集方案 9.5 节末对
  `FlagSelected` 的处理相同；
- `DmEntity::setHighlighted`、`isHighlighted` 与 9 个重写，`DmSpline` 里注释掉的那段；
- `DmEntity::restoreStream` 与 16 个 `clone()` 里的清位；
- `EntityTableCmd.cpp` 的 3 处、`Modification.cpp:462` 的 1 处清位。

### 3.2 Render

- 新增只读接口 `IHighlightSource`（`render/view/IHighlightSource.h`）：

  ```cpp
  /// @brief 要按高亮绘制的顶层实体：只含当前实体表里可见、未删除的
  virtual std::vector<DmEntity*> highlightedEntities() const = 0;
  ```

  与 `ISelectionSource` 的逐实体查询不同，这里交出列表（D3）：高亮通常只有一到几个实体，画笔据此组高亮组，
  不必对每个顶层实体查一次。
- `DmCachePainter` 持有可为空的来源指针。`regroup` 去掉 `isHighlighted()` 分支，遍历完实体后对来源给出的每个实体，
  未选中的放进高亮组，选中优先的规则不变。为空时没有实体高亮。
- `GuiDocumentView::setDocumentHighlightSource()`，做法同 `setDocumentSelectionSource`：画笔在 `initializeGL` 里才创建，
  先存为成员，建画笔时交给它（选择集方案 9.3 节第 9 条）。预览画笔不设来源（D6）。
- 高亮改变时文档画笔照旧 `cacheAll()`（D4）。Render 不认识 `HighlightSet`。

### 3.3 Application

**`HighlightSet`**（`application/HighlightSet.h`），每个视图一个（D1）：

- **构造**：`HighlightSet(DmDocument&)`，用文档的当前实体表解析 id。
- **存储**：`DmId` 的集合，理由同选择集（选择集方案 3.3 节：实体会被真正 delete，存指针会悬空）。集合很小，不像选择集那样
  监听文档剔除已删除的实体：查询时过滤，命令结束时整体清空。
- **修改**：`add(DmEntity*)`、`remove(DmEntity*)`、`clear()`。空指针、不在集合里的 `remove` 什么也不做，调用处不用先判断。
  `add` 只接受当前实体表里的实体（id 有效且 `EntityTable::find(id)` 是它本身，同选择集方案 9.4 节第 4 条），所以预览里的
  克隆、拾取到的子实体都不加入；后者与现在的显示效果相同（2.3 节第 4 条）。
- **查询**：`contains`、`entities()`，按当前实体表的顺序，过滤已删除与不可见的实体。
- **通知**：`HighlightSet` 是 `QObject`，内容真正改变时发 `changed()`。悬停时光标在同一实体上移动不会重复重建缓存。
  这一点与 `SelectionSet` 每次调用都发不同。
- 实现 `IHighlightSource`。

**持有与清空**：`UIView` 持有。构造时交给画布（`setDocumentHighlightSource`），订阅 `changed()`，重建缓存并重绘（与选择集
的订阅写在一起，`UIView.cpp:60`）；`onCommandFinished()`（`UIView.cpp:134`）里清空（D2）。成员声明在 `m_pCommandBus` 之前：
总线析构时结束命令、发 `commandFinished()`，高亮集这时还在。析构时先把画布的来源置空，基类画布析构时不再读它。

**访问入口**：

| 调用方 | 入口 |
|--------|------|
| 交互命令 | `ICommandHost::highlight()`（`UIView` 与 `tests/support/TestCommandHost.h` 实现）、`BaseExclusiveCommand::highlight()`，公开，与 `selection()` 相同（选择集方案 9.3 节第 2 条） |
| 放置工具 | `command().highlight()` |

即时命令、控件与扩展的其他部分都不用高亮，不加入口。

### 3.4 扩展

| 现在 | 改为 |
|------|------|
| `e->setHighlighted(true)`，再 `specifyDocumentModified()`、`redraw()` | `command().highlight()->add(e)` |
| `e->setHighlighted(false)`，再刷新 | `remove(e)` |
| `if (e && e->isHighlighted()) e->setHighlighted(false)` | `remove(e)` |
| 退回第一步、提交后逐个清自己记下的实体 | `clear()` |
| `onFinish` 里只为取消高亮写的部分 | 删除（D2）；修剪恢复可见等其他收尾保留 |
| 克隆后 `setHighlighted(false)` | 删除 |

只为高亮而调的 `specifyDocumentModified()`、`redraw()` 一并删除；同一处还为预览或别的原因刷新的，保留。

逐个要注意的：

- **倒圆、倒角、查询角度**：提交或输出结果、回到第一步时 `clear()`，不再依赖修改命令清掉第一个对象（2.3 节第 3 条）。
- **修剪**：提交后按边界列表重设高亮集，被修剪掉的边界已不在列表里。
- **内切椭圆**：已选的线直接加入高亮集，不再往预览里放置位的克隆（D6）。
- **角平分线**：删除 `isHighlighted()` 的判断。
- **两点打断**：删除提交时的清位（2.3 节第 5 条）。

---

## 4. 实施步骤

每步单独提交。每步都要求 `cmake --build --preset Release`、`ctest` 通过，`python tools/check_layering.py` 通过，
安装后运行程序走查第 7 节所列的命令。

| 步 | 内容 | 行为 |
|----|------|------|
| 0 | 定下第 6 节的决策（全部已定）；分层方案 11.2 节已指向本文档，并按 D5 记下不在本方案的会话状态（2026-09-27） | — |
| 1 | **引入 `HighlightSet` 与 `IHighlightSource`，还没有调用方**：新增两者；`UIView` 持有、交给画布、订阅 `changed()`、命令结束时清空；`ICommandHost`、`BaseExclusiveCommand` 加入口，`TestCommandHost` 实现；`DmCachePainter` 的高亮组暂时既收来源给出的实体、也收 `isHighlighted()` 的实体。新增 `test_highlight_set.cpp`（`test_interaction`）：只接受当前实体表里的实体、内容不变时不发 `changed()`、已删除与不可见的实体不交出、实体被 `remove_direct` 释放后查询不出错、命令结束时清空 | 不变 |
| 2 | **迁移 15 个工具**：按 3.4 节改；`DmCachePainter` 去掉 `isHighlighted()` 分支。2.2 节的 6 个测试改为断言高亮集；补上 2.3 节第 2、3 条的场景：打断选中后高亮集立即非空并通知视图，倒圆提交后清空，修剪掉一条边界后它移出高亮集，查询角度先悬停第二条线再点、量完后清空 | 见第 5 节 |
| 3 | **删除 Model 里的高亮**：按 3.1 节删除；grep 确认 `FlagHighlighted`、`setHighlighted`、`isHighlighted` 只剩注释掉的旧代码 | 不变 |
| 4 | 回填第 8 节执行记录；分层方案 2.2 节目录加上 `HighlightSet`、`IHighlightSource`，11.2 节注明高亮一项已完成；`AGENTS.md` 的源码布局说明加上它们 | — |

---

## 5. 行为差异

第 1、3 步行为不变。第 2 步有以下差异，前两行按代码推断，第 2 步走查时实测：

| 场景 | 现在 | 之后 |
|------|------|------|
| 打断、两切圆、三切圆点选对象，内切椭圆光标移到线上 | 被选的实体不变色，等别的原因重建缓存才变（2.3 节第 2 条） | 立即变为高亮色 |
| 按 Esc 或右键结束过点切线、角平分线、两切圆、三切圆、内切椭圆、打断 | 高亮留在屏幕上，直到下一次重建缓存 | 立即恢复原色 |
| 查询角度先悬停第二条线再点 | 量完后第一条线一直是高亮色，结束命令也不恢复（2.3 节第 3 条） | 量完回到第一步即恢复 |
| 任何命令结束（Esc、右键退出、启动别的命令、关闭图纸） | 各工具清自己记下的实体，漏掉的留着 | 视图清空整个高亮集 |
| 内切椭圆已选的线 | 预览里一条高亮色的克隆盖在原线上 | 原线本身显示为高亮色，看上去相同 |
| 倒圆、倒角提交，修剪掉一条边界 | 被修改的实体由修改命令清位 | 工具清空或移出高亮集，看上去相同 |
| 预览里的克隆 | 文字、标注、块参照等的克隆带着原实体的高亮位，预览画笔按高亮色画 | 预览不涉及高亮 |
| 命令进行中写出的文件（自动保存等） | 标志字里可能带高亮位，读回时清掉 | 不带 |
| 拾取到块参照、标注、文字里的子实体 | 置位，但不显示 | 不加入，不显示（不变） |
| 已选中的实体又被高亮 | 按选中色画 | 不变 |

---

## 6. 待确认决策

| 编号 | 决策 | 建议 | 状态 |
|------|------|------|------|
| D1 | 高亮集的作用域与持有者 | A. 每个视图一个，`UIView` 持有；B. 每份文档一个，`AppDocument` 持有，与 `SelectionSet` 并列；C. 每个命令一个，像 `CommandPreview` 那样由命令持有。建议 A：现有用法全是命令在某个视图里的拾取反馈；命令总线每视图一个、同一时刻最多一个活动命令（`ExclusiveCommandBus.h` 的说明），视图知道命令何时结束；画笔也是每视图一个，来源本来就要按视图给。B 在一份文档开多个视口时（现在是一对一）会把一个视口里的悬停显示到所有视口，而且 `AppDocument` 要知道命令何时结束。C 仍要按视图给画笔来源，等于在 A 上再包一层；将来空闲态的悬停预选（AutoCAD 的 selection preview，由选择层来做）也没有命令可挂。定为 A | 已定（2026-09-27） |
| D2 | 命令结束时谁清 | A. 视图在 `commandFinished` 时清空，工具里只为结束写的清理删除；B. 照旧由工具各自清，视图不管。建议 A：统一，不再遗漏（2.3 节第 3 条的查询角度）；DS 的操作器结束后同样由框架把视图拉回权威状态（`SelectionViewSync::ApplyToHoopsView` 的说明）。工具在退回、提交后仍要自己清。定为 A | 已定（2026-09-27） |
| D3 | Render 接口的形式 | A. 交出列表 `highlightedEntities()`；B. 逐实体查询 `isHighlighted(const DmEntity&)`，与 `ISelectionSource` 对称。建议 A：B 在每次重建缓存时对每个顶层实体取 id 查一次集合，键是 36 字符的字符串（选择集方案 9.4 节第 7 条说明了这类查找的开销），悬停时光标每换一个实体就要全查一遍；A 只看集合里的一到几个实体，也为 D4 留好了只重建高亮组的入口。定为 A | 已定（2026-09-27） |
| D4 | 高亮改变时只重建高亮组，不重建整张图 | A. 不在本方案，照旧整张重建，记入渲染专项；B. 本方案最后加一步。建议 A：`ARCHITECTURE_EVOLUTION_PLAN.md` 文首把缓存增量化划给渲染专项；现有的按组删除 `GLCachePainter::removeCacheByGroup` 会删掉全部图片纹理（`GLCachePainter.cpp:101`），直接用会让普通组里的图片丢纹理，要先改 GL 缓存的纹理管理；整张重建在大图纸上的耗时也还没有测过（`BASELINE.md` 第 4 节的数据未采集）。定为 A | 已定（2026-09-27） |
| D5 | 范围 | 另外两类会话状态也写在 Model 里：块列表的选中 `DmBlock::selectedInBlockList`；命令预览时不经事务、直接把文档实体设为不可见（修剪 `ModifyTrimCommand.cpp:207`、延伸 `ModifyExtendCommand.cpp:262`、两点打断 `ModifyCutCommands.cpp:350`），改的是存盘的 `FlagVisible`。编辑多行文字隐藏原文字（`DrawMTextCommand.cpp:278`）经事务进撤销历史，是图纸修改，不算。建议这次都不动：前者与高亮无关；后者的做法要另议（例如画笔按"隐藏集"跳过这些实体）。定为都不动，已记入分层方案 11.2 节 | 已定（2026-09-27） |
| D6 | 预览里的高亮 | A. 预览不涉及高亮，预览画笔不设来源，内切椭圆改为高亮文档里的线；B. 预览画笔也设来源，由预览维护一个高亮集合。建议 A：与选择集方案 9.4 节第 1 条的结论一致（预览里是命令的临时实体）；B 要先解决选择集方案 9.3 节第 11 条的问题（一个视图有多个 `Preview`，共用一个预览画笔）。定为 A | 已定（2026-09-27） |
| D7 | 命名 | Application 的 `HighlightSet`，Render 的 `IHighlightSource`，入口 `ICommandHost::highlight()`、`BaseExclusiveCommand::highlight()`，画布的 `setDocumentHighlightSource()`，与选择集的 `SelectionSet`、`ISelectionSource`、`selection()`、`setDocumentSelectionSource()` 一一对应。按此命名 | 已定（2026-09-27） |

---

## 7. 验证

```powershell
cmake --build --preset Release
ctest --test-dir build/Release -C Release --output-on-failure
cmake --install build/Release --config Release
python tools/check_layering.py
```

编译期保证：第 3 步之后，任何地方调用 `setHighlighted`、`isHighlighted` 都会编译失败（接口已删除）；在 Render 的文件里
临时包含 `HighlightSet.h`，构建 `YiCadRender` 应报 C1083（找不到头文件）。

界面走查（第 2 步起每步都做）：修剪（选边界时移动光标、点选边界、修剪、修剪掉一条边界、右键退回、Esc）、倒圆、倒角、
单个偏移、打断、两切圆、三切圆、内切椭圆、角平分线、过点切线、两圆公切线、垂直切线、查询角度（先悬停第二条线再点）、
多段线加点与删点。每个都看两件事：光标移到或点到实体上时是否立即变为高亮色；右键退回、Esc 结束、提交之后是否立即恢复原色。
另在块编辑里做一遍修剪。

---

## 8. 执行记录

### 8.1 第 1 步：引入 `HighlightSet` 与 `IHighlightSource`，还没有调用方

2026-09-27 完成，基线 `be34287`。行为不变：没有工具写高亮集，画笔照旧按实体上的高亮位分组。

**方案未写、执行时定的**：

1. **`add` 另外拒绝已删除的实体**：3.3 节只写了"只接受当前实体表里的实体"，而 `EntityTable::find` 按 id 也找得到已删除的
   实体（`m_entMap` 含已删除的）。锁定图层上的实体照样加入，与 `SelectionSet` 不同：3.3 节没有这一条，高亮是拾取反馈，
   能不能拾取由工具与捕捉器决定，高亮集不另加限制。
2. **`entities()` 的开销只随集合大小**（D3 的用意）：先对集合里的每个 id 在当前实体表里查，过滤已删除与不可见的；
   只有一个时直接返回（悬停的常态），多于一个时遍历实体表按顺序排，只比较指针、不取 id、不查哈希，找齐即止。
   没有照搬 `SelectionSet::entities()` 对每个实体取 id 查集合的写法，否则每次重建缓存都要对整张图做一遍 D3 想避开的查找。
3. **`contains` 同时要求 `find(id)` 是这个实体本身**，与 `entities()` 的结果一致；集合很小，多查一次不算开销。
4. **"内容真正改变"按集合算**：`add` 已有的、`remove` 没有的、`clear` 空集合都不发 `changed()`；集合里只剩已删除实体的 id 时
   `clear()` 仍发，画面上看不出差别，只多重建一次缓存。
5. **`UIView`**：高亮集与选择层等成员一样只在有文档时创建，声明紧跟 `m_pSelection`，在 `m_pCommandBus` 之前；
   `onCommandFinished()` 第一件事清空；析构函数在释放总线之后把画布的来源置空。
6. **`DmCachePainter` 的过渡写法**：`regroup` 遍历完实体后，对来源给出的实体里未选中、且没有置高亮位的放进高亮组；
   置了位的已经由 `addGroupEntity` 放进去，这样不会重复。第 2 步随 `isHighlighted()` 分支一并去掉这个判断。
7. **放置工具的入口不用改**：`BaseExclusiveCommand::highlight()` 是公开的，工具经 `command().highlight()` 就能取到，
   `BasePlaceTool` 不动。
8. **`TestCommandHost` 自己持有高亮集**（以构造时的文档构造），与 `UIView` 一样在命令结束时清空；夹具不用改，
   用例经 `host.highlight()` 取。
9. `YiCAD/CMakeLists.txt` 分区说明里的目录清单随之加上两者（同选择集方案第 3 步的做法）；`AGENTS.md` 按第 4 节留到第 4 步。

**改动**：

| 位置 | 改法 |
|------|------|
| `render/view/IHighlightSource.h`（新增） | 只读接口 `highlightedEntities()` |
| `DmCachePainter` | 持有可为空的来源 `setHighlightSource()`，`regroup` 见上第 6 条 |
| `GuiDocumentView` | `setDocumentHighlightSource()`，先存为成员，建画笔时交给文档画笔；预览画笔不设 |
| `application/HighlightSet.*`（新增） | 见 3.3 节与上第 1 至 4 条 |
| `ICommandHost`、`BaseExclusiveCommand` | 各加 `highlight()`，后者公开 |
| `UIView` | 见上第 5 条 |
| `tests/support/TestCommandHost.h` | 见上第 8 条 |
| `YiCAD/CMakeLists.txt`、`tests/interaction/CMakeLists.txt` | 说明里加上两者，登记新用例文件 |

**测试**：新增 `test_highlight_set.cpp`（`test_interaction`）六例：只接受当前实体表里的实体（空指针、不在表里的、克隆、
多段线的一段子实体都不加入，不发 `changed()`）、内容不变时不发 `changed()`、已删除与不可见的实体不交出（不可见的重新可见后
仍高亮，已删除的不能再加入）、按实体表的顺序给出（第 4 节没有列，3.3 节写了顺序，补上）、实体被 `remove_direct` 释放后查询
不出错（同一地址的新实体不算高亮）、命令结束时视图清空（命令经 `BaseExclusiveCommand::highlight()` 加入，取到的是宿主的
高亮集）。

**验证**：`cmake --build --preset Release`（没有新增警告）、`ctest`（4 个测试二进制全部通过，`test_interaction` 331 例）、
`python tools/check_layering.py` 通过；在 `DmCachePainter.cpp` 临时包含 `HighlightSet.h`，构建 `YiCadRender` 报 C1083，恢复后
通过。安装后启动程序（新建第一张图纸即构造 `UIView` 的高亮集并交给文档画笔），能正常响应，关闭后退出码为 0。这一步没有调用方，
第 7 节的界面走查从第 2 步起做。

### 8.2 第 2 步：迁移 15 个工具

2026-09-27 完成代码与单测，基线 `d45b48a`；界面走查待做（见本节末）。扩展里已没有 `setHighlighted`、`isHighlighted` 的调用，
Render 只按高亮集分高亮组；Model 里的标志位与清位留到第 3 步。

**方案未写、执行时定的**：

1. **光标在同一实体上移动不动高亮集**：过点切线、两圆公切线、垂直切线、角平分线原先光标每动一下都先清位再置位并重建整张图；
   现在光标下的实体没换时不先 `remove` 再 `add`（那样集合先变后变回，照样发两次 `changed()`）。
2. **倒圆、倒角、查询角度**：`unhighlightEntity()` 改为 `clearHighlight()`（清空高亮集），用于选第一个对象时光标换了实体、
   提交或输出结果、右键退回。点选第一个对象时另外把它加入高亮集：提交或量完后不移动光标接着点，悬停时没有高亮过它
   （查询角度原先靠没清掉的第二条线显得高亮）。
3. **修剪**：`unhighlightLimitingEntity()` 改为 `highlightLimitingEntities()`（把高亮集重设为边界列表），提交后调用（3.4 节），
   另在按回车进入"选要修剪的实体"时也调用：原先选边界时光标下还没点选的实体一直高亮，结束命令只清边界，它的位留着。
   `onFinish()` 留下恢复可见之后的重建缓存与重绘：原先由取消高亮顺带完成，改由视图清空后，高亮集为空时不发 `changed()`，
   预览时隐藏的实体结束后就不重画。
4. **打断**：点了无效的打断点（不在实体上等）原先清位，但仍停在"指定打断点"、要打断的还是这个实体；现在高亮留着，
   与 2.3 节"已选定的对象留到提交、退回或命令结束"一致。
5. **多段线删点**：删掉一个节点后仍停在"指定节点"、可以接着删；按 3.4 节提交后清空，与原先看到的一致（原先清位后，
   事务引起的重建缓存让它恢复原色）。
6. **角平分线**：选第一条线时光标离开直线原先只清位、不忘掉这条线，回到同一条线上不再高亮；现在离开即忘掉。提交与退回第一步
   时连同悬停记录一起清空。构造函数里只为刷新高亮调的 `specifyDocumentModified()`、`redraw()` 删除。
7. **内切椭圆**：去掉预览里的克隆后，悬停第四条线时先清预览再放椭圆。LibreCAD 原版这里就先清预览；移植时为保住预览里的克隆
   去掉了这一句，换一条第四条线时椭圆在预览里越积越多。
8. `BasePlaceTool::onFinish()` 的说明从"取消高亮等收尾"改为"恢复预览时隐藏的实体等收尾"，注明高亮由视图清空。
9. 交互命令夹具 `CommandFixture` 加 `highlight()`，取宿主（`TestCommandHost`）的高亮集。

**改动**：

| 位置 | 改法 |
|------|------|
| `DmCachePainter` | `addGroupEntity` 去掉 `isHighlighted()` 分支，`regroup` 去掉第 1 步的过渡判断：高亮组只收来源给出的、未选中的实体 |
| 14 个文件里的 15 个工具 | 按 3.4 节与上文；各工具只为取消高亮写的 `onFinish()` 删除 |
| `ModifySingleOffsetCommand::commitOffset`、`UINestedBlockSelectDialog.cpp` | 删除克隆后的清位 |
| `BasePlaceTool.h` | 见上第 8 条 |
| `tests/support/CommandTestFixture.h` | 见上第 9 条 |

**测试**：2.2 节的 6 个用例改为断言高亮集。新增四例：倒圆提交后清空（不裁剪，两条线都不被修改）、修剪掉一条边界后它移出高亮集
（连同选边界时悬停的实体在回车后不再高亮）、查询角度先悬停第二条线再点、量完后清空（再不移动光标直接点，它作为第一条线高亮）、
内切椭圆已选的线高亮且预览里只有椭圆（换第四条线时换掉椭圆，右键退回时只取消第三条线）。打断的用例补上
"选中后高亮集立即非空、发一次 `changed()`"。

**验证**：`cmake --build --preset Release`（没有新增警告）、`ctest`（4 个测试二进制全部通过，`test_interaction` 335 例，
其中一例是原有的 DXF 基准图纸用例 SKIPPED）、`python tools/check_layering.py` 通过；安装后启动程序能正常响应，关闭后退出码为 0。

**界面走查（待做）**：本会话不能操作程序界面，第 7 节的走查没有做，第 5 节前两行"按代码推断"的差异也就还没有实测。
走查后在此补记结果。
