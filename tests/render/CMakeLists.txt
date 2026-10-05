# tests/render —— 渲染：参考图纸出图比对（doc/RENDER_PLAN.md 第 5 节 0.2 步）、增量更新（阶段 1 起，第 4 阶段起
# 是图形系统的：变更集、选中、高亮、临时隐藏、预览变换）、RHI 一致性测试与着色器工具链（阶段 3）。
#
# 出图用 Mesa 的软件 OpenGL（llvmpipe），结果在不同机器上一致，基准图像才能入库（第 7 节 D9）：
# - tools/fetch_mesa.py 把 Mesa 取到 external/mesa/x64/（YICAD_MESA_DIR）；
# - test_render 以延迟加载方式链接 opengl32.dll，启动时按完整路径装入 Mesa（MesaLoader.cpp）。
#   Mesa 不放进 build/<cfg>/bin：那个目录会被 cmake --install 整个装进发布包。
# 图形系统的着色器从构建目录读（环境变量 YICAD_SHADER_DIR，GsDevice），构建时由 yicad_add_shaders 编好。
#
# 参考图纸经 DXF 插件读入，插件运行时在 YiCadShell 里（与 test_interaction 的 test_dxf_encoding 相同）。

set(YICAD_MESA_DIR "${CMAKE_SOURCE_DIR}/external/mesa/x64" CACHE PATH
    "Mesa 软件 OpenGL（opengl32.dll 与 libgallium_wgl.dll）所在目录，出图测试 test_render 用；由 tools/fetch_mesa.py 取得")

yicad_add_test(render
    SOURCES
        MesaLoader.cpp
        RenderHarness.cpp
        test_render_environment.cpp
        test_render_incremental.cpp
        test_render_linetype.cpp
        test_render_lod.cpp
        test_render_reference.cpp
        test_render_rhi.cpp
        test_render_custom.cpp
        ../support/SamplePipeEntity.cpp
    LINK YiCadShell
    LABELS "render;unit")

# RHI 一致性测试的着色器（shaders/），构建时经工具链编译（RENDER_PLAN.md 第 4.7.4 节）；
# 布局头文件 RhiTestShaders.h 生成在构建目录里
yicad_add_shaders(rhi_test_shaders
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders"
    HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated/RhiTestShaders.h"
    NAMESPACE RhiTestShaders)
add_dependencies(test_render rhi_test_shaders)
target_include_directories(test_render PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")

target_compile_definitions(test_render PRIVATE
    YICAD_MESA_DIR="${YICAD_MESA_DIR}"
    YICAD_SHADER_DIR="${YICAD_SHADER_OUTPUT_DIR}"
    YICAD_SUPPORT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../YiCAD/support"
    YICAD_RENDER_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    YICAD_RENDER_OUTPUT_DIR="${CMAKE_CURRENT_BINARY_DIR}/output"
    YICAD_RHI_TEST_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders"
    YICAD_DXF_PLUGIN_DLL="$<TARGET_FILE:YiCadDxfPlugin>")

# 参考图纸经构建出来的 DXF 插件读入，插件先于测试构建
add_dependencies(test_render YiCadDxfPlugin)

# 进程启动时不装入系统的 opengl32.dll，留给 MesaLoader.cpp 先装入 Mesa
if(MSVC)
    target_link_options(test_render PRIVATE "/DELAYLOAD:opengl32.dll")
    target_link_libraries(test_render PRIVATE delayimp)
endif()

# 着色器工具链本身：与清单不一致、用了 GL 4.3 不支持的写法时要失败（tools/test_compile_shaders.py）
add_test(NAME shader_toolchain
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tools/test_compile_shaders.py"
            --glslang "${YICAD_GLSLANG_VALIDATOR}" --spirv-cross "${YICAD_SPIRV_CROSS}")
set_tests_properties(shader_toolchain PROPERTIES
    LABELS "render;unit"
    ENVIRONMENT "PYTHONUTF8=1")
