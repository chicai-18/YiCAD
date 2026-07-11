English | [中文版](README_zh.md)

# YiCAD

YiCAD is an open-source 2D CAD application that provides core features found in mainstream CAD software. Built with **C++23** and **Qt 5.15**, it uses **OpenGL** for high-performance graphics rendering.

![YiCAD UI](screenshot.png)

Current version: **v0.20.0**

## Features

- **2D Drawing Entities**: Lines, arcs, circles, ellipses, polylines, splines, hatches, solids
- **Block System**: Block definitions, block references, block attributes, nested blocks
- **Dimensioning**: Linear, aligned, angular, radius/diameter dimensions
- **Text System**: Single-line text, multi-line text (MTEXT)
- **Layer Management**: Create, lock, freeze, and control layer visibility
- **Linetype / Color / Line Weight**: Rich entity property settings
- **Image Support**: Insert and manage raster images
- **Snap System**: Endpoint, midpoint, center, intersection, perpendicular, and more
- **Trim / Extend**: Trim and Extend operations
- **Undo / Redo**: Full undo/redo framework based on a command stack
- **Office Ribbon UI**: Modern ribbon interface powered by SARibbonBar
- **Command Line Input**: Quick command-line operations

## Build

### Build Overview

The complete build steps **must be executed in order**:

```
① Install Qt 5.15         ← Manual install, set Qt5_DIR env variable
② Clone external deps     ← git clone SARibbonBar and CDT sources
③ Build & install SARibbonBar ← cmake build, install to external/SARibbonBar/install-*/
④ Build & install CDT        ← cmake build, install to external/CDT/install-*/
⑤ Install Conan deps      ← conan install (auto-downloads Boost, Eigen, GLEW, etc.)
⑥ CMake configure & build ← cmake --preset + cmake --build
⑦ Install                 ← cmake --install (auto-collects all DLLs)
```

> **Dependency notes:**
> - Steps ②③④ all depend on ① (SARibbonBar compilation requires Qt headers)
> - Step ⑤ is independent of ②③④ and can run in parallel
> - Step ⑥ requires ①③④⑤ all ready (CMake needs Qt, SARibbonBar, CDT, and Conan toolchain)

### Dependencies

- **CMake** 3.21+
- **Visual Studio 2022** (Windows)
- **Conan 2** — C/C++ package manager for most third-party dependencies
- **Qt 5.15** — Must be installed by the developer (see below)
- **SARibbonBar** — Must be built and installed separately (see below)
- **CDT** — Must be built and installed separately (see below)

### ① Install Qt 5.15

Qt 5.15 must be installed by the developer:

