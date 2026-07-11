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

add_library(YiCadDxfLibdxfrw SHARED ${_yicad_libdxfrw_sources})
add_library(YiCAD::DxfLibdxfrw ALIAS YiCadDxfLibdxfrw)

target_compile_features(YiCadDxfLibdxfrw PUBLIC cxx_std_23)
target_compile_definitions(YiCadDxfLibdxfrw PRIVATE YICAD_LIBDXFRW_BUILD)
target_include_directories(YiCadDxfLibdxfrw PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>")

if(MSVC)
    target_compile_options(YiCadDxfLibdxfrw PRIVATE /utf-8)
endif()

set_target_properties(YiCadDxfLibdxfrw PROPERTIES
    OUTPUT_NAME "YiCadLibdxfrw220")
