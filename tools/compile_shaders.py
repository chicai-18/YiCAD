#!/usr/bin/env python3
"""着色器工具链（doc/RENDER_PLAN.md 第 4.7.4 节，D3-A），由 CMake 的 yicad_add_shaders 在构建时调用。

着色器只写一份：GLSL 450（Vulkan 方言），可以用 #include（GL_GOOGLE_include_directive）。
一个目录里的全部着色器程序由清单 shaders.json 描述：

    {
      "bindGroupLayouts": {
        "Frame": [ { "binding": 0, "type": "uniformBuffer", "stages": ["vertex", "fragment"] } ],
        ...
      },
      "programs": {
        "line": { "vertex": "line.vert", "fragment": "line.frag", "bindGroups": ["Frame", null, "Image"] }
      }
    }

bindGroupLayouts 是有名字的绑定组布局，各程序共用（Vulkan 要求布局一致才能共用绑定组）。绑定的
type 取 uniformBuffer、storageBuffer、texelBuffer、combinedTextureSampler；可选的 "dynamicOffset": true
表示绑定时另给动态偏移。programs 的 bindGroups 按组号排列，null 表示该组不用。

对每个程序：
1. glslangValidator 把各阶段编成 SPIR-V（<程序>.<阶段>.spv，Vulkan 后端用）；
2. spirv-cross --reflect 取出各阶段用到的资源，对照清单检查（组、绑定、类型、阶段），不一致就失败；
   另外拒绝 GL 4.3 上不能照搬的写法：顶点阶段的存储缓冲、内建实例序号（第 4.7.3 节）；
3. 把 (组, 绑定) 换成 GL 的平铺绑定点：改写 SPIR-V 的 Binding/DescriptorSet 修饰，再由 spirv-cross
   生成 GLSL 430（<程序>.<阶段>.glsl，GL 后端用）。平铺规则与 GLRhiPipeline 相同：按组号、组内按绑定号
   依次编号，常量缓冲、存储缓冲、纹理单元（纹素缓冲与纹理）各自从 0 起；
4. 生成 C++ 头文件：各绑定组布局（RhiBindGroupLayoutEntry 数组）与每个程序按组号排列的布局，
   C++ 据此建 RhiBindGroupLayout 与管线，与着色器天然一致。

只在内容变化时改写输出文件，免得引用生成头文件的源文件无谓地重新编译。
"""

import argparse
import json
import os
import struct
import subprocess
import sys
import tempfile

STAGES = {
    "vertex": {"glslang": "vert", "suffix": "vert", "cpp": "RhiShaderStage::Vertex"},
    "fragment": {"glslang": "frag", "suffix": "frag", "cpp": "RhiShaderStage::Fragment"},
}

BINDING_TYPES = {
    "uniformBuffer": "RhiBindingType::UniformBuffer",
    "storageBuffer": "RhiBindingType::StorageBuffer",
    "texelBuffer": "RhiBindingType::TexelBuffer",
    "combinedTextureSampler": "RhiBindingType::CombinedTextureSampler",
}

# GL 的三个绑定点命名空间
GL_NAMESPACE = {
    "uniformBuffer": "ubo",
    "storageBuffer": "ssbo",
    "texelBuffer": "texture",
    "combinedTextureSampler": "texture",
}

# SPIR-V 的常量（SPIR-V 规范 3.x 节）
SPIRV_MAGIC = 0x07230203
OP_DECORATE = 71
DECORATION_BUILTIN = 11
DECORATION_BINDING = 33
DECORATION_DESCRIPTOR_SET = 34
FORBIDDEN_BUILTINS = {
    43: "gl_InstanceIndex",
    4424: "gl_BaseVertex",
    4425: "gl_BaseInstance",
    4426: "gl_DrawID",
}


class ShaderError(Exception):
    """清单或着色器与约定不符；消息直接给用户看"""


def run(args):
    result = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output = result.stdout.decode("utf-8", errors="replace")
    if result.returncode != 0:
        raise ShaderError("命令失败：" + " ".join(args) + "\n" + output)
    return output


def write_if_changed(path, data):
    """data 为 bytes；内容相同时不动文件"""
    if os.path.exists(path):
        with open(path, "rb") as f:
            if f.read() == data:
                return
    with open(path, "wb") as f:
        f.write(data)


# ---------------------------------------------------------------------------
# 清单
# ---------------------------------------------------------------------------

