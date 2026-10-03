#!/usr/bin/env python3
"""tools/compile_shaders.py 的测试：着色器与清单不一致、用了 GL 4.3 不支持的写法时要失败，
正常时按平铺规则改写绑定号（doc/RENDER_PLAN.md 第 4.7.4 节）。

由 CTest 运行（tests/render/CMakeLists.txt 的 shader_toolchain），也可以手工运行：

    python tools/test_compile_shaders.py --glslang <glslangValidator> --spirv-cross <spirv-cross>
"""

import argparse
import json
import os
import subprocess
import sys
import tempfile
import unittest

SCRIPT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "compile_shaders.py")
TOOLS = {}

FRAG_SOLID = """#version 450
layout(location = 0) out vec4 outColor;
void main() { outColor = vec4(1.0); }
"""


class CompileShadersTest(unittest.TestCase):
    def compile(self, manifest, sources):
        """在临时目录里写清单与源码并编译，返回 (退出码, 输出, 输出目录)"""
        work = tempfile.mkdtemp()
        self.addCleanup(lambda: __import__("shutil").rmtree(work, ignore_errors=True))
        source_dir = os.path.join(work, "src")
        os.makedirs(source_dir)
        with open(os.path.join(source_dir, "shaders.json"), "w", encoding="utf-8") as f:
            json.dump(manifest, f)
        for name, text in sources.items():
            with open(os.path.join(source_dir, name), "w", encoding="utf-8") as f:
                f.write(text)
        out_dir = os.path.join(work, "out")
        result = subprocess.run(
            [sys.executable, SCRIPT, "--manifest", os.path.join(source_dir, "shaders.json"),
             "--glslang", TOOLS["glslang"], "--spirv-cross", TOOLS["spirv_cross"],
             "--output-dir", out_dir, "--header", os.path.join(out_dir, "Shaders.h"), "--namespace", "TestShaders"],
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=dict(os.environ, PYTHONUTF8="1"))
        return result.returncode, result.stdout.decode("utf-8", errors="replace"), out_dir

    def program(self, vert, layouts, groups, frag=FRAG_SOLID):
        manifest = {"bindGroupLayouts": layouts,
                    "programs": {"p": {"vertex": "p.vert", "fragment": "p.frag", "bindGroups": groups}}}
        return self.compile(manifest, {"p.vert": vert, "p.frag": frag})

    def test_平铺绑定号_各命名空间按组号依次编号(self):
        vert = """#version 450
layout(set = 0, binding = 0) uniform A { vec4 a; };
layout(set = 1, binding = 3) uniform samplerBuffer t;
layout(set = 2, binding = 0) uniform B { vec4 b; };
void main() { gl_Position = a + b + texelFetch(t, 0); }
"""
        frag = """#version 450
layout(set = 2, binding = 1) uniform sampler2D image;
layout(location = 0) out vec4 outColor;
void main() { outColor = texture(image, vec2(0.5)); }
"""
        layouts = {
            "G0": [{"binding": 0, "type": "uniformBuffer", "stages": ["vertex"]}],
            "G1": [{"binding": 3, "type": "texelBuffer", "stages": ["vertex"]}],
            "G2": [{"binding": 0, "type": "uniformBuffer", "stages": ["vertex"]},
                   {"binding": 1, "type": "combinedTextureSampler", "stages": ["fragment"]}],
        }
        code, output, out_dir = self.program(vert, layouts, ["G0", "G1", "G2"], frag)
        self.assertEqual(code, 0, output)
        with open(os.path.join(out_dir, "p.vert.glsl"), encoding="utf-8") as f:
            vert_glsl = f.read()
        with open(os.path.join(out_dir, "p.frag.glsl"), encoding="utf-8") as f:
            frag_glsl = f.read()
        # 常量缓冲：组 0 -> 0，组 2 -> 1；纹理单元：纹素缓冲 -> 0，纹理 -> 1
        self.assertRegex(vert_glsl, r"layout\(binding = 0, std140\) uniform A")
        self.assertRegex(vert_glsl, r"layout\(binding = 1, std140\) uniform B")
        self.assertRegex(vert_glsl, r"layout\(binding = 0\) uniform samplerBuffer t")
        self.assertRegex(frag_glsl, r"layout\(binding = 1\) uniform sampler2D image")
        self.assertTrue(os.path.exists(os.path.join(out_dir, "p.vert.spv")))
        with open(os.path.join(out_dir, "Shaders.h"), encoding="utf-8") as f:
            header = f.read()
        self.assertIn("inline constexpr Program p{\"p\", pBindGroups};", header)

    def test_资源不在清单里_失败(self):
        vert = """#version 450
layout(set = 0, binding = 1) uniform A { vec4 a; };
void main() { gl_Position = a; }
"""
        layouts = {"G0": [{"binding": 0, "type": "uniformBuffer", "stages": ["vertex"]}]}
        code, output, _ = self.program(vert, layouts, ["G0"])
        self.assertNotEqual(code, 0)
        self.assertIn("不在绑定组布局 G0 里", output)

    def test_类型与清单不同_失败(self):
        vert = """#version 450
layout(set = 0, binding = 0) uniform samplerBuffer t;
void main() { gl_Position = texelFetch(t, 0); }
"""
        layouts = {"G0": [{"binding": 0, "type": "uniformBuffer", "stages": ["vertex"]}]}
        code, output, _ = self.program(vert, layouts, ["G0"])
        self.assertNotEqual(code, 0)
        self.assertIn("声明的是 uniformBuffer", output)

    def test_阶段与清单不同_失败(self):
        vert = """#version 450
layout(set = 0, binding = 0) uniform A { vec4 a; };
void main() { gl_Position = a; }
"""
        layouts = {"G0": [{"binding": 0, "type": "uniformBuffer", "stages": ["fragment"]}]}
        code, output, _ = self.program(vert, layouts, ["G0"])
        self.assertNotEqual(code, 0)
        self.assertIn("没有声明这个阶段", output)

    def test_顶点阶段的存储缓冲_失败(self):
        vert = """#version 450
layout(set = 0, binding = 0) readonly buffer S { vec4 s[]; };
void main() { gl_Position = s[0]; }
"""
        layouts = {"G0": [{"binding": 0, "type": "storageBuffer", "stages": ["vertex"]}]}
        code, output, _ = self.program(vert, layouts, ["G0"])
        self.assertNotEqual(code, 0)
        self.assertIn("存储缓冲只能给片段着色器用", output)

    def test_内建实例序号_失败(self):
        vert = """#version 450
void main() { gl_Position = vec4(float(gl_InstanceIndex)); }
"""
        code, output, _ = self.program(vert, {}, [])
        self.assertNotEqual(code, 0)
        self.assertIn("gl_InstanceIndex", output)

    def test_推送常量_失败(self):
        vert = """#version 450
layout(push_constant) uniform P { vec4 p; };
void main() { gl_Position = p; }
"""
        code, output, _ = self.program(vert, {}, [])
        self.assertNotEqual(code, 0)
        self.assertIn("推送常量", output)

    def test_语法错误_失败(self):
        code, output, _ = self.program("#version 450\nvoid main() { gl_Position = ; }\n", {}, [])
        self.assertNotEqual(code, 0)
        self.assertIn("命令失败", output)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--glslang", required=True)
    parser.add_argument("--spirv-cross", required=True)
    args, rest = parser.parse_known_args()
    TOOLS["glslang"] = args.glslang
    TOOLS["spirv_cross"] = args.spirv_cross
    unittest.main(argv=[sys.argv[0]] + rest, verbosity=2)


if __name__ == "__main__":
    main()
