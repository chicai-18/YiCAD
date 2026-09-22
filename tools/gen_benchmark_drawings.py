# -*- coding: utf-8 -*-
"""
生成方案阶段 0 要求的三份基准图纸（doc/ARCHITECTURE_EVOLUTION_PLAN.md 3.2 第 4 条）。

    小   约 1,000   实体
    中   约 50,000  实体
    大   约 500,000 实体

每份都含直线、圆、圆弧、多段线、样条、填充、块引用、单行文字与多行文字，
覆盖 YiCAD 各条绘制与拾取路径。输出为 DXF R2000（AC1015）ASCII，
由 YiCadDxfPlugin 导入。

图纸不入库——大图纸上百兆，靠本脚本按需重现。生成是确定性的：同样的
--seed 与 --count 一定产出逐字节相同的文件，这样不同机器、不同时间采集
的基线数据才可比。

用法:
    python tools/gen_benchmark_drawings.py                     # 三份都生成到 build/benchmarks
    python tools/gen_benchmark_drawings.py --size small
    python tools/gen_benchmark_drawings.py --out-dir D:/bench
    python tools/gen_benchmark_drawings.py --validate          # 若装了 ezdxf 则顺带校验

脚本本身不依赖任何第三方库；--validate 需要 ezdxf（pip install ezdxf），
装不上时会跳过校验并提示，不影响生成。
"""

import argparse
import math
import os
import random
import sys

NEWLINE = chr(10)

# 三份基准图纸的目标实体数
SIZES = {
    'small': 1000,
    'medium': 50000,
    'large': 500000,
}

# 实体类型的混合比例。数值是权重，不是百分比。
# 直线与圆弧占大头，贴近真实图纸；样条与填充成本高，占比刻意压低。
ENTITY_MIX = [
    ('LINE', 40),
    ('CIRCLE', 12),
    ('ARC', 16),
    ('LWPOLYLINE', 12),
    ('SPLINE', 4),
    ('HATCH', 3),
    ('INSERT', 6),
    ('TEXT', 5),
    ('MTEXT', 2),
]

LAYERS = [
    ('0', 7),
    ('CENTER', 1),
    ('HIDDEN', 2),
    ('DIM', 3),
    ('HATCH', 4),
    ('TEXT', 5),
]

# 块引用指向的块名
BLOCK_NAME = 'BENCH_BLOCK'

# 图纸范围（毫米）
EXTENT = 100000.0


class HandleAllocator(object):
    """DXF R2000 要求每个对象有唯一句柄（组码 5），十六进制大写。"""

    def __init__(self, start=0x100):
        self._next = start

    def take(self):
        value = self._next
        self._next += 1
        return '%X' % value

    def peek(self):
        return '%X' % self._next


