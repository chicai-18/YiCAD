# -*- coding: utf-8 -*-
"""
生成出图测试（test_render）的参考图纸，见 doc/RENDER_PLAN.md 第 5 节 0.2 步。

输出到 tests/render/drawings/，与生成结果一起提交（小文件，改动一目了然）：

    entities.dxf        每种实体各一两个：点、直线、圆弧、圆、椭圆（整椭圆与椭圆弧）、样条（开放与闭合）、
                        多段线（凸度、宽度、闭合）、SOLID、TRACE、射线、构造线、实心填充、引线、块参照
    linetypes.dxf       五种线型（DASHED、HIDDEN、CENTER、DASHDOT、DOT）× 直线（短于一个周期、1.4、1.6、2.2、长线）、
                        圆弧、两个圆、整椭圆、闭合四边形（线型生成禁用）
    lineweights.dxf     0、0.13、0.25、0.50、1.00、2.11 毫米与 ByLayer 线宽，直线、圆弧、圆
    colors.dxf          ACI 1~7、真彩色、RGB 0,0,0、ByLayer（图层颜色）、ByBlock（块参照不同颜色）、0 层块内实体随插入图层
    blocks.dxf          嵌套块；等比、X≠Y 的非等比（两个方向）、镜像、旋转、阵列插入；块内虚线按两个比例插入（D10）
    far_coords.dxf      离原点 3.5e6 的坐标（测绘坐标量级），毫米级的小图形
    image.dxf           两张光栅图像（render_image.png 与 render_image_checker.png，本脚本生成，同一画笔）与压在上面的直线
    text_shx.dxf        单行文字、多行文字、三类标注，文字样式用 txt.shx（本机没有 SHX 字体时 test_render 跳过）
    text_truetype.dxf   单行文字、多行文字，文字样式用 arial.ttf（系统没有 Arial 时跳过）

图案填充没有参考图：填充图案来自用户自备的 .pat 文件（不入库），本机与 CI 都没有。

另有 autocad_linetype.dxf：用户用 AutoCAD 画的线型对照图纸（第 7 节 D10 说明），不由本脚本生成。

输出为 DXF R2000（AC1015）ASCII，由 YiCadDxfPlugin 导入。生成是确定性的。

用法:
    python tools/gen_render_references.py
    python tools/gen_render_references.py --out-dir D:/refs
"""

import argparse
import math
import os
import struct
import sys
import zlib

NEWLINE = chr(10)

# 与 acad.lin 的英制定义相同的图案（图形单位）；AutoCAD 的线型只在对齐字段 A 之后列出元素
LINETYPES = [
    ('DASHED', 'Dashed __ __ __', [0.5, -0.25]),
    ('HIDDEN', 'Hidden _ _ _ _', [0.25, -0.125]),
    ('CENTER', 'Center ____ _ ____', [1.25, -0.25, 0.25, -0.25]),
    ('DASHDOT', 'Dash dot __ . __', [0.5, -0.25, 0.0, -0.25]),
    ('DOT', 'Dot . . . .', [0.0, -0.25]),
]


class Handles(object):
    """DXF R2000 要求每个对象有唯一句柄（组码 5），十六进制大写。"""

    def __init__(self, start=0x100):
        self._next = start

    def take(self):
        value = self._next
        self._next += 1
        return '%X' % value


class Drawing(object):
    """一张参考图纸：图层、线型、文字样式、块与实体，最后一次写出。"""

    MODEL = '1F'
    PAPER = '1E'

    def __init__(self):
        self.handles = Handles()
        self.layers = [('0', 7, 'CONTINUOUS', -3)]
        self.linetypes = []
        self.styles = [('Standard', 'txt')]
        self.blocks = []            # (名字, 块记录句柄, [实体写出函数])
        self.entities = []          # 模型空间实体写出函数
        self.images = []            # (IMAGEDEF 句柄, 路径)
        self._next_block_record = 0x30

    # -- 资源 --------------------------------------------------------------

    def layer(self, name, color=7, linetype='CONTINUOUS', lineweight=-3):
        self.layers.append((name, color, linetype, lineweight))

    def linetype(self, name, description, elements):
        self.linetypes.append((name, description, elements))

    def style(self, name, font):
        self.styles.append((name, font))

    def block(self, name):
        """新建一个块，返回往块里加实体的列表。"""
        handle = '%X' % self._next_block_record
        self._next_block_record += 1
        items = []
        self.blocks.append((name, handle, items))
        return handle, items

    # -- 写出 --------------------------------------------------------------

    def write(self, path):
        with open(path, 'w', encoding='utf-8', newline='') as fp:
            w = Writer(fp, self.handles)
            w.header()
            w.tables(self)
            w.begin('BLOCKS')
            w.block_begin(self.MODEL, '*Model_Space')
            w.block_end(self.MODEL)
            w.block_begin(self.PAPER, '*Paper_Space')
            w.block_end(self.PAPER)
            for name, handle, items in self.blocks:
                w.block_begin(handle, name)
                for item in items:
                    item(w, handle)
                w.block_end(handle)
            w.end()
            w.begin('ENTITIES')
            for item in self.entities:
                item(w, self.MODEL)
            w.end()
            w.objects(self.images)
            w.tag(0, 'EOF')


