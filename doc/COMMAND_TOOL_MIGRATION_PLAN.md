# YiCAD 业务工具化方案：取消 Action 概念

本文档给出 YiCAD 交互层从 Action 体系迁移到 DS 的"命令 + 放置工具"模型的执行方案。
它独立于 `ARCHITECTURE_EVOLUTION_PLAN.md` 的阶段编号，但执行顺序排在该方案阶段 4
任务⑤与阶段 5 之前，理由见第 2 节。

> 本方案于 2026-09-24 提出，文中的行号与数量基于 `57c7d96` 实测。引用
> `ARCHITECTURE_EVOLUTION_PLAN.md` 的章节时写作"主计划 x.y 节"。

---

## 1. 目标

取消 Action 概念，交互层与 DS 采用同一模型：

- **命令**（`XxxCommand`）管生命周期：启动、选项条、预览、提交、结束。同一视图
  同一时刻最多一个活动命令，由命令总线管理。
- **放置工具**（`XxxTool`，实现 `IViewTool`）管事件：交互状态机、捕捉、光标、
  按键提示，把用户输入翻译成对所属命令的调用。
- **选择工具**（`SelectTool`）是业务工具栈的兜底：业务工具不处理的事件（返回
  `NotHandled`）落到选择层，没有命令时全部事件都由它处理。选择不再以 Action
  或命令的形式存在。

同时补齐主计划阶段 2 没有收尾的两项：5.4 第 4 项（光标仲裁覆盖全部业务工具）与
第 6 项（`isExclusive` 等四个标志由栈层次替代）。完成后代码中不再有 Action。

## 2. 与主计划的关系

本方案排在主计划阶段 4 任务⑤与阶段 5 之前。这几项都依赖它，反过来本方案不依赖
主计划中任何未完成的项（测试地基、库拆分都已就位）：

- **阶段 4 任务⑤**要把文字、块、填充、打印的 Action 搬进扩展。先做会按即将删除的
  Action 形式写一遍、本方案再迁一遍；扩展注册命令的接口（`CommandFactory` 返回
  `ActionInterface*`）也会在调用方变多之后才改签名。2026-09-24 调整：文字、块、
  填充改在第三步迁移时直接做成扩展，另把文件、图层、选项也做成扩展（见 5.2 节），
  任务⑤只剩打印。
- **阶段 4 遗留的"每个扩展独立成库"**被主计划 6.7 节记录的两处双向依赖卡住：
  `GuiEventHandler` 持有 Action 栈，Action 直接包含 UI/APP 头文件。这两处正是
  本方案要拆掉的。
- **阶段 4 遗留的 `DM::ActionType` 降级**就是本方案第四步的内容，分开做会动两次。
- **阶段 5 的 `QMouseEvent::x()/y()` 改 `position()`** 集中在 Action 的事件处理
  代码里。先做本方案，Qt 6 迁移只需处理新代码一次；两个高风险改动也不重叠，交互
  回归能分清来源。

## 3. 现状证据

- **Action 规模**：106 个 Action 类，`src/actions/` 97 个、`src/extensions/dim/actions/`
  9 个；基类为 `PreviewActionInterface` 58、`ActionInterface` 42、`ActionDimension` 6。
  每个类同时承担命令生命周期与事件处理。
- **默认 Action**：`ActionDefault`（123 行）只剩逐方法转发给 `SelectTool`。空闲态
  事件的实际路径是 `LegacyActionTool → GuiEventHandler → ActionDefault → SelectTool`；
  注册为选择层的 `SelectTool` 只在中键和空闲态 Ctrl+左键两种情况下被
  `ViewToolControl` 直接问到（主计划 5.7 节偏差表）。依赖"默认 Action"类型身份的
  地方：
  - `ActionInterface::finish()` 对 `DM::ActionDefault` 的特判（`ActionInterface.cpp:205`）；
  - `GuiEventHandler::inSelectionMode()`，无外部调用方；
  - `ActionBlocksEdit.cpp:295–366`、`ActionModifyMText.cpp:116` 直接调用
    `getDefaultAction()->mouseXxxEvent(e)`；
  - `GuiEventHandler::getCurrentAction()` 空闲态返回默认 Action，调用方据此假定
    返回值非空；`killAllActions()` 结束全部命令后对默认 Action `init(0)` 复位。
- **三个选择 Action**：
  - `ActionSelect` + `ActionSelectMultiple`：14 个"先选后建"命令在没有选择集时的
    选择阶段（`makeSelectFirstFactory`，以及 `ActionModifyDelete`、`ActionBlocksEdit`
    的专用工厂）。`ActionSelectMultiple` 的 `Neutral`/`Dragging`/`SetCorner2` 与
    `SelectTool` 基本是复制（头文件自注"主要参考ActionDefault"），只多了实体类型
    过滤与交还父 Action。
  - `ActionSelectSingle`：`ActionSelect` 里使用它的一行已注释
    （`ActionSelect.cpp:55`）；`select.single` 已注册，但 Ribbon、keyconfig、命令行
    都没有入口；唯一真实调用方是手写板橡皮擦（`GuiDocumentView.cpp:1540`），且构造
    时不传父 Action。`GuiEventHandler::killSelectActions()` 只清理这一种类型。
- **选择阶段的现有交互**（迁移须保持一致）：
  - 确认：主键盘与小键盘回车都经 `ApplicationWindow::slotEnter()` 转成
    `GuiEventHandler::enter()` 合成的 `Qt::Key_Enter`，`ActionSelect` 只在已有
    选择时接受，随后启动真正的命令；
  - 取消：右键（`GuiEventHandler::back()`）结束整个命令；Esc 与空格未被接受，
    `ApplicationWindow` 随即结束全部命令并清空选择（`ApplicationWindow.cpp:572`）；
  - 提示：`ActionSelect::updateMouseButtonHints` 为 9 个命令给出"Select to …"
    文案，其余 5 个为空——其中 `ActionNoSelectCopyToLayer` 的 case 缺 `break`，
    文案被 default 分支立即覆盖为空（既有缺陷，注释里已有 TODO）；
  - `ActionModifyDelete` 始终先进入选择阶段，不看是否已有选择（主计划 7.7 节）。
- **键盘事件不经过画布**：有文档时 `GuiDocumentView::keyPressEvent` 直接返回
  （`GuiDocumentView.cpp:1696`），键盘事件由 `ApplicationWindow::keyPressEvent` 转给
  `GuiEventHandler`，`ViewToolControl` 的键盘分发实际上从未被画布调用。
- **命令栈语义**（`GuiEventHandler`，632 行）：
  - 启动新命令时，前一个命令默认被**挂起**（`canBeInterrupt()` 默认 `true`），
    新命令结束后前一个**恢复**；
  - 8 个 `isExclusive()` 命令清空全部：文件新建、打开、保存、另存、导出图片，
    块创建、块另存、插入块准备；
  - `isViewAction()`（`ActionZoomIn`、`ActionZoomPan`）总是叠加；
  - `canBeInterrupt()` 为 `false` 的被结束：`ActionDrawMText`、`ActionModifyMText`，
    以及 `ActionBlockInsertPrepare` 的部分状态；
  - `isSubAction()`（`ActionBlocksInsert`）另有规则；
  - 块编辑（`ActionBlocksEdit`）依赖"可挂起"：编辑块期间启动的绘图命令叠在它
    上面，结束后回到块编辑。
- **被外部结束时的保存提示**：`ActionDrawMText::finish()` 在编辑态弹出"Save the
  changes?"（是/否），是则提交、否则放弃（`ActionDrawMText.cpp:181–197`）。被新命令
  结束、结束全部命令（Esc/空格）、关闭视图都走 `finish()`，所以三种情况都会提示。
  `ActionModifyMText` 每次改动即提交，没有提示。块编辑的右键退出另有"是/否/取消"
  对话框（`ActionBlocksEdit.cpp` 的 `mouseReleaseEvent`）。
- **命令行输入**：坐标（绝对/相对 × 直角/极坐标）在 `GuiEventHandler::commandEvent`
  里解析，转成 `coordinateEvent` 发给当前 Action；45 个 Action 头文件声明了
  `coordinateEvent`，38 个声明了 `commandEvent`。
- **即时命令**：34 个 Action 在 `init()` 里直接 `trigger()` 后结束，没有画布交互
  （文件、图层、样式、撤销、信息、选项等）。
- **嵌套启动**：`ActionDrawArc3p → ActionDrawArc`；`ActionDrawLine → ActionEditUndo`
  （3 处）；`ActionModifyEntity`、`ActionSelectedChanged → ActionModifyMText`。
- **光标**：105 个业务 Action 仍在 `updateMouseCursor()` 里直接设置光标，不经
  `ViewToolControl::refreshCursor()` 仲裁（主计划 5.7 节）。
- **UI 耦合**：18 个选项条表单（`ui/forms/`、`extensions/dim/ui/`）接收
  `ActionInterface*`；29 个 Action 直接包含 `UI*.h`、`ApplicationWindow.h` 或
  `MDIWindow.h`，这是主计划 6.7 节记录的双向依赖之一。
- **视图接口**：`IDocumentView`（MODEL 层）暴露 `getEventHandler()`、
  `setCurrentAction()`、`getCurrentAction()`、`killSelectActions()`。

## 4. 参考设计（`E:\dev\DS-master`）

主计划阶段 2 引用的 `E:\dev\DS` 是同一套框架的较早副本：`IViewTool.h`、
`ViewToolControl.h`、`IExclusiveCommand.h`、`BaseExclusiveCommand.h`、
`ExclusiveCommandBus.h` 两边一致，`Select/SelectViewTool.h` 只多一个成员
（2026-09-24 比对）。本方案以 DS-master 为准，路径相对 `DimX/Source/`。

- `Application/IExclusiveCommand.h`、`BaseExclusiveCommand.h`、`ExclusiveCommandBus.h`：
  - 命令接口：`CommandId`、`Activate(host)`、`Deactivate`、`IsActive`；
  - 通用基类：提供 `RequestCancel`；
  - 命令总线：每个视图一个；同一时刻最多一个活动命令，`Start` 先结束旧命令；
    变化经 `signal_activeChanged` 广播。
- `Extensions/LengthDimension/LengthDimensionCommand` + `LengthDimPlaceTool`：典型的
  命令 + 放置工具。
  - 命令的 `OnActivate` 创建工具、`viewToolControl()->Activate(tool)`，并弹出参数面板；
  - 工具的状态机（`PickFirstPoint → PickSecondPoint → PickDimensionPosition`）
    回调命令的业务方法（`SelectFirstPoint`、`PreviewAt`、`CommitAt`）；
  - 工具对中键返回 `HOP_NOT_HANDLED`，交给导航层；
  - 命令的 `OnDeactivate` 停用工具、清理预览。
  - DS-master 的 `doc/图元创建编辑全流程.md` 附录 A 是同一流程的文字版。