class DxfWriter(object):
    """极简 DXF R2000 写出器。只支持基准图纸用到的那几种实体。"""

    def __init__(self, stream):
        self._out = stream
        self._handles = HandleAllocator()

    # -- 底层 --------------------------------------------------------------

    def tag(self, code, value):
        self._out.write('%d%s%s%s' % (code, NEWLINE, value, NEWLINE))

    def num(self, code, value):
        # DXF 浮点用定点表示，避免 1e+05 这类科学计数法
        self._out.write('%d%s%.6f%s' % (code, NEWLINE, value, NEWLINE))

    def handle(self):
        return self._handles.take()

    # -- 段 ----------------------------------------------------------------

    def begin_section(self, name):
        self.tag(0, 'SECTION')
        self.tag(2, name)

    def end_section(self):
        self.tag(0, 'ENDSEC')

    # -- HEADER ------------------------------------------------------------

    def write_header(self):
        self.begin_section('HEADER')
        self.tag(9, '$ACADVER')
        self.tag(1, 'AC1015')
        self.tag(9, '$INSBASE')
        self.num(10, 0.0)
        self.num(20, 0.0)
        self.num(30, 0.0)
        self.tag(9, '$EXTMIN')
        self.num(10, 0.0)
        self.num(20, 0.0)
        self.num(30, 0.0)
        self.tag(9, '$EXTMAX')
        self.num(10, EXTENT)
        self.num(20, EXTENT)
        self.num(30, 0.0)
        self.tag(9, '$INSUNITS')
        self.tag(70, '4')          # 毫米
        self.tag(9, '$LTSCALE')
        self.num(40, 1.0)
        self.tag(9, '$HANDSEED')
        self.tag(5, 'FFFFFF')
        self.end_section()

    # -- TABLES ------------------------------------------------------------

    def _table_begin(self, name, count):
        self.tag(0, 'TABLE')
        self.tag(2, name)
        self.tag(5, self.handle())
        self.tag(100, 'AcDbSymbolTable')
        self.tag(70, str(count))

    def _table_end(self):
        self.tag(0, 'ENDTAB')

    def write_tables(self, block_record_handles):
        self.begin_section('TABLES')

        # -- VPORT --
        self._table_begin('VPORT', 1)
        self.tag(0, 'VPORT')
        self.tag(5, self.handle())
        self.tag(100, 'AcDbSymbolTableRecord')
        self.tag(100, 'AcDbViewportTableRecord')
        self.tag(2, '*ACTIVE')
        self.tag(70, '0')
        self.num(10, 0.0)
        self.num(20, 0.0)
        self.num(11, 1.0)
        self.num(21, 1.0)
        self.num(12, EXTENT / 2.0)
        self.num(22, EXTENT / 2.0)
        self.num(40, EXTENT)
        self.num(41, 1.5)
        self._table_end()

    # -- LTYPE --
        self._table_begin('LTYPE', 3)
        for name, descr in (('ByBlock', ''), ('ByLayer', ''), ('CONTINUOUS', 'Solid line')):
            self.tag(0, 'LTYPE')
            self.tag(5, self.handle())
            self.tag(100, 'AcDbSymbolTableRecord')
            self.tag(100, 'AcDbLinetypeTableRecord')
            self.tag(2, name)
            self.tag(70, '0')
            self.tag(3, descr)
            self.tag(72, '65')
            self.tag(73, '0')
            self.num(40, 0.0)
        self._table_end()

        # -- LAYER --
        self._table_begin('LAYER', len(LAYERS))
        for name, color in LAYERS:
            self.tag(0, 'LAYER')
            self.tag(5, self.handle())
            self.tag(100, 'AcDbSymbolTableRecord')
            self.tag(100, 'AcDbLayerTableRecord')
            self.tag(2, name)
            self.tag(70, '0')
            self.tag(62, str(color))
            self.tag(6, 'CONTINUOUS')
            self.tag(370, '-3')
            self.tag(390, 'F')
        self._table_end()

        # -- STYLE --
        self._table_begin('STYLE', 1)
        self.tag(0, 'STYLE')
        self.tag(5, self.handle())
        self.tag(100, 'AcDbSymbolTableRecord')
        self.tag(100, 'AcDbTextStyleTableRecord')
        self.tag(2, 'Standard')
        self.tag(70, '0')
        self.num(40, 0.0)
        self.num(41, 1.0)
        self.num(50, 0.0)
        self.tag(71, '0')
        self.num(42, 2.5)
        self.tag(3, 'txt')
        self.tag(4, '')
        self._table_end()

        # -- APPID --
        self._table_begin('APPID', 1)
        self.tag(0, 'APPID')
        self.tag(5, self.handle())
        self.tag(100, 'AcDbSymbolTableRecord')
        self.tag(100, 'AcDbRegAppTableRecord')
        self.tag(2, 'ACAD')
        self.tag(70, '0')
        self._table_end()

        # -- DIMSTYLE --
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
        self._table_end()

        # -- BLOCK_RECORD --
        self._table_begin('BLOCK_RECORD', len(block_record_handles))
        for name, h in block_record_handles:
            self.tag(0, 'BLOCK_RECORD')
            self.tag(5, h)
            self.tag(100, 'AcDbSymbolTableRecord')
            self.tag(100, 'AcDbBlockTableRecord')
            self.tag(2, name)
            self.tag(70, '0')
            self.tag(280, '1')
            self.tag(281, '0')
        self._table_end()

        self.end_section()

    # -- 实体通用头 ---------------------------------------------------------

    def _entity_head(self, dxftype, owner, layer, subclass):
        self.tag(0, dxftype)
        self.tag(5, self.handle())
        self.tag(330, owner)
        self.tag(100, 'AcDbEntity')
        self.tag(8, layer)
        self.tag(100, subclass)

    # -- 各类实体 -----------------------------------------------------------

    def line(self, owner, layer, x1, y1, x2, y2):
        self._entity_head('LINE', owner, layer, 'AcDbLine')
        self.num(10, x1)
        self.num(20, y1)
        self.num(30, 0.0)
        self.num(11, x2)
        self.num(21, y2)
        self.num(31, 0.0)

    def circle(self, owner, layer, cx, cy, r):
        self._entity_head('CIRCLE', owner, layer, 'AcDbCircle')
        self.num(10, cx)
        self.num(20, cy)
        self.num(30, 0.0)
        self.num(40, r)

    def arc(self, owner, layer, cx, cy, r, a1, a2):
        self._entity_head('ARC', owner, layer, 'AcDbCircle')
        self.num(10, cx)
        self.num(20, cy)
        self.num(30, 0.0)
        self.num(40, r)
        self.tag(100, 'AcDbArc')
        self.num(50, a1)
        self.num(51, a2)

    def lwpolyline(self, owner, layer, points, closed=False):
        self._entity_head('LWPOLYLINE', owner, layer, 'AcDbPolyline')
        self.tag(90, str(len(points)))
        self.tag(70, '1' if closed else '0')
        self.num(43, 0.0)
        for px, py in points:
            self.num(10, px)
            self.num(20, py)

    def spline(self, owner, layer, control_points):
        """三次均匀样条，钳制端点。控制点数须 >= 4。"""
        degree = 3
        n = len(control_points)
        # 钳制节点矢量：首尾各重复 degree+1 次
        knots = [0.0] * (degree + 1)
        inner = n - degree - 1
        for i in range(1, inner + 1):
            knots.append(float(i))
        knots.extend([float(inner + 1)] * (degree + 1))

        self._entity_head('SPLINE', owner, layer, 'AcDbSpline')
        self.num(210, 0.0)
        self.num(220, 0.0)
        self.num(230, 1.0)
        self.tag(70, '8')             # 平面样条
        self.tag(71, str(degree))
        self.tag(72, str(len(knots)))
        self.tag(73, str(n))
        self.tag(74, '0')
        self.num(42, 0.0000001)
        self.num(43, 0.0000001)
        for k in knots:
            self.num(40, k)
        for px, py in control_points:
            self.num(10, px)
            self.num(20, py)
            self.num(30, 0.0)

    def hatch(self, owner, layer, points):
        """实体填充，单条闭合多段线边界。"""
        self._entity_head('HATCH', owner, layer, 'AcDbHatch')
        self.num(10, 0.0)             # 高程点
        self.num(20, 0.0)
        self.num(30, 0.0)
        self.num(210, 0.0)
        self.num(220, 0.0)
        self.num(230, 1.0)
        self.tag(2, 'SOLID')
        self.tag(70, '1')             # 实体填充
        self.tag(71, '0')             # 非关联
        self.tag(91, '1')             # 一条边界回路
        # -- 边界回路 --
        self.tag(92, '3')             # 外边界 + 多段线
        self.tag(72, '0')             # 无凸度
        self.tag(73, '1')             # 闭合
        self.tag(93, str(len(points)))
        for px, py in points:
            self.num(10, px)
            self.num(20, py)
        self.tag(97, '0')             # 源边界对象数
        self.tag(75, '0')             # 孤岛检测：奇偶
        self.tag(76, '1')             # 图案类型：预定义
        self.num(47, 1.0)             # 像素尺寸
        self.tag(98, '0')             # 种子点数

    def insert(self, owner, layer, name, x, y, scale=1.0, rotation=0.0):
        self._entity_head('INSERT', owner, layer, 'AcDbBlockReference')
        self.tag(2, name)
        self.num(10, x)
        self.num(20, y)
        self.num(30, 0.0)
        self.num(41, scale)
        self.num(42, scale)
        self.num(43, scale)
        self.num(50, rotation)

    def text(self, owner, layer, x, y, height, content):
        self._entity_head('TEXT', owner, layer, 'AcDbText')
        self.num(10, x)
        self.num(20, y)
        self.num(30, 0.0)
        self.num(40, height)
        self.tag(1, content)
        self.tag(7, 'Standard')
        self.tag(100, 'AcDbText')

    def mtext(self, owner, layer, x, y, height, content):
        self._entity_head('MTEXT', owner, layer, 'AcDbMText')
        self.num(10, x)
        self.num(20, y)
        self.num(30, 0.0)
        self.num(40, height)
        self.num(41, height * 20.0)
        self.tag(71, '1')             # 左上对齐
        self.tag(72, '1')             # 从左至右
        self.tag(1, content)
        self.tag(7, 'Standard')

    def block_begin(self, block_record_handle, name, layer='0'):
        self.tag(0, 'BLOCK')
        self.tag(5, self.handle())
        self.tag(330, block_record_handle)
        self.tag(100, 'AcDbEntity')
        self.tag(8, layer)
        self.tag(100, 'AcDbBlockBegin')
        self.tag(2, name)
        self.tag(70, '0')
        self.num(10, 0.0)
        self.num(20, 0.0)
        self.num(30, 0.0)
        self.tag(3, name)
        self.tag(1, '')

    def block_end(self, block_record_handle, layer='0'):
        self.tag(0, 'ENDBLK')
        self.tag(5, self.handle())
        self.tag(330, block_record_handle)
        self.tag(100, 'AcDbEntity')
        self.tag(8, layer)
        self.tag(100, 'AcDbBlockEnd')

    def write_objects(self):
        self.begin_section('OBJECTS')
        root = self.handle()
        self.tag(0, 'DICTIONARY')
        self.tag(5, root)
        self.tag(100, 'AcDbDictionary')
        self.tag(281, '1')
        self.end_section()

    def write_eof(self):
        self.tag(0, 'EOF')