class Writer(object):
    """极简 DXF R2000 写出器，结构与 tools/gen_benchmark_drawings.py 相同。"""

    def __init__(self, stream, handles):
        self._out = stream
        self._handles = handles

    def tag(self, code, value):
        self._out.write('%d%s%s%s' % (code, NEWLINE, value, NEWLINE))

    def num(self, code, value):
        # 定点表示，避免科学计数法；3.5e6 量级的坐标也保留 6 位小数
        self._out.write('%d%s%.6f%s' % (code, NEWLINE, value, NEWLINE))

    def point(self, code, x, y):
        self.num(code, x)
        self.num(code + 10, y)
        self.num(code + 20, 0.0)

    def handle(self):
        return self._handles.take()

    def begin(self, name):
        self.tag(0, 'SECTION')
        self.tag(2, name)

    def end(self):
        self.tag(0, 'ENDSEC')

    def header(self):
        self.begin('HEADER')
        self.tag(9, '$ACADVER')
        self.tag(1, 'AC1015')
        self.tag(9, '$DWGCODEPAGE')
        self.tag(3, 'ANSI_1252')
        self.tag(9, '$INSUNITS')
        self.tag(70, '4')
        self.tag(9, '$LTSCALE')
        self.num(40, 1.0)
        self.tag(9, '$HANDSEED')
        self.tag(5, 'FFFFFF')
        self.end()

    def _table_begin(self, name, count):
        self.tag(0, 'TABLE')
        self.tag(2, name)
        self.tag(5, self.handle())
        self.tag(100, 'AcDbSymbolTable')
        self.tag(70, str(count))

    def _record(self, kind, subclass, name):
        self.tag(0, kind)
        self.tag(5, self.handle())
        self.tag(100, 'AcDbSymbolTableRecord')
        self.tag(100, subclass)
        self.tag(2, name)
        self.tag(70, '0')

    def tables(self, drawing):
        self.begin('TABLES')

        self._table_begin('LTYPE', 3 + len(drawing.linetypes))
        for name, descr in (('ByBlock', ''), ('ByLayer', ''), ('CONTINUOUS', 'Solid line')):
            self._record('LTYPE', 'AcDbLinetypeTableRecord', name)
            self.tag(3, descr)
            self.tag(72, '65')
            self.tag(73, '0')
            self.num(40, 0.0)
        for name, descr, elements in drawing.linetypes:
            self._record('LTYPE', 'AcDbLinetypeTableRecord', name)
            self.tag(3, descr)
            self.tag(72, '65')
            self.tag(73, str(len(elements)))
            self.num(40, sum(abs(e) for e in elements))
            for e in elements:
                self.num(49, e)
                self.tag(74, '0')
        self.tag(0, 'ENDTAB')

        self._table_begin('LAYER', len(drawing.layers))
        for name, color, linetype, lineweight in drawing.layers:
            self._record('LAYER', 'AcDbLayerTableRecord', name)
            self.tag(62, str(color))
            self.tag(6, linetype)
            self.tag(370, str(lineweight))
            self.tag(390, 'F')
        self.tag(0, 'ENDTAB')

        self._table_begin('STYLE', len(drawing.styles))
        for name, font in drawing.styles:
            self._record('STYLE', 'AcDbTextStyleTableRecord', name)
            self.num(40, 0.0)
            self.num(41, 1.0)
            self.num(50, 0.0)
            self.tag(71, '0')
            self.num(42, 2.5)
            self.tag(3, font)
            self.tag(4, '')
        self.tag(0, 'ENDTAB')

        self._table_begin('APPID', 1)
        self._record('APPID', 'AcDbRegAppTableRecord', 'ACAD')
        self.tag(0, 'ENDTAB')

        self.tag(0, 'TABLE')
        self.tag(2, 'DIMSTYLE')
        self.tag(5, self.handle())
        self.tag(100, 'AcDbSymbolTable')
        self.tag(70, '1')
        self.tag(100, 'AcDbDimStyleTable')
        self.tag(71, '0')
        self.tag(0, 'DIMSTYLE')
        self.tag(105, self.handle())
        self.tag(100, 'AcDbSymbolTableRecord')
        self.tag(100, 'AcDbDimStyleTableRecord')
        self.tag(2, 'Standard')
        self.tag(70, '0')
        self.num(41, 0.18)          # DIMASZ 箭头
        self.num(140, 0.18)         # DIMTXT 文字高
        self.tag(0, 'ENDTAB')

        records = [('*Model_Space', Drawing.MODEL), ('*Paper_Space', Drawing.PAPER)]
        records += [(name, handle) for name, handle, _ in drawing.blocks]
        self._table_begin('BLOCK_RECORD', len(records))
        for name, handle in records:
            self.tag(0, 'BLOCK_RECORD')
            self.tag(5, handle)
            self.tag(100, 'AcDbSymbolTableRecord')
            self.tag(100, 'AcDbBlockTableRecord')
            self.tag(2, name)
            self.tag(70, '0')
            self.tag(280, '1')
            self.tag(281, '0')
        self.tag(0, 'ENDTAB')

        self.end()

    def block_begin(self, owner, name):
        self.tag(0, 'BLOCK')
        self.tag(5, self.handle())
        self.tag(330, owner)
        self.tag(100, 'AcDbEntity')
        self.tag(8, '0')
        self.tag(100, 'AcDbBlockBegin')
        self.tag(2, name)
        self.tag(70, '0')
        self.point(10, 0.0, 0.0)
        self.tag(3, name)
        self.tag(1, '')

    def block_end(self, owner):
        self.tag(0, 'ENDBLK')
        self.tag(5, self.handle())
        self.tag(330, owner)
        self.tag(100, 'AcDbEntity')
        self.tag(8, '0')
        self.tag(100, 'AcDbBlockEnd')

    def objects(self, images):
        self.begin('OBJECTS')
        self.tag(0, 'DICTIONARY')
        self.tag(5, self.handle())
        self.tag(100, 'AcDbDictionary')
        self.tag(281, '1')
        for handle, path, width, height in images:
            self.tag(0, 'IMAGEDEF')
            self.tag(5, handle)
            self.tag(100, 'AcDbRasterImageDef')
            self.tag(90, '0')
            self.tag(1, path)
            self.num(10, float(width))
            self.num(20, float(height))
            self.num(11, 1.0)
            self.num(21, 1.0)
            self.tag(280, '1')
            self.tag(281, '0')
        self.end()

    # -- 实体 --------------------------------------------------------------

    def head(self, kind, owner, attrs, subclass):
        """实体通用头。attrs：layer、color（ACI，0 为 ByBlock，256 为 ByLayer）、rgb、linetype、lineweight。"""
        self.tag(0, kind)
        self.tag(5, self.handle())
        self.tag(330, owner)
        self.tag(100, 'AcDbEntity')
        self.tag(8, attrs.get('layer', '0'))
        if 'linetype' in attrs:
            self.tag(6, attrs['linetype'])
        if 'color' in attrs:
            self.tag(62, str(attrs['color']))
        if 'lineweight' in attrs:
            self.tag(370, str(attrs['lineweight']))
        if 'rgb' in attrs:
            r, g, b = attrs['rgb']
            self.tag(420, str((r << 16) | (g << 8) | b))
        if subclass:
            self.tag(100, subclass)


