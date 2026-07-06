# cmake/dependencies.cmake
#
# YiCAD dependency resolution via Conan 2.
#
# Qt 5.15 is NOT managed by Conan -- it is expected to be installed separately
# by the developer and discovered via the Qt5_DIR environment variable.
# Set Qt5_DIR to the Qt installation path (e.g. C:/Qt/5.15.2/msvc2019_64).
#
# SARibbonBar is NOT managed by Conan. It is provided by the user and
# discovered via find_package with SARIBBON_DIR.
#
# CDT is NOT managed by Conan. It is built and installed separately from
# https://github.com/artem-ogre/CDT (v1.4.4, MPL-2.0) and discovered via
# find_package with CDT_DIR.
#
# Usage:
#   conan install . --output-folder=build/conan-<config> \
#     --profile=profiles/windows-msvc-<config> --build=never
#   cmake --preset <config>
#
# CMAKE_TOOLCHAIN_FILE, SARIBBON_DIR, and CDT_DIR are set in CMakePresets.json.

# ---------------------------------------------------------------------------
# Qt 5.15 (not managed by Conan)
# ---------------------------------------------------------------------------
# First try default find_package. If not found, try Qt5_DIR environment variable.
find_package(Qt5 5.15 COMPONENTS Core Widgets Gui OpenGL QUIET)
if(NOT Qt5_FOUND AND DEFINED ENV{Qt5_DIR})
    message(STATUS "Qt5 not found in default paths, trying Qt5_DIR environment variable: $ENV{Qt5_DIR}")
    find_package(Qt5 5.15 COMPONENTS Core Widgets Gui OpenGL
        HINTS $ENV{Qt5_DIR}
        NO_DEFAULT_PATH
    )
endif()
if(Qt5_FOUND)
    message(STATUS "Qt5: ${Qt5_VERSION} (${Qt5_DIR})")
    # Expose Qt5 sub-module dirs so find_package(Qt5Xxx) works even when
    # CMAKE_PREFIX_PATH has been overridden (e.g. by Conan toolchain).
    set(Qt5LinguistTools_DIR "${Qt5_DIR}/../Qt5LinguistTools" CACHE PATH "" FORCE)
else()
    message(FATAL_ERROR
        "Qt5 not found. Install Qt 5.15 and either:\n"
        "  1. Add it to CMAKE_PREFIX_PATH, or\n"
        "  2. Set the Qt5_DIR environment variable (e.g. C:/Qt/5.15.2/msvc2019_64)")
endif()

# ---------------------------------------------------------------------------
# find_package for all Conan-managed dependencies
# ---------------------------------------------------------------------------

# -- nlohmann_json (header-only) --
find_package(nlohmann_json 3.11 REQUIRED CONFIG)
message(STATUS "  nlohmann_json: ${nlohmann_json_VERSION}")

# -- Eigen (header-only) --
find_package(Eigen3 3.4 REQUIRED CONFIG)
message(STATUS "  Eigen3: ${Eigen3_VERSION}")

# -- GLM (header-only) --
find_package(glm 1.0 REQUIRED CONFIG)
message(STATUS "  glm: ${glm_VERSION}")

# -- Boost (header-only) --
find_package(Boost 1.90 REQUIRED CONFIG)
message(STATUS "  Boost: ${Boost_VERSION}")

# -- GLEW --
find_package(GLEW 2.2 REQUIRED CONFIG)
message(STATUS "  GLEW: ${GLEW_VERSION}")

# -- FreeType --
# Note: Conan freetype CMake config reports libtool version (e.g. 26.2.20),
# not the freetype release version (2.13.x). Omit version check here;
# the lockfile pins the exact recipe revision.
find_package(freetype REQUIRED CONFIG)
message(STATUS "  freetype: ${freetype_VERSION}")

# -- OpenGL (system) --
find_package(OpenGL REQUIRED)
message(STATUS "  OpenGL found")

# -- zlib --
find_package(ZLIB 1.3 REQUIRED CONFIG)
message(STATUS "  ZLIB: ${ZLIB_VERSION}")

# -- muparser --
find_package(muparser 2.3 REQUIRED CONFIG)
message(STATUS "  muparser: ${muparser_VERSION}")

# -- libdxfrw --
# 仅为后续 DXF 插件导入依赖，此处不实现 DXF 过滤器。
find_package(libdxfrw 2.2 REQUIRED CONFIG)
if(NOT TARGET libdxfrw::libdxfrw)
    message(FATAL_ERROR
        "libdxfrw imported target (libdxfrw::libdxfrw) not found.")
endif()
message(STATUS "  libdxfrw: ${libdxfrw_VERSION}")

# -- pugixml (replaces Xerces-C, MIT license) --
find_package(pugixml 1.14 REQUIRED CONFIG)
message(STATUS "  pugixml: ${pugixml_VERSION}")

# -- minizip-ng (replaces zipios++, zlib license) --
find_package(minizip REQUIRED CONFIG)
message(STATUS "  minizip-ng: ${minizip_VERSION}")

# -- SARibbonBar (MIT license) --
# Built/imported separately. User provides SARIBBON_DIR or adds the install
# prefix to CMAKE_PREFIX_PATH.
find_package(SARibbonBar REQUIRED MODULE)
if(NOT TARGET SARibbonBar::SARibbonBar)
    message(FATAL_ERROR
        "SARibbonBar imported target (SARibbonBar::SARibbonBar) not found. "
        "Set -DSARIBBON_DIR=<path> to the SARibbonBar install prefix.")
endif()

# -- CDT (Constrained Delaunay Triangulation, from GitHub upstream
#     artem-ogre/CDT v1.4.4, MPL-2.0 license) --
# Built and installed separately from external/CDT/CDT. User provides
# CDT_DIR or adds the install prefix to CMAKE_PREFIX_PATH.
# See README for build instructions.
find_package(CDT REQUIRED CONFIG)
if(NOT TARGET CDT::CDT)
    message(FATAL_ERROR
        "CDT imported target (CDT::CDT) not found. "
        "Set -DCDT_DIR=<path> to the CDT cmake config directory.")
endif()
message(STATUS "  CDT: found (MPL-2.0)")

message(STATUS "All dependencies resolved successfully.")
