# -*- coding: utf-8 -*-
"""
分层护栏：内核不得反向依赖 UI 层。

对应 doc/ARCHITECTURE_EVOLUTION_PLAN.md 的 P7。阶段 3 才会用 CMake 把依赖
方向变成物理约束；在那之前，本脚本在 CI 里把方向锁住，防止新的反向依赖混进来。

白名单里是基线 d8e0be5 上已经存在的三处违规。阶段 3 任务 6.4.1 修完之后，
请连同白名单条目一起删除——脚本会在白名单条目失效时报错，避免白名单腐化。

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

# 基线 d8e0be5 上已知的反向依赖。键是相对 YiCAD/src/kernel 的路径，
# 值是该文件允许包含的 UI 头文件集合。
WHITELIST = {
    'builder_model/DmDocument.cpp': {'UITabDrawWidget.h'},
    'builder_model/DmEntityContainer.cpp': {'UIDialogFactory.h'},
    'builder_model/DmHatch.cpp': {'UIDialogFactory.h'},
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
    print('分层检查通过（%d 处已知违规在白名单内，待阶段 3 清零）' % total)
    return 0


if __name__ == '__main__':
    sys.exit(main())