- `Application/Select/SelectViewTool` + `SelectionService`：选择层写全局选择集；
  需要选择集的命令在启动前读取（`SelectedCoordTableCommand::PrepareFromSelection`）。
- `View/UIView.cpp:204–219`：视图创建导航、选择两个兜底工具并注册；2D 模式下另把
  `Application/Edit/EditTool`（夹点编辑）常驻激活在业务栈底部。

DS 没有、YiCAD 需要保留的：
- 命令行坐标与文本输入、选项条、鼠标按键提示、相对零点；
- 不需要画布交互的即时命令；
- 块编辑这类在其内部还能运行其它命令的长驻模式；
- 命令被外部结束前保存或放弃未提交修改的机会（见 5.1 节）。

## 5. 设计决策（2026-09-24 确认）

| 项 | 结论 | 说明 |
|----|------|------|
| 命令与工具 | 拆成两个对象，与 DS 一致 | 命令持有业务数据、预览、选项条与提交逻辑；工具只持有交互状态机与捕捉会话，通过回调驱动命令。即时命令没有工具 |
| 命令并存 | 与 DS 一致：启动新命令即结束当前命令 | 两类例外不占总线：① 视图工具（平移、缩放）作为临时工具叠在业务栈顶，结束后当前命令照常继续；② 块编辑改为"编辑模式"，进入时在业务栈底部常驻一个块编辑工具（与 DS 的 `EditTool` 常驻用法相同），它负责右键询问是否保存并退出、其余事件让给选择层，编辑期间启动的命令叠在它上面，结束后回到块编辑。行为变化：被打断的普通命令不再恢复（与 AutoCAD 一致）。`isExclusive`/`isSubAction`/`canBeInterrupt`/`isViewAction` 全部删除 |
| 结束前回调 | 命令被外部结束前，总线先回调命令，由命令保存、放弃或否决 | DS 没有这一环，是 YiCAD 的补充，见 5.1 节 |
| 先选后建 | 保持现有交互：没有选择集时命令先进入"选择对象"阶段 | 该阶段命令的工具对鼠标事件返回 `NotHandled`，由兜底的 `SelectTool` 完成点选/框选。`SelectTool` 增加两项约束：实体类型过滤、禁用夹点拖拽，由当前命令设置，命令结束时由总线保证清除。确认、取消、提示文案与 `ActionModifyDelete` 的差异按第 3 节"选择阶段的现有交互"逐项保持（`ActionNoSelectCopyToLayer` 提示为空的缺陷照原样保留，不在迁移中顺手修） |
| 夹点编辑 | 不拆出 `EditTool`，留在 `SelectTool` | 理由同主计划 5.7 节：框选与拖夹点在同一次拖拽的中途才分叉，共享未决状态 |
| 命名 | DS 后缀式 | 命令 `XxxCommand`、工具 `XxxTool`、框架接口 `I*`，例如 `ActionDrawLine` → `DrawLineCommand` + `DrawLineTool`。框架类沿用 DS 名称（`ExclusiveCommandBus` 等）；方法名按 YiCAD 已有移植（`IViewTool`）的写法用小驼峰。`Action*` 只留给尚未迁移的旧类，不再新增。`AGENTS.md` 的命名规则已同步修改 |
| 执行顺序 | 先于主计划阶段 4 任务⑤与阶段 5，分四步 | 见第 2 节与第 6 节 |
| 第三步与扩展化（2026-09-24 确认） | 块、文字、填充在第三步直接做成扩展，文件、图层、选项也做成扩展 | 见 5.2 节 |

### 5.1 结束前回调

命令自己结束（在自己的工具事件里提交或取消）时不回调，由命令自己决定。
被**外部**结束时，总线先调用命令的回调，再改变任何状态：

| 原因 | 触发点（现状） | 能否否决 | 否决的效果 |
|------|----------------|----------|------------|
| 被新命令替换 | 启动任意交互命令 | 能 | 新命令实例直接销毁、不激活，当前命令继续 |
| 结束全部命令 | Esc/空格未被当前工具接受时的 `ApplicationWindow::slotKillAllActions()`（`ApplicationWindow.cpp:580`）；`ActionEditKillAllActions` 命令（`UIActionHandler.cpp:95`） | 能 | 当前命令继续，"清空选择"也不执行 |
| 视图或文档关闭 | `UITabDrawWidget::doClose()` 里的 `killAllActions()`（`UITabDrawWidget.cpp:1138`） | 不能 | 返回值被忽略；命令仍可在回调里保存或放弃 |

接口示意（名称以实现为准）：

```cpp
/// @brief 命令被外部结束的原因
enum class CommandEndReason
{
    Replaced,    ///< 启动了新命令
    Cancelled,   ///< 用户结束全部命令
    ViewClosing, ///< 视图或文档关闭
};

/// @brief 命令被外部结束前调用，命令在此保存或放弃未提交的修改
/// @param reason 结束原因
/// @return false 表示否决：命令继续运行；ViewClosing 时返回值被忽略
virtual bool onEndRequested(CommandEndReason reason) { return true; }
```

约束：
- 回调里可以弹模态对话框。回调返回前总线不改变任何状态；回调期间再次请求启动或
  结束命令（例如对话框的事件循环里又点了 Ribbon）时，总线忽略这次请求。
- 默认实现直接放行，没有未提交修改的命令不用覆盖。

已知的使用者：
- **多行文字编辑**（`ActionDrawMText` 迁移后）：在回调里弹出与现状相同的"Save the
  changes?"（是/否），是则提交、否则放弃，返回 `true`。按钮与现状一致，不在迁移中
  新增"取消"；需要时再利用否决能力加上。
- **块编辑模式**：它不是命令，但视图关闭时同样要给它保存或放弃的机会，接口形式
  相同；右键退出的"是/否/取消"对话框保持现状。

### 5.2 第三步与扩展化（2026-09-24 确认）

第三步开始前确认，改动第 6 节第三步的分批：

- **块、文字、填充**（主计划阶段 4 任务⑤的三个领域）在第三步迁移时直接做成扩展
  `ext.block`、`ext.text`、`ext.hatch`，不再先在 `src/actions/` 迁一遍、任务⑤再
  搬一次。第二步已迁移的块命令（创建块、编辑块与块编辑模式）随 `ext.block` 一起搬。
- **文件、图层、选项**也做成扩展 `ext.file`、`ext.layer`、`ext.options`。主计划
  任务⑤原先没有列出它们；这几组命令依赖主窗口，做成扩展后经 `IExtensionContext`
  访问宿主。
- 做法与 `ext.dim`（主计划 7.10 节）相同：命令 ID 改为 `ext.<领域>.*`，Ribbon 条目、
  命令行别名由扩展注册，keyconfig.xml 删除对应条目，翻译拆到扩展自己的 ts。
- 分层：扩展将来各自成库，可以依赖 `YiCadUi`（包含 `ui/` 的头文件），不能包含
  `main/` 的头文件（`ApplicationWindow`、`MDIWindow`）；核心命令（`src/actions/`）
  两者都不能包含，第四步由 `check_layering.py` 检查。
- 节奏：每批一个提交，每批完成后停下，核对后再做下一批。

## 6. 任务

四步依次落地，每一步都能单独构建、运行、回退。未迁移的 Action 始终经
`LegacyActionTool` 运行。

**第一步：选择层独立（删除 `ActionDefault`、`ActionSelectSingle`）**

1. **交互回归清单**：主计划 5.6 节计划过，一直没有写。先写
   `doc/INTERACTION_CHECKLIST.md`，覆盖以下场景；之后每一步、每一批迁移都按它
   手工核对：
   - 空闲态：点选、框选、交叉选、Shift 反选、夹点拖拽、拖动实体；
   - Esc 与空格；中键平移、Ctrl+左键平移；
   - 14 个先选后建命令的选择阶段；
   - 块编辑、多行文字编辑（含 5.1 节的三种外部结束）、手写板橡皮擦。
2. **`SelectTool` 改由视图持有**：`GuiDocumentView` 直接持有 `SelectTool` 及其
   捕捉器、预览容器（原由 `ActionDefault` 持有），`GuiEventHandler::setSnapMode`/
   `setSnapRestriction` 同步给它。鼠标离开/进入画布时的挂起/恢复，改由
   `IViewTool::leaveEvent`/`enterEvent` 承接。
3. **`LegacyActionTool` 整体让路**：没有业务 Action 活动时，对全部事件返回
   `NotHandled`，空闲态事件直接落到选择层。
4. **键盘事件改走 `ViewToolControl`**：`ApplicationWindow::keyPressEvent` 里原先
   交给 `GuiEventHandler` 的 Esc/空格/回车，改为交给当前视图的 `ViewToolControl`。
   不改的话，删掉默认 Action 后空闲态的 Esc、Shift 到不了 `SelectTool`。"未被接受
   则结束全部命令并清空选择"的规则保持不变。
5. **旧 Action 的临时让路钩子**：`ActionInterface` 增加一个按事件询问的虚函数，
   默认不让路；`LegacyActionTool` 据此返回 `NotHandled`。
   - `ActionBlocksEdit` 在编辑态用它让出除右键外的鼠标、键盘事件；
   - `ActionModifyMText` 在双击时用它；
   - 两者删除对 `getDefaultAction()` 的直调；
   - 钩子随 `ActionInterface` 在第四步删除。
6. **删除 `ActionDefault`**：连同 `GuiEventHandler` 的默认 Action 成员与
   `getDefaultAction()`（含 `GuiDocumentView` 上的同名方法）、`ActionInterface::finish()`
   的特判、`inSelectionMode()` 一并删除。
   - `killAllActions()` 末尾的复位改为复位 `SelectTool`；
   - 逐一核对 `getCurrentAction()` 的调用方：空闲态原先返回默认 Action，改后返回
     `nullptr`。
7. **删除 `ActionSelectSingle`**：
   - 删除 `select.single` 注册；
   - 手写板橡皮擦改为经 `SelectTool` 新增的单点拾取接口选中实体，再启动删除；
   - `killSelectActions()` 只清理这一种类型，连同 `IDocumentView`、`UIActionHandler`
     上的同名方法与 3 处调用（`ActionBlocksCreate` 2 处、`ActionEditCopy` 1 处）一并删除；
   - `DM::ActionDefault`/`DM::ActionSelectSingle` 枚举值与 `Commands.cpp` 的映射留到
     第四步统一处理。