def weighted_choices(rng, count):
    """按 ENTITY_MIX 的权重抽出 count 个实体类型。"""
    kinds = [k for k, _ in ENTITY_MIX]
    weights = [w for _, w in ENTITY_MIX]
    total = float(sum(weights))
    cumulative = []
    acc = 0.0
    for w in weights:
        acc += w / total
        cumulative.append(acc)

    out = []
    for _ in range(count):
        r = rng.random()
        for i, edge in enumerate(cumulative):
            if r <= edge:
                out.append(kinds[i])
                break
        else:
            out.append(kinds[-1])
    return out


def generate(path, entity_count, seed):
    """生成一份基准图纸。返回实际写出的实体数。"""
    rng = random.Random(seed)

    with open(path, 'w', encoding='utf-8', newline='') as fp:
        w = DxfWriter(fp)

        # 块记录的句柄要在 TABLES 里定义、在 BLOCKS 与实体里引用，
        # 所以先占好三个固定句柄。
        model_space = '1F'
        paper_space = '1E'
        user_block = '1D'
        block_records = [
            ('*Model_Space', model_space),
            ('*Paper_Space', paper_space),
            (BLOCK_NAME, user_block),
        ]

        w.write_header()
        w.write_tables(block_records)

        # -- BLOCKS --
        w.begin_section('BLOCKS')

        w.block_begin(model_space, '*Model_Space')
        w.block_end(model_space)

        w.block_begin(paper_space, '*Paper_Space')
        w.block_end(paper_space)

        # 用户块：一个带十字标记的小方框，块引用都指向它
        w.block_begin(user_block, BLOCK_NAME)
        w.lwpolyline(user_block, '0',
                     [(-5.0, -5.0), (5.0, -5.0), (5.0, 5.0), (-5.0, 5.0)], closed=True)
        w.line(user_block, '0', -5.0, 0.0, 5.0, 0.0)
        w.line(user_block, '0', 0.0, -5.0, 0.0, 5.0)
        w.circle(user_block, '0', 0.0, 0.0, 2.0)
        w.block_end(user_block)

        w.end_section()

        # -- ENTITIES --
        w.begin_section('ENTITIES')

        layer_names = [name for name, _ in LAYERS]
        written = 0

        for kind in weighted_choices(rng, entity_count):
            x = rng.uniform(0.0, EXTENT)
            y = rng.uniform(0.0, EXTENT)
            layer = layer_names[rng.randrange(len(layer_names))]

            if kind == 'LINE':
                w.line(model_space, layer, x, y,
                       x + rng.uniform(-500.0, 500.0),
                       y + rng.uniform(-500.0, 500.0))

            elif kind == 'CIRCLE':
                w.circle(model_space, layer, x, y, rng.uniform(1.0, 200.0))

            elif kind == 'ARC':
                a1 = rng.uniform(0.0, 360.0)
                a2 = a1 + rng.uniform(15.0, 300.0)
                w.arc(model_space, layer, x, y, rng.uniform(1.0, 200.0), a1, a2 % 360.0)

            elif kind == 'LWPOLYLINE':
                n = rng.randint(3, 8)
                pts = [(x + rng.uniform(-300.0, 300.0), y + rng.uniform(-300.0, 300.0))
                       for _ in range(n)]
                w.lwpolyline(model_space, layer, pts, closed=rng.random() < 0.3)

            elif kind == 'SPLINE':
                n = rng.randint(4, 7)
                pts = [(x + i * 50.0 + rng.uniform(-20.0, 20.0),
                        y + rng.uniform(-150.0, 150.0)) for i in range(n)]
                w.spline(model_space, layer, pts)

            elif kind == 'HATCH':
                r = rng.uniform(20.0, 120.0)
                pts = []
                for i in range(6):
                    ang = 2.0 * math.pi * i / 6.0
                    pts.append((x + r * math.cos(ang), y + r * math.sin(ang)))
                w.hatch(model_space, 'HATCH', pts)

            elif kind == 'INSERT':
                w.insert(model_space, layer, BLOCK_NAME, x, y,
                         scale=rng.uniform(0.5, 4.0),
                         rotation=rng.uniform(0.0, 360.0))

            elif kind == 'TEXT':
                w.text(model_space, 'TEXT', x, y, rng.uniform(2.5, 12.0),
                       '基准 %d' % written)

            elif kind == 'MTEXT':
                w.mtext(model_space, 'TEXT', x, y, rng.uniform(2.5, 12.0),
                        '基准图纸 多行文字 %d' % written)

            written += 1

        w.end_section()
        w.write_objects()
        w.write_eof()

    return written


