# 文件操作

YiCAD 使用 OCD（Open CAD Document）作为原生文件格式，同时支持图片导出和打印功能。文件操作通过"文件"菜单或快速访问工具栏进行。

## 文件管理

### 新建图纸 (ActionFileNew)

创建一个空白的新图纸。

- **快捷键**：`Ctrl+N`
- **操作步骤**：
  1. 点击新建或按 Ctrl+N
  2. 新图纸在 MDI 窗口中作为新标签页打开
  3. 默认包含图层 0，网格自动显示

### 打开图纸 (ActionFileOpen)

打开已有的 OCD 文件。

- **快捷键**：`Ctrl+O`
- **操作步骤**：
  1. 点击打开或按 Ctrl+O
  2. 在文件对话框中选择 `.ocd` 文件
  3. 图纸加载并在新标签页中显示

### 保存 (ActionFileSave)

保存当前图纸到已有文件。

- **快捷键**：`Ctrl+S`
- **操作步骤**：
  1. 如果文件已有关联路径，直接保存
  2. 如果是未保存过的新图纸，弹出另存为对话框

### 另存为 (ActionFileSaveAs)

将当前图纸以新名称或新路径保存。

- **快捷键**：`Ctrl+Shift+S`
- **操作步骤**：
  1. 指定保存路径和文件名
  2. 图纸以 OCD 压缩格式保存

### 导出 (ActionFileExport)

将图纸导出为其他格式。

### 导出图片 (ActionFileExportImage)

将当前视图或选定区域导出为图片文件。

- **操作步骤**：
  1. 选择导出图片命令
  2. 选择导出区域（全部/窗口选择）
  3. 选择图片格式（PNG、JPG、BMP 等）
  4. 设置图片分辨率
  5. 指定保存路径

### 关闭图纸 (ActionFileClose)

关闭当前图纸标签页。如有未保存修改，会提示保存。

- **快捷键**：`Ctrl+W`

### 退出 (ActionFileQuit)

退出 YiCAD 应用程序。所有打开的图纸会提示保存。

- **快捷键**：`Alt+F4`

## 打印与输出

### 打印 (ActionFilePrint)

将图纸输出到打印机。

- **快捷键**：`Ctrl+P`
- **操作步骤**：
  1. 选择打印机
  2. 设置纸张大小（ISO A/B/C、ANSI、Architectural 等标准纸型）
  3. 设置打印比例（1:1、适合纸张、自定义比例）
  4. 设置打印范围（全部图形、窗口选择、当前显示）
  5. 预览打印效果
  6. 确认打印

### 打印预览 (ActionFilePrintPreview)

在屏幕上预览打印效果。

### PDF 导出 (ActionFilePrintPDF)

将图纸导出为 PDF 文件。

- **操作步骤**：
  1. 设置 PDF 参数（纸张大小、比例、范围）
  2. 选择保存路径
  3. 生成 PDF 文件

## 撤销与重做

### 撤销 (ActionEditUndo)

撤销上一步操作。

- **快捷键**：`Ctrl+Z`
- 支持多步撤销，回到之前的任意状态

### 重做 (ActionEditRedo)

恢复被撤销的操作。

- **快捷键**：`Ctrl+Y` 或 `Ctrl+Shift+Z`
- 必须在执行撤销后才能使用

### 剪切/复制/粘贴

- **剪切** (ActionEditCut)：`Ctrl+X`，将选中实体剪切到剪贴板
- **复制** (ActionEditCopy)：`Ctrl+C`，将选中实体复制到剪贴板
- **粘贴** (ActionEditPaste)：`Ctrl+V`，从剪贴板粘贴实体到当前图纸