# ---------------------------------------------------------------------------
# 实体构造：返回 (writer, owner) -> None 的函数，放进 Drawing.entities 或块的列表
# ---------------------------------------------------------------------------

def line(x1, y1, x2, y2, **attrs):
    def emit(w, owner):
        w.head('LINE', owner, attrs, 'AcDbLine')
        w.point(10, x1, y1)
        w.point(11, x2, y2)
    return emit


def point(x, y, **attrs):
    def emit(w, owner):
        w.head('POINT', owner, attrs, 'AcDbPoint')
        w.point(10, x, y)
    return emit


def circle(cx, cy, r, **attrs):
    def emit(w, owner):
        w.head('CIRCLE', owner, attrs, 'AcDbCircle')
        w.point(10, cx, cy)
        w.num(40, r)
    return emit


def arc(cx, cy, r, a1, a2, **attrs):
    def emit(w, owner):
        w.head('ARC', owner, attrs, 'AcDbCircle')
        w.point(10, cx, cy)
        w.num(40, r)
        w.tag(100, 'AcDbArc')
        w.num(50, a1)
        w.num(51, a2)
    return emit


def ellipse(cx, cy, mx, my, ratio, p1=0.0, p2=2.0 * math.pi, **attrs):
    def emit(w, owner):
        w.head('ELLIPSE', owner, attrs, 'AcDbEllipse')
        w.point(10, cx, cy)
        w.point(11, mx, my)
        w.num(40, ratio)
        w.num(41, p1)
        w.num(42, p2)
    return emit


