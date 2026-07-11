cmake_minimum_required(VERSION 3.21)

project(YiCadDxfPlugin VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)

if(NOT TARGET YiCAD::PluginSdk)
    find_package(YiCADPluginSdk CONFIG REQUIRED)
endif()
add_subdirectory(third_party/libdxfrw)

add_library(YiCadDxfPlugin MODULE
    src/DxfPlugin.cpp
    src/DxfExporter.cpp
    src/DxfExporter.h
    src/DxfImporter.cpp
    src/DxfImporter.h
    src/DxfInterfaceAdapter.h
    src/DxfMapping.cpp
    src/DxfMapping.h)

if(WIN32)
    target_sources(YiCadDxfPlugin PRIVATE
        YiCadDxfPlugin.def)
endif()

target_link_libraries(YiCadDxfPlugin PRIVATE
    YiCAD::PluginSdk
    YiCAD::DxfLibdxfrw)

if(MSVC)
    target_compile_options(YiCadDxfPlugin PRIVATE /utf-8)
endif()

set_target_properties(YiCadDxfPlugin PROPERTIES
    PREFIX ""
    OUTPUT_NAME "YiCadDxfPlugin")

install(TARGETS YiCadDxfPlugin
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}/plugins/dxf"
        LIBRARY DESTINATION "${CMAKE_INSTALL_BINDIR}/plugins/dxf"
        COMPONENT Runtime)
install(FILES dxf.xml.example
        DESTINATION "${CMAKE_INSTALL_BINDIR}/plugins"
        RENAME dxf.xml
        COMPONENT Runtime)
