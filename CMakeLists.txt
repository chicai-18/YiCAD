# 设置cmake 最低版本
cmake_minimum_required (VERSION 3.21)

# 设置项目名称
project (YiCAD)

# Include GNUInstallDirs for standard install directories
include(GNUInstallDirs)
# 提供可重定位的 CMake 包配置和版本文件生成函数。
include(CMakePackageConfigHelpers)

set(PACKAGE_VERSION_MAJOR "0")
set(PACKAGE_VERSION_MINOR "20")
set(PACKAGE_VERSION_PATCH "0")
set(PACKAGE_VERSION_SUFFIX "") # either "dev" for development snapshot or "" (empty string)
set(YICAD_VERSION_PATCH "0") # number of patch release (e.g. "4" for the 0.18.4 release)
set(YICAD_VERSION "${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMakePredefinedTargets")

# Add Path for searching cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmakeutil.cmake)

# All third-party dependencies are resolved via Conan 2 (see cmake/dependencies.cmake).
# Qt 5.15, SARibbonBar, and CDT are discovered via find_package from user-provided paths.
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/dependencies.cmake)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/deployqt.cmake)

# Where binaries should be placed. Multi-config generators such as Visual
# Studio append the configuration name unless each config is set explicitly.
set(YICAD_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${YICAD_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${YICAD_OUTPUT_DIRECTORY})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${YICAD_OUTPUT_DIRECTORY})
set(_yicad_output_configs Debug Release RelWithDebInfo MinSizeRel)
if(CMAKE_CONFIGURATION_TYPES)
    list(APPEND _yicad_output_configs ${CMAKE_CONFIGURATION_TYPES})
endif()
list(REMOVE_DUPLICATES _yicad_output_configs)
foreach(_yicad_config IN LISTS _yicad_output_configs)
    string(TOUPPER "${_yicad_config}" _yicad_config_upper)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${_yicad_config_upper} ${YICAD_OUTPUT_DIRECTORY})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${_yicad_config_upper} ${YICAD_OUTPUT_DIRECTORY})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${_yicad_config_upper} ${YICAD_OUTPUT_DIRECTORY})
endforeach()
unset(_yicad_config_upper)
unset(_yicad_config)
unset(_yicad_output_configs)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Define compiler warnings
if (NOT MSVC) # Too much warnings on MSVC
     add_definitions("-Wall")
endif()

# 设置 std 和 c++版本为
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)

# Boost is consumed through Conan (header-only). See cmake/dependencies.cmake.

# settings path
set(SETTINGS_PATH "${PROJECT_BINARY_DIR}/bin")

#disable warnings
#4819 non Unicode format warning
if(MSVC)
     add_compile_options(/MP /W3 /wd4819 /Zc:inline)
endif()

OPTION(UPDATE_TRANSLATIONS "Update translations on compile" OFF)
MESSAGE(STATUS "UPDATE TRANSLATIONS: ${UPDATE_TRANSLATIONS}")
option(USE_VISUAL_LEAK_DETECTOR "Use visual leak detector to detect memory leakage" OFF)
# 设置子项目
add_subdirectory("YiCAD")
add_subdirectory("examples/demo_plugin" "examples/demo_plugin")

# YiCADPluginSdk CMake package 的生成、安装和消费流程：
# 1. 配置阶段：
#    - configure_package_config_file 根据 YiCADPluginSdkConfig.cmake.in 生成
#      YiCADPluginSdkConfig.cmake，作为 find_package 加载的包入口；
#    - write_basic_package_version_file 生成 YiCADPluginSdkConfigVersion.cmake，
#      用于检查调用方请求的 SDK 版本和目标架构。
# 2. 安装阶段：
#    - install(EXPORT) 根据 YiCadPluginSdk 接口目标生成
#      YiCADPluginSdkTargets.cmake，其中定义安装后的 YiCAD::PluginSdk；
#    - 上述三个文件一起安装到 lib/cmake/YiCADPluginSdk。
# 3. 插件消费阶段：
#    - find_package(YiCADPluginSdk CONFIG REQUIRED) 定位包配置；
#    - 指定版本时，ConfigVersion.cmake 先判断版本和架构是否兼容；
#    - Config.cmake 随后加载 Targets.cmake，创建 YiCAD::PluginSdk imported target；
#    - 插件链接该目标后继承公开头文件目录、C++23 要求和插件导出宏。
set(YICAD_PLUGIN_SDK_VERSION
    "${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}.${PACKAGE_VERSION_PATCH}")
set(YICAD_PLUGIN_SDK_CMAKE_INSTALL_DIR
    "${CMAKE_INSTALL_LIBDIR}/cmake/YiCADPluginSdk")