def load_manifest(path):
    with open(path, "r", encoding="utf-8") as f:
        manifest = json.load(f)

    layouts = manifest.get("bindGroupLayouts", {})
    for name, entries in layouts.items():
        if not name.isidentifier():
            raise ShaderError(f"绑定组布局名 {name} 不是合法的 C++ 标识符")
        seen = set()
        for entry in entries:
            where = f"绑定组布局 {name} 的绑定 {entry.get('binding')}"
            if not isinstance(entry.get("binding"), int) or entry["binding"] < 0:
                raise ShaderError(f"{where}：binding 要是非负整数")
            if entry["binding"] in seen:
                raise ShaderError(f"{where}：绑定号重复")
            seen.add(entry["binding"])
            if entry.get("type") not in BINDING_TYPES:
                raise ShaderError(f"{where}：type 只能是 {', '.join(BINDING_TYPES)}")
            stages = entry.get("stages")
            if not stages or any(s not in STAGES for s in stages):
                raise ShaderError(f"{where}：stages 只能取 {', '.join(STAGES)}")
            if entry["type"] == "storageBuffer" and "vertex" in stages:
                raise ShaderError(f"{where}：存储缓冲只能给片段着色器用，GL 4.3 不保证顶点阶段可用"
                                  "（RENDER_PLAN.md 第 4.7.3 节）；顶点阶段改用纹素缓冲或常量缓冲")
            if entry.get("dynamicOffset", False) and entry["type"] not in ("uniformBuffer", "storageBuffer"):
                raise ShaderError(f"{where}：只有常量缓冲与存储缓冲可以带动态偏移")
        entries.sort(key=lambda e: e["binding"])

    programs = manifest.get("programs", {})
    if not programs:
        raise ShaderError(f"{path} 里没有程序")
    for name, program in programs.items():
        if not name.isidentifier():
            raise ShaderError(f"程序名 {name} 不是合法的 C++ 标识符")
        for stage in ("vertex", "fragment"):
            if stage not in program:
                raise ShaderError(f"程序 {name} 缺少 {stage} 阶段")
        for key in program:
            if key not in STAGES and key != "bindGroups":
                raise ShaderError(f"程序 {name}：不认识的键 {key}")
        for group in program.get("bindGroups", []):
            if group is not None and group not in layouts:
                raise ShaderError(f"程序 {name} 用到的绑定组布局 {group} 没有声明")
    return layouts, programs


def gl_bindings(layouts, program):
    """(组, 绑定) -> GL 平铺绑定点；规则与 GLRhiPipeline 相同"""
    counters = {"ubo": 0, "ssbo": 0, "texture": 0}
    result = {}
    for set_index, group in enumerate(program.get("bindGroups", [])):
        if group is None:
            continue
        for entry in layouts[group]:
            namespace = GL_NAMESPACE[entry["type"]]
            result[(set_index, entry["binding"])] = counters[namespace]
            counters[namespace] += 1
    return result


# ---------------------------------------------------------------------------
# 反射检查
# ---------------------------------------------------------------------------

def reflected_resources(reflection, where):
    """spirv-cross --reflect 的结果 -> [(组, 绑定, 类型, 名字)]"""
    resources = []
    for item in reflection.get("ubos", []):
        resources.append((item, "uniformBuffer"))
    for item in reflection.get("ssbos", []):
        resources.append((item, "storageBuffer"))
    for item in reflection.get("textures", []):
        resources.append((item, "texelBuffer" if "Buffer" in item["type"] else "combinedTextureSampler"))
    for item in reflection.get("separate_images", []):
        if "Buffer" not in item["type"]:
            raise ShaderError(f"{where}：{item['name']} 是分离的纹理，RHI 只支持纹理与采样器合一的绑定")
        resources.append((item, "texelBuffer"))
    for key in ("separate_samplers", "images", "push_constants", "subpass_inputs",
                "acceleration_structures", "atomic_counters"):
        for item in reflection.get(key, []):
            raise ShaderError(f"{where}：{item.get('name')} 的资源类型（{key}）RHI 不支持；"
                              "GL 没有推送常量，每次绘制的数据放在第 2 组的缓冲里（第 4.7.3 节）")

    result = []
    for item, binding_type in resources:
        if "array" in item:
            raise ShaderError(f"{where}：{item['name']} 是资源数组，RHI 不支持")
        result.append((item.get("set", 0), item.get("binding", 0), binding_type, item["name"]))
    return result