8. **测试**：
   - `test_select_tool` 改为直接构造选择层（不经 `ActionDefault`），补空闲态经
     `ViewToolControl` 分发的鼠标、键盘用例；
   - `test_legacy_action_tool` 补"无业务 Action 时整体让路"与让路钩子的用例；
   - `test_command_registry` 删除 `select.single` 的两例；
   - `FakeDocumentView` 随 `IDocumentView` 同步。

**第二步：命令框架与先选后建（删除 `ActionSelect`、`ActionSelectMultiple`）**

1. **命令框架**：放在 `kernel/actions/` 下，名称沿用 DS：`IExclusiveCommand`、
   `BaseExclusiveCommand`，以及每个视图一个的 `ExclusiveCommandBus`。与 DS 的差异：
   - 总线持有命令：`CommandRegistry` 每次启动都新建实例，而 DS 的命令由扩展持有、
     总线只借用；
   - 命令在自己工具的事件处理中结束时延迟销毁，参考 DS
     `ViewCommandManager::QueueFinishExclusive`；
   - 结束前回调按 5.1 节实现；
   - 视图工具（平移、缩放）不占总线，直接叠在业务栈顶。
2. **两类新注册**：`CommandRegistry` 支持交互命令（工厂返回
   `std::unique_ptr<IExclusiveCommand>`）与即时命令（一个函数，不建命令对象、不占
   总线；没有打开图纸时也能执行）。
   - 即时命令取代 `UIActionHandler::activateCommand` 里"没有视图就 `trigger()` 再
     删除"的特判；
   - 旧的 `ActionInterface` 工厂并行保留到第四步，`activateCommand` 按注册类型分派；
     新旧并存期间，启动新命令时对旧 Action 仍按现有 `GuiEventHandler` 规则处理，
     对新命令按 5.1 节处理；
   - 扩展经 `IExtensionContext` 注册命令的接口同步支持新类型。
3. **业务工具的输入与界面**：
   - 命令行坐标解析（绝对/相对 × 直角/极坐标）从 `GuiEventHandler::commandEvent`
     抽成独立函数，并补单测；
   - `IViewTool` 增加坐标输入与命令文本两个回调（默认 `NotHandled`），
     `ViewToolControl` 只沿业务栈分发；
   - 选项条与按键提示由命令、工具经 `GuiDialogFactoryInterface` 提供，选项条表单
     改为接收命令类型。
4. **选择阶段**：`SelectTool` 增加选择阶段约束（实体类型过滤、禁用夹点拖拽）；
   `BaseExclusiveCommand` 提供进入、退出选择阶段的辅助方法，总线在命令结束时保证
   清除约束。
5. **第一批迁移**：`DM::*NoSelect` 对应的 14 个先选后建命令——移动、复制、旋转、
   缩放、镜像、分解、反向、删除（`ActionModify*`），复制到剪贴板、剪切
   （`ActionEditCopy`/`Cut`），复制到图层，创建块、编辑块，总长度。编辑块按第 5 节
   "命令并存"改为编辑模式。随后删除：
   - `ActionSelect`、`ActionSelectMultiple`；
   - `makeSelectFirstFactory`；
   - `ActionModifyDelete`、`ActionBlocksEdit` 的专用工厂。

   **至此选择 Action 全部消失。**
6. **测试**：
   - 总线生命周期：启动、替换、取消、延迟销毁；
   - 结束前回调：三种原因、否决与不否决、`ViewClosing` 忽略否决、回调期间的重入
     请求被忽略；
   - 即时命令；
   - 选择阶段约束的设置与清除；
   - 14 个命令各自的选择阶段：`FakeDocumentView` + 空文档 + `EntityTable::add_direct`。

**第三步：按领域迁移其余 Action**

1. **视图**：`ActionZoomPan` 作为不占总线的临时工具；`ActionZoomIn` 是即时命令。
2. **即时命令**：`init()` 里直接 `trigger()` 的那批（文件、图层、样式、撤销/重做、
   信息、选项等），第二步已迁走的除外。
3. **绘图**：直线、多段线、圆弧、圆、椭圆、样条、点、填充等。
   - `ActionDrawLine → ActionEditUndo` 的嵌套改为命令内直接调用撤销；
   - `ActionDrawArc3p → ActionDrawArc` 改为启动另一个命令。
4. **修改**：其余修改命令。
5. **块与文字**：
   - 插入块：`ActionBlockInsertPrepare` + `ActionBlocksInsert` 合为一个两阶段命令；
   - 单行、多行文字：`ActionDrawMText`（保存提示移入 5.1 节的回调）、`ActionModifyMText`；
   - `ActionModifyEntity`、`ActionSelectedChanged` 对多行文字编辑的启动，后者改为
     选择变化的监听者，不再是命令。
6. **标注扩展**：`ext.dim` 的 9 个 Action。

按 5.2 节调整后的分批（2026-09-24），每批一个提交：

| 批 | 内容 |
|----|------|
| ① 视图与核心即时命令 | 平移（临时视图工具）；缩放、撤销/重做、选中信息（即时命令）；删除两个从未注册的捕捉设置 Action |
| ② 绘图·直线类 | 直线、多段线、矩形、多边形 ×2、角平分线、切线 ×2、正交切线、徒手线、射线、构造线、点 |
| ③ 绘图·曲线类 | 圆弧 ×3、圆 ×5、椭圆 ×2、样条 ×2、云线 ×3、图片 |
| ④ 修改与查询 | 修剪、延伸、倒角、圆角、打断 ×2、偏移、多段线编辑 ×3、查询角度/面积/距离、粘贴、修改实体 |
| ⑤ 扩展：文件、图层、选项 | `ext.file`、`ext.layer`、`ext.options` |
| ⑥ 扩展：块 | `ext.block`：插入块（两阶段命令）、属性定义、块的删除/保存/另存/导入，连同第二步的创建块、编辑块与块编辑模式 |
| ⑦ 扩展：文字 | `ext.text`：单行文字、多行文字、多行文字属性、文字样式；选择变化改为监听者 |
| ⑧ 扩展：填充 | `ext.hatch` |
| ⑨ 标注扩展 | `ext.dim` 的 9 个 Action |

每一批都要求：
- 业务工具经 `getCursor()` 提供光标，删除 `updateMouseCursor()`；
- 命令与工具不直接包含 `ui/`、`main/` 的头文件（扩展里的可以包含 `ui/` 的，见
  5.2 节），需要的能力经 `GuiDialogFactoryInterface`、`IExtensionContext` 或新增的
  窄接口获取；
- 有未提交修改的命令实现 5.1 节的回调；
- 构建、`ctest`、`check_layering.py` 通过，并按回归清单手工核对。

**第四步：删除旧体系**

1. **删除旧类**：`ActionInterface`、`PreviewActionInterface`、`GuiEventHandler`、
   `LegacyActionTool`，四个优先级标志，以及第一步加的让路钩子。
2. **视图接口**：`IDocumentView` 删除 `getEventHandler`/`setCurrentAction`/
   `getCurrentAction`，改为暴露命令总线的窄接口。
3. **`DM::ActionType` 退出命令 ID**（主计划阶段 4 遗留）：
   - `CommandRegistry` 删除 legacy 桥接；
   - keyconfig.xml 与"命令设置"对话框改为以字符串 ID 为键，读取用户目录下的
     旧格式文件时按原映射表转换一次；
   - 删除 `Commands.cpp` 的枚举映射；
   - 枚举只保留仍有非命令用途的值，没有则整个删除。
4. **分层检查**：`tools/check_layering.py` 增加"命令与工具不得包含 `ui/`、`main/`
   头文件"的检查。
5. **目录**：`src/actions/` 目录更名（候选 `src/commands/`，名称届时再定）；
   `kernel/actions/` 并入第一步新建的 `kernel/interaction/`，与分区名
   `YiCadInteraction` 一致。同步 CMake 分区与 `check_layering.py`。

## 7. 验收标准

- 源码中没有 `ActionInterface`、`PreviewActionInterface`、`GuiEventHandler`、
  `LegacyActionTool`，也没有任何 `Action*` 类。
- 选择只由选择层完成：没有选择类的命令或 Action，没有 `getDefaultAction()` 式的
  直调；空闲态的鼠标、键盘事件由 `ViewToolControl` 直接分发给 `SelectTool`。
- 先选后建的 14 个命令、块编辑、多行文字编辑、手写板橡皮擦按回归清单逐项与现状
  一致；"被打断的普通命令不再恢复"这一行为变化在清单中注明。
- 多行文字编辑态在被新命令替换、结束全部命令、关闭视图三种情况下都弹出保存提示，
  与现状一致。
- 光标全部经 `ViewToolControl::refreshCursor()` 仲裁，源码中没有
  `updateMouseCursor()`（完成主计划 5.4 第 4 项）。
- 四个优先级标志删除（完成主计划 5.4 第 6 项）。
- 命令行坐标输入四种格式的行为与现状一致，有单测。
- 命令与工具可脱离 `GuiDocumentView` 单测（`FakeDocumentView` + 命令总线）。
- 命令与工具不直接包含 `ui/`、`main/` 的头文件，由 `check_layering.py` 检查。
  这解开了主计划 6.7 节记录的 Action→UI/APP 双向依赖，是阶段 4"每个扩展独立
  成库"的前提。
- `DM::ActionType` 不再作为命令 ID；用户目录下旧格式的 keyconfig.xml 能正确读入。

## 8. 风险与回退

**高**。改动面覆盖：
- 全部 106 个 Action 与 18 个选项条表单；
- `GuiEventHandler`、`GuiDocumentView`、`UIActionHandler`，以及 `ApplicationWindow`
  的键盘分发；
- 标注扩展。

具体风险：
- 交互回归难以自动化；
- "被打断的命令不再恢复"是用户能感知的行为变化；
- 结束前回调里弹模态对话框，事件循环中可能重入总线；
- keyconfig 改格式会影响用户目录下已有的文件。

缓解与回退：
- `LegacyActionTool` 让新命令与未迁移的 Action 共存，任何时刻都可构建可运行，
  每一批都可单独回退；
- 第一步先写交互回归清单，之后每一步、每一批都按清单核对；
- 每一批都跑 `ctest` 与 `check_layering.py`，Debug、Release 都构建；
- 总线对回调期间的重入请求一律忽略，并有单测；
- keyconfig 读取兼容旧格式，并保留旧文件的备份；
- 遵守主计划第 10 节的通用约束：不出大爆炸式 PR，超过三个核心文件的改动先说明
  影响范围。

**工作量**：XL。

## 9. 执行结果

### 9.1 第一步（2026-09-24）

按第 6 节第一步的 8 项落地：

1. **交互回归清单**：`doc/INTERACTION_CHECKLIST.md`，按空闲态选择、键盘与导航、
   先选后建、块编辑、手写板橡皮擦、多行文字分节，期望写现有行为；第 7 节登记各步
   有意的行为变化。