def lwpolyline(vertices, closed=False, width=0.0, **attrs):
    """vertices：(x, y) 或 (x, y, 凸度) 或 (x, y, 凸度, 起始宽, 终止宽)。"""
    def emit(w, owner):
        w.head('LWPOLYLINE', owner, attrs, 'AcDbPolyline')
        w.tag(90, str(len(vertices)))
        w.tag(70, '1' if closed else '0')
        w.num(43, width)
        for v in vertices:
            w.num(10, v[0])
            w.num(20, v[1])
            if len(v) >= 5:
                w.num(40, v[3])
                w.num(41, v[4])
            if len(v) >= 3 and v[2] != 0.0:
                w.num(42, v[2])
    return emit


def spline(control_points, closed=False, **attrs):
    """三次样条，钳制节点矢量。"""
    def emit(w, owner):
        degree = 3
        n = len(control_points)
        knots = [0.0] * (degree + 1)
        inner = n - degree - 1
        knots += [float(i) for i in range(1, inner + 1)]
        knots += [float(inner + 1)] * (degree + 1)
        w.head('SPLINE', owner, attrs, 'AcDbSpline')
        w.num(210, 0.0)
        w.num(220, 0.0)
        w.num(230, 1.0)
        w.tag(70, str(8 | (1 if closed else 0)))
        w.tag(71, str(degree))
        w.tag(72, str(len(knots)))
        w.tag(73, str(n))
        w.tag(74, '0')
        w.num(42, 0.0000001)
        w.num(43, 0.0000001)
        for k in knots:
            w.num(40, k)
        for x, y in control_points:
            w.point(10, x, y)
    return emit


def spline_fit(fit_points, closed=False, **attrs):
    """三次样条，只给拟合点（YiCAD 按拟合点求控制点）。"""
    def emit(w, owner):
        w.head('SPLINE', owner, attrs, 'AcDbSpline')
        w.num(210, 0.0)
        w.num(220, 0.0)
        w.num(230, 1.0)
        w.tag(70, str(8 | (1 if closed else 0)))
        w.tag(71, '3')
        w.tag(72, '0')
        w.tag(73, '0')
        w.tag(74, str(len(fit_points)))
        w.num(44, 0.0000000001)
        for x, y in fit_points:
            w.point(11, x, y)
    return emit


def solid(p1, p2, p3, p4, kind='SOLID', **attrs):
    """DXF 的第三、四点次序与绕向相反：画出的四边形是 p1 p2 p4 p3。"""
    def emit(w, owner):
        w.head(kind, owner, attrs, 'AcDbTrace')
        w.point(10, *p1)
        w.point(11, *p2)
        w.point(12, *p3)
        w.point(13, *p4)
    return emit


def ray(x, y, dx, dy, kind='RAY', **attrs):
    def emit(w, owner):
        subclass = 'AcDbRay' if kind == 'RAY' else 'AcDbXline'
        w.head(kind, owner, attrs, subclass)
        w.point(10, x, y)
        length = math.hypot(dx, dy)
        w.point(11, dx / length, dy / length)
    return emit


def solid_hatch(points, **attrs):
    def emit(w, owner):
        w.head('HATCH', owner, attrs, 'AcDbHatch')
        w.point(10, 0.0, 0.0)
        w.num(210, 0.0)
        w.num(220, 0.0)
        w.num(230, 1.0)
        w.tag(2, 'SOLID')
        w.tag(70, '1')
        w.tag(71, '0')
        w.tag(91, '1')
        w.tag(92, '3')
        w.tag(72, '0')
        w.tag(73, '1')
        w.tag(93, str(len(points)))
        for x, y in points:
            w.num(10, x)
            w.num(20, y)
        w.tag(97, '0')
        w.tag(75, '0')
        w.tag(76, '1')
        w.num(47, 1.0)
        w.tag(98, '0')
    return emit


def solid_hatch_edges(points, **attrs):
    """实心填充，边界由直线边组成（边界类型 1），而不是一条多段线。"""
    def emit(w, owner):
        w.head('HATCH', owner, attrs, 'AcDbHatch')
        w.point(10, 0.0, 0.0)
        w.num(210, 0.0)
        w.num(220, 0.0)
        w.num(230, 1.0)
        w.tag(2, 'SOLID')
        w.tag(70, '1')
        w.tag(71, '0')
        w.tag(91, '1')
        w.tag(92, '1')
        w.tag(93, str(len(points)))
        for i, (x1, y1) in enumerate(points):
            x2, y2 = points[(i + 1) % len(points)]
            w.tag(72, '1')
            w.num(10, x1)
            w.num(20, y1)
            w.num(11, x2)
            w.num(21, y2)
        w.tag(97, '0')
        w.tag(75, '0')
        w.tag(76, '1')
        w.num(47, 1.0)
        w.tag(98, '0')
    return emit


