# cmake/conan_helpers.cmake
#
# Utility functions for the YiCAD Conan 2 workflow.
# These are convenience wrappers -- they do NOT replace the need to run
# `conan install` before CMake configure.

# ---------------------------------------------------------------------------
# conan_check_install()
#
# Verifies that Conan-generated files exist in the expected location.
# ---------------------------------------------------------------------------
function(conan_check_install)
    set(_conan_toolchain "${CMAKE_TOOLCHAIN_FILE}")
    if(NOT _conan_toolchain)
        message(WARNING
            "CMAKE_TOOLCHAIN_FILE is not set.\n"
            "  Expected: cmake -S . -B build \\\n"
            "    -DCMAKE_TOOLCHAIN_FILE=<build_dir>/conan_toolchain.cmake")
    else()
        if(NOT EXISTS "${_conan_toolchain}")
            message(FATAL_ERROR
                "Conan toolchain file not found: ${_conan_toolchain}\n"
                "  Did you run `conan install` first?\n"
                "  Expected command:\n"
                "    conan install . \\\n"
                "      --output-folder=build/conan-<config> \\\n"
                "      --profile=profiles/windows-msvc-<config> \\\n"
                "      --build=never")
        endif()
        message(STATUS "Conan toolchain found: ${_conan_toolchain}")
    endif()
endfunction()

# ---------------------------------------------------------------------------
# conan_print_summary()
#
# Prints a summary of discovered Conan packages.
# ---------------------------------------------------------------------------
function(conan_print_summary)
    message(STATUS "")
    message(STATUS "=== YiCAD Dependency Summary ===")

    set(_packages
        "nlohmann_json;${nlohmann_json_VERSION}"
        "Eigen3;${Eigen3_VERSION}"
        "glm;${glm_VERSION}"
        "Boost;${Boost_VERSION}"
        "GLEW;${GLEW_VERSION}"
        "freetype;${freetype_VERSION}"
        "ZLIB;${ZLIB_VERSION}"
        "muparser;${muparser_VERSION}"
        "libdxfrw;${libdxfrw_VERSION}"
    )

    foreach(_pair IN LISTS _packages)
        string(REPLACE ";" ";" _parts "${_pair}")
        list(GET _parts 0 _name)
        list(GET _parts 1 _ver)
        if(_ver)
            message(STATUS "  ${_name}: ${_ver}")
        else()
            message(STATUS "  ${_name}: (not found)")
        endif()
    endforeach()

    message(STATUS "=================================")
    message(STATUS "")
endfunction()
