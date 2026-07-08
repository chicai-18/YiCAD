cmake_minimum_required(VERSION 3.21)

project(YiCadDxfPlugin VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)

if(NOT TARGET YiCAD::PluginSdk)
    find_package(YiCADPluginSdk CONFIG REQUIRED)
endif()
if(NOT TARGET libdxfrw::libdxfrw)
    find_package(libdxfrw 2.2 CONFIG REQUIRED)
endif()

add_library(YiCadDxfPlugin MODULE
    src/DxfPlugin.cpp
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
    libdxfrw::libdxfrw)

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