2. **`SelectTool` 由视图持有**：对照 DS 的 `HQWidget`/`UIView`，把 `GuiDocumentView`
   拆成两层（见下文偏差 7）。交互视图 `UIView`（`kernel/interaction/`）持有 `ViewToolControl`、
   `PanZoomTool`、`LegacyActionTool` 与 `SelectTool` 及其 `Snapper`、`Preview`；
   `setDefaultSnapMode`/`setSnapRestriction` 由 `UIView` 覆写，同步给这个捕捉器（它是
   捕捉器的拥有者，没有经 `GuiEventHandler` 转一道）。鼠标离开/进入画布由
   `IViewTool::leaveEvent`/`enterEvent` 承接：`SelectTool` 只在空闲态挂起/恢复，
   `LegacyActionTool` 把它们转给栈顶业务 Action。
3. **`LegacyActionTool` 整体让路**：没有业务 Action 时全部事件返回 `NotHandled`；
   原先的"空闲态 Ctrl+左键让路"并入这条规则。
4. **键盘改走 `ViewToolControl`**：`ApplicationWindow` 的 Esc/空格交给
   `GuiDocumentView::processKeyEvent()`（对应 DS 的 `HQWidget::processKeyEvent`），由
   `UIView` 覆写为经 `ViewToolControl` 分发；回车（`GuiDocumentView::enter()`）改为
   合成按键交给 `processKeyEvent()`，`GuiEventHandler::enter()` 随之删除。画布自己的
   `keyPressEvent`/`keyReleaseEvent`、左键双击、滚轮缩放后补发的移动也改走
   `ViewToolControl`。
5. **让路钩子**：`ActionInterface::passesToSelection(const QEvent*)`，见下文偏差 1。
6. **删除 `ActionDefault`**：连同 `GuiEventHandler`/`GuiDocumentView` 的默认 Action
   成员与存取方法、`ActionInterface::finish()` 的特判、`inSelectionMode()`。
   `getCurrentAction()` 空闲态返回 `nullptr`，调用方逐一核对：
   - 画布的捕捉标记与捕捉提示：改为读虚函数 `currentSnapResult()`/`currentSnapSpot()`，
     基类返回"无捕捉"，`UIView` 覆写为有业务 Action 时读它的捕捉器、空闲态读选择层的；
   - 释放后的 `updateMouseCursor()`、`UICommandWidget::appCmdTempText`：已判空，
     原先默认 Action 的对应实现本就是空操作或空说明，行为不变；
   - `UIActionHandler::getAvailableCommands()`：空闲态返回值从空列表变为
     `line`/`rectangle`，但全仓库没有调用方；
   - `Snapper::finishOrthogonal()`、`ActionDrawEllipseAxis`：只在业务 Action 内部调用，
     此时必有当前 Action。
7. **删除 `ActionSelectSingle`**：删除 `select.single` 注册；手写板橡皮擦改用
   `SelectTool::pickAt()`；`killSelectActions()` 连同 `IDocumentView`、
   `UIActionHandler` 上的同名方法与 3 处调用删除；两个枚举值与 `Commands.cpp` 映射
   按计划保留到第四步。
8. **测试**：`test_select_tool` 新增 12 例（中键让路、单点拾取、有业务 Action 时不更新
   提示、只在空闲态挂起恢复，以及按 `UIView` 装配、经 `ViewToolControl`
   分发的 8 例空闲态鼠标、键盘用例）；`test_legacy_action_tool` 改为用记录事件的探针
   Action，覆盖整体让路、转发、钩子、转发前询问与进入/离开；`test_command_registry`
   删除 `select.single` 用例；`FakeDocumentView` 同步。

**与方案的偏差与补充**

1. **钩子的语义是"处理后继续下传"，不是"不交给 Action"**。`passesToSelection()`
   在转发前询问（`GuiEventHandler` 转发释放后会 `cleanUp()`，结束了的 Action 此时已被
   删除）；为真时照常转给 Action，再返回 `NotHandled`。原因有二：
   - `ActionModifyMText` 双击时要先取消选择、结束自己，再由选择层进入文字编辑，这件事
     留在它自己的 `mouseDoubleClickEvent` 里，钩子保持为纯查询；
   - 块编辑下 `GuiEventHandler` 在释放后照常 `cleanUp()`，恢复块编辑的提示与选项条，
     与原先经默认 Action 转发时一致。

   钩子不是 `const`：`ActionInterface::getStatus()` 不是 `const`。
2. **空闲态的三处切换由 `GuiEventHandler` 调选择层**。原默认 Action 靠
   `GuiEventHandler` 的 `suspend`/`resume`/`init` 完成：旧 Action 从空闲态启动时挂起、
   回到空闲态时恢复（刷新按键提示）、`killAllActions()` 复位。现由
   `GuiEventHandler::setSelectTool()` 登记的非持有指针在原位置调用
   `SelectTool::suspend()`/`resume()`/`init()`，第四步随 `GuiEventHandler` 删除。
   排他命令（文件新建、打开等）经 `killAllActions()` 复位选择层的行为因此保持不变。
3. **有业务 Action 时 `SelectTool` 不更新按键提示**，与 `getCursor()` 已有的规则一致。
   否则块编辑下选择层回到 `Neutral` 时会清空提示，而 `cleanUp()` 在它之前运行，
   "Edit block entities"提示不再恢复。副作用：块编辑中拖框、拖动实体时提示不再短暂
   清空（清单第 7 节）。
4. **修复中键平移**。`SelectTool::mousePressEvent` 对左、右键以外的按下一律返回
   `Handled`，把 `LegacyActionTool` 让出的中键吞在选择层，导航层收不到按下——37ac325
   把 `SelectTool` 注册为选择层时引入，空闲态和命令中的中键平移都失效。现在
   `SelectTool` 对中键返回 `NotHandled`，由 `PanZoomTool` 处理；只放中键，其它按键
   （如 XButton）仍由选择层接住，否则会被导航层当成平移起点。回归测试去掉修复后失败
   3 例。
5. **Shift 不转发**。第 6 节第 4 项说不改的话"空闲态的 Esc、Shift 到不了
   `SelectTool`"，实测改动前 Shift 也到不了：`ApplicationWindow` 只转发 Esc/空格、
   不转发按键释放，画布拿不到焦点；`Snapper::setSnapRestriction()` 本身也是空实现。
   保持不转发。
6. **橡皮擦不再压 Action**。原流程临时压入 `ActionSelectSingle` 会挂起当前命令，
   直到下一次 `cleanUp()`；现在直接拾取，不影响当前命令。拾取仍是切换选中、随后删除
   整个选择集（清单 E2–E4 的既有行为）。
7. **`GuiDocumentView` 拆成画布与交互视图两层**（2026-09-24 确认，对照 DS 的
   `HQWidget`/`UIView`）。方案原写"`GuiDocumentView` 直接持有 `SelectTool`"；但
   `GuiDocumentView` 在 `kernel/gui/`（现 `kernel/view/`，`YiCadRender`），按主计划 6.3 节不应认识交互层
   类型，主计划 6.7 节已把它 new 工具记为第一处双向依赖。现在：
   - `GuiDocumentView` 只留渲染与视图状态，不再包含任何交互层工具或 Action 头文件；
     新增 `processKeyEvent()`（对应 DS 的 `HQWidget::processKeyEvent`）与捕捉结果的
     两个虚函数，基类实现都是"无交互层"；
   - `UIView`（沿用 DS 类名）继承它，持有 `ViewToolControl` 与三层工具，接收全部
     Qt 输入事件，滚轮缩放（`ActionZoomIn`）与橡皮擦删除（`ActionModifyDelete`）也
     随之移过来；
   - 目录：`kernel/gui/` 更名 `kernel/view/`（`YiCadRender`），`UIView` 放在新建的
     `kernel/interaction/`（`YiCadInteraction`）。两者不同目录，因为一个目录归一个
     分区；DS 把 `HQWidget` 与 `UIView` 同放 `View/`，是因为它不按目录分库。
     `UIView` 是内核里唯一以 `UI` 开头的类型，`check_layering.py` 的白名单只放行
     `UIView.cpp` 包含自身头文件；
   - `MDIWindow` 创建 `UIView`；导出 PDF 用的临时无文档视图仍是基类，不需要交互；
   - 与 DS 的区别：DS 的 `EditTool`、`ExclusiveCommandBus` 以 `UIView*` 构造，工具层
     反过来认识派生类。YiCAD 的命令与工具包含 `UIView.h` 会被 `check_layering.py`
     拦下，只能经 `IDocumentView`/`GuiDocumentView` 认识视图；第二步的总线也照此
     设计，由 `UIView` 持有；
   - `GuiEventHandler` 暂留基类：`IDocumentView` 的 `getEventHandler()`/
     `setCurrentAction()`/`getCurrentAction()` 要求画布实现，第四步随它一起删除。
     `GuiEventHandler` 包含 `ActionInterface.h`，所以 6.7 节那处依赖在第四步才完全解开。

**验证**：Debug、Release 构建通过；`ctest` 4 个测试程序全部通过（`test_interaction`
76 例）；`check_layering.py` 通过；安装后程序能启动。交互回归清单尚待手工核对，
核对结果记入清单第 8 节。

### 9.2 第二步（2026-09-24）

2026-09-24 确认分三个提交落地：① 命令框架与先选后建的试点（删除、总长度）；
② 其余 11 个先选后建命令；③ 块编辑模式与 `blocks.edit`，删除选择 Action。

**提交①：命令框架与试点**

1. **命令框架**（`kernel/actions/`）：`IExclusiveCommand`（含 `CommandEndReason` 与
   `onEndRequested()`）、`BaseExclusiveCommand`、`ExclusiveCommandBus`，总线由 `UIView`
   持有。与 DS 的差异按第 6 节第二步第 1 项：
   - 总线持有命令，结束后销毁；
   - `ExclusiveCommandBus::DispatchScope` 标出一次分发的范围，`UIView` 的每个事件入口
     都包一层。范围内请求的结束延迟到范围结束，范围内被外部结束的命令也延迟到那时才
     销毁（它的工具可能还在调用栈上）；范围外的请求（如选项条按钮）经 0 毫秒定时器；
   - 5.1 节：`approveEnd()` 只问不改，调用方全部征得同意后再 `end()`；
     `UIView::startCommand()`/`killAllActions()`/`killAllActionsOnClose()` 与排他旧 Action
     的启动都先问；回调期间的启动与结束请求一律忽略；
   - 命令经总线拿到文档、视图、工具控制器与选择层（命令不能认识 `UIView`）。
