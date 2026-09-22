# YiCAD 自动化测试树（方案阶段 0）
#
# 目录按 doc/ARCHITECTURE_EVOLUTION_PLAN.md 6.3 节的目标库分区，与
# YiCAD/cmake/sources/*.cmake 一一对应。阶段 3 拆库后，每个测试目标
# 只需链接对应的那一个库，而不是现在的整个 YiCadCore。
#
#   tests/math/         -> YiCadMath
#   tests/persistence/  -> YiCadPersistence
#   tests/geometry/     -> YiCadModel 中的几何实体
#
# 所有测试二进制以 test_ 开头：YiCAD/CMakeLists.txt 的安装规则按
# "test_*" 模式把它们排除在安装包之外。

find_package(GTest REQUIRED CONFIG)

# ---------------------------------------------------------------------------
# 测试进程的 PATH
#
# 构建树里只有 windeployqt 部署的 Qt DLL；SARibbonBar（以及按动态库构建时的
# CDT）要等 cmake --install 才会进安装目录。测试二进制跑在构建树里，因此把
# 这些 DLL 所在目录加进测试进程的 PATH，而不是往输出目录复制文件。
#
# ENVIRONMENT 属性的值是分号分隔的列表，而 Windows 的 PATH 也用分号，
# 所以两边都要转义。
# ---------------------------------------------------------------------------
if(WIN32)
    set(_yicad_test_dll_dirs "")
    foreach(_dll IN ITEMS "${SARibbonBar_DLL_RELEASE}" "${SARibbonBar_DLL_DEBUG}"
                          "${CDT_DLL_RELEASE}" "${CDT_DLL_DEBUG}")
        # find_file 找不到时是 <VAR>-NOTFOUND，在 if() 里为假
        if(_dll AND EXISTS "${_dll}")
            get_filename_component(_dir "${_dll}" DIRECTORY)
            list(APPEND _yicad_test_dll_dirs "${_dir}")
        endif()
    endforeach()

    if(_yicad_test_dll_dirs)
        list(REMOVE_DUPLICATES _yicad_test_dll_dirs)
        string(REPLACE ";" "\;" _yicad_host_path "$ENV{PATH}")
        string(REPLACE ";" "\;" _yicad_test_dll_dirs "${_yicad_test_dll_dirs}")
        set(YICAD_TEST_DLL_PATH "${_yicad_test_dll_dirs}\;${_yicad_host_path}")
    endif()
    unset(_yicad_test_dll_dirs)
    unset(_yicad_host_path)
    unset(_dll)
    unset(_dir)
endif()

# 刻意不用 gtest_discover_tests：它在构建后运行测试二进制来枚举用例，
# 而这些二进制需要 windeployqt 部署的 Qt DLL 就位才能启动，构建期还不一定
# 满足。一个子系统注册成一个 CTest 条目就够了——失败时 GoogleTest 自己的
# 输出会指明是哪个用例。代价是无法用 ctest -R 单独跑某个用例，
# 那种场景直接调二进制加 --gtest_filter 更方便。

# ---------------------------------------------------------------------------
# yicad_add_test(<name> SOURCES <...> [LABELS <...>])
#
# 建一个链接 YiCadCore 的 GoogleTest 目标并注册到 CTest。
# <name> 不带 test_ 前缀，函数会自动加上。
# ---------------------------------------------------------------------------
function(yicad_add_test name)
    cmake_parse_arguments(ARG "" "" "SOURCES;LABELS" ${ARGN})
    if(NOT ARG_SOURCES)
        message(FATAL_ERROR "yicad_add_test(${name}) 缺少 SOURCES")
    endif()

    set(target "test_${name}")
    # 用共用入口而非 GTest::gtest_main：测试跑起来之前要先初始化
    # QApplication 与 DmSystem 的类型系统，见 support/yicad_test_main.cpp。
    add_executable(${target} ${ARG_SOURCES}
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/support/yicad_test_main.cpp")

    # YiCadCore 是 OBJECT 库：链接它会带入全部目标文件和使用要求
    # （include 路径、编译定义、Qt 与第三方依赖），测试不必重复编译内核。
    target_link_libraries(${target} PRIVATE YiCadCore GTest::gtest)

    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

    set_target_properties(${target} PROPERTIES FOLDER "Tests")

    # 测试是控制台程序；YiCAD 可执行目标的 /SUBSYSTEM:WINDOWS 不适用于它们。
    if(WIN32)
        set_target_properties(${target} PROPERTIES
            LINK_FLAGS_RELEASE "/SUBSYSTEM:CONSOLE"
            LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE"
            LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:CONSOLE"
            LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
    endif()

    add_test(NAME ${target} COMMAND ${target})

    # 测试二进制与 YiCAD.exe 输出到同一目录，直接复用 windeployqt 部署的 Qt DLL。
    set_tests_properties(${target} PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:${target}>")

    # SARibbonBar 的 DLL 只在 cmake --install 时才进安装目录，构建树里没有，
    # 测试二进制会以 0xc0000135（STATUS_DLL_NOT_FOUND）起不来。
    # 这里只给测试进程加 PATH，不往构建产物里复制任何东西。
    if(WIN32 AND YICAD_TEST_DLL_PATH)
        set_tests_properties(${target} PROPERTIES
            ENVIRONMENT "PATH=${YICAD_TEST_DLL_PATH}")
    endif()

    if(ARG_LABELS)
        set_tests_properties(${target} PROPERTIES LABELS "${ARG_LABELS}")
    endif()

    # 测试要跑就得先有 Qt DLL；让测试目标依赖可执行目标，windeployqt 先跑完。
    add_dependencies(${target} YiCAD)
endfunction()

add_subdirectory(math)
add_subdirectory(geometry)
add_subdirectory(persistence)
