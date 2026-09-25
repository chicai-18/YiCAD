# cmake/dependencies.cmake
#
# YiCAD dependency resolution via Conan 2.
#
# Qt 6.8 is NOT managed by Conan -- it is expected to be installed separately
# by the developer and discovered via the Qt6_DIR environment variable.
# Set Qt6_DIR to the Qt installation path (e.g. C:/Qt/6.8.0/msvc2022_64).
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
# Qt 6.8 (not managed by Conan)
# ---------------------------------------------------------------------------
# First try default find_package. If not found, try Qt6_DIR environment variable.
# Qt6Config.cmake locates its components (Qt6Widgets, Qt6LinguistTools, ...)
# relative to its own directory, so later find_package(Qt6 COMPONENTS ...)
# calls work even when CMAKE_PREFIX_PATH is overridden by the Conan toolchain.
#
# Imported targets are directory-scoped: find every component used anywhere in
# the tree here, at the top level, so that YiCAD/, tests/ and plugins/ all see
# Qt6::Widgets and friends.
set(YICAD_QT6_COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets Xml Svg Network LinguistTools)
find_package(Qt6 6.8 COMPONENTS ${YICAD_QT6_COMPONENTS} QUIET)
if(NOT Qt6_FOUND AND DEFINED ENV{Qt6_DIR})
    message(STATUS "Qt6 not found in default paths, trying Qt6_DIR environment variable: $ENV{Qt6_DIR}")
    find_package(Qt6 6.8 COMPONENTS ${YICAD_QT6_COMPONENTS}
        HINTS $ENV{Qt6_DIR}
        NO_DEFAULT_PATH
    )
endif()
if(Qt6_FOUND)
    message(STATUS "Qt6: ${Qt6_VERSION} (${Qt6_DIR})")
else()
    message(FATAL_ERROR
        "Qt6 not found. Install Qt 6.8 and either:\n"
        "  1. Add it to CMAKE_PREFIX_PATH, or\n"
        "  2. Set the Qt6_DIR environment variable (e.g. C:/Qt/6.8.0/msvc2022_64)")
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
