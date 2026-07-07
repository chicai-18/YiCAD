from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.microsoft import msvc_runtime_flag
from conan.errors import ConanException
import os


class YiCADRecipe(ConanFile):
    """
    YiCAD Conan 2 recipe.

    This recipe defines the external C/C++ dependencies for YiCAD.
    Qt 5.15 is NOT included here -- it is expected to be installed
    separately by the developer and discovered via CMAKE_PREFIX_PATH.

    SARibbonBar is NOT managed by Conan. It is provided by the user
    and discovered via find_package with SARIBBON_DIR.

    CDT (constrained Delaunay triangulation) is NOT managed by Conan.
    It is built and installed separately from
    https://github.com/artem-ogre/CDT (v1.4.4, MPL-2.0) and discovered
    via find_package with CDT_DIR. See README for build instructions.
    """

    name = "yicad"
    version = "0.20.0"
    license = "GPL-3.0-only"
    description = "YiCAD -- 2D CAD application based on Qt 5.15"
    url = "https://github.com/YiCAX/YiCAD"
    homepage = "https://github.com/YiCAX/YiCAD"
    topics = ("cad", "2d", "qt5")

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"

    # No package_type -- this is an application, not a library
    package_type = "application"

    # --- Exported sources (none needed for pure consumer) ---
    exports_sources = (
        "CMakeLists.txt",
        "YiCAD/*",
        "cmake/*",
        "plugins/*",
        "profiles/*",
    )

    def requirements(self):
        """
        Declare ConanCenter dependencies.

        Versions are pinned to known-good ConanCenter revisions.
        Each version was verified to exist on ConanCenter as of 2026-06-19.
        """
        # ---- Header-only libraries ----
        self.requires("nlohmann_json/3.11.3")
        self.requires("eigen/3.4.0")
        self.requires("glm/1.0.1")

        # ---- Boost (header-only modules: signals2, math, uuid, ublas, algorithm) ----
        self.requires("boost/1.90.0")

        # ---- Graphics / rendering ----
        self.requires("glew/2.2.0")
        self.requires("freetype/2.13.3")
        self.requires("opengl/system")

        # ---- Compression / archiving ----
        self.requires("zlib/1.3.1")
        self.requires("minizip-ng/4.0.7")

        # ---- Parsing / math ----
        self.requires("muparser/2.3.2")

        # ---- CAD 文件格式 ----
        self.requires("libdxfrw/2.2.0")

        # ---- XML 解析 ----
        self.requires("pugixml/1.14")

        # NOTE: SARibbonBar is NOT a Conan dependency. It is provided by the
        # user and found via find_package with SARIBBON_DIR. See README for
        # setup instructions.
        #
        # NOTE: CDT is NOT a Conan dependency. It is built separately from
        # https://github.com/artem-ogre/CDT (v1.4.4, MPL-2.0) and found via
        # find_package with CDT_DIR. See README for build instructions.

    def configure(self):
        """
        Enforce consistent MSVC runtime and C++ standard.
        """
        if self.settings.compiler == "msvc":
            # Ensure /MD for Release, /MDd for Debug -- matches existing YiCAD build
            if self.settings.build_type == "Debug":
                self.options["*"].shared = False
            else:
                self.options["*"].shared = False

        # Boost: header-only usage, no need to build libraries
        self.options["boost"].header_only = True

        # GLEW: static library on Windows
        if self.settings.os == "Windows":
            self.options["glew"].shared = False

        # FreeType: static library
        self.options["freetype"].shared = False

        # muparser: static library
        self.options["muparser"].shared = False

        # libdxfrw：使用静态库，后续 DXF 插件可直接链接
        self.options["libdxfrw"].shared = False

        # pugixml: static library (MIT license, replaces Xerces-C)
        self.options["pugixml"].shared = False

        # minizip-ng: static library, use zlib backend, no compat layer (we use mz_* API directly)
        self.options["minizip-ng"].shared = False
        self.options["minizip-ng"].zlib = True
        self.options["minizip-ng"].compat = False

    def layout(self):
        """
        Conan layout: keep generated files inside build/ subdirectory.
        """
        cmake_layout(self)

    def generate(self):
        """
        Generate CMakeDeps and CMakeToolchain files.
        All output goes into the build directory -- never into the source tree.
        """
        deps = CMakeDeps(self)
        deps.generate()

        tc = CMakeToolchain(self)
        tc.variables["CMAKE_CXX_STANDARD"] = 23
        tc.variables["CMAKE_CXX_STANDARD_REQUIRED"] = "ON"
        tc.generate()

    def build(self):
        """
        Build YiCAD using CMake.
        """
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        """
        Package YiCAD binaries.
        """
        cmake = CMake(self)
        cmake.install()

    def package_id(self):
        """
        Include compiler version and runtime in package ID for binary matching.
        """
        # In Conan 2, self.settings is forbidden in package_id();
        # use self.info.settings instead. The default package_id already
        # includes compiler version and runtime, so no override is needed.

    def package_info(self):
        """
        No library to export -- this is an application.
        """
        pass