def insert(name, x, y, sx=1.0, sy=1.0, rotation=0.0, cols=1, rows=1, dx=0.0, dy=0.0, **attrs):
    def emit(w, owner):
        w.head('INSERT', owner, attrs, 'AcDbBlockReference')
        w.tag(2, name)
        w.point(10, x, y)
        w.num(41, sx)
        w.num(42, sy)
        w.num(43, 1.0)
        w.num(50, rotation)
        if cols > 1 or rows > 1:
            w.tag(70, str(cols))
            w.tag(71, str(rows))
            w.num(44, dx)
            w.num(45, dy)
    return emit


def text(x, y, height, content, style='Standard', rotation=0.0, **attrs):
    def emit(w, owner):
        w.head('TEXT', owner, attrs, 'AcDbText')
        w.point(10, x, y)
        w.num(40, height)
        w.tag(1, content)
        w.num(50, rotation)
        w.tag(7, style)
        w.tag(100, 'AcDbText')
    return emit


def mtext(x, y, height, width, content, style='Standard', **attrs):
    def emit(w, owner):
        w.head('MTEXT', owner, attrs, 'AcDbMText')
        w.point(10, x, y)
        w.num(40, height)
        w.num(41, width)
        w.tag(71, '1')
        w.tag(72, '1')
        w.tag(1, content)
        w.tag(7, style)
    return emit


def leader(points, **attrs):
    def emit(w, owner):
        w.head('LEADER', owner, attrs, 'AcDbLeader')
        w.tag(3, 'Standard')
        w.tag(71, '1')
        w.tag(72, '0')
        w.tag(73, '3')
        w.tag(76, str(len(points)))
        for x, y in points:
            w.point(10, x, y)
    return emit


def dimension(kind, points, text_point, angle=0.0, **attrs):
    """kind：'linear'（10 尺寸线点、13/14 两个定义点）、'aligned'（同上）、'radius'（10 圆心、15 圆上点）。"""
    def emit(w, owner):
        w.head('DIMENSION', owner, attrs, 'AcDbDimension')
        w.tag(2, '')
        type_code = {'linear': 0, 'aligned': 1, 'radius': 4}[kind]
        w.point(10, *points[0])
        w.point(11, *text_point)
        w.tag(70, str(32 | type_code))
        w.tag(3, 'Standard')
        if kind in ('linear', 'aligned'):
            w.tag(100, 'AcDbAlignedDimension')
            w.point(13, *points[1])
            w.point(14, *points[2])
            if kind == 'linear':
                w.num(50, angle)
                w.tag(100, 'AcDbRotatedDimension')
        else:
            w.tag(100, 'AcDbRadialDimension')
            w.point(15, *points[1])
            w.num(40, 0.0)
    return emit


def image(handle, x, y, u, v, width, height, **attrs):
    """u、v：一个像素在世界里的向量。"""
    def emit(w, owner):
        w.head('IMAGE', owner, attrs, 'AcDbRasterImage')
        w.tag(90, '0')
        w.point(10, x, y)
        w.point(11, *u)
        w.point(12, *v)
        w.num(13, float(width))
        w.num(23, float(height))
        w.tag(340, handle)
        w.tag(70, '1')
        w.tag(280, '0')
        w.tag(281, '50')
        w.tag(282, '50')
        w.tag(283, '0')
    return emit


# ---------------------------------------------------------------------------
# 各参考图纸
# ---------------------------------------------------------------------------