2. **两类新注册**：`CommandRegistry::registerExclusiveCommand`/`registerInstantCommand`，
   `kind()` 区分三类；`bindLegacyType()` 给新类型命令建 `DM::ActionType` 桥接（keyconfig
   仍以枚举为键，第四步删除）。`UIActionHandler::activateCommand` 按类型分派：交互命令
   交给视图的总线，没有视图时不构造；即时命令直接执行，没有视图时 document/view 为空。
   `IExtensionContext`/`ExtensionManager` 同步增加两个注册方法，命名空间规则不变。
3. **输入与界面**：
   - 坐标解析抽成 `GuiCoordinateInput`（放在 `kernel/view/`，旧 Action 栈与命令共用，
     渲染层不反向依赖交互层），逐分支保持原行为；
   - `IViewTool` 增加 `coordinateEvent`/`commandEvent`，`ViewToolControl` 只沿业务栈分发；
     `UIView::commandEvent` 在没有旧 Action、有命令时解析坐标交给命令的工具，同样遵守
     `enableCoordinateInput`/`disableCoordinateInput` 的开关；
   - 右键释放与 `back()`（命令行 "escape"）在没有旧 Action、有命令时经 `ViewToolControl`
     交给命令的工具（主计划 5.7 节：右键释放原先不走 `ViewToolControl`）；
   - 选项条：`GuiDialogFactoryInterface::requestCommandOptions` 与
     `CommandInfo::commandOptionsFactory`，供交互命令注册选项条。第二步的 14 个命令都
     没有选项条，块编辑的选项条见提交③。
4. **选择阶段**：`SelectTool::beginSelectionPhase`/`endSelectionPhase`，约束为实体类型
   过滤与不拖夹点；`BaseExclusiveCommand::enterSelectionPhase`/`leaveSelectionPhase`；
   总线在命令结束时清除约束。先选后建的公共部分是 `SelectFirstCommand`：没有选择集时
   激活选择阶段工具，只接管回车、右键、Esc、其余按键与双击，鼠标事件落到选择层；有
   选择集或回车确认后调用子类的 `onSelectionReady()`，相当于原先选择完成后才构造的那个
   Action。
5. **试点**：`modify.delete`（`ModifyDeleteCommand`，总是先进入选择阶段）、
   `modify.delete_no_select`（即时命令：Delete 键、手写板橡皮擦）、`info.total_length`
   （`InfoTotalLengthCommand`）。删除 `ActionModifyDelete`、`ActionInfoTotalLength` 与
   `info.total_length_no_select`（只供 `ActionSelect` 选择完成后使用）。
6. **测试**：新增 `test_exclusive_command_bus`（16 例：生命周期、替换与否决、三种结束
   原因、`ViewClosing` 忽略否决、回调期间的重入、分发范围内外的延迟结束与延迟销毁、
   激活失败与激活期间完成、挂起恢复、清除选择阶段约束、捕捉设置同步）、
   `test_select_first_commands`（14 例：清单 P1–P11、中键平移、实体类型过滤与新旧并存）、`test_coordinate_input`
   （8 例）；`test_command_registry` 补 6 例、`test_command_dispatch` 与
   `test_extension_manager` 各补 1 例；`FakeDocumentView` 记录 `selectedChanged`。

**与方案的偏差与补充**

1. **选择阶段的提示与空格按代码为准**（2026-09-24 确认）。第 3 节"选择阶段的现有交互"
   与清单 P1、P6 由代码推出，但推错了两处：
   - 提示：`ActionSelect` 启动后立即把 `ActionSelectMultiple` 压在自己上面，后者恢复时
     刷新提示，屏幕上一直是 "Click and drag for the selection window" / "Cancel"（框选
     第二点时 "Choose second edge" / "Back"）；`ActionSelect` 按命令给出的 "Select to …"
     从未显示过，`ActionNoSelectCopyToLayer` 缺 `break` 的缺陷也因此没有可见影响；
   - 空格：`ActionSelectMultiple::keyPressEvent` 不处理空格也不忽略，事件保持接受，
     主窗口因此不结束命令；只有 Esc（`ActionSelect` 转给基类后被忽略）结束全部命令。
   迁移按实际行为实现，清单 P1、P6 已更正。
2. **新旧并存的规则**（2026-09-24 确认，第 6 节第二步第 2 项只写了一个方向）：
   - 启动命令：先按 5.1 节请当前命令让位，再结束全部旧 Action、不再恢复——提前适用
     "被打断的命令不再恢复"（清单第 7 节）。原方案"旧 Action 仍按 `GuiEventHandler`
     规则处理"会让挂起的旧 Action 夹在命令之下，旧 Action 栈清空时恢复的对象要跨两套
     栈判断，放弃；
   - 命令运行中启动旧 Action：命令被挂起，旧 Action 全部结束后恢复，与原先旧 Action
     之间的规则相同，撤销、缩放、图层操作等因此不会结束命令。`GuiEventHandler` 原先
     直接调选择层的三处（从空栈启动时挂起、栈空时恢复、`killAllActions()` 后复位）改为
     调 `ILegacyStackBase`，由 `UIView` 实现为"命令与选择层"；`GuiEventHandler` 随之
     不再包含 `SelectTool.h`。排他的旧 Action（文件新建、打开等）要结束全部，先按 5.1
     节（`Replaced`）请命令让位；
   - 即时命令不碰总线；只结束不可打断（`canBeInterrupt()` 为 false）的旧 Action，与原先
     压栈时一致，否则多行文字属性编辑会继续编辑被删除的文字。
3. **选择阶段复刻 `ActionSelectMultiple` 的细节**：
   - Ctrl+左键不让给平移，双击与其它按键到此为止（清单 P9、P10）；
   - 选中后只刷新选择计数，不发 `selectedChanged`：发了会启动 `ActionSelectedChanged`，
     按上一条规则挂起当前命令，单选多行文字时还会进入属性编辑；
   - 选择层区分"命令活动"与"旧 Action 叠在命令之上"（`SelectTool::Overlay`）：后者时
     选择阶段让出提示与光标。
4. **`Preview` 增加以视图构造的重载**，命令用它预览到自己视图的预览容器，
   `FakeDocumentView` 下也能构造；原构造函数的行为不变。
5. **`ViewToolControl` 在业务栈变化时重新应用光标**：旧 Action 绕过仲裁直接设置光标，
   `m_lastAppliedCursor` 会过期，按它去重会漏掉命令工具激活、停用时的光标切换。
6. **命令行的 "[说明]" 前缀**：交互命令按 ID 反查桥接的枚举（`CommandRegistry::legacyType`）
   取 keyconfig 里的说明；"是否有命令在运行"改由 `GuiDocumentView::hasActiveCommand()`
   回答（主窗口右键、命令行浮窗）。
7. **翻译**：新类的 `tr()` 上下文随类名变化，`YiCAD_zh_cn.ts` 里的译文从原上下文搬到
   新上下文（`SelectTool`、`InfoTotalLengthCommand`），删除类的上下文随之删除。

提交①验证：Debug、Release 构建通过；`ctest` 4 个测试程序全部通过（`test_interaction`
122 例）；`check_layering.py` 通过；安装后程序能启动。交互回归清单尚待手工核对。

**提交②：其余 11 个先选后建命令**

1. **放置工具与命令的预览**：原 `PreviewActionInterface` 拆成两半，放在 `kernel/actions/`：
   - `BasePlaceTool`（事件那一半）：交互状态（`setStatus()` 只在变化时刷新提示，
     `restart()`/`stepBack()` 对应原 `init(status)`/`init(getStatus() - 1)`）、捕捉会话
     （离开画布或被停用时挂起并清除预览，回到画布或被激活时刷新提示、恢复并重绘预览，
     结束时清除捕捉标记并复位正交零点）、光标经 `getCursor()` 仲裁；事件归属与原先经
     `LegacyActionTool` 转发时一致（中键与按着中键的移动让给导航层，按键不接受也不
     下传）；
   - `CommandPreview`（预览那一半），由命令持有；
   - `SelectFirstCommand::activateTool()` 接管放置工具：命令结束时停用并结束捕捉会话，
     旧 Action 叠上来时停用、结束后重新激活。
2. **迁移**：移动 `ModifyMoveCommand`、复制 `ModifyCopyCommand`、旋转
   `ModifyRotateCommand`、缩放 `ModifyScaleCommand`、镜像 `ModifyMirrorCommand`、分解
   `ModifyExplodeCommand`、反向 `ModifyReverseCommand`、复制到剪贴板与剪切
   `EditCopyCommand`、复制到图层 `CopyToLayerCommand`、创建块 `BlocksCreateCommand`；
   有画布交互的各带一个放置工具（`XxxTool`，定义在命令的 .cpp 里），分解、反向在选择集
   就绪后直接完成。删除对应的 10 个 Action 类与它们的 `_no_select` 注册。
   `CommandRegistry` 增加 `exclusiveCommandFactory<>()` 与带枚举桥接的
   `registerExclusiveCommand()` 重载。
3. **测试**：`test_select_first_commands` 补 12 例（13 个命令都进入选择阶段、回车后与
   已有选择集时进入第一步、右键退步、命令行坐标与文本、Esc 与中键平移、旋转/缩放/
   复制/镜像的命令行输入、复制到图层、旧 Action 叠在放置工具之上）；
   `test_command_registry` 的迁移用例覆盖全部 13 个命令的注册类型与枚举桥接。
   不执行提交：默认构造的 `DmDocument` 走事务会崩溃（`test_geometry_spatial_query`
   的说明）。

**与方案的偏差与补充**

1. **原样保留的既有行为**（迁移不顺手修）：
   - 缩放：设置基点时命令行文本被接受但不起作用；设置比例时输入无效仍按上一次鼠标
     位置的比例缩放，还没移动鼠标就输入无效文本时按 0 缩放（原 `ScaleData` 值初始化
     为 0）；
   - 旋转：设置中心时不接受命令行文本（文本被当作新命令），设置角度时接受；
   - 复制、镜像：输入复制数量或 Y/N 后，提示里的数值到状态变化时才刷新；
   - 复制到图层：提示写在命令行；右键退步不清除预览、不重新初始化捕捉器；
   - 复制到剪贴板、剪切：结束时不复位正交零点（原 `ActionEditCopy` 没有设置类型）；
   - 创建块：文档没有块表时停在原地，不结束。
2. **创建块不再是"排他"的**：原 `ActionBlocksCreate::isExclusive()` 让它启动时结束全部
   命令。命令模型里启动任何命令都结束当前命令，过渡期也结束全部旧 Action，效果相同。
3. **可见的细微变化**：
   - 复制到图层没有自己的光标（原先也没有）：从选择阶段进入时沿用选择层复位后的箭头，
     原先是 `ActionSelectMultiple` 留下的选择光标；
   - 命令在自己的事件里结束后，同一次事件里发出的命令行消息仍带"[说明]"前缀（反向的
     结果、复制到图层的"Finish"）：结束延迟到分发结束，这时命令仍是活动命令；原先
     Action 先结束再发消息，前缀取决于栈里是否还有别的 Action。

