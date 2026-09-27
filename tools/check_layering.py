# -*- coding: utf-8 -*-
"""
分层护栏：只检查 CMake 管不到的两条。

层次见 doc/LAYER_RESTRUCTURE_PLAN.md 第 2 节，自下而上：
  base/ -> model/ -> render/ -> application/ -> ui/ -> 扩展 -> shell/
分层重组 S6 之后每一层是一个库，下层看不到上层的头文件、扩展看不到 shell/ 与别的扩展，
都由 CMake 的 target_include_directories 保证，在下层包含上层的头文件直接编译失败。
本脚本只检查：

  1. YiCAD/src 下没有重名的头文件。include 写的是扁平的文件名，重名的头文件会按
     include 路径的顺序互相遮蔽；
  2. application/ 里 application/view/ 以外的文件不包含 application/view/ 的头文件
     （交互视图 UIView）：命令与工具只经 IDocumentView/GuiDocumentView 认识视图。两者
     同在 YiCadApplication 里，CMake 分不开。

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

INCLUDE_RE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.MULTILINE)


def iter_files():
    for dirpath, _dirnames, filenames in os.walk(SRC):
        rel_dir = os.path.relpath(dirpath, SRC).replace(os.sep, '/')
        for name in filenames:
            if name.endswith(('.h', '.cpp')):
                yield rel_dir, name, os.path.join(dirpath, name)


def is_view_dir(rel_dir):
    return rel_dir == 'application/view' or rel_dir.startswith('application/view/')


def main():
    # 头文件名 -> 所在目录
    header_dir = {}
    duplicates = []
    for rel_dir, name, _full in iter_files():
        if not name.endswith('.h') or rel_dir == '.':
            continue
        if name in header_dir:
            duplicates.append((name, header_dir[name], rel_dir))
        else:
            header_dir[name] = rel_dir
    if duplicates:
        print('头文件重名（include 路径是扁平的，重名的头文件会互相遮蔽）:')
        for name, a, b in sorted(duplicates):
            print('  %s: YiCAD/src/%s/ 与 YiCAD/src/%s/' % (name, a, b))
        return 1

    violations = []
    for rel_dir, name, full in iter_files():
        if rel_dir.split('/')[0] != 'application' or is_view_dir(rel_dir):
            continue
        rel = rel_dir + '/' + name
        try:
            text = io.open(full, encoding='utf-8', errors='replace').read()
        except OSError as exc:
            print('无法读取 %s: %s' % (rel, exc))
            return 2
        for inc in INCLUDE_RE.findall(text):
            head = os.path.basename(inc)
            if is_view_dir(header_dir.get(head, '')):
                violations.append((rel, head))

    if violations:
        print('分层违规：application/view/ 以外不得包含交互视图 application/view/ 的头文件')
        for rel, head in sorted(violations):
            print('  YiCAD/src/%s  ->  %s' % (rel, head))
        print('')
        print('命令与工具只经 IDocumentView/GuiDocumentView 认识视图。')
        return 1

    print('分层检查通过（头文件无重名；application/view/ 以外不包含 application/view/）')
    return 0


if __name__ == '__main__':
    sys.exit(main())