def gen_entities():
    d = Drawing()
    e = d.entities
    # 第一行：点、直线、圆弧、圆、椭圆、椭圆弧
    e.append(point(0.0, 0.0))
    e.append(point(0.5, 0.0))
    e.append(line(1.5, -1.0, 3.5, 1.0))
    e.append(arc(6.0, 0.0, 1.0, 30.0, 240.0))
    e.append(circle(9.5, 0.0, 1.2))
    e.append(ellipse(13.5, 0.0, 1.8, 0.6, 0.5))
    e.append(ellipse(17.5, 0.0, 0.0, 1.5, 0.6, 0.5, 4.0))
    # 第二行：样条（开放、闭合）、多段线（凸度、宽度、闭合）
    e.append(spline([(0.0, -4.0), (1.0, -2.5), (2.0, -5.0), (3.0, -3.0), (4.0, -4.5)]))
    # 闭合样条用拟合点给出，首尾重合（YiCAD 导入闭合样条的要求，HostApi::createSpline）
    e.append(spline_fit([(5.5, -4.0), (6.5, -2.8), (8.0, -3.2), (8.5, -4.8), (7.0, -5.5), (5.8, -5.0),
                         (5.5, -4.0)], closed=True))
    e.append(lwpolyline([(10.0, -5.0, 0.0), (11.5, -5.0, 0.5), (12.5, -3.0, 0.0), (10.0, -3.0, -1.0)], closed=True))
    e.append(lwpolyline([(14.0, -5.0, 0.0, 0.0, 0.6), (15.5, -3.0, 0.4, 0.6, 0.1), (17.5, -4.5, 0.0, 0.1, 0.1)]))
    e.append(lwpolyline([(18.5, -5.0), (19.5, -3.0), (20.0, -5.0)], width=0.25))
    # 第三行：SOLID、TRACE、实心填充、引线
    e.append(solid((0.0, -9.0), (2.0, -9.0), (0.0, -7.0), (2.5, -7.5), color=3))
    e.append(solid((3.5, -9.0), (5.0, -8.5), (3.5, -7.5), (5.0, -7.0), kind='TRACE', color=4))
    # 实心填充两种边界：多段线（AutoCAD 的常见写法）与直线边。多段线边界的现在画不出来：
    # Edge::getPoints（FindClosedRegion.cpp）没有多段线分支，剖分得不到三角形；修好后基准图像随之更新
    e.append(solid_hatch([(7.0, -9.0), (9.5, -9.0), (10.0, -7.5), (8.0, -6.8), (6.8, -7.8)], color=5))
    e.append(solid_hatch_edges([(10.5, -9.0), (11.5, -9.0), (11.5, -8.0), (10.5, -8.0)], color=6))
    e.append(leader([(12.0, -9.0), (13.5, -7.0), (15.0, -7.0)]))
    # 块参照：块里一个方框加对角线
    _, items = d.block('BOX')
    items.append(lwpolyline([(-0.5, -0.5), (0.5, -0.5), (0.5, 0.5), (-0.5, 0.5)], closed=True))
    items.append(line(-0.5, -0.5, 0.5, 0.5))
    e.append(insert('BOX', 17.0, -8.0, 1.5, 1.5, 15.0))
    # 射线与构造线：穿过整个画面
    e.append(ray(0.0, -11.0, 1.0, 0.2, color=1))
    e.append(ray(10.0, 2.5, 1.0, -0.4, kind='XLINE', color=2))
    return d


def gen_linetypes():
    d = Drawing()
    for name, descr, elements in LINETYPES:
        d.linetype(name, descr, elements)
    e = d.entities
    row = 0.0
    for name, _, _ in LINETYPES:
        attrs = {'linetype': name}
        # 直线：0.5（短于一个周期）、1.4、1.6、2.2（D10 对照长度）、6
        x = 0.0
        for length in (0.5, 1.4, 1.6, 2.2, 6.0):
            e.append(line(x, row, x + length, row, **attrs))
            x += length + 0.6
        # 圆弧（半径 1，0° 到 100°）、圆（半径 1 与 1.15）、整椭圆（1 × 0.5）、闭合四边形
        e.append(arc(x + 0.5, row - 0.8, 1.0, 0.0, 100.0, **attrs))
        e.append(circle(x + 3.2, row - 0.2, 1.0, **attrs))
        e.append(circle(x + 5.8, row - 0.2, 1.15, **attrs))
        e.append(ellipse(x + 8.5, row - 0.2, 1.0, 0.0, 0.5, **attrs))
        e.append(lwpolyline([(x + 10.0, row - 1.0), (x + 11.2, row + 0.6), (x + 12.0, row - 1.0),
                             (x + 10.4, row - 1.0)], closed=True, **attrs))
        row -= 3.0
    return d


def gen_lineweights():
    d = Drawing()
    d.layer('LW50', 7, 'CONTINUOUS', 50)
    e = d.entities
    y = 0.0
    for weight in (0, 13, 25, 50, 100, 211):
        e.append(line(0.0, y, 8.0, y + 1.0, lineweight=weight))
        e.append(arc(11.0, y, 1.0, 0.0, 180.0, lineweight=weight))
        e.append(circle(14.0, y, 0.8, lineweight=weight))
        y -= 2.5
    # ByLayer：图层线宽 0.5 毫米
    e.append(line(0.0, y, 8.0, y + 1.0, layer='LW50'))
    e.append(circle(14.0, y, 0.8, layer='LW50'))
    return d