提交②验证：Debug、Release 构建通过；`ctest` 4 个测试程序全部通过（`test_interaction`
134 例）；`check_layering.py` 通过；安装后程序能启动。交互回归清单尚待手工核对。

**提交③：块编辑模式，删除选择 Action**

1. **编辑模式**：`IEditMode`（`kernel/actions/`），由命令总线持有：
   - `ExclusiveCommandBus::enterEditMode()` 把模式的工具常驻在业务栈底部
     （`ViewToolControl::activateAtBottom()`），模式里启动的命令与旧版 Action 叠在它
     上面；命令启动时模式收起界面（`suspendMode()`），命令结束时恢复（`resumeMode()`），
     旧版 Action 叠在模式上时由 `UIView` 同样处理；
   - 启动命令不问模式；结束全部命令、排他的旧 Action 与视图关闭时先问命令、再问模式
     （`approveEndAll()`/`endAll()`），模式可以否决（视图关闭除外）；
   - 模式退出自己（右键、选项条"完成"）经 `requestExitEditMode()`，延迟规则同命令；
     撤销/重做离开块编辑时 `exitEditMode()` 立即退出。
2. **块编辑**：`BlockEditTool`（模式）取代 `ActionBlocksEdit`。右键弹出"Finish editing
   and save changes?"（是/否/取消）；鼠标按下、移动、左键释放与按键让给选择层；双击
   到此为止（清单 B5）；命令行坐标被接受但不起作用。进入与退出的文档操作
   （`BlockEditEnterCmd`/`BlockEditExitCmd` 事务、嵌套块选择）原样移过来。
   - 编辑块命令 `BlocksEditCommand`（先选后建）：已在块编辑中时不构造命令、给出警告；
     选择集就绪后取第一个块参照，先确定要编辑的块、把模式交给总线，再跑进入的事务，
     然后结束命令。顺序是为了事务触发的撤销栈变化通知能看到模式已经存在，
     `UIActionHandler` 才不会把它当作撤销后的重新进入；
   - `UIActionHandler::slotCmdStateChanged` 改为看总线上有没有编辑模式：文档进入了块
     编辑而视图没有模式时新建模式并 `reenter()`，反之 `exitEditMode()`；
   - 块编辑选项条改为接收 `IBlockEditSession`（模型层接口，`BlockEditTool` 实现），
     选项条与对话框工厂因此不认识交互层的类型；对话框工厂增加
     `requestYesNoCancelDialog()`、`requestNestedBlockSelectDialog()`、
     `requestBlockEditOptions()`，块编辑不再直接包含 `ui/` 的头文件。
3. **删除**：`ActionSelect`、`ActionSelectMultiple`、`ActionBlocksEdit`、
   `makeSelectFirstFactory`、`blocks.edit_no_select` 与 `CommandContext::handler`（只有
   `ActionSelect` 用它回调 `UIActionHandler`）。**至此选择 Action 全部消失**，先选后建的
   14 个命令都是交互命令。
4. **测试**：`test_exclusive_command_bus` 补 9 例（模式常驻栈底、命令叠在模式上与恢复、
   结束全部先问命令再问模式、模式否决与视图关闭、退出的延迟与延迟销毁）；
   `test_select_first_commands` 补 9 例（编辑块没有块参照、清单 B1–B7、结束全部与视图
   关闭时的对话框、选项条"完成"），P1 覆盖 14 个命令；删除 `makeSelectFirstFactory`
   的 2 例。

**与方案的偏差与补充**

1. **结束全部命令遇到块编辑时弹出保存对话框**（2026-09-24 确认）：Esc/空格未被接受、
   Ribbon 的结束全部、排他的旧 Action（文件新建、打开、保存、另存、导出图片，块另存、
   插入块）都弹出与右键相同的对话框，是保存后退出、否放弃后退出、取消即否决（"清空
   选择"也不执行，排他的旧 Action 也不启动）。原先它们结束 `ActionBlocksEdit`，但文档
   仍停在块编辑态，选项条与右键退出都没了——既有缺陷随之消失（清单 B10、B11）。
   视图关闭时不提问、不改动文档，与原先一致。
2. **创建块在块编辑中不再结束块编辑**：原 `ActionBlocksCreate` 是排他的，会连带结束
   `ActionBlocksEdit`（文档仍在块编辑态，块在块内创建）；现在它与别的命令一样叠在编辑
   模式上，结束后回到块编辑（清单 B12）。
3. **块定义不存在时结束命令**：原先非嵌套路径上只给出提示就返回，`ActionBlocksEdit`
   停在"编辑中"而文档并没有进入块编辑；现在提示后结束命令、不进入模式。
4. **可见的细微变化**：
   - 块编辑中没有命令时，命令行消息不再带"[说明]"前缀（原先当前 Action 是
     `ActionBlocksEdit`）；
   - 选项条"完成"后的退出经 0 毫秒定时器：原先在按钮自己的槽函数里就删除了选项条
     控件；
   - 撤销/重做后重新进入时不再查找块参照：原先查到了也没有用到。
5. **翻译**："Cannot edit block references while already editing a block." 在阶段 4 从
   `UIActionHandler` 搬走后一直没有译文，这次随 `BlocksEditCommand` 的上下文补回。

提交③验证：Debug、Release 构建通过；`ctest` 4 个测试程序全部通过（`test_interaction`
150 例）；`check_layering.py` 通过；安装后程序能启动。交互回归清单尚待手工核对。

**第二步的结果**：命令框架、两类新注册、坐标解析与工具的命令行回调、选择阶段约束
都已就位；先选后建的 14 个命令都迁成交互命令，块编辑成为编辑模式，`ActionSelect`、
`ActionSelectMultiple` 与 `makeSelectFirstFactory` 删除。其余 Action 仍经
`LegacyActionTool` 运行，按第三步分批迁移。

### 9.3 第三步（2026-09-24 起）

按 5.2 节调整后的分批，每批一个提交，每批完成后停下核对。

**提交①：视图与核心即时命令**

1. **临时视图工具**：基类 `TransientViewTool`（`kernel/actions/`），`CommandRegistry`
   第四种注册类型 `CommandKind::ViewTool`（`registerViewTool`/`createViewTool`），
   `UIActionHandler::activateCommand` 交给 `UIView::startViewTool()`。由 `UIView`
   持有，不占命令总线：
   - 启动时挂起其下各层，结束时恢复，与原先视图 Action 压在旧 Action 栈顶时一致：
     有旧 Action 时挂起栈顶并收起它的选项条，否则挂起命令（或编辑模式）与选择层；
   - 叠在业务栈顶；进入/离开画布只通知它（原先只通知旧 Action 栈顶）；右键释放、
     命令行 "escape"、命令行输入先交给它；
   - 启动命令、启动旧版 Action、结束全部命令与视图关闭时结束它；
   - 工具在自己的事件处理中请求结束时，经新增的 `ExclusiveCommandBus::post()`
     延迟到这次分发返回之后（分发范围外经 0 毫秒定时器）；
   - 选择层的 `SelectTool::Overlay` 增加 `ViewTool`：提示与光标都归它；画布的
     捕捉标记与提示读"无捕捉"；命令行 "[说明]" 前缀与"是否有命令"都算上它。
2. **平移**：`zoom.pan` 的 `ZoomPanTool`（`src/actions/`）取代 `ActionZoomPan`，逐项
   对照原先经 `LegacyActionTool` 转发时的行为：左键拖动超过 7 像素才平移；右键退出
   并重绘；左键释放只结束一次拖动；中键与中键平移中的移动让给导航层；双击、按键
   到此为止，按键不接受；命令行坐标丢弃、文本不接受。光标经 `getCursor()` 仲裁：
   等待时张开的手，拖动中握紧的手。
3. **即时命令**：`zoom.in`/`zoom.out`（`ZoomCommands.cpp`）、`edit.undo`/`edit.redo`
   （`EditUndoCommand`）、`info.selected`（`InfoSelectedCommand.cpp`）。
   `CommandInfo::instantInterrupt` 登记即时命令执行前如何处理正在运行的命令：
   缩放为 `KeepAll`（原视图 Action 不打断任何命令，多行文字编辑中缩放不结束它），
   其余默认 `EndUninterruptible`（与原先压栈时一致）。`CommandRegistry` 增加带
   legacy 桥接的 `registerInstantCommand` 重载。
4. **滚轮缩放**直接调用视图的 `zoomIn`/`zoomOut`，不再压入 `ActionZoomIn`。
5. **画直线的撤销/重做按钮**直接调用 `EditUndoCommand::run()`，不再嵌套启动
   `ActionEditUndo`（直线本身在第②批迁移）。
6. **删除**：`ActionZoomPan`、`ActionZoomIn`、`ActionEditUndo`、`ActionInfoSelected`，
   以及从未注册、没有调用方的 `ActionSetSnapMode`、`ActionSetSnapRestriction`（捕捉
   模式一直由 `UIActionHandler::slotSnap*`/`slotRestrict*` 直接设置）。
7. **测试**：新增 `test_zoom_pan_tool`（8 例）；`test_exclusive_command_bus` 补 4 例
   （`post()` 在范围内、范围外、结束命令之后、定时器在分发中触发）；
   `test_command_registry` 补 4 例（临时视图工具、带桥接的即时命令与视图工具、
   打断策略、本批迁移的命令注册为新类型）。

**与方案的偏差与补充**

1. **平移模式遇到启动命令或旧版 Action 时结束**，不再挂起后恢复：原 `ActionZoomPan`
   可被打断，新 Action 结束后回到平移模式。这是"被打断的命令不再恢复"的一部分
   （清单第 7 节）。过渡期启动任何旧版 Action 都会结束平移模式，包括图层开关这类
   立即完成的；它们在第⑤批改成即时命令后不再如此。
2. **即时命令不挂起当前命令**：撤销、重做、选中信息原先作为旧 Action 压栈，会挂起
   当前命令（清除预览、收起选项条、刷新提示），结束后恢复；现在与第二步的 Delete 键
   一样直接执行，当前命令不受影响。滚轮缩放同样不再挂起、恢复当前旧 Action，选项条
   不再随每一格滚轮收起又显示。
3. **没有打开图纸时**执行 `zoom.in`/`zoom.out`/`info.selected`：原先"没有视图就
   `trigger()` 再删除"会解引用空的视图或文档，现在什么也不做。
4. **保留的副作用**：`info.selected` 结束时复位视图的正交零点（原 Action 设置了
   Action 类型，`ActionInterface::finish()` 因此复位）。

