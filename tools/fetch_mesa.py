# -*- coding: utf-8 -*-
"""
取出图测试（test_render）用的 Mesa 软件 OpenGL（llvmpipe），见 doc/RENDER_PLAN.md 第 7 节 D9。

出图测试要在没有显卡的 CI 虚拟机上跑，而且每次结果必须一样，才能和基准图像比对，
所以测试一律用 Mesa 的 CPU 实现，本机与 CI 相同。Qt 自带的 opengl32sw.dll 是
Mesa 11.2.2（最高 GL 3.3），跑不了现在 #version 430 的着色器，因此用
pal1000/mesa-dist-win 的发布包。

版本与校验和写死在下面；升级时与其他依赖一样显式修改（AGENTS.md），并同步 README.md。

只取 OpenGL 用到的两个文件（opengl32.dll 与它依赖的 libgallium_wgl.dll），放到
external/mesa/x64/（external/ 不入库）。它们不进 build/<cfg>/bin：那个目录会被
cmake --install 整个装进发布包。test_render 以延迟加载方式链接 opengl32.dll，
启动时按完整路径装入这里的文件，见 tests/render/MesaLoader.cpp。

解压用 7-Zip（7z.exe）：先找 PATH，再找常见的安装位置。

用法:
    python tools/fetch_mesa.py                          # 下载并解压
    python tools/fetch_mesa.py --archive D:/mesa.7z     # 用已经下载好的包（同样校验）
"""

import argparse
import hashlib
import os
import shutil
import subprocess
import sys
import tempfile
import urllib.request

MESA_VERSION = '26.2.3'
ARCHIVE_NAME = 'mesa3d-%s-release-msvc.7z' % MESA_VERSION
ARCHIVE_URL = ('https://github.com/pal1000/mesa-dist-win/releases/download/%s/%s'
               % (MESA_VERSION, ARCHIVE_NAME))
ARCHIVE_SIZE = 71063681
ARCHIVE_SHA256 = '3f3613adb43cfd0f2e665ce2400b130c275f0b3317cb3a05566320a3a67589ed'

# OpenGL 只需要这两个文件；其余（d3d12、Vulkan、视频编码等）用不到
FILES = ['opengl32.dll', 'libgallium_wgl.dll']

# 解压完成后写入，内容是版本号；版本一致时脚本什么也不做
STAMP = 'VERSION'


def repo_root():
    return os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def find_7z():
    found = shutil.which('7z')
    if found:
        return found
    for base in (os.environ.get('ProgramFiles'), os.environ.get('ProgramFiles(x86)'),
                 r'C:\Program Files', r'D:\Program Files'):
        if base:
            candidate = os.path.join(base, '7-Zip', '7z.exe')
            if os.path.isfile(candidate):
                return candidate
    return None


def sha256_of(path):
    digest = hashlib.sha256()
    with open(path, 'rb') as fp:
        for chunk in iter(lambda: fp.read(1 << 20), b''):
            digest.update(chunk)
    return digest.hexdigest()


def download(url, path):
    print('下载 %s ...' % url)
    with urllib.request.urlopen(url) as response, open(path, 'wb') as out:
        shutil.copyfileobj(response, out)


def verify(path):
    size = os.path.getsize(path)
    if size != ARCHIVE_SIZE:
        raise SystemExit('大小不符：%s 为 %d 字节，应为 %d' % (path, size, ARCHIVE_SIZE))
    actual = sha256_of(path)
    if actual != ARCHIVE_SHA256:
        raise SystemExit('sha256 不符：%s\n  实际 %s\n  应为 %s' % (path, actual, ARCHIVE_SHA256))


def extract(seven_zip, archive, out_dir):
    if os.path.isdir(out_dir):
        shutil.rmtree(out_dir)
    os.makedirs(out_dir)
    members = ['x64/' + name for name in FILES]
    subprocess.check_call([seven_zip, 'e', '-y', '-o' + out_dir, archive] + members,
                          stdout=subprocess.DEVNULL)
    for name in FILES:
        if not os.path.isfile(os.path.join(out_dir, name)):
            raise SystemExit('解压后缺少 %s' % name)
    with open(os.path.join(out_dir, STAMP), 'w', encoding='utf-8') as fp:
        fp.write(MESA_VERSION + '\n')


def main(argv):
    parser = argparse.ArgumentParser(description='取出图测试用的 Mesa 软件 OpenGL')
    parser.add_argument('--archive', help='已下载的 %s，不再下载' % ARCHIVE_NAME)
    parser.add_argument('--out-dir', default=None,
                        help='输出目录，默认 external/mesa/x64')
    args = parser.parse_args(argv)

    out_dir = args.out_dir or os.path.join(repo_root(), 'external', 'mesa', 'x64')
    stamp = os.path.join(out_dir, STAMP)
    if os.path.isfile(stamp) and all(os.path.isfile(os.path.join(out_dir, f)) for f in FILES):
        with open(stamp, encoding='utf-8') as fp:
            if fp.read().strip() == MESA_VERSION:
                print('Mesa %s 已在 %s' % (MESA_VERSION, out_dir))
                return 0

    seven_zip = find_7z()
    if seven_zip is None:
        raise SystemExit('找不到 7-Zip（7z.exe）：装上 7-Zip 或把它加进 PATH')

    temp_dir = None
    try:
        if args.archive:
            archive = args.archive
        else:
            temp_dir = tempfile.mkdtemp(prefix='yicad-mesa-')
            archive = os.path.join(temp_dir, ARCHIVE_NAME)
            download(ARCHIVE_URL, archive)
        verify(archive)
        extract(seven_zip, archive, out_dir)
    finally:
        if temp_dir:
            shutil.rmtree(temp_dir, ignore_errors=True)

    print('Mesa %s 已解压到 %s' % (MESA_VERSION, out_dir))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
