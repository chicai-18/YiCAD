cmake_minimum_required(VERSION 3.21)

set(_yicad_libdxfrw_sources
    src/drw_base.cpp
    src/drw_classes.cpp
    src/drw_entities.cpp
    src/drw_header.cpp
    src/drw_objects.cpp
    src/libdwgr.cpp
    src/libdxfrw.cpp
    src/intern/drw_dbg.cpp
    src/intern/drw_textcodec.cpp
    src/intern/dwgbuffer.cpp
    src/intern/dwgreader.cpp
    src/intern/dwgreader15.cpp
    src/intern/dwgreader18.cpp
    src/intern/dwgreader21.cpp
    src/intern/dwgreader24.cpp
    src/intern/dwgreader27.cpp
    src/intern/dwgutil.cpp
    src/intern/dxfreader.cpp
    src/intern/dxfwriter.cpp
    src/intern/rscodec.cpp)

# 静态库，只链接进 YiCadDxfPlugin.dll，不单独部署，也不安装。
add_library(YiCadDxfLibdxfrw STATIC ${_yicad_libdxfrw_sources})
add_library(YiCAD::DxfLibdxfrw ALIAS YiCadDxfLibdxfrw)

target_compile_features(YiCadDxfLibdxfrw PUBLIC cxx_std_23)
target_include_directories(YiCadDxfLibdxfrw PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>")

if(MSVC)
    target_compile_options(YiCadDxfLibdxfrw PRIVATE /utf-8)
endif()

# 要链接进插件这个共享模块，非 Windows 平台需要位置无关代码。
set_target_properties(YiCadDxfLibdxfrw PROPERTIES
    OUTPUT_NAME "YiCadLibdxfrw220"
    POSITION_INDEPENDENT_CODE ON)
