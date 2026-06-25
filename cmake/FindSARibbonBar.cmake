# FindSARibbonBar.cmake
# ---------------------
# Find the SARibbonBar library (from https://github.com/czyt1988/SARibbon).
#
# This module defines:
#   SARibbonBar_FOUND        - True if SARibbonBar was found
#   SARibbonBar::SARibbonBar - Imported target (shared library)
#
# User hints:
#   SARIBBON_DIR             - Root of the SARibbonBar installation
#   CMAKE_PREFIX_PATH        - Standard CMake search path
#
# The expected installed layout is:
#   <prefix>/include/SARibbon/SARibbonBar.h     (upstream install layout)
#   <prefix>/include/SARibbonBar/SARibbonBar.h  (or <prefix>/include/SARibbonBar.h)
#   <prefix>/lib/SARibbonBar.lib                (Windows release)
#   <prefix>/lib/SARibbonBard.lib               (Windows debug)
#   <prefix>/bin/SARibbonBar.dll                (Windows release)
#   <prefix>/bin/SARibbonBard.dll               (Windows debug)

if(TARGET SARibbonBar::SARibbonBar)
    return()
endif()

# -- Header --
find_path(SARibbonBar_INCLUDE_DIR
    NAMES SARibbonBar.h
    HINTS ${SARIBBON_DIR}
    PATH_SUFFIXES include/SARibbon include/SARibbonBar include
)

# -- Library --
# On Windows the debug build uses the "d" suffix (CMAKE_DEBUG_POSTFIX).
# The upstream cmake install rule puts libraries in bin/<config>/ on Windows.
# Try to find both release and debug variants independently.
find_library(SARibbonBar_LIBRARY_RELEASE
    NAMES SARibbonBar
    HINTS ${SARIBBON_DIR}
    PATH_SUFFIXES lib lib/Release bin bin/Release
)

find_library(SARibbonBar_LIBRARY_DEBUG
    NAMES SARibbonBard SARibbonBar
    HINTS ${SARIBBON_DIR}
    PATH_SUFFIXES lib lib/Debug bin bin/Debug
)

# Accept either release or debug library — a debug-only install is valid.
set(_SARibbonBar_LIB "${SARibbonBar_LIBRARY_RELEASE}")
if(NOT _SARibbonBar_LIB)
    set(_SARibbonBar_LIB "${SARibbonBar_LIBRARY_DEBUG}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SARibbonBar
    REQUIRED_VARS SARibbonBar_INCLUDE_DIR _SARibbonBar_LIB
)

if(SARibbonBar_FOUND)
    if(NOT TARGET SARibbonBar::SARibbonBar)
        add_library(SARibbonBar::SARibbonBar SHARED IMPORTED)
        set_target_properties(SARibbonBar::SARibbonBar PROPERTIES
            IMPORTED_LOCATION "${_SARibbonBar_LIB}"
            INTERFACE_INCLUDE_DIRECTORIES "${SARibbonBar_INCLUDE_DIR}"
        )
        if(SARibbonBar_LIBRARY_DEBUG AND SARibbonBar_LIBRARY_RELEASE)
            set_target_properties(SARibbonBar::SARibbonBar PROPERTIES
                IMPORTED_LOCATION_DEBUG "${SARibbonBar_LIBRARY_DEBUG}"
            )
        endif()
        if(WIN32)
            # Import lib for DLL linking
            set_target_properties(SARibbonBar::SARibbonBar PROPERTIES
                IMPORTED_IMPLIB "${_SARibbonBar_LIB}"
            )
            if(SARibbonBar_LIBRARY_DEBUG AND SARibbonBar_LIBRARY_RELEASE)
                set_target_properties(SARibbonBar::SARibbonBar PROPERTIES
                    IMPORTED_IMPLIB_DEBUG "${SARibbonBar_LIBRARY_DEBUG}"
                )
            endif()
        endif()
    endif()

    # Find DLL for installation
    find_file(SARibbonBar_DLL_RELEASE
        NAMES SARibbonBar.dll
        HINTS ${SARIBBON_DIR}
        PATH_SUFFIXES bin bin/Release
    )
    find_file(SARibbonBar_DLL_DEBUG
        NAMES SARibbonBard.dll
        HINTS ${SARIBBON_DIR}
        PATH_SUFFIXES bin bin/Debug
    )

    message(STATUS "SARibbonBar: ${SARibbonBar_LIBRARY_RELEASE}")
    message(STATUS "SARibbonBar include: ${SARibbonBar_INCLUDE_DIR}")
endif()

mark_as_advanced(SARibbonBar_INCLUDE_DIR SARibbonBar_LIBRARY_RELEASE SARibbonBar_LIBRARY_DEBUG)
