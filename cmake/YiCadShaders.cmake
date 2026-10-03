# 着色器工具链（doc/RENDER_PLAN.md 第 4.7.4 节，D3-A）
#
# 着色器写成 GLSL 450（Vulkan 方言），由清单 shaders.json 描述绑定组布局与程序；构建时
# tools/compile_shaders.py 调 glslangValidator 编成 SPIR-V、用 spirv-cross 反射检查并生成
# GL 用的 GLSL 430，同时生成 C++ 布局头文件。glslangValidator 与 spirv-cross 是 Conan 的
# tool_requires，CMakeToolchain 把它们的目录加进 CMAKE_PROGRAM_PATH。

find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_program(YICAD_GLSLANG_VALIDATOR glslangValidator REQUIRED
    DOC "glslangValidator（Conan 的 tool_requires glslang 提供）")
find_program(YICAD_SPIRV_CROSS spirv-cross REQUIRED
    DOC "spirv-cross（Conan 的 tool_requires spirv-cross 提供）")

set(YICAD_COMPILE_SHADERS_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/../tools/compile_shaders.py")
cmake_path(NORMAL_PATH YICAD_COMPILE_SHADERS_SCRIPT)

# ---------------------------------------------------------------------------
# yicad_add_shaders(<target>
#     SOURCE_DIR <dir>               着色器源码与清单 shaders.json 所在目录
#     OUTPUT_DIR <dir>               生成的 <程序>.<阶段>.spv 与 .glsl 放在这里
#     HEADER <file>                  生成的 C++ 头文件（绑定组布局与程序）
#     NAMESPACE <name>               头文件里的命名空间
#     [INSTALL_DESTINATION <dir>])   给出时 cmake --install 把 .spv 与 .glsl 装到这里（Runtime 组件）
#
# 建一个自定义目标 <target>；用到生成头文件或着色器的目标要 add_dependencies 它。
# SOURCE_DIR 里任何文件变了都重新编译整组（着色器不多，比维护 #include 依赖简单）。
# ---------------------------------------------------------------------------
function(yicad_add_shaders target)
    cmake_parse_arguments(ARG "" "SOURCE_DIR;OUTPUT_DIR;HEADER;NAMESPACE;INSTALL_DESTINATION" "" ${ARGN})
    foreach(_arg IN ITEMS SOURCE_DIR OUTPUT_DIR HEADER NAMESPACE)
        if(NOT ARG_${_arg})
            message(FATAL_ERROR "yicad_add_shaders(${target}) 缺少 ${_arg}")
        endif()
    endforeach()
    if(NOT EXISTS "${ARG_SOURCE_DIR}/shaders.json")
        message(FATAL_ERROR "yicad_add_shaders(${target})：${ARG_SOURCE_DIR} 里没有 shaders.json")
    endif()

    file(GLOB _inputs CONFIGURE_DEPENDS "${ARG_SOURCE_DIR}/*")
    set(_stamp "${ARG_OUTPUT_DIR}/${target}.stamp")
    add_custom_command(
        OUTPUT "${_stamp}"
        BYPRODUCTS "${ARG_HEADER}"
        COMMAND "${Python3_EXECUTABLE}" "${YICAD_COMPILE_SHADERS_SCRIPT}"
                --manifest "${ARG_SOURCE_DIR}/shaders.json"
                --glslang "${YICAD_GLSLANG_VALIDATOR}"
                --spirv-cross "${YICAD_SPIRV_CROSS}"
                --output-dir "${ARG_OUTPUT_DIR}"
                --header "${ARG_HEADER}"
                --namespace "${ARG_NAMESPACE}"
                --stamp "${_stamp}"
                --repo-root "${PROJECT_SOURCE_DIR}"
        DEPENDS ${_inputs} "${YICAD_COMPILE_SHADERS_SCRIPT}"
        COMMENT "编译着色器 ${target}"
        VERBATIM)
    add_custom_target(${target} DEPENDS "${_stamp}" SOURCES ${_inputs})
    set_target_properties(${target} PROPERTIES FOLDER "Shaders")

    if(ARG_INSTALL_DESTINATION)
        install(DIRECTORY "${ARG_OUTPUT_DIR}/"
                DESTINATION "${ARG_INSTALL_DESTINATION}"
                COMPONENT Runtime
                FILES_MATCHING PATTERN "*.spv" PATTERN "*.glsl")
    endif()
endfunction()
