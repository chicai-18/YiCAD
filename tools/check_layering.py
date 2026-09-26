# -*- coding: utf-8 -*-
"""
分层护栏：下层不得包含上层的头文件，扩展不得依赖壳层与别的扩展。

层次见 doc/LAYER_RESTRUCTURE_PLAN.md 第 2 节，自下而上：
  base/ -> model/ -> render/ -> application/ -> ui/ -> 扩展 -> shell/
YiCadBase、YiCadModel 是独立静态库，对这两层而言依赖方向由 CMake 的
target_include_directories 物理强制；render/ 以上仍合编进 YiCadCore，include 路径
是扁平的，靠本脚本在 CI 里早期预警（分层重组 S6 拆库后改由 CMake 保证）。

规则按目录判断：头文件归它所在的目录（YiCAD/src 下没有重名的头文件，重名时
脚本报错），不看文件名前缀。
  - base/、model/、render/  不得包含 application/、ui/、shell/ 与扩展的头文件
                           （也不得包含更上层的 model/、render/，这一条 CMake 已保证）；
  - application/           不得包含 ui/、shell/ 与扩展的头文件；application/view/ 以外
                           的文件还不得包含交互视图 application/view/（UIView）——命令
                           与工具只经 IDocumentView/GuiDocumentView 认识视图；
  - ui/                    不得包含 shell/ 与扩展的头文件；
  - extensions/<X>/        不得包含 shell/ 的头文件（可以包含 ui/ 的，扩展将来链接
                           YiCadUi），也不得包含别的扩展的头文件（每个扩展自包含）；
  - shell/                 不受限制。

"这处例外是有意的"必须写成白名单条目，不能悄悄新增一行 include；登记了却
不再命中的条目也会报错。

用法:
    python tools/check_layering.py
退出码 0 表示通过。
"""
import io
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(REPO, 'YiCAD', 'src')

# 键是相对 YiCAD/src 的路径，值是该文件允许包含的、按规则本应禁止的头文件集合。
#
# 全部是 ui/ 对 shell/ 的既有依赖（分层重组 S2 新增"ui/ 不得包含 shell/"时登记，
# doc/LAYER_RESTRUCTURE_PLAN.md 1.2 节 L4）：ApplicationWindow.h、MDIWindow.h，
# S5 新增 IDocumentManager、把壳层部件搬进 shell/ 时清除。原有的 Fileio.h 两条已随
# S4b 删除 FileIO 清除。
WHITELIST = {
    'ui/UIActionHandler.cpp': {'MDIWindow.h'},
    'ui/UIActionHandler.h': {'MDIWindow.h'},
    'ui/UIBottomWidget.cpp': {'MDIWindow.h'},
    'ui/UIBottomWidget.h': {'ApplicationWindow.h'},
    'ui/UICommandWidget.h': {'MDIWindow.h'},
    'ui/UICurrentActivePen.cpp': {'ApplicationWindow.h'},
    'ui/UIDialogFactory.cpp': {'ApplicationWindow.h'},
    'ui/UILineTypeBox.cpp': {'ApplicationWindow.h'},
    'ui/UITabDrawWidget.cpp': {'MDIWindow.h'},
}

INCLUDE_RE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.MULTILINE)

# 各区域的层次，数字大的在上层。扩展位于 ui/ 与 shell/ 之间。
RANK = {
    'base': 0,
    'model': 1,
    'render': 2,
    'application': 3,
    'application/view': 3,
    'ui': 4,
    'extensions': 5,
    'shell': 6,
}


def area_of_dir(rel_dir):
    """目录归属的区域：base、model、render、application、application/view、ui、shell、extensions/<X>"""
    parts = rel_dir.split('/')
    if parts[0] == 'extensions' and len(parts) > 1:
        return 'extensions/' + parts[1]
    if parts[0] == 'application' and len(parts) > 1 and parts[1] == 'view':
        return 'application/view'
    return parts[0]


