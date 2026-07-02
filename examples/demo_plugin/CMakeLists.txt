cmake_minimum_required(VERSION 3.21)

project(YiCadDemoPlugin VERSION 1.0.0 LANGUAGES CXX)

find_package(YiCADPluginSdk CONFIG REQUIRED)

add_library(YiCadDemoPlugin MODULE
    DemoPlugin.cpp)

if(WIN32)
    target_sources(YiCadDemoPlugin PRIVATE
        YiCadDemoPlugin.def)
endif()

target_link_libraries(YiCadDemoPlugin PRIVATE
    YiCAD::PluginSdk)

if(MSVC)
    target_compile_options(YiCadDemoPlugin PRIVATE /utf-8)
endif()

set_target_properties(YiCadDemoPlugin PROPERTIES
    PREFIX ""
    OUTPUT_NAME "YiCadDemoPlugin")
