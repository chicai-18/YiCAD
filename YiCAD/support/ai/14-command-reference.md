# 命令别名速查表

YiCAD 支持通过命令行输入英文命令或拼音简写来调用各种功能。以下为完整命令别名参考。

## 直线类

| 功能 | 英文命令 | 拼音简写 |
|------|----------|----------|
| 两点直线 | `line`, `li`, `l` | `zx`, `z` |
| 矩形 | `rectangle`, `rectang`, `rect`, `rec` | `jx` |
| 平行线 | `offset`, `parallel`, `o`, `pa` | `pxx`, `px` |
| 过点平行线 | `ptp`, `pp` | `gdpxx`, `gdpx` |
| 角平分线 | `bisect`, `bi` | `jpfx`, `jpf` |
| 点-圆切线 | `tangentpc`, `tanpc` | `qx` |
| 角度画线 | — | `jdx` |
| 水平线 | `horizontal`, `hor` | `spx`, `sp` |
| 竖直线 | `vertical`, `ver` | `szx`, `sz` |
| 圆-圆切线 | — | `qxyy` |
| 正交切线 | — | `zjqx` |
| 正交线 | `perp`, `ortho` | `zj` |
| 相对角度 | — | `xdjd` |
| 多边形(角点) | `polygon2v`, `poly2` | `dbx` |
| 射线 | — | `sx` |
| 构造线 | — | `gzx`, `gz` |

## 曲线类

| 功能 | 英文命令 | 拼音简写 |
|------|----------|----------|
| 三点弧 | `arc`, `a` | `yh`, `h` |
| 相切弧 | — | `xqh` |
| 控制点样条 | `spline`, `spl` | `kzyt` |
| 过点样条 | `spline2`, `stp` | `gdyt` |
| 徒手绘线 | `free`, `fhl` | `tshx` |

## 多段线与云线

| 功能 | 英文命令 | 拼音简写 |
|------|----------|----------|
| 多段线 | `polyline`, `pl` | `ddx` |
| 矩形云线 | — | `jxyx` |
| 多边形云线 | — | `dbxyx` |
| 自由云线 | — | `zyyx` |

## 圆类

| 功能 | 英文命令 | 拼音简写 |
|------|----------|----------|
| 圆心-半径 | `circle`, `c` | `y`, `yuan` |
| 两点圆 | `circle2`, `c2` | `ldy` |
| 三点圆 | `circle3`, `c3` | `sdy` |
| 三切圆 | `tan3`, `ct3` | `xqsy` |

## 椭圆类

| 功能 | 英文命令 | 拼音简写 |
|------|----------|----------|
| 轴椭圆 | — | `ty` |
| 内切椭圆 | `ellipseinscribed`, `ei`, `ie` | `nqty` |

## 标注类

| 功能 | 英文命令 | 拼音简写 |
|------|----------|----------|
| 对齐标注 | `dimaligned`, `da` | `dqbz`, `dq` |
| 线性标注 | `dimlinear`, `dl`, `dr` | `xxbz`, `xx` |
| 半径标注 | `dimradial`, `dimradius` | `bjbz`, `bj` |
| 直径标注 | `dimdiametric`, `dimdiameter`, `dd` | `zjbz`, `zj` |
| 角度标注 | `dimangular`, `dan` | `jdbz`, `jd` |
| 引线标注 | `dimleader`, `ld` | `yxbz`, `yx` |

## 文字类

| 功能 | 英文命令 | 拼音简写 |
|------|----------|----------|
| 单行文字 | `text`, `txt` | `d`, `dhwz` |
| 多行文字 | `mtext`, `mtxt` | `dhwz` |
| 分解文字 | — | `fjwz` |

## 修改类

| 功能 | 英文命令 | 拼音简写 |
|------|----------|----------|
| 移动/复制 | `move`, `mv` | `fz` |
| 旋转 | `rotate`, `ro` | `xz` |
| 缩放 | `scale`, `sz` | `sf` |
| 镜像 | `mirror`, `mi` | `jx` |
| 修剪 | `trim`, `tm` | `xz` |
| 延伸 | — | `yc` |
| 偏移 | — | `py` |
| 倒角 | `bevel`, `bev`, `ch` | `dj` |
| 圆角 | `fillet`, `fi` | `yj` |
| 打断 | `divide`, `cut`, `div`, `di` | `dd` |
| 两点打断 | — | `lddd` |
| 拉伸 | `stretch`, `ss` | `ls` |
| 特性 | `properties`, `prop`, `mp` | `tx` |
| 反向 | `reverse` | `fx` |
| 反转 | `revert`, `rev` | `fz` |
| 分解 | `explode`, `xp` | `fj` |
| 矩形阵列 | `arrayrect`, `ar` | `jz` |

## 测量类

| 功能 | 英文命令 | 拼音简写 |
|------|----------|----------|
| 点对点距离 | `distance`, `dist`, `dpp` | `jl` |
| 两线夹角 | `angle`, `ang` | `jj` |
| 面积 | `area`, `ar` | `mj` |
| 实体信息 | `info` | `ck` |

## 快捷键参考

| 快捷键 | 功能 |
|--------|------|
| `Ctrl+N` | 新建图纸 |
| `Ctrl+O` | 打开文件 |
| `Ctrl+S` | 保存 |
| `Ctrl+Shift+S` | 另存为 |
| `Ctrl+P` | 打印 |
| `Ctrl+W` | 关闭图纸 |
| `Ctrl+Z` | 撤销 |
| `Ctrl+Y` | 重做 |
| `Ctrl+X` | 剪切 |
| `Ctrl+C` | 复制 |
| `Ctrl+V` | 粘贴 |
| `Delete` | 删除选中实体 |
| `Esc` | 取消当前操作 |
| `F8` | 切换正交模式 |
| `鼠标滚轮` | 缩放视图 |
| `按住中键拖动` | 平移视图 |
| `双击中键` | 适合窗口 |