def rank_of(area):
    if area.startswith('extensions/'):
        return RANK['extensions']
    return RANK.get(area)


def forbidden_areas(src_area):
    """源文件所在区域不得包含的头文件区域；返回判断函数，不受限制时返回 None"""
    src_rank = rank_of(src_area)
    if src_rank is None or src_area == 'shell':
        return None

    def forbidden(a):
        if a.startswith('extensions/') and a != src_area:
            return True
        if src_area == 'application' and a == 'application/view':
            return True
        rank = rank_of(a)
        return rank is not None and rank > src_rank

    return forbidden


def iter_files():
    for dirpath, _dirnames, filenames in os.walk(SRC):
        rel_dir = os.path.relpath(dirpath, SRC).replace(os.sep, '/')
        for name in filenames:
            if name.endswith(('.h', '.cpp')):
                yield rel_dir, name, os.path.join(dirpath, name)


def main():
    # 头文件名 -> 所在区域
    header_area = {}
    duplicates = []
    for rel_dir, name, _full in iter_files():
        if not name.endswith('.h') or rel_dir == '.':
            continue
        area = area_of_dir(rel_dir)
        if name in header_area and header_area[name] != area:
            duplicates.append((name, header_area[name], area))
        header_area.setdefault(name, area)
    if duplicates:
        print('头文件重名，无法按目录判断归属（include 路径是扁平的，也会互相遮蔽）:')
        for name, a, b in sorted(duplicates):
            print('  %s: %s / %s' % (name, a, b))
        return 2

    unknown = sorted({area_of_dir(d) for d, _n, _f in iter_files() if d != '.'
                      and rank_of(area_of_dir(d)) is None})
    if unknown:
        print('未知的源码目录，请在 tools/check_layering.py 的 RANK 里给出它的层次:')
        for area in unknown:
            print('  YiCAD/src/%s/' % area)
        return 2

    violations = []
    used_whitelist = {}
    for rel_dir, name, full in iter_files():
        if rel_dir == '.':
            continue
        src_area = area_of_dir(rel_dir)
        forbidden = forbidden_areas(src_area)
        if forbidden is None:
            continue
        rel = rel_dir + '/' + name
        try:
            text = io.open(full, encoding='utf-8', errors='replace').read()
        except OSError as exc:
            print('无法读取 %s: %s' % (rel, exc))
            return 2
        for inc in INCLUDE_RE.findall(text):
            head = os.path.basename(inc)
            area = header_area.get(head)
            if area is None or not forbidden(area):
                continue
            if head in WHITELIST.get(rel, set()):
                used_whitelist.setdefault(rel, set()).add(head)
            else:
                violations.append((rel, head, area))

    # 白名单腐化检查：登记了却不再命中的条目必须删除
    stale = []
    for rel, heads in WHITELIST.items():
        hit = used_whitelist.get(rel, set())
        for head in heads - hit:
            stale.append((rel, head))

    if violations:
        print('分层违规：')
        for rel, head, area in sorted(violations):
            print('  YiCAD/src/%s  ->  %s（%s/）' % (rel, head, area))
        print('')
        print('层次自下而上为 base/、model/、render/、application/、ui/、扩展、shell/，下层不得包含')
        print('上层的头文件；application/view/ 以外不得包含 application/view/；扩展不得包含别的')
        print('扩展。需要的能力经 GuiDialogFactoryInterface、IExtensionContext 或新增的窄接口获取。')

    if stale:
        print('白名单已失效，请从 tools/check_layering.py 的 WHITELIST 中删除:')
        for rel, head in sorted(stale):
            print('  %s -> %s' % (rel, head))

    if violations or stale:
        return 1

    total = sum(len(v) for v in WHITELIST.values())
    if total:
        print('分层检查通过（%d 处已登记的例外在白名单内）' % total)
    else:
        print('分层检查通过（白名单为空，无已知例外）')
    return 0


if __name__ == '__main__':
    sys.exit(main())
