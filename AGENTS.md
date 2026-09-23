# Repository Guidelines

## Project Structure & Module Organization

YiCAD is a Windows-first C++23/Qt 5.15 CAD application built with CMake and Conan. The application target lives under `YiCAD/`. Source code is organized by subsystem: `YiCAD/src/actions/` for user interaction actions, `YiCAD/src/cmd/` for command parsing, `YiCAD/src/kernel/` for data model, geometry, rendering, persistence, history, and GUI abstractions, `YiCAD/src/ui/` for Qt widgets and `.ui` forms, `YiCAD/src/extensions/` for in-process extensions (one self-contained subdirectory per extension, such as `ai/` for the assistant; the extension framework lives in `YiCAD/src/kernel/extension/`), and `YiCAD/src/main/` for application entry points. Runtime assets are in `YiCAD/res/`, translations in `YiCAD/ts/`, support files in `YiCAD/support/`, build helpers in `cmake/`, Conan profiles in `profiles/`, and license texts in `licenses/`. External dependency sources and build outputs belong in `external/` and `build/` and should not be committed.

## Build, Test, and Development Commands

Install Qt 5.15 and set `Qt5_DIR`, then build SARibbonBar and CDT into the install paths expected by `CMakePresets.json`.

```powershell
conan install . --output-folder=build/conan-release --profile=profiles/windows-msvc-release --build=never --lockfile=conan.lock
cmake --preset Release "-DCMAKE_PREFIX_PATH=$env:Qt5_DIR"
cmake --build --preset Release
cmake --install build/Release --config Release
```

Use the matching `Debug` preset and `profiles/windows-msvc-debug` for debug builds. The installed runnable binary is `build/<config>/bin/YiCAD.exe`.

## Coding Style & Naming Conventions

Follow the existing C++ style: 4-space indentation, braces on their own lines for namespaces/classes/functions, Qt idioms, and concise comments only where they clarify non-obvious behavior. Write new or updated comments in Chinese and use Doxygen-style documentation comments, for example `/// @brief 功能说明`; add tags such as `@param` and `@return` when they provide useful interface information. Keep source files encoded as UTF-8; the build passes `/utf-8` on MSVC.

Name classes, structs, enums, and other user-defined types in PascalCase with the subsystem prefix attached directly to the semantic name, as in `DmArc`. Use `Dm*` for data-model and geometry types, `Action*` for interaction commands, `UI*` for widgets and dialogs, `GL*` for OpenGL helpers, `Meta*` for serialization metadata, and `Filter*` for file formats. The suffix should be a clear, usually singular description of the type's responsibility, such as `Arc`, `DrawArc`, or `BlockDialog`; preserve established abbreviations and do not introduce generic, unprefixed type names where a subsystem prefix applies. Member variables follow the existing `m_` convention, including established ownership/type hints such as `m_p...` for pointers and `m_sp...` for shared pointers.

## Testing Guidelines

Unit tests live under `tests/`, use GoogleTest (provided by Conan), and run through CTest. The tree is split by subsystem, matching the target library boundaries in `doc/ARCHITECTURE_EVOLUTION_PLAN.md`: `tests/math/`, `tests/geometry/`, `tests/persistence/`. Run them with `ctest --test-dir build/<config> -C <config> --output-on-failure`; CI runs the same command. Tests are built by default and can be turned off with `-DYICAD_BUILD_TESTS=OFF`.

Add a test by dropping a `.cpp` into the matching subsystem directory and listing it in that directory's `CMakeLists.txt`. Keep one test binary per subsystem — every test target links the whole `YiCadCore` object library, so splitting further only multiplies link time. Test binaries are named `test_<subsystem>`; the install rules exclude `test_*` from the runtime package. Test executables share `tests/support/yicad_test_main.cpp`, which brings up `QApplication` and `DmSystem` (the type system registration that persistence needs) before running the suite.

Assert what the code actually does, not what a name or comment implies; when the two disagree, say so in a comment on the test. When a test uncovers a genuine defect that is out of scope to fix, keep the test with a `DISABLED_` prefix and a comment giving the file, line, mechanism, and reachability, so the defect stays visible and the test becomes the fix's acceptance check.

For changes, at minimum verify `cmake --build --preset <config>`, run `ctest`, and run the installed application.