# INSTALL_DESTINATION 用于让生成的包入口根据自身位置推导安装前缀，避免写死路径。
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/YiCADPluginSdkConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/YiCADPluginSdkConfig.cmake"
    INSTALL_DESTINATION "${YICAD_PLUGIN_SDK_CMAKE_INSTALL_DIR}")
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/YiCADPluginSdkConfigVersion.cmake"
    VERSION "${YICAD_PLUGIN_SDK_VERSION}"
    COMPATIBILITY SameMajorVersion)

install(EXPORT YiCADPluginSdkTargets
        FILE YiCADPluginSdkTargets.cmake
        NAMESPACE YiCAD::
        DESTINATION "${YICAD_PLUGIN_SDK_CMAKE_INSTALL_DIR}"
        COMPONENT PluginSDK)
install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/YiCADPluginSdkConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/YiCADPluginSdkConfigVersion.cmake"
        DESTINATION "${YICAD_PLUGIN_SDK_CMAKE_INSTALL_DIR}"
        COMPONENT PluginSDK)
install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/doc/PLUGIN_SDK.md"
        "${CMAKE_CURRENT_SOURCE_DIR}/doc/PLUGIN_ABI_EVOLUTION.md"
        "${CMAKE_CURRENT_SOURCE_DIR}/doc/PLUGIN_ABI_V3_REFERENCE.md"
        "${CMAKE_CURRENT_SOURCE_DIR}/doc/PLUGIN_ABI_V3_RELEASE.md"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/YiCAD/sdk"
        COMPONENT PluginSDK)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/YiCAD/sdk/licenses"
        COMPONENT PluginSDK)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/examples/demo_plugin/"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/YiCAD/examples/demo_plugin"
        COMPONENT PluginSDK
        PATTERN "build" EXCLUDE
        PATTERN "build-*" EXCLUDE)

# Install third-party DLLs
# Helper function to install DLLs from a directory
function(install_third_party_dlls)
    if(NOT WIN32)
        return()
    endif()

    # Install SARibbonBar DLLs
    if(SARibbonBar_DLL_RELEASE)
        install(FILES "${SARibbonBar_DLL_RELEASE}"
                DESTINATION ${CMAKE_INSTALL_BINDIR}
                CONFIGURATIONS Release RelWithDebInfo MinSizeRel
                COMPONENT Runtime)
    endif()
    if(SARibbonBar_DLL_DEBUG)
        install(FILES "${SARibbonBar_DLL_DEBUG}"
                DESTINATION ${CMAKE_INSTALL_BINDIR}
                CONFIGURATIONS Debug
                COMPONENT Runtime)
    endif()

    # Install CDT DLLs
    if(CDT_DIR)
        find_file(CDT_DLL_RELEASE
            NAMES CDT.dll
            HINTS ${CDT_DIR}/../../bin ${CDT_DIR}/../bin ${CDT_DIR}/bin
            PATH_SUFFIXES Release
            NO_DEFAULT_PATH
        )
        find_file(CDT_DLL_DEBUG
            NAMES CDTd.dll CDT.dll
            HINTS ${CDT_DIR}/../../bin ${CDT_DIR}/../bin ${CDT_DIR}/bin
            PATH_SUFFIXES Debug
            NO_DEFAULT_PATH
        )

        if(CDT_DLL_RELEASE)
            message(STATUS "CDT DLL (Release): ${CDT_DLL_RELEASE}")
            install(FILES "${CDT_DLL_RELEASE}"
                    DESTINATION ${CMAKE_INSTALL_BINDIR}
                    CONFIGURATIONS Release RelWithDebInfo MinSizeRel
                    COMPONENT Runtime)
        endif()
        if(CDT_DLL_DEBUG)
            message(STATUS "CDT DLL (Debug): ${CDT_DLL_DEBUG}")
            install(FILES "${CDT_DLL_DEBUG}"
                    DESTINATION ${CMAKE_INSTALL_BINDIR}
                    CONFIGURATIONS Debug
                    COMPONENT Runtime)
        endif()
    endif()

    # The YiCAD target install rule copies the deployed runtime bundle from
    # the runtime output directory. Do not copy ${CMAKE_BINARY_DIR}/bin here:
    # if the install prefix is placed below that directory, CMake recursively
    # installs the destination into itself.
endfunction()

# Call the install function
install_third_party_dlls()

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
        DESTINATION ${CMAKE_INSTALL_BINDIR}/licenses
        COMPONENT Runtime)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/licenses/"
        DESTINATION ${CMAKE_INSTALL_BINDIR}/licenses
        COMPONENT Runtime
        FILES_MATCHING PATTERN "*.txt")

# 通过构建目标为 CMake build preset 提供组件化安装入口。
add_custom_target(install-runtime
    COMMAND "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}"
            --config "$<CONFIG>"
            --component Runtime
    COMMENT "Install YiCAD runtime component"
    USES_TERMINAL)
add_dependencies(install-runtime YiCAD)

add_custom_target(install-plugin-sdk
    COMMAND "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}"
            --config "$<CONFIG>"
            --component PluginSDK
    COMMENT "Install YiCAD plugin SDK component"
    USES_TERMINAL)