def validate(path):
    """若装了 ezdxf 则读一遍生成的文件，确认结构可解析。"""
    try:
        import ezdxf
    except ImportError:
        print('  跳过校验：未安装 ezdxf（pip install ezdxf）')
        return None

    doc = ezdxf.readfile(path)
    counts = {}
    for entity in doc.modelspace():
        counts[entity.dxftype()] = counts.get(entity.dxftype(), 0) + 1
    total = sum(counts.values())
    detail = ', '.join('%s=%d' % (k, counts[k]) for k in sorted(counts))
    print('  校验通过：模型空间 %d 个实体（%s）' % (total, detail))
    return total


def main(argv):
    parser = argparse.ArgumentParser(
        description='生成 YiCAD 性能基准图纸（DXF R2000）')
    parser.add_argument('--size', choices=sorted(SIZES) + ['all'], default='all',
                        help='生成哪一份，默认三份都生成')
    parser.add_argument('--out-dir', default=None,
                        help='输出目录，默认 build/benchmarks')
    parser.add_argument('--seed', type=int, default=20260922,
                        help='随机种子；同一种子产出完全相同的文件')
    parser.add_argument('--count', type=int, default=None,
                        help='覆盖实体数，只在指定单一 --size 时有效')
    parser.add_argument('--validate', action='store_true',
                        help='生成后用 ezdxf 读回校验（需已安装 ezdxf）')
    args = parser.parse_args(argv)

    repo = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    out_dir = args.out_dir or os.path.join(repo, 'build', 'benchmarks')
    if not os.path.isdir(out_dir):
        os.makedirs(out_dir)

    targets = sorted(SIZES) if args.size == 'all' else [args.size]
    if args.count is not None and len(targets) != 1:
        parser.error('--count 只能与单一 --size 搭配使用')

    for name in targets:
        count = args.count if args.count is not None else SIZES[name]
        path = os.path.join(out_dir, 'benchmark_%s.dxf' % name)
        print('生成 %s（目标 %d 个实体）...' % (path, count))

        written = generate(path, count, args.seed)
        size_mb = os.path.getsize(path) / (1024.0 * 1024.0)
        print('  写出 %d 个实体，%.1f MB' % (written, size_mb))

        if args.validate:
            validate(path)

    print('')
    print('基准图纸不入库，用本脚本按需重现。')
    print('采集基线数据的步骤见 doc/BASELINE.md。')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