Note that a plain build does not produce a runnable tree: shaders, `keyconfig.xml`, translations, SHX fonts and `SARibbonBar.dll` are copied by `cmake --install`, not by the build. Running `build/<config>/bin/YiCAD.exe` before installing starts the app but leaves the drawing area blank, because `GLPainterCommon` loads shaders from `<exe dir>/resources/shaders/`. Always install after building. Tests are unaffected — they do not render, and CTest puts the DLL directories on their `PATH`.

A static check also runs in CI and should be run locally before pushing:

```bash
python tools/check_layering.py               # src/kernel/ 是否反向依赖 UI 层
```

## Source File Collection

`YiCAD/CMakeLists.txt` collects sources with `yicad_collect_sources(<partition> <dir>...)`, one call per partition, using `file(GLOB ... CONFIGURE_DEPENDS)`. The partitions follow the target library boundaries in `doc/ARCHITECTURE_EVOLUTION_PLAN.md` section 6.3, so the phase 3 library split is a matter of feeding each partition's variables to its own `add_library`.

**The directory is the boundary.** Put a file in a directory and it joins that partition — there is no list to keep in sync. `CONFIGURE_DEPENDS` makes the build system re-collect when files are added or removed, so a new file cannot silently miss the build. `yicad_collect_sources` errors out on a directory that does not exist, which keeps dead entries from accumulating the way they had before.

Extensions are the exception to the one-call-per-partition rule: everything under `src/extensions/` is collected by `GLOB_RECURSE` (sources, `.ui` forms, `.qrc`, `ts/*.ts`, and `support/` install rules), so adding or removing an extension never touches CMake; only the `#include` and `Register` lines in `ApplicationWindow::registerExtensions()` name it. An extension registers its commands, Ribbon entries and settings pages through `IExtensionContext`, and every ID it registers must start with its own extension ID plus a dot (for example `ext.dim.linear`).

Everything except `src/main/Main.cpp` compiles into the `YiCadCore` OBJECT library; the `YiCAD` executable and the test binaries both link it, so tests do not recompile the kernel. `main()` stays in the executable so test binaries can supply their own.

## Logging and Profiling

Use the facade in `YiCAD/src/kernel/debug/`, not `std::cout`:

- `YICAD_LOG(category, level) << ...` (`YiCadLog.h`) — filtered by category and level; the right-hand side is not evaluated when filtered out. Default level is Warning, so nothing prints on hot paths. Configure at runtime with `YICAD_LOG=*:warning,render:debug`.
- `YICAD_SCOPED_TIMER(counter)` (`ScopedTimer.h`) — off by default, costs one relaxed atomic read when off. Uses `steady_clock`; never `system_clock`, which can run backwards. Enable with `YICAD_PROFILE=1`, summarize with `yicad::Profiler::report()`.

Never add per-frame printing to `paintGL` or other hot paths. See `doc/BASELINE.md` for how the counters feed the performance baseline.

## Commit & Pull Request Guidelines

Recent history uses short imperative subjects such as `add license` and `init`; keep commit subjects concise and focused. Pull requests should describe the user-visible change, list build/test commands run, link related issues, and include screenshots or recordings for UI changes. Note any dependency, license, resource, or packaging impact explicitly.

## Security & Configuration Tips

Do not commit local Qt paths, generated Conan files, binaries, credentials, API keys, or user-provided `*.pat`, `*.lin`, and `*.shx` resources. Keep third-party version changes synchronized across `README.md`, `conanfile.py`, `conan.lock`, CMake presets, and CI.

## Stop-and-Confirm Rules for Coding Tasks

Strictly follow these stop-and-confirm rules when performing coding tasks:

1. If the requirement is ambiguous or allows multiple reasonable interpretations, first list your understanding and the points that need confirmation, then wait for my confirmation before starting.
2. If there are multiple technical implementation paths and no clearly best option, list the core options with their pros and cons, then wait for me to choose before proceeding.
3. Before deleting files, changing core configuration, running high-risk commands, adding heavy new dependencies, or modifying more than three core files, first explain the scope of impact and wait for my confirmation before executing.
4. Do not force multiple options unnecessarily; stop only when there is a real key decision or risk.