提交①验证：Debug、Release 构建通过；`ctest` 4 个测试程序全部通过（`test_interaction`
166 例）；`check_layering.py` 通过；安装后程序能启动。交互回归清单尚待手工核对。

**提交②：绘图·直线类**

1. **放置命令的基类** `PlaceCommand`（`kernel/actions/`）：原 `PreviewActionInterface`
   派生、不先选后建的 Action 迁移后的公共部分。启动时构造预览与放置工具（原 `init()`，
   连同清除一次预览）、显示选项条、激活工具；结束时停用工具并结束捕捉会话、收起选项条、
   清除预览（原 `finish()`）；旧 Action 叠上来时停用工具并收起选项条，结束后重新激活并
   显示（原 `suspend()`/`hideOptions()` 与 `resume()`/`showOptions()`）。分工：工具持有交互
   状态机与这次交互采集的点，命令持有选项条参数、预览与提交；选项条上作用于交互状态的
   按钮（撤销、闭合、重做）由命令转给工具。
2. **`BasePlaceTool` 补充**：`restart()` 连同清除预览（原 `PreviewActionInterface::init`）；
   `onFinish()` 钩子承接原 Action 的 `finish()` 覆盖（取消高亮等）；`finishIfOrthogonal()`
   取代 `Snapper::finishOrthogonal()`（后者经 `getCurrentAction()` 结束当前 Action，对命令
   无效，最后一个调用方迁走后删除）；双击、按键、按键释放的钩子。
3. **迁移**（13 个）：直线 `DrawLineCommand`、多段线 `DrawPolylineCommand`、矩形、正多边形
   （中心+角点、中心+切点共用基类 `LinePolygonCommand`）、角平分线
   `DrawLineBisectorCommand`、过点切线、两圆公切线、正交切线、徒手线、射线与构造线
   （`DrawInfiniteLineCommands.cpp` 共用一个工具模板）、点。没有选项条的命令只在自己的
   .cpp 里定义命令类。
4. **选项条**：`UILineOptions`、`UIPolylineOptions`、`UILinePolygonOptions`、
   `UILineBisectorOptions` 改为 `setCommand(IExclusiveCommand*)`，按命令类型
   `dynamic_cast`；`UIDialogFactory::requestCommandOptions` 在注册表之外按命令 ID 分派
   内置命令的选项条，`requestOptions` 的 switch 删去对应分支。
5. **测试**：新增公共夹具 `tests/support/CommandTestFixture.h`（与 `UIView` 相同的装配，
   记录提示、命令行消息与选项条请求）与 `test_draw_line_commands`（22 例：注册与桥接、
   13 个命令的第一步提示与选项条、右键退回、命令行输入、预览、光标、挂起时收起选项条）。

**与方案的偏差与补充**

1. **原样保留的既有行为**：
   - `Commands::checkCommand` 对 help/close/undo 以外的关键字一律返回真
     （`cmd/Commands.cpp` 的 `checkCommand`）。因此画直线时任何命令行文本都被当作
     `redo` 接受；正多边形在前两步输入任何文本都进入"输入边数"（且不接受这段文本）；
     角平分线在前两步输入任何文本都进入"输入长度"，命令行进不了"输入数量"。测试按此
     断言并注明；
   - 多段线与角平分线的事务名是"Add cloud line"；画多段线在第一步输入 help 列出命令后
     不接受；正交切线提交后不清除切线，不移动鼠标再次单击会再画一条；两圆公切线
     提交后"切线有效"标记不复位。
2. **顺手处理的崩溃与泄漏**（行为不变）：两圆公切线在选中第一个圆、还没悬停到第二个时
   结束命令会解引用空指针；正交切线求不出切线时预览会解引用空指针，现在跳过预览；正交
   切线每次移动鼠标都泄漏一条直线。
3. **可见的细微变化**：
   - 中心+切点正多边形原先借用中心+角点的 Action 类型，选项条靠两个类相同的内存布局
     才能设置边数，命令行 "[说明]" 前缀也显示中心+角点的；现在两者共用基类，前缀是
     自己的；
   - 射线、构造线的原类没有 `Q_OBJECT`，`tr()` 落到 `ActionInterface` 的翻译上下文，
     提示与事务名一直显示英文；现在显示译文；
   - 徒手线原先把视图共用的预览容器设为不持有实体后不再复原，此后所有命令的预览
     实体都不被释放；现在命令结束时复原；
   - 角平分线选第一条线时的悬停高亮原先记在函数内的静态变量里，命令结束时不取消；
     现在记在工具里，结束时取消。

提交②验证：Debug、Release 构建通过（Release 的 `YiCAD.exe` 被正在运行的程序占用，
没有重新链接，库与测试程序都已构建）；Debug、Release 的 `ctest` 4 个测试程序全部通过
（`test_interaction` 188 例）；`check_layering.py` 通过；安装后的 Debug 程序能启动。
交互回归清单新增 D1–D16，尚待手工核对。

**提交③：绘图·曲线类**

1. **迁移**（16 个 Action，17 个命令 ID）：圆心圆弧 `DrawArcCommand` 与三点圆弧（同一文件）、
   相切圆弧 `DrawArcTangentialCommand`、圆心/两点/三点画圆（`DrawCircleCommands.cpp`）、
   两切圆 `DrawCircleTan2Command`、三切圆、轴端点椭圆与椭圆弧（一个命令类，两个 ID）、内切
   椭圆、控制点样条与拟合点样条（`DrawSplineCommand.h`，共用基类 `SplineCommand`）、矩形/
   多边形/自由云线（`DrawCloudLineCommand.h`，共用基类 `CloudLineCommand`）、插入图片
   `DrawImageCommand`。填充归 `ext.hatch`（第⑧批）。
2. **`BaseExclusiveCommand::replaceWith()`**：结束本命令并启动另一个命令，取代原 Action 里
   `finish()` 之后 `setCurrentAction(new ...)` 的写法（三点圆弧在命令行切换为圆心圆弧）。
3. **选项条**：`UIArcOptions`、`UIArcTangentialOptions`、`UICircleTan2Options`、
   `UIImageOptions` 改为接收命令；`UISplineOptions`、`UICloudLineOptions` 原先按 Action
   类型分支，现在按命令类型 `dynamic_cast`。`UIDialogFactory` 的命令 ID 表补上 10 个 ID。
4. **做法**：大段几何计算（三切圆求解、云线分段、两圆公切线等）由脚本把原 Action 的成员
   函数机械改写（状态、捕捉、预览、翻译上下文），再手工处理生命周期，避免手抄出错。
5. **测试**：新增 `test_draw_curve_commands`（16 例：注册与桥接、16 个命令的第一步提示、
   选项条与右键结束、插入图片取消对话框时启动失败、逐步提示与命令行输入、三点圆弧切换、
   选项条参数转给工具、云线的结束与错误提示）。

**与方案的偏差与补充**

1. **选项条参数的归属**：圆心圆弧的方向、两切圆的半径、样条的阶数与闭合记在工具正在画的
   数据里（与原 Action 一致：圆心圆弧每画完一段、每次右键退回都复位为逆时针），命令把
   选项条的调用转给工具；相切圆弧的锁定参数、云线的弧长与反向、插入图片的角度与缩放在
   命令上。
2. **原样保留的既有行为**：三点圆弧命令行输入任何文字都切换为圆心圆弧（`checkCommand`
   对 "center" 一律返回真），文字不被接受，随后还会被当作新命令解析；插入图片在指定插入点
   时输入任何文字都进入"输入角度"；椭圆弧用鼠标指定终止角后结束命令、用命令行输入则不
   结束；三种云线画完一条即结束命令；多边形云线点不够时回车只给出错误提示。
3. **插入图片取消选择对话框**：原先 Action 被标记为结束、随后删除；现在命令启动失败，
   效果相同（没有选项条、回到空闲态）。
4. **多边形云线的弧长**：原 Action 没有初始化最小/最大弧长，选项条显示时才设置；现在取
   矩形云线的默认值 5、10，选项条照样覆盖。
5. **去掉的 UI 依赖**：原相切圆弧 Action 包含了没有用到的 `ui_UIArcTangentialOptions.h`。

提交③验证：Debug、Release 构建通过；Debug、Release 的 `ctest` 4 个测试程序全部通过
（`test_interaction` 204 例）；`check_layering.py` 通过；安装后程序能启动。交互回归清单
新增 D17–D24，尚待手工核对。

**提交④：修改与查询**

1. **迁移**（15 个）：查询距离 `InfoDistCommand`、查询角度与面积（`InfoAngleAreaCommands.cpp`）、
   粘贴 `EditPasteCommand`、修改实体属性 `ModifyEntityCommand`、打断与两点打断
   （`ModifyCutCommands.cpp`）、单个偏移 `ModifySingleOffsetCommand`、多段线添加/追加/删除节点
   （`PolylineEditCommands.cpp`）、修剪 `ModifyTrimCommand`、倒角 `ModifyBevelCommand`、圆角
   `ModifyRoundCommand`、延伸 `ModifyExtendCommand`。只有倒角、圆角有头文件（选项条要用命令
   类型），其余只在自己的 .cpp 里定义。
2. **选项参数归命令**：倒角的两段长度与是否修剪、圆角的半径与是否修剪、单个偏移的
   `OffsetData` 都在命令上，工具经命令读写；偏移的选项条仍经 `double&` 直接改写距离，接口
   `requestModifySingleOffsetOptions` 不变。
3. **选项条**：`UIBevelOptions`、`UIRoundOptions` 改为接收命令；命令 ID 表补上
   `modify.bevel`、`modify.round`，`requestOptions` 的 switch 只剩文字与插入块两个分支。
4. **延伸监听视图变化**：原 Action 是 QObject，用槽接 `viewChanged()`；工具不是 QObject，
   改为保存 `QMetaObject::Connection`，连到 `GuiDocumentView::viewChanged`，析构时断开
   （测试用的假视图没有 QObject，跳过）。
5. **修改实体属性的多行文字分支**：仍把旧版 `ActionModifyMText` 叠在命令之上，结束后回到
   本命令；第⑦批迁到 `ext.text` 后改为启动命令。
6. **测试**：新增 `test_modify_commands`（19 例：注册与桥接、15 个命令的第一步提示与右键
   结束、倒角/圆角的选项条与命令行、偏移经选项条改距离后的预览、修剪的按键、修剪与延伸
   结束后实体恢复可见、多段线节点命令的拾取检查、追加节点、查询距离/角度/面积、打断、
   修改实体属性、剪贴板为空时粘贴）。夹具的 `UiRecorder` 增加记录偏移选项条与属性对话框。

**与方案的偏差与补充**

