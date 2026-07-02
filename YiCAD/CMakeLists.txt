# 设置cmake 最低版本
cmake_minimum_required (VERSION 3.21)

PROJECT (YiCAD)

# 加载Qt模块
# Qt 5.15 is discovered from the user's installation via CMAKE_PREFIX_PATH or Qt5_DIR.
set(YICAD_USE_QT_MODULES Widgets Gui Core PrintSupport Designer Svg Network)

find_package(Qt5 5.15 REQUIRED COMPONENTS ${YICAD_USE_QT_MODULES})

set(CMAKE_AUTOMOC ON)
SET(QT_USE_QTDESIGNER ON)

# Qt5 imported targets (Qt5::Core, Qt5::Widgets, etc.) carry their include
# directories transitively. No need for explicit include_directories().

# Dependencies come from find_package in cmake/dependencies.cmake.
# The Conan-generated *Config.cmake files provide imported targets that
# carry their own include directories.

# 设置.qrc目录
set(qrc_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/res/ribbon/ribbon_icon.qrc)

# #####################################################
# ====================actions==========================
FILE(GLOB ACTIONS_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/actions/*.h)
file(GLOB ACTIONS_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/actions/*.cpp)

# #####################################################
# ======================cmd============================
file(GLOB CMD_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/cmd/*.h)
file(GLOB CMD_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/cmd/*.cpp)

# #####################################################
# ======================lib============================
file(GLOB LIB_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/actions/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/data_model/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/data_model/dimension/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/data_model/text/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/builder_model/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/builder_model/array/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/builder_model/dimension/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/builder_model/text/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/debug/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/fileio/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/filters/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/generators/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/gui/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/history/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/information/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/math/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/modification/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/printing/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/solver/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/painters/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/painters/opengl/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/temp/entity_builders/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/temp/entity_data/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/temp/geometry/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/utility/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/persistence/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/persistence/Meta/*.h
)

file(GLOB LIB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/actions/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/data_model/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/data_model/dimension/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/data_model/text/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/builder_model/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/builder_model/array/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/builder_model/dimension/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/builder_model/text/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/debug/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/fileio/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/filters/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/generators/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/gui/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/history/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/information/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/math/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/modification/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/printing/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/solver/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/painters/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/painters/opengl/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/temp/entity_builders/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/temp/entity_data/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/temp/geometry/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/utility/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/persistence/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/persistence/Meta/*.cpp
)

# #####################################################
# ======================main===========================
file(GLOB MAIN_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/main/*.h)

file(GLOB MAIN_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/main/*.cpp)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/YiCAD.rc.cmake ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/YiCAD.rc)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/main/icon.ico DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(GLOB MAIN_RES ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/YiCAD.rc)

# #####################################################
# =======================ai=============================
file(GLOB AI_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/ai/*.h)
file(GLOB AI_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/ai/*.cpp)

# #####################################################
# =================plugin runtime======================
file(GLOB PLUGIN_RUNTIME_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugin_runtime/*.h)
file(GLOB PLUGIN_RUNTIME_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugin_runtime/*.cpp)
set(PLUGIN_SDK_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugin_runtime/YiCadPluginAbi.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugin_runtime/YiCadPluginSdk.h)

# SDK 仅传播公开头文件、C++ 标准和插件导出宏，不链接 YiCAD 或 Qt 内部库。
add_library(YiCadPluginSdk INTERFACE)
add_library(YiCAD::PluginSdk ALIAS YiCadPluginSdk)
# 安装导出时与 YiCAD:: 命名空间组合为 YiCAD::PluginSdk。
set_target_properties(YiCadPluginSdk PROPERTIES EXPORT_NAME PluginSdk)
# 构建树使用源码目录，安装后使用稳定的公开头文件目录。
target_include_directories(YiCadPluginSdk INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/plugin_runtime>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/YiCAD/plugin-sdk>")
target_compile_features(YiCadPluginSdk INTERFACE cxx_std_23)
target_compile_definitions(YiCadPluginSdk INTERFACE YICAD_PLUGIN_BUILD)
target_sources(YiCadPluginSdk INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/plugin_runtime/YiCadPluginAbi.h>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/plugin_runtime/YiCadPluginSdk.h>")

# PluginSDK 是安装组件名称，与 find_package 的 COMPONENTS 参数无关。
install(TARGETS YiCadPluginSdk
        EXPORT YiCADPluginSdkTargets
        COMPONENT PluginSDK)
install(FILES ${PLUGIN_SDK_HEADERS}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/YiCAD/plugin-sdk
        COMPONENT PluginSDK)

# #####################################################
# =======================UI============================
file(GLOB UI_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/*.h
${CMAKE_CURRENT_SOURCE_DIR}/src/ui/forms/*.h)

file(GLOB UI_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/ui/forms/*.cpp)

# 设置.ui目录
file(GLOB UI_UIS_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/*.ui
${CMAKE_CURRENT_SOURCE_DIR}/src/ui/forms/*.ui)

# #####################################################
file(GLOB TS_FILES "${CMAKE_CURRENT_SOURCE_DIR}/ts/*.ts")
   
# 将.ui转成.h
QT5_WRAP_UI(ui_FILES ${UI_UIS_HEADERS})

# 将.qrc转成.cpp
qt5_add_resources(qrc_FILES ${qrc_DIRS})

# 生成moc文件
qt5_wrap_cpp(moc_FILES ${UI_HEADERS})

# 将生成的uic文件夹添加到包含目录
include_directories(${CMAKE_CURRENT_BINARY_DIR})


# 附加库路径(lib路径)
find_package(Qt5LinguistTools REQUIRED)
set (ALL_RSC_FILES ${ACTIONS_SOURCES}
                    ${ACTIONS_HEADERS}
                    ${CMD_SOURCES}
                    ${CMD_HEADERS}
                    ${MAIN_SOURCES}
                    ${MAIN_HEADERS}
                    ${AI_SOURCES}
                    ${AI_HEADERS}
                    ${PLUGIN_RUNTIME_SOURCES}
                    ${PLUGIN_RUNTIME_HEADERS}
                    ${UI_SOURCES}
                    ${LIB_SOURCES}
                    ${LIB_HEADERS}
		    ${MAIN_RES}
                     )

if(UPDATE_TRANSLATIONS)
    QT5_CREATE_TRANSLATION_Exclude_SubFolder(QM_FILES ${ALL_RSC_FILES} ${TS_FILES})
else()
    QT5_ADD_TRANSLATION(QM_FILES ${TS_FILES})
endif(UPDATE_TRANSLATIONS )

SET(QM_FILES ${QM_FILES} PARENT_SCOPE)
ADD_CUSTOM_TARGET (TS_YiCAD ALL DEPENDS ${QM_FILES})
set_target_properties(TS_YiCAD PROPERTIES FOLDER "CMakePredefinedTargets")

add_executable(${PROJECT_NAME} ${ALL_RSC_FILES}                                 
                                ${UI_HEADERS}                               
                                ${ui_FILES}
                                ${moc_FILES}
                                 ${qrc_DIRS}
                                 ${qrc_FILES}
                                 ${QM_FILES})

# 支持linux相对路径库加载，参考：https://stackoverflow.com/questions/43330165/how-to-link-a-shared-library-with-cmake-with-relative-path/69707790#69707790
# https://gitlab.kitware.com/cmake/community/-/wikis/doc/cmake/RPATH-handling
if(NOT WIN32)
set_target_properties(${PROJECT_NAME} PROPERTIES
    LINK_FLAGS "-Wl,-rpath,$ORIGIN")
endif()

# muparser headers come from the Conan package.
# SARibbonBar comes from find_package.

include_directories(src/actions
src/cmd
src/kernel/data_model
src/kernel/data_model/dimension
src/kernel/data_model/text
src/kernel/builder_model
src/kernel/builder_model/array
src/kernel/builder_model/dimension
src/kernel/builder_model/text
src/kernel/actions
src/kernel/debug
src/kernel/engine
src/kernel/fileio
src/kernel/filters
src/kernel/generators
src/kernel/gui
src/kernel/history
src/kernel/information
src/kernel/math
src/kernel/modification
src/kernel/printing
src/kernel/scripting
src/kernel/solver
src/kernel/painters
src/kernel/painters/opengl
src/kernel/temp/entity_builders
src/kernel/temp/entity_data
src/kernel/utility
src/kernel/persistence
src/kernel/persistence/Meta
src/main
src/plugin_runtime
src/ai
src/ui
src/ui/forms)


# 添加预处理器定义值
Add_Definitions(-D_USE_MATH_DEFINES -D_ENABLE_EXTENDED_ALIGNED_STORAGE -DAPPDIR="YiCAD" -DVERSION="0.5.0.0-alpha")
remove_definitions(-D_HAS_STD_BYTE)

# MSVC: 强制源文件和执行字符集为 UTF-8，避免中文乱码
if(MSVC)
    add_compile_options(/utf-8)
endif()

# 根据选项
use_visual_leak_detector(${PROJECT_NAME})

# Link against Conan-generated imported targets.
# GLEW, OpenGL, FreeType, ZLIB, pugixml, CDT come from find_package in dependencies.cmake.

# nlohmann_json is header-only and used by internal JSON serialization.
if(TARGET nlohmann_json::nlohmann_json)
    target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)
endif()

# Eigen3 is header-only; INTERFACE propagation provides include directories.
if(TARGET Eigen3::Eigen)
    target_link_libraries(${PROJECT_NAME} PRIVATE Eigen3::Eigen)
endif()

# GLM is header-only; INTERFACE propagation provides include directories.
if(TARGET glm::glm)
    target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)
endif()

# Boost is header-only (signals2, math, uuid, ublas, algorithm).
# Conan provides Boost::boost which transitively pulls Boost::headers.
if(TARGET Boost::boost)
    target_link_libraries(${PROJECT_NAME} PRIVATE Boost::boost)
endif()

if(TARGET GLEW::GLEW)
    target_link_libraries(${PROJECT_NAME} PRIVATE GLEW::GLEW)
endif()
if(TARGET OpenGL::GL)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL OpenGL::GLU)
endif()
if(TARGET Freetype::Freetype)
    target_link_libraries(${PROJECT_NAME} PRIVATE Freetype::Freetype)
endif()
if(TARGET ZLIB::ZLIB)
    target_link_libraries(${PROJECT_NAME} PRIVATE ZLIB::ZLIB)
endif()

# pugixml: replaces Xerces-C (MIT license, GPLv2-compatible)
if(TARGET pugixml::pugixml)
    target_link_libraries(${PROJECT_NAME} PRIVATE pugixml::pugixml)
endif()

# minizip-ng: replaces zipios++ (zlib license)
if(TARGET MINIZIP::minizip)
    target_link_libraries(${PROJECT_NAME} PRIVATE MINIZIP::minizip)
endif()

# CDT: replaces CGAL for constrained Delaunay triangulation (MPL-2.0)
if(TARGET CDT::CDT)
    target_link_libraries(${PROJECT_NAME} PRIVATE CDT::CDT)
endif()

# Windows system libraries
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE Dwrite.lib crypt32.lib)
endif()

# 添加依赖项(lib项)
# muparser: consumed through Conan imported target (step 7)
if(TARGET muparser::muparser)
    target_link_libraries(${PROJECT_NAME} PRIVATE muparser::muparser)
else()
    message(FATAL_ERROR "muparser::muparser target not found. "
        "Run 'conan install' with --build=never before configuring CMake.")
endif()
# SARibbonBar: imported target from find_package (step 8)
target_link_libraries(${PROJECT_NAME} PRIVATE SARibbonBar::SARibbonBar)

if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
    set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_DEFINITIONS_DEBUG "_CONSOLE")
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:WINDOWS")
    set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "/SUBSYSTEM:WINDOWS")
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS")
endif()

#add QT one by one
foreach(qtmod ${YICAD_USE_QT_MODULES})
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::${qtmod})
endforeach(qtmod)

#copy all third party files to target folder
if(WIN32)

    #copy all qt related files
    windeployqt(${PROJECT_NAME})

    # DLL deployment from Conan packages.
    # Conan-generated DLLs are found via the imported targets
    # and deployed by the build system automatically for MSVC generators.
endif()

set(_yicad_install_guard_code [=[
set(_yicad_runtime_output_dir "@YICAD_OUTPUT_DIRECTORY@")
set(_yicad_install_bindir "@CMAKE_INSTALL_BINDIR@")
if(NOT IS_ABSOLUTE "${_yicad_install_bindir}")
    set(_yicad_install_bindir "${CMAKE_INSTALL_PREFIX}/${_yicad_install_bindir}")
endif()

cmake_path(ABSOLUTE_PATH _yicad_runtime_output_dir
           NORMALIZE
           OUTPUT_VARIABLE _yicad_runtime_output_dir_abs)
cmake_path(ABSOLUTE_PATH _yicad_install_bindir
           NORMALIZE
           OUTPUT_VARIABLE _yicad_install_bindir_abs)

set(_yicad_runtime_output_dir_cmp "${_yicad_runtime_output_dir_abs}")
set(_yicad_install_bindir_cmp "${_yicad_install_bindir_abs}")
if(WIN32)
    string(TOLOWER "${_yicad_runtime_output_dir_cmp}" _yicad_runtime_output_dir_cmp)
    string(TOLOWER "${_yicad_install_bindir_cmp}" _yicad_install_bindir_cmp)
endif()
string(REGEX REPLACE "/+$" "" _yicad_runtime_output_dir_cmp "${_yicad_runtime_output_dir_cmp}")
string(REGEX REPLACE "/+$" "" _yicad_install_bindir_cmp "${_yicad_install_bindir_cmp}")

if(NOT _yicad_install_bindir_cmp STREQUAL _yicad_runtime_output_dir_cmp)
    string(FIND "${_yicad_install_bindir_cmp}/"
                "${_yicad_runtime_output_dir_cmp}/"
                _yicad_nested_install_bindir)
    if(_yicad_nested_install_bindir EQUAL 0)
        message(FATAL_ERROR
            "YiCAD install destination is inside the runtime output directory.\n"
            "  runtime output: ${_yicad_runtime_output_dir_abs}\n"
            "  install bindir: ${_yicad_install_bindir_abs}\n"
            "Choose a CMAKE_INSTALL_PREFIX outside the runtime output tree.")
    endif()
endif()
]=])
string(CONFIGURE "${_yicad_install_guard_code}" _yicad_install_guard_code @ONLY)
install(CODE "${_yicad_install_guard_code}"
        COMPONENT Runtime)
unset(_yicad_install_guard_code)

#install all resources
INSTALL(FILES ${QM_FILES}
        DESTINATION ${CMAKE_INSTALL_BINDIR}/resources/qm
        COMPONENT Runtime)
# User-provided resource directories: install matching files only (ignore .gitkeep, README.md).
# These directories are empty by default; users place their own PAT/LIN/SHX files here.
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/support/patterns/
        DESTINATION ${CMAKE_INSTALL_BINDIR}/resources/patterns
        COMPONENT Runtime
        FILES_MATCHING PATTERN "*.pat")
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/support/linetypes/
        DESTINATION ${CMAKE_INSTALL_BINDIR}/resources/linetypes
        COMPONENT Runtime
        FILES_MATCHING PATTERN "*.lin")
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/support/fonts/
        DESTINATION ${CMAKE_INSTALL_BINDIR}/resources/fonts
        COMPONENT Runtime
        FILES_MATCHING PATTERN "*.shx")
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/res/shaders/
        DESTINATION ${CMAKE_INSTALL_BINDIR}/resources/shaders
        COMPONENT Runtime
        FILES_MATCHING PATTERN "*.*")

#install config file
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/support/config/
        DESTINATION ${CMAKE_INSTALL_BINDIR}/
        COMPONENT Runtime
        FILES_MATCHING PATTERN "*.*")

# ai
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../README.md
        DESTINATION ${CMAKE_INSTALL_BINDIR}/ai/
        COMPONENT Runtime)
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/support/ai/
        DESTINATION ${CMAKE_INSTALL_BINDIR}/ai/
        COMPONENT Runtime
        FILES_MATCHING PATTERN "*.*")

# Install the deployed runtime bundle produced by the build. On Windows this
# includes the files copied by windeployqt.
INSTALL(DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT Runtime
        PATTERN "test_*" EXCLUDE)

#structure files
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src/ui" PREFIX "UIs" FILES ${UI_UIS_HEADERS})
GroupSourcesByFolder(${PROJECT_NAME})