def gen_colors():
    d = Drawing()
    d.layer('RED', 1)
    d.layer('BLUE', 5)
    e = d.entities
    for aci in range(1, 8):
        x = (aci - 1) * 2.0
        e.append(line(x, 0.0, x + 1.5, 1.5, color=aci))
        e.append(circle(x + 0.75, -1.0, 0.6, color=aci))
    # 真彩色、RGB 全 0（现在画成白色，P12）、ByLayer
    e.append(line(0.0, -3.5, 3.0, -2.5, rgb=(255, 128, 0)))
    e.append(line(4.0, -3.5, 7.0, -2.5, rgb=(0, 0, 0)))
    e.append(line(8.0, -3.5, 11.0, -2.5, layer='RED'))
    e.append(line(12.0, -3.5, 15.0, -2.5, layer='BLUE', color=256))
    # 块：ByBlock 颜色的实体随块参照的颜色；0 层的 ByLayer 实体随块参照的图层；自带颜色的实体不变
    _, items = d.block('MIX')
    items.append(circle(0.0, 0.0, 0.8, color=0))
    items.append(line(-0.8, -0.8, 0.8, 0.8, color=0))
    items.append(line(-0.8, 0.8, 0.8, -0.8, layer='0'))
    items.append(lwpolyline([(-1.0, -1.0), (1.0, -1.0), (1.0, 1.0), (-1.0, 1.0)], closed=True, color=3))
    e.append(insert('MIX', 1.5, -6.0, color=1))
    e.append(insert('MIX', 5.0, -6.0, color=5))
    e.append(insert('MIX', 8.5, -6.0, layer='RED'))
    e.append(insert('MIX', 12.0, -6.0, layer='BLUE', rgb=(200, 200, 0)))
    return d


def gen_blocks():
    d = Drawing()
    for name, descr, elements in LINETYPES[:1]:
        d.linetype(name, descr, elements)
    e = d.entities
    # 内层块：圆加一条直线（旋转后能看出方向）
    _, inner = d.block('INNER')
    inner.append(circle(0.0, 0.0, 0.5))
    inner.append(line(0.0, 0.0, 0.5, 0.0))
    # 外层块：方框，内含旋转 30° 的内层块
    _, outer = d.block('OUTER')
    outer.append(lwpolyline([(-1.0, -1.0), (1.0, -1.0), (1.0, 1.0), (-1.0, 1.0)], closed=True))
    outer.append(insert('INNER', 0.2, 0.1, 1.0, 1.0, 30.0))
    outer.append(arc(0.0, 0.0, 0.9, 200.0, 340.0))
    # 等比、X≠Y（两个方向）、镜像、旋转
    e.append(insert('OUTER', 0.0, 0.0))
    e.append(insert('OUTER', 4.0, 0.0, 2.0, 1.0))
    e.append(insert('OUTER', 8.5, 0.0, 1.0, 2.0))
    e.append(insert('OUTER', 12.0, 0.0, -1.0, 1.0))
    e.append(insert('OUTER', 15.5, 0.0, 1.0, 1.0, 45.0))
    # 阵列插入：3 列 2 行
    e.append(insert('INNER', 0.0, -4.0, 1.0, 1.0, 0.0, cols=3, rows=2, dx=1.5, dy=-1.5))
    # 块内虚线按两个比例插入：划线长度应相同（D10）
    _, dashed = d.block('DASHED_TRI')
    dashed.append(lwpolyline([(0.0, 0.0), (1.0, 0.0), (0.5, 0.8)], closed=True, linetype='DASHED'))
    e.append(insert('DASHED_TRI', 6.0, -6.0, 1.0, 1.0))
    e.append(insert('DASHED_TRI', 8.0, -7.5, 5.0, 5.0))
    return d


def gen_far_coords():
    # CGCS2000 投影坐标量级：Y≈3,500,000，X≈500,000；图形只有毫米级
    d = Drawing()
    x0, y0 = 500123.456, 3500789.012
    e = d.entities
    e.append(lwpolyline([(x0, y0), (x0 + 0.1, y0), (x0 + 0.1, y0 + 0.1), (x0, y0 + 0.1)], closed=True))
    e.append(circle(x0 + 0.05, y0 + 0.05, 0.03))
    e.append(line(x0, y0, x0 + 0.1, y0 + 0.1))
    e.append(line(x0 + 0.02, y0 + 0.08, x0 + 0.021, y0 + 0.081, color=1))
    e.append(arc(x0 + 0.05, y0 + 0.05, 0.045, 0.0, 90.0, color=3))
    return d