1. **原样保留的既有行为**：
   - `checkCommand` 对 help/close/undo 以外的关键字一律返回真：倒角在前两步输入任何文字都
     进入"输入长度 1"，命令行设不了长度 2、切换不了修剪；圆角输入任何文字都进入"输入半径"，
     "trim" 分支到不了（到得了的话会停在一个没有任何处理的状态，代码里注明）；
   - 修剪只响应小键盘回车（`Qt::Key_Enter`），主键盘回车不切换，按键也不被接受；
   - 多段线添加节点在"指定插入位置"时右键退回到"指定参考点"，但已选的多段线被清空，下一次
     单击提示"No Entity found."，要再右键回到第一步；
   - 删除节点的事务名是"Append polyline point"；追加节点在任何一步右键都直接结束；
   - 粘贴原先没有设置 Action 类型，结束时不复位正交零点，工具照此设置。
2. **修正的实体隐藏问题**：修剪、延伸悬停预览时把光标下的原实体设为不可见，原 Action 在
   修剪右键退回、两者右键或 Esc 结束时都不恢复，这个实体一直不可见（延伸还因此不再能拾取
   它）。现在退回、结束时恢复可见。
3. **单个偏移的提示**：原 Action 只在开始时显示一次"Choose the original entity"，现在离开
   画布再回来时也重新显示。

提交④验证：Debug、Release 构建通过；Debug、Release 的 `ctest` 4 个测试程序全部通过
（`test_interaction` 223 例）；`check_layering.py` 通过；安装后程序能启动。交互回归清单
新增 D25–D34，尚待手工核对。

**提交⑤：扩展 `ext.file`、`ext.layer`、`ext.options`**

1. **三个扩展**（`src/extensions/file|layer|options/`）：
   - `ext.file`：即时命令 `ext.file.new/open/save/save_as/export_image`，按钮注册进宿主占位的
     "文件""导出"面板；
   - `ext.layer`：即时命令 `ext.layer.activate/add/rename/color/delete/freeze/lock/print/
     freeze_all/defreeze_all/lock_all/unlock_all`；
   - `ext.options`：即时命令 `ext.options.general/drawing`，按钮注册进"设置"面板。

   删除 17 个旧 Action（文件 5、图层 10、选项 2），原内置 ID 与枚举桥接随之去掉；
   keyconfig.xml 删去两条别名为空的 `ActionOptionsDrawing`。文字样式（`text.style`）按第 6 节
   的分批留给第⑦批 `ext.text`。
2. **新的打断方式 `InstantInterrupt::EndAll`**：原 `isExclusive()` 的 Action（新建、打开、保存、
   另存为、导出图片）启动前先结束全部命令。`UIView::prepareInstantCommand` 改为返回能否执行：
   先征求命令与编辑模式同意（原因 `Replaced`，与排他的旧 Action 相同），被否决（如块编辑中在
   保存提示里取消）或处在 5.1 节的回调中时不执行，否则结束命令总线、平移模式与旧 Action 栈。
3. **访问宿主**：`IExtensionHost`/`IExtensionContext` 新增 `tabDrawWidget()`（图纸标签页；类型
   在 `ui/`，内核只前置声明）。`UITabDrawWidget` 新增 `getDocumentViews()`，系统设置改了颜色后
   逐个刷新视图，取代遍历 `ApplicationWindow` 的 MDI 区域；`slotFileOpen()` 打开后自己同步捕捉
   设置（原先打开命令与快速访问栏各自同步）。
4. **图层面板仍由宿主构造**：它在 `ApplicationWindow` 里有 46 处引用，搬进扩展超出本批范围；
   面板的按钮改为按 `ext.layer.*` 启动。原 Action 遍历主窗口的图层下拉列表，按按钮指针找到所在
   的行；现在 `ComboBoxData::tagButtons()` 把图层名记在每行按钮上（动态属性，改名时随之更新），
   命令用 `ComboBoxData::layerNameOf(sender)` 找到图层，不再包含 `main/` 的头文件。
5. **宿主按 ID 启动扩展命令**：快速访问栏的新建/保存/另存为、Ctrl+N/O/S、标签栏的"+"启动
   `ext.file.*`，图层面板启动 `ext.layer.*`。移除这两个扩展后这些入口什么也不做，"文件"类目
   不再装配（与移除 `ext.dim` 后标注面板消失同理）。
6. **删除死代码**：`UIActionHandler::slotLayersFreezeAll/slotLayersLockAll` 没有调用方（主计划
   7.10 节已列出），且依赖本批去掉的枚举桥接。
7. **翻译**：图层的 9 个上下文并入 `layer_zh_cn.ts` 的 `LayerExtension` 上下文；按钮文字从主程序
   的 `QObject` 上下文拆到 `FileExtension`、`OptionsExtension`（新建、打开、保存、另存为仍被快速
   访问栏使用，复制而不搬走）。
8. **测试**：新增 `test_host_extensions`（6 例：即时命令与打断方式、原 ID 与枚举桥接不再存在、
   按钮所在面板、没有标签页或文档时什么也不做、Shutdown 后注销、图层行按钮记着图层名）。假扩展
   宿主移到 `tests/support/FakeExtensionHost.h`，与 `test_extension_manager` 共用。

**与方案的偏差与补充**

1. **图层按钮的切换目标**：显示/隐藏、锁定、打印原先取下拉框显示的状态取反，现在取图层自身的
   状态取反；两者由 `updateLayerTable` 同步，正常情况下相同。
2. **扩展的注册顺序**：文件、图层、选项排在 AI、标注之前，"系统设置""图纸设置"仍排在 AI
   扩展的设置页入口之前，与迁移前一致。
3. **图标留在主程序资源里**：新建、打开、保存、另存为的图标快速访问栏也在用；导出图片、图纸
   设置的图标没有搬，只影响资源位置。
4. **没有打开图纸时**：即时命令不占命令总线，新建、打开照样执行，与原先"没有视图时直接
   trigger"一致。

提交⑤验证：Debug、Release 构建通过；Debug、Release 的 `ctest` 4 个测试程序全部通过
（`test_interaction` 229 例）；`check_layering.py` 通过；安装后程序能启动，"文件""绘图""设置"
三个类目与图层面板正常显示（截图核对；这台机器上注入的鼠标点击到不了程序，没有逐个点开类目）。
交互回归清单新增 X1–X6，尚待手工核对。

**提交⑥：扩展 `ext.block`**

1. **块扩展**（`src/extensions/block/`）：
   - 交互命令：创建块 `ext.block.create`、编辑块 `ext.block.edit`（第二步的两个命令与块编辑
     模式 `BlockEditTool` 原样搬入 `commands/`，去掉自注册与枚举桥接，由扩展注册）、插入块
     `ext.block.insert`、定义属性 `ext.block.define_attributes`；
   - 即时命令：删除 `ext.block.delete`、保存 `ext.block.save`、另存为 `ext.block.save_as`、
     导入 `ext.block.import`（`BlockFileCommands`，主体从原 Action 搬来），以及宿主用的
     `ext.block.reenter_edit`；
   - 按钮注册进宿主占位的"绘图/块"面板，顺序与迁移前一致。

   删除 7 个旧 Action（插入准备、插入、删除、保存、另存为、导入、定义属性）；keyconfig.xml 删去
   `ActionBlocksCreate`、`ActionBlocksSave` 共 4 条（别名为空）。
2. **插入块合为一个两阶段命令**：启动时在主窗口右侧弹出块列表（原"插入准备"），这时在画布上
   单击结束命令；在列表里点一个块进入放置（原"插入"），可连续放置，右键回到选块；命令结束时
   关闭并释放块列表。选项（角度、比例、阵列）在命令上，选项条 `UIInsertOptions` 随扩展搬走，
   经 `CommandInfo::commandOptionsFactory` 注册，只在放置阶段显示；`UIDialogFactory` 删去
   `requestInsertOptions`，`requestOptions` 的 switch 只剩文字一个分支。原"插入准备"是排他的：
   命令工厂先按 `InstantInterrupt::EndAll` 结束全部命令与块编辑，被否决时不启动。
3. **撤销/重做后恢复块编辑**：宿主（`UIActionHandler::slotCmdStateChanged`）不再直接构造
   `BlockEditTool`，改为运行 `ext.block.reenter_edit`；只有块扩展的命令会让文档进入块编辑，
   没有它时走不到这里。退出块编辑的一支不依赖块的类型，仍在宿主。
4. **对话框**：`UIBlockListWidget`、`UIBlockSaveAs` 不再依赖 `UIActionHandler`（原先经
   `slotBlocksInsert`/`slotBlocksSave` 启动插入、保存），改为构造时传入回调；这两个槽随之删除。
   其余块相关对话框（新建块、嵌套块选择、属性编辑、属性定义、块编辑选项条）仍在 `ui/`，经
   `GuiDialogFactoryInterface` 调用，本批不搬。
5. **翻译**：7 个按钮文字从 `QObject` 上下文、6 个类的上下文（含 `Ui_InsertOptions`）并入
   `block_zh_cn.ts`；原插入、保存、导入、定义属性的上下文改为新类名。
6. **测试**：新增 `test_block_extension`（7 例：命令类型与打断方式、原 ID 与枚举桥接不再存在、
   按钮所在面板、插入块的两个阶段与选项条、命令行改角度、定义属性取消对话框时启动失败、没有
   文档时即时命令什么也不做）；`test_select_first_commands` 的夹具启动块扩展，命令 ID 改名。

**与方案的偏差与补充**

1. **插入块的细微变化**：
   - 选块阶段清空按键提示（原"插入准备"没有提示，留着上一个命令的）；
   - 放置时在列表里换块，直接换成新块继续放置（原先再叠一个插入 Action，右键要退两次）；
   - 块列表在命令结束时释放（原先隐藏后一直留在内存）；
   - 命令行选项的文字照旧不被接受，随后还会被当作新命令解析（原有行为，测试注明）。
2. **定义属性**：原 ShowDialog 一步改为启动前弹出对话框，取消时启动失败（同插入图片）；
   命令行输入的插入点生效（原 Action 取最后一次鼠标移动的位置，输入的坐标不起作用）。
3. **另存为块**：对话框关闭时释放（原 Action 每次新建一个、从不释放）；原先第三个参数由
   字符串字面量隐式转成 true，对话框一直是模态的，现在显式传 true。
4. **对话框的父窗口**：删除、保存、导入用扩展上下文的主窗口作父窗口（原先文件对话框挂在
   当前 MDI 子窗口上），只影响对话框的初始位置。

提交⑥验证：Debug、Release 构建通过；Debug、Release 的 `ctest` 4 个测试程序全部通过
（`test_interaction` 236 例）；`check_layering.py` 通过；安装后程序能启动，"绘图/块"面板的
7 个按钮正常显示（截图核对）。交互回归清单新增 K1–K6，尚待手工核对。