def check_resources(resources, layouts, program, stage, where):
    groups = program.get("bindGroups", [])
    for set_index, binding, binding_type, name in resources:
        group = groups[set_index] if set_index < len(groups) else None
        if group is None:
            raise ShaderError(f"{where}：{name} 在第 {set_index} 组，程序没有声明这一组")
        entry = next((e for e in layouts[group] if e["binding"] == binding), None)
        if entry is None:
            raise ShaderError(f"{where}：{name}（组 {set_index}，绑定 {binding}）不在绑定组布局 {group} 里")
        if entry["type"] != binding_type:
            raise ShaderError(f"{where}：{name}（组 {set_index}，绑定 {binding}）在着色器里是 {binding_type}，"
                              f"绑定组布局 {group} 声明的是 {entry['type']}")
        if stage not in entry["stages"]:
            raise ShaderError(f"{where}：{name}（组 {set_index}，绑定 {binding}）在 {stage} 阶段用到，"
                              f"绑定组布局 {group} 没有声明这个阶段")


# ---------------------------------------------------------------------------
# SPIR-V 改写
# ---------------------------------------------------------------------------

def remap_spirv(data, mapping, where):
    """检查禁用的内建变量，并把 Binding 换成 GL 平铺绑定点、DescriptorSet 置 0"""
    if len(data) % 4 != 0:
        raise ShaderError(f"{where}：SPIR-V 长度不是 4 的倍数")
    words = list(struct.unpack(f"<{len(data) // 4}I", data))
    if words[0] != SPIRV_MAGIC:
        raise ShaderError(f"{where}：不是小端序的 SPIR-V")

    sets = {}       # id -> 组号所在字的下标
    bindings = {}   # id -> 绑定号所在字的下标
    index = 5
    while index < len(words):
        word_count = words[index] >> 16
        opcode = words[index] & 0xFFFF
        if word_count == 0:
            raise ShaderError(f"{where}：SPIR-V 指令长度为 0")
        if opcode == OP_DECORATE and word_count >= 4:
            target = words[index + 1]
            decoration = words[index + 2]
            if decoration == DECORATION_BINDING:
                bindings[target] = index + 3
            elif decoration == DECORATION_DESCRIPTOR_SET:
                sets[target] = index + 3
            elif decoration == DECORATION_BUILTIN and words[index + 3] in FORBIDDEN_BUILTINS:
                raise ShaderError(f"{where}：用了 {FORBIDDEN_BUILTINS[words[index + 3]]}。GL 4.3 的 gl_InstanceID 不含"
                                  " baseInstance，取记录用步进为 1 的实例属性（RENDER_PLAN.md 第 4.7.3 节）")
        index += word_count

    for target, binding_index in bindings.items():
        set_index = sets.get(target)
        set_value = words[set_index] if set_index is not None else 0
        key = (set_value, words[binding_index])
        if key not in mapping:
            raise ShaderError(f"{where}：SPIR-V 里有组 {key[0]}、绑定 {key[1]} 的资源，清单里找不到")
        words[binding_index] = mapping[key]
        if set_index is not None:
            words[set_index] = 0
    return struct.pack(f"<{len(words)}I", *words)


# ---------------------------------------------------------------------------
# 生成
# ---------------------------------------------------------------------------

def compile_program(name, program, layouts, args, work_dir):
    mapping = gl_bindings(layouts, program)
    for stage in ("vertex", "fragment"):
        source = os.path.join(args.source_dir, program[stage])
        where = f"{program[stage]}（程序 {name}，{stage}）"
        suffix = STAGES[stage]["suffix"]
        spv_path = os.path.join(args.output_dir, f"{name}.{suffix}.spv")

        spv_tmp = os.path.join(work_dir, f"{name}.{suffix}.spv")
        run([args.glslang, "-V", "--target-env", "vulkan1.3", "-S", STAGES[stage]["glslang"],
             "-I" + args.source_dir, "-o", spv_tmp, source])
        with open(spv_tmp, "rb") as f:
            spirv = f.read()

        reflection = json.loads(run([args.spirv_cross, spv_tmp, "--reflect"]))
        check_resources(reflected_resources(reflection, where), layouts, program, stage, where)

        gl_spv = os.path.join(work_dir, f"{name}.{suffix}.gl.spv")
        with open(gl_spv, "wb") as f:
            f.write(remap_spirv(spirv, mapping, where))
        glsl = run([args.spirv_cross, gl_spv, "--version", "430", "--no-es"])
        # spirv-cross 为模拟 Vulkan 语义而加的隐藏 uniform（如 SPIRV_Cross_BaseInstance）GL 后端不会去设
        if "SPIRV_Cross_" in glsl:
            raise ShaderError(f"{where}：生成的 GLSL 430 里有 spirv-cross 的辅助 uniform，GL 后端不支持：\n{glsl}")

        write_if_changed(spv_path, spirv)
        write_if_changed(os.path.join(args.output_dir, f"{name}.{suffix}.glsl"),
                         glsl.replace("\r\n", "\n").encode("utf-8"))