def gen_image(images):
    """images：[(文件名, 宽, 高), ...]，第一张是渐变图，第二张是棋盘格。"""
    d = Drawing()
    handles = []
    for name, width, height in images:
        handle = d.handles.take()
        handles.append(handle)
        d.images.append((handle, name, width, height))
    e = d.entities
    # 渐变图每像素 0.05 个图形单位，3.2 × 2.4
    (_, width, height), (_, checker_width, checker_height) = images
    e.append(image(handles[0], 0.0, 0.0, (0.05, 0.0), (0.0, 0.05), width, height))
    # 棋盘格与渐变图在同一图层、同一颜色，即同一画笔：每张图片要贴自己的纹理（RENDER_PLAN.md 第 10 节阶段 1）
    e.append(image(handles[1], 4.0, 0.0, (0.075, 0.0), (0.0, 0.075), checker_width, checker_height))
    e.append(line(-0.5, -0.5, 3.7, 2.9, color=1))
    e.append(circle(1.6, 1.2, 0.8, color=3))
    return d


def gen_text(style_font):
    d = Drawing()
    d.style('REF', style_font)
    e = d.entities
    e.append(text(0.0, 0.0, 0.5, 'AaBb 0123 +-%', style='REF'))
    e.append(text(0.0, -1.2, 0.35, 'ROTATED 15', style='REF', rotation=15.0))
    e.append(mtext(0.0, -3.0, 0.3, 6.0, 'MTEXT line one\\Pline two, wraps if long enough', style='REF'))
    if style_font == 'txt':
        e.append(line(0.0, -6.0, 4.0, -6.0))
        e.append(dimension('linear', [(2.0, -5.0), (0.0, -6.0), (4.0, -6.0)], (2.0, -5.0)))
        e.append(line(5.0, -6.5, 7.0, -5.0))
        e.append(dimension('aligned', [(5.6, -4.8), (5.0, -6.5), (7.0, -5.0)], (5.6, -4.8)))
        e.append(circle(9.5, -6.0, 1.0))
        e.append(dimension('radius', [(9.5, -6.0), (10.207107, -5.292893)], (10.6, -5.0)))
    return d


def gradient_pixel(x, y, width, height):
    """横向色相渐变、纵向亮度渐变，外加一条对角线与 1 像素边框。"""
    if x == 0 or y == 0 or x == width - 1 or y == height - 1 or abs(x * height - y * width) < width:
        return 255, 255, 255
    t = x / float(width - 1)
    s = 0.35 + 0.65 * (1.0 - y / float(height - 1))
    r = int(255 * s * max(0.0, 1.0 - 2.0 * t))
    g = int(255 * s * (1.0 - abs(2.0 * t - 1.0)))
    b = int(255 * s * max(0.0, 2.0 * t - 1.0))
    return r, g, b


def checker_pixel(x, y, width, height):
    """8 像素一格的黄、深蓝棋盘格。"""
    if (x // 8 + y // 8) % 2 == 0:
        return 230, 200, 40
    return 20, 40, 120


def write_png(path, width, height, pixel):
    """确定性的测试图，pixel(x, y, width, height) 给出每个像素的 (r, g, b)。"""
    rows = []
    for y in range(height):
        row = bytearray([0])  # 滤波类型 None
        for x in range(width):
            row += bytes(pixel(x, y, width, height))
        rows.append(bytes(row))
    raw = b''.join(rows)

    def chunk(kind, data):
        return (struct.pack('>I', len(data)) + kind + data
                + struct.pack('>I', zlib.crc32(kind + data) & 0xffffffff))

    png = b'\x89PNG\r\n\x1a\n'
    png += chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0))
    png += chunk(b'IDAT', zlib.compress(raw, 9))
    png += chunk(b'IEND', b'')
    with open(path, 'wb') as fp:
        fp.write(png)


def main(argv):
    parser = argparse.ArgumentParser(description='生成出图测试的参考图纸（DXF R2000）')
    parser.add_argument('--out-dir', default=None, help='输出目录，默认 tests/render/drawings')
    args = parser.parse_args(argv)

    repo = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    out_dir = args.out_dir or os.path.join(repo, 'tests', 'render', 'drawings')
    if not os.path.isdir(out_dir):
        os.makedirs(out_dir)

    images = [('render_image.png', 64, 48, gradient_pixel), ('render_image_checker.png', 32, 32, checker_pixel)]
    for name, width, height, pixel in images:
        write_png(os.path.join(out_dir, name), width, height, pixel)

    drawings = [
        ('entities.dxf', gen_entities()),
        ('linetypes.dxf', gen_linetypes()),
        ('lineweights.dxf', gen_lineweights()),
        ('colors.dxf', gen_colors()),
        ('blocks.dxf', gen_blocks()),
        ('far_coords.dxf', gen_far_coords()),
        ('image.dxf', gen_image([(name, width, height) for name, width, height, _ in images])),
        ('text_shx.dxf', gen_text('txt')),
        ('text_truetype.dxf', gen_text('arial.ttf')),
    ]
    for name, drawing in drawings:
        path = os.path.join(out_dir, name)
        drawing.write(path)
        print('写出 %s' % path)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
