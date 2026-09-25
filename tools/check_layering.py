# -*- coding: utf-8 -*-
"""
分层护栏：内核与命令机制不得反向依赖界面层，扩展不得依赖主窗口。

对应 doc/ARCHITECTURE_EVOLUTION_PLAN.md 的 P7 与
doc/COMMAND_TOOL_MIGRATION_PLAN.md 第 6 节第四步第 4 项。阶段 3 已经把
YiCadMath/YiCadModel/YiCadPersistence 拆成独立静态库，对这三层而言依赖
方向由 CMake 的 target_include_directories 物理强制，不必等这个脚本；其余
分区仍合编进 YiCadCore，include 路径是扁平的，靠本脚本在 CI 里早期预警。

规则按目录判断：头文件归它所在的目录（YiCAD/src 下没有重名的头文件，重名时
脚本报错），不看文件名前缀。
  - src/kernel/        不得包含 ui/、main/ 与任何扩展的头文件；
  - src/application/   命令与视图工具的机制（含扩展框架 framework/）：同上，另外
                       不得包含交互视图 kernel/interaction/（UIView）的头文件——
                       命令与工具只经 IDocumentView/GuiDocumentView 认识视图；
  - src/extensions/<X>/ 不得包含 main/ 的头文件（可以包含 ui/ 的，扩展将来各自
                       成库时依赖 YiCadUi），也不得包含别的扩展的头文件（每个扩展
                       自包含）。

"这处例外是有意的"必须写成白名单条目，不能悄悄新增一行 include；登记了却
不再命中的条目也会报错。基线 d8e0be5 上的三处已知违规已在阶段 3 修复（见
doc/ARCHITECTURE_EVOLUTION_PLAN.md 6.7 节）。

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
# 目前没有例外。
WHITELIST = {}

INCLUDE_RE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.MULTILINE)


def area_of_dir(rel_dir):
    """目录归属的区域：ui、main、kernel、kernel/interaction、application、extensions/<X> 等"""
    parts = rel_dir.split('/')
    if parts[0] == 'extensions' and len(parts) > 1:
        return 'extensions/' + parts[1]
    if parts[0] == 'kernel' and len(parts) > 1 and parts[1] == 'interaction':
        return 'kernel/interaction'
    return parts[0]


def forbidden_areas(src_area):
    """源文件所在区域不得包含的头文件区域；返回判断函数"""
    if src_area in ('kernel', 'kernel/interaction'):
        return lambda a: a in ('ui', 'main') or a.startswith('extensions/')
    if src_area == 'application':
        return lambda a: a in ('ui', 'main', 'kernel/interaction') or a.startswith('extensions/')
    if src_area.startswith('extensions/'):
        return lambda a: a == 'main' or (a.startswith('extensions/') and a != src_area)
    return None


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
        print('src/kernel/、src/application/ 不得包含 ui/、main/ 与扩展的头文件，src/application/')
        print('也不得包含交互视图 kernel/interaction/ 的头文件；扩展不得包含 main/ 与别的扩展的')
        print('头文件。需要的能力经 GuiDialogFactoryInterface、IExtensionContext 或新增的窄接口获取。')

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