def stage_flags(stages):
    return " | ".join(STAGES[s]["cpp"] for s in stages)


def generate_header(layouts, programs, args):
    manifest_name = os.path.relpath(args.manifest, args.repo_root).replace("\\", "/") if args.repo_root \
        else os.path.basename(args.manifest)
    lines = [
        f"// 由 tools/compile_shaders.py 依据 {manifest_name} 生成，不要手改。",
        "// 绑定组布局与各程序按组号排列的布局；着色器在构建时已对照它们检查过（RENDER_PLAN.md 第 4.7.4 节）。",
        "#pragma once",
        "",
        "#include <span>",
        "#include <string_view>",
        "",
        "#include \"RhiTypes.h\"",
        "",
        f"namespace {args.namespace}",
        "{",
        "",
    ]
    for name, entries in layouts.items():
        lines.append(f"/// @brief 绑定组布局 {name}")
        lines.append(f"inline constexpr RhiBindGroupLayoutEntry {name}[] = {{")
        for entry in entries:
            dynamic = "true" if entry.get("dynamicOffset", False) else "false"
            lines.append(f"    {{{entry['binding']}, {BINDING_TYPES[entry['type']]}, "
                         f"{stage_flags(entry['stages'])}, {dynamic}}},")
        lines.append("};")
        lines.append("")

    lines.append("/// @brief 一个着色器程序：名字（文件名 <名字>.<阶段>.spv/.glsl）与按组号排列的绑定组布局，空表示该组不用")
    lines.append("struct Program")
    lines.append("{")
    lines.append("    std::string_view name;")
    lines.append("    std::span<const std::span<const RhiBindGroupLayoutEntry>> bindGroups;")
    lines.append("};")
    lines.append("")
    for name, program in programs.items():
        groups = program.get("bindGroups", [])
        if groups:
            items = ", ".join(g if g is not None else "std::span<const RhiBindGroupLayoutEntry>{}" for g in groups)
            lines.append(f"inline constexpr std::span<const RhiBindGroupLayoutEntry> {name}BindGroups[] = {{{items}}};")
            lines.append(f"inline constexpr Program {name}{{\"{name}\", {name}BindGroups}};")
        else:
            lines.append(f"inline constexpr Program {name}{{\"{name}\", {{}}}};")
        lines.append("")
    lines.append(f"}}  // namespace {args.namespace}")
    lines.append("")
    write_if_changed(args.header, "\n".join(lines).encode("utf-8"))


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--manifest", required=True, help="着色器清单 shaders.json")
    parser.add_argument("--glslang", required=True, help="glslangValidator 的路径")
    parser.add_argument("--spirv-cross", required=True, help="spirv-cross 的路径")
    parser.add_argument("--output-dir", required=True, help="生成的 .spv 与 .glsl 放在这里")
    parser.add_argument("--header", required=True, help="生成的 C++ 头文件")
    parser.add_argument("--namespace", required=True, help="头文件里的命名空间")
    parser.add_argument("--stamp", help="成功后写这个文件，供构建系统判断是否要重跑")
    parser.add_argument("--repo-root", help="头文件注释里按它写清单的相对路径")
    args = parser.parse_args()
    args.source_dir = os.path.dirname(os.path.abspath(args.manifest))

    try:
        layouts, programs = load_manifest(args.manifest)
        os.makedirs(args.output_dir, exist_ok=True)
        os.makedirs(os.path.dirname(os.path.abspath(args.header)), exist_ok=True)
        with tempfile.TemporaryDirectory() as work_dir:
            for name, program in programs.items():
                compile_program(name, program, layouts, args, work_dir)
        generate_header(layouts, programs, args)
    except ShaderError as error:
        # 带上清单路径，VS 的输出窗口里能直接看出是哪一组着色器
        print(f"{args.manifest}: error: {error}", file=sys.stderr)
        return 1

    if args.stamp:
        with open(args.stamp, "w", encoding="utf-8") as f:
            f.write("ok\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
