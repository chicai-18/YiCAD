# -*- coding: utf-8 -*-
"""
分层护栏：内核不得反向依赖 UI 层。

对应 doc/ARCHITECTURE_EVOLUTION_PLAN.md 的 P7。阶段 3 已经把
YiCadMath/YiCadModel/YiCadPersistence 拆成独立静态库，对这三层而言依赖
方向现在由 CMake 的 target_include_directories 物理强制——包含 UI 头文件
会直接编译失败，不必等这个脚本。

本脚本继续覆盖整个 src/kernel/（含仍与 UI/APP 合编的 kernel/actions、
kernel/gui、kernel/painters、kernel/printing），作为比重新配置+编译更快的
CI 早期预警，并把"这处例外是有意的"这件事强制写成白名单条目而不是悄悄
新增一行 include。基线 d8e0be5 上的三处已知违规已在阶段 3 修复（见
doc/ARCHITECTURE_EVOLUTION_PLAN.md 6.7 节）：DmDocument.cpp 经
GuiDialogFactoryInterface 新增的 requestUntitledDocumentName() 接口注入
解决；DmEntityContainer.cpp/DmHatch.cpp 的 include 本身就是死代码，已删除。
白名单现为空。

用法:
    python tools/check_layering.py
退出码 0 表示通过。
"""
import io
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
KERNEL = os.path.join(REPO, 'YiCAD', 'src', 'kernel')

# 内核禁止包含的头文件前缀（UI 层命名约定见 AGENTS.md）
FORBIDDEN_PREFIXES = ('UI',)

# 键是相对 YiCAD/src/kernel 的路径，值是该文件允许包含的 UI 头文件集合。
# 目前为空：kernel/actions、kernel/gui 等仍与 UI 合编的分区如果将来确实
# 需要引用某个 UI 头文件，在这里显式登记。
WHITELIST = {
}

INCLUDE_RE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.MULTILINE)


def iter_kernel_files():
    for dirpath, _dirnames, filenames in os.walk(KERNEL):
        for name in filenames:
            if name.endswith(('.h', '.cpp')):
                full = os.path.join(dirpath, name)
                rel = os.path.relpath(full, KERNEL).replace(os.sep, '/')
                yield rel, full


def main():
    violations = []
    used_whitelist = {}

    for rel, full in iter_kernel_files():
        try:
            text = io.open(full, encoding='utf-8', errors='replace').read()
        except OSError as exc:
            print('无法读取 %s: %s' % (rel, exc))
            return 2

        for inc in INCLUDE_RE.findall(text):
            head = os.path.basename(inc)
            if not head.startswith(FORBIDDEN_PREFIXES):
                continue
            allowed = WHITELIST.get(rel, set())
            if head in allowed:
                used_whitelist.setdefault(rel, set()).add(head)
            else:
                violations.append((rel, head))

    # 白名单腐化检查：登记了却不再命中的条目必须删除
    stale = []
    for rel, heads in WHITELIST.items():
        hit = used_whitelist.get(rel, set())
        for head in heads - hit:
            stale.append((rel, head))

    if violations:
        print('分层违规：src/kernel/ 不得包含 UI 层头文件（P7）')
        for rel, head in sorted(violations):
            print('  YiCAD/src/kernel/%s  ->  %s' % (rel, head))
        print('')
        print('内核是数据模型与几何层，不应直接触达界面。请改为由调用方决定是否提示，')
        print('或注入接口（见方案 6.4.1）。')

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