1. Install Qt 5.15.x from [Qt official](https://www.qt.io/download-qt-installer-oss) or an open-source mirror. Select the **MSVC 2019 64-bit** component.
2. Set the system environment variable `Qt5_DIR` to point to the Qt installation path:

```powershell
# Set permanently (requires terminal/IDE restart)
[System.Environment]::SetEnvironmentVariable("Qt5_DIR", "C:\Qt\5.15.2\msvc2019_64", "User")
```

**Qt5 lookup order:**
1. CMake default paths (`CMAKE_PREFIX_PATH`, system PATH, etc.)
2. System environment variable `Qt5_DIR`

If Qt is installed in a standard location (e.g. `C:\Qt\5.15.2\msvc2019_64`), setting the environment variable is sufficient.

### ② Clone External Dependencies

The `external/` directory holds third-party library sources. It is empty by default (only `.gitkeep`); you need to clone the dependencies manually:

```powershell
# Clone SARibbonBar
git clone https://github.com/czyt1988/SARibbon.git external/SARibbonBar
git -C external/SARibbonBar checkout b5d3818

# Clone CDT
git clone --branch 1.4.4 --depth 1 https://github.com/artem-ogre/CDT.git external/CDT
```

### ③ Build & Install SARibbonBar

YiCAD uses [SARibbonBar b5d3818](https://github.com/czyt1988/SARibbon) (MIT License) for the Office-style Ribbon UI.
SARibbonBar is not distributed with YiCAD source, and no modifications to SARibbonBar are required. Use a user-built, installed, or imported SARibbonBar, and specify its install prefix via `SARIBBON_DIR` in YiCAD's CMake configuration.

```powershell
# 1. Clone the upstream repo (if not already cloned)
git clone https://github.com/czyt1988/SARibbon.git external/SARibbonBar
git -C external/SARibbonBar checkout b5d3818

# 2. Build and install (Release + Debug)
cmake -G "Visual Studio 17 2022" -A x64 -S external/SARibbonBar -B external/SARibbonBar/build `
  -DCMAKE_PREFIX_PATH="$env:Qt5_DIR" `
  -DSARIBBON_INSTALL_IN_CURRENT_DIR=OFF
cmake --build external/SARibbonBar/build --config Release
cmake --build external/SARibbonBar/build --config Debug
cmake --install external/SARibbonBar/build --config Release --prefix external/SARibbonBar/install-release
cmake --install external/SARibbonBar/build --config Debug --prefix external/SARibbonBar/install-debug
```

After building, specify the install path in YiCAD's CMake configuration via `SARIBBON_DIR`:

```powershell
# Release build
"-DSARIBBON_DIR=external/SARibbonBar/install-release"

# Debug build
"-DSARIBBON_DIR=external/SARibbonBar/install-debug"
```

`SARIBBON_DIR` should point to the install prefix containing `include`, `lib`, `bin` subdirectories. YiCAD's lookup logic supports common header layouts such as `include/SARibbon/SARibbonBar.h`, `include/SARibbonBar/SARibbonBar.h`, or `include/SARibbonBar.h`.

> YiCAD has no official affiliation with or endorsement by czyt1988/SARibbon.

### ④ Build & Install CDT

YiCAD uses [CDT](https://github.com/artem-ogre/CDT) (v1.4.4, MPL-2.0 License) for constrained Delaunay triangulation.
CDT is not distributed with YiCAD source and must be obtained, built, and installed separately:

```powershell
# 1. Clone the upstream repo (if not already cloned)
git clone --branch 1.4.4 --depth 1 https://github.com/artem-ogre/CDT.git external/CDT

# 2. Build and install (Release + Debug)
cmake -G "Visual Studio 17 2022" -A x64 -S external/CDT/CDT -B external/CDT/build `
  "-DCDT_USE_AS_COMPILED_LIBRARY=ON"
cmake --build external/CDT/build --config Release
cmake --build external/CDT/build --config Debug
cmake --install external/CDT/build --config Release --prefix external/CDT/install-release
cmake --install external/CDT/build --config Debug --prefix external/CDT/install-debug
```

After building, specify the install path in YiCAD's CMake configuration via `CDT_DIR`:

```powershell
# Release build
"-DCDT_DIR=external/CDT/install-release/cmake"

# Debug build
"-DCDT_DIR=external/CDT/install-debug/cmake"
```

> CDT installs the library as `lib/CDT.lib`. Use separate install prefixes for Release and Debug to avoid Debug libraries overwriting Release ones, which causes MSVC runtime mismatch linker errors.

### ⑤ Install Third-Party Dependencies via Conan

Most third-party dependencies are installed via [Conan 2](https://conan.io/). Install Conan 2 first, then run:

```powershell
# Install Conan 2 (if not already installed)
pip install conan

# Install Release dependencies
conan install . --output-folder=build/conan-release --profile=profiles/windows-msvc-release --build=never --lockfile=conan.lock

# Install Debug dependencies
conan install . --output-folder=build/conan-debug --profile=profiles/windows-msvc-debug --build=never --lockfile=conan.lock
```

When Conan 2 uses `cmake_layout()`, the CMake toolchain is generated at `build/conan-<config>/build/generators/conan_toolchain.cmake`.

### Other Third-Party Libraries

| Library | Purpose | Source |
|---------|---------|--------|
| [SARibbonBar b5d3818](https://github.com/czyt1988/SARibbon) | Office Ribbon UI | Built separately |
| [Boost 1.90](https://www.boost.org/) | Quartic equation solving, etc. | Conan |
| [Eigen 3.4](https://eigen.tuxfamily.org/) | Linear algebra | Conan |
| [GLM 1.0](https://github.com/g-truc/glm) | Graphics math | Conan |
| [GLEW 2.2](https://glew.sourceforge.net/) | OpenGL extension loading | Conan |
| [FreeType 2.13](https://www.freetype.org/) | Font rendering | Conan |
| [zlib 1.3](https://www.zlib.net/) | Compression | Conan |
| [minizip-ng 4.0](https://github.com/nmoinvaz/minizip) | ZIP archive | Conan |
| [muparser 2.3](https://beltoforion.de/en/muparser/) | Math expression parsing | Conan |
| [libdxfrw 2.2](https://github.com/LibreCAD/libdxfrw) | DXF parsing for the DXF plugin | Bundled source |
| [nlohmann/json 3.11](https://github.com/nlohmann/json) | JSON serialization | Conan |
| [pugixml 1.14](https://pugixml.org/) | XML parsing | Conan |

### ⑥ CMake Configure, Build & Install

Before building, ensure the following dependencies are ready:

1. **Qt 5.15** — Installed, `Qt5_DIR` environment variable set (see above)
2. **SARibbonBar** — Cloned, built, and installed to `external/SARibbonBar/install-release/` and `external/SARibbonBar/install-debug/` (see above)
3. **CDT** — Cloned, built, and installed to `external/CDT/install-release/` and `external/CDT/install-debug/` (see above)
4. **Conan dependencies** — Downloaded via `conan install` (see below)

The project provides CMake presets (`CMakePresets.json`) to simplify configuration:

```powershell
# 0. Set Qt5_DIR environment variable (if not already set)
$env:Qt5_DIR = "C:/Qt/5.15.2/msvc2019_64"

# 1. Install Conan dependencies (Release example)
conan install . --output-folder=build/conan-release --profile=profiles/windows-msvc-release --build=never --lockfile=conan.lock

# 2. Configure with CMake preset (outputs to build/Release, toolchain/SARibbon/CDT paths are baked into the preset)
cmake --preset Release "-DCMAKE_PREFIX_PATH=$env:Qt5_DIR"

# 3. Build and install the Release runtime (without plugin development files)
cmake --build --preset Release-Runtime

# Optional: install the third-party plugin SDK
cmake --build --preset Release-PluginSDK

# Build output: build/Release/bin/YiCAD.exe
# Install output: build/Release/bin/YiCAD.exe + all third-party DLLs
```

This Release flow has been verified with Windows 11, Visual Studio 2022/v143,
Qt 5.15.2, CMake 3.26.3, and Conan 2.29.1. It completed dependency resolution,
CMake configuration, compilation, Runtime installation, and a launch smoke test.

The project presets deliberately do not fix a CMake generator. CMake reuses the
generator, platform, and toolset recorded in the build directory. If
`build/Release` was previously configured by another IDE or with different
generator options, refresh only the generated CMake state before configuring:

```powershell
cmake --fresh --preset Release "-DCMAKE_PREFIX_PATH=$env:Qt5_DIR"
```

Use `--fresh` when switching generators or resolving a generator/platform/toolset
cache mismatch; it is unnecessary for normal incremental builds.

**Install notes:**

Install components:

- `Runtime`: YiCAD executable, runtime libraries, resources, translations, and runtime licenses.
- `PluginSDK`: public headers, CMake package, SDK documentation, license, and standalone Demo sources.
- The `Debug-Runtime`, `Debug-PluginSDK`, `Release-Runtime`, and `Release-PluginSDK` build presets install their corresponding components automatically.
- Omitting `--component` installs both components.

Installing `Runtime` automatically copies the following dependencies to `build/<config>/bin/`:
- SARibbonBar.dll
- CDT.dll (if present)
- Conan-managed third-party DLLs (GLEW, FreeType, zlib, etc.)

This makes the install directory self-contained — no manual DLL copying needed.

**Developing with CLion:**

1. After opening the project, CLion automatically detects `CMakePresets.json`
2. Select a preset in the CMake tool window (Debug/Release/RelWithDebInfo)
3. Build directory is automatically set to `build/<presetName>` (e.g. `build/Release`)
4. Build and install outputs are unified under `build/<presetName>/bin/`

The presets already include `CMAKE_TOOLCHAIN_FILE` (Conan toolchain), `SARIBBON_DIR`, and `CDT_DIR` — no extra arguments needed.

**Important:** Before using CLion, ensure the `Qt5_DIR` environment variable is set:
- Open Windows System Settings → Search for "Environment Variables"
- Add a user environment variable: `Qt5_DIR` = `C:\Qt\5.15.2\msvc2019_64`
- Restart CLion for the environment variable to take effect

In PowerShell, keep the quotes around `"-D...=..."` arguments, especially for paths ending in `.cmake` like `conan_toolchain.cmake`. Test programs are built in the build directory for verification, but `cmake --install` does not install `test_*` programs to `bin/`.

If MSBuild reports `MSB6001` with duplicate `PATH`/`Path` keys, start a clean
Developer PowerShell for Visual Studio 2022. To diagnose the current process:

```powershell
[System.Environment]::GetEnvironmentVariables().Keys |
  Where-Object { $_ -ieq "path" }
```

If both spellings are listed, normalize the environment before rerunning CMake.
Do not add a second PATH variable that differs only by letter case.

> **Note:** Linux support is planned for the future.

## Architecture Overview

The project uses an **MVC + Action** architecture:

| Layer | Path | Description |
|-------|------|-------------|
| **Data Model** | `YiCAD/src/kernel/data_model/` | Dm* classes — CAD entity data |
| **View** | `YiCAD/src/kernel/gui/` | QOpenGLWidget subclasses, 4-layer rendering |
| **Actions** | `YiCAD/src/actions/` | ~75 Action classes handling user interaction |
| **Commands** | `YiCAD/src/cmd/` | Command-line input parsing and dispatch |
| **Undo/Redo** | `YiCAD/src/kernel/history/` | Command stack, transactions, macro commands |
| **Math** | `YiCAD/src/kernel/math/` | Computational geometry, KD-tree, R-tree, Delaunay triangulation |
| **Rendering** | `YiCAD/src/kernel/painters/` | OpenGL drawing abstraction layer |
| **Persistence** | `YiCAD/src/kernel/persistence/` | XML serialization (pugixml) |

## Development

- **Code style**: UTF-8 with BOM encoding
- **Naming conventions**: `Dm*` (data model), `Action*` (interaction commands), `UI*` (UI components), `GL*` (OpenGL), `Meta*` (serialization), `Filter*` (file formats)

## License

YiCAD contains derivative code, translations, and resources originating from QCAD Community Edition.
Those portions retain their original copyright notices and are released under the GNU General Public License version 3 (GPLv3).
All modifications and additions made by YiCAD are also released under GPLv3.

The full GPLv3 license text is at [`licenses/gpl-3.0.txt`](licenses/gpl-3.0.txt).
See the [`LICENSE`](LICENSE) file for detailed licensing information.

### Provenance

YiCAD is based on derivative code from the following upstream project:

| Project | Homepage | License |
|---------|----------|---------|
| QCAD Community Edition 2.0.5.0 | <https://github.com/qcad/qcad> | GPLv3 |

Key modifications by YiCAD include:
- Restructured project layout and namespaces (e.g. `RS_` → `Dm` prefix changes);
- Migrated to Qt 5.15 + C++23 + OpenGL rendering architecture;
- Added Office Ribbon-style UI (based on SARibbonBar);
- Extended and modified the data model, command system, and persistence layer.

### Third-Party Components

Third-party components are subject to their own licenses. See the [`LICENSE`](LICENSE) file and [`licenses/`](licenses/) directory for details.

### Disclaimer

YiCAD has no official affiliation with, authorization from, or endorsement by the QCAD project.
