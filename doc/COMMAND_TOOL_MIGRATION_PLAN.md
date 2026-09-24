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
  `ActionInterface*`）也会在调用方变多之后才改签名。
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

每一批都要求：
- 业务工具经 `getCursor()` 提供光标，删除 `updateMouseCursor()`；
- 命令与工具不直接包含 `ui/`、`main/` 的头文件，需要的能力经
  `GuiDialogFactoryInterface`、`IExtensionContext` 或新增的窄接口获取；
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
5. **目录**：`src/actions/` 目录更名（候选 `src/commands/`），同步 CMake 分区与
   `check_layering.py`。

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
