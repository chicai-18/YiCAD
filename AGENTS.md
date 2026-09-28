# Repository Guidelines

## Project Structure & Module Organization

YiCAD is a Windows-first C++23/Qt 6.8 CAD application built with CMake and Conan. The application target lives under `YiCAD/`. Source code is organized by layer (see `doc/LAYER_RESTRUCTURE_PLAN.md` section 2): `YiCAD/src/base/` for the type system, serialization machinery, logging and basic geometry (`core/`, `debug/`, `geometry/`); `YiCAD/src/model/` for the document, properties, entities and their data structures, symbol tables and the spatial index, undo/redo, editing, geometric algorithms, the GI through which every entity describes its geometry to the graphics system (`worldDraw()`, `doc/RENDER_PLAN.md` section 4.2), and the native file format with the format registry `FilterRegistry` that plugin formats also register into (`document/`, `property/`, `entity/`, `entity_data/`, `table/`, `history/`, `edit/`, `algorithm/`, `graphics/`, `io/`; the model never sees a view or host type: the document notifies views through `DmDocumentListener`, and the renderer never branches on entity types: it only consumes GI output); `YiCAD/src/render/` for the drawing abstraction, the OpenGL implementation, the drawing canvas `GuiDocumentView` and the `IDocumentView`/`ISnapService` interfaces that commands and tools see (`painter/`, `opengl/`, `view/`); `YiCAD/src/application/` for the command and view tool mechanism that extensions build on (commands, the command bus and registry, the place/select-first command bases, `ViewToolControl`, the selection layer, the snapper, command-line aliases and `keyconfig.xml` parsing, the open drawing `AppDocument` (one per document, holding the `DmDocument`, its save policy and its selection set; the drawing window `MDIWindow` holds it), the per-document selection set `SelectionSet` (the canvas reads it through `ISelectionSource` in `render/view/`), the per-view highlight set `HighlightSet` (the running command's pick feedback, reached through `highlight()` and cleared by the view when the command finishes; the canvas reads it through `IHighlightSource` in `render/view/`), the per-document save policy `DocumentFileService`, and the host service interface `GuiDialogFactoryInterface`, mirroring DS's `Application/`), with the in-process extension framework in `application/framework/` (DS's `Application/Framework/`; it also holds `IDocumentManager`, the host's open drawings as extensions and widgets see them) and the interactive view `UIView` that assembles the view tool stack on top of the canvas in `application/view/` (mirroring DS's `HQWidget`/`UIView` split); `YiCAD/src/ui/` for the Qt widgets and `.ui` forms that extensions and the shell share (they never include `shell/`; a widget that needs the current document gets the host's `IDocumentManager` injected); `YiCAD/src/extensions/` for in-process extensions (one self-contained subdirectory per extension, such as `ai/` for the assistant; every business command lives in an extension, including the former built-in ones in `draw/`, `modify/`, `measure/`, `edit/` and `view/`); and `YiCAD/src/shell/` for the main window and the parts only it uses (drawing tabs `UITabDrawWidget`, per-drawing windows, status bar, command line, the host service implementation `UIDialogFactory`, the Ribbon assembler and their forms), application entry points and the C ABI plugin runtime (`plugin_runtime/`). Runtime assets are in `YiCAD/res/`, translations in `YiCAD/ts/`, support files in `YiCAD/support/`, build helpers in `cmake/`, Conan profiles in `profiles/`, and license texts in `licenses/`. External dependency sources and build outputs belong in `external/` and `build/` and should not be committed.

## Build, Test, and Development Commands

Install Qt 6.8 and set `Qt6_DIR`, then build SARibbonBar and CDT into the install paths expected by `CMakePresets.json`.

```powershell
conan install . --output-folder=build/conan-release --profile=profiles/windows-msvc-release --build=never --lockfile=conan.lock
cmake --preset Release "-DCMAKE_PREFIX_PATH=$env:Qt6_DIR"
cmake --build --preset Release
cmake --install build/Release --config Release
```

Use the matching `Debug` preset and `profiles/windows-msvc-debug` for debug builds. The installed runnable binary is `build/<config>/bin/YiCAD.exe`.

Translations: the build compiles `.ts` into `.qm` and `cmake --install` copies them; neither ever rewrites a `.ts`. After adding or changing user-visible strings, run `cmake --build --preset Release --target update_translations` to refresh `YiCAD/ts/YiCAD_zh_cn.ts` and every `src/extensions/<name>/ts/*.ts`, then fill in the new entries. `YiCAD/ts/qtbase_zh_CN.ts` is maintained by hand and is not scanned. lupdate must be able to see the context: a class calling `tr()` needs `Q_OBJECT` or `Q_DECLARE_TR_FUNCTIONS`, and a template must not call `tr()` through a template parameter (use `QCoreApplication::translate("Context", "text")` instead).

## Coding Style & Naming Conventions

Follow the existing C++ style: 4-space indentation, braces on their own lines for namespaces/classes/functions, Qt idioms, and concise comments only where they clarify non-obvious behavior. Write new or updated comments in Chinese and use Doxygen-style documentation comments, for example `/// @brief 功能说明`; add tags such as `@param` and `@return` when they provide useful interface information. Keep source files encoded as UTF-8; the build passes `/utf-8` on MSVC.

Name classes, structs, enums, and other user-defined types in PascalCase with the subsystem prefix attached directly to the semantic name, as in `DmArc`. Use `Dm*` for data-model and geometry types, `IGi*`/`Gi*` for the GI interfaces and value types in `model/graphics/` (`IGiGeometry`, `GiTransform`), `UI*` for widgets and dialogs, `GL*` for OpenGL helpers, `Meta*` for serialization metadata, and `Filter*` for file formats. The suffix should be a clear, usually singular description of the type's responsibility, such as `Arc` or `BlockDialog`; preserve established abbreviations and do not introduce generic, unprefixed type names where a subsystem prefix applies.

The interaction layer follows the DS naming instead of a prefix: interactive commands are `XxxCommand`, view tools (placement tools and the navigation and selection layers) are `XxxTool`, and framework interfaces are `I*`, as in `DrawLineCommand`, `DrawLineTool`, `SelectTool`, and `IViewTool`; framework classes keep their DS names, such as `ViewToolControl` and `ExclusiveCommandBus`. The legacy `Action*` framework has been removed (see `doc/COMMAND_TOOL_MIGRATION_PLAN.md`); do not add new `Action*` types. Member variables follow the existing `m_` convention, including established ownership/type hints such as `m_p...` for pointers and `m_sp...` for shared pointers.

## Testing Guidelines

Unit tests live under `tests/`, use GoogleTest (provided by Conan), and run through CTest. The tree is split by subsystem, matching the target library boundaries in `doc/ARCHITECTURE_EVOLUTION_PLAN.md`: `tests/math/`, `tests/geometry/`, `tests/persistence/`, `tests/interaction/`, plus `tests/graphics/` for the GI and each entity's `worldDraw()` output (`doc/RENDER_PLAN.md` stage 2) and `tests/render/` for rendering (`doc/RENDER_PLAN.md` step 0.2). Run them with `ctest --test-dir build/<config> -C <config> --output-on-failure`; CI runs the same command. Tests are built by default and can be turned off with `-DYICAD_BUILD_TESTS=OFF`.

Add a test by dropping a `.cpp` into the matching subsystem directory and listing it in that directory's `CMakeLists.txt`. Keep one test binary per subsystem — each test target links its whole layer (`YiCadModel` for `test_math`, `test_geometry`, `test_graphics` and `test_persistence`; the `YiCadShell` OBJECT library, which brings `YiCadUi` and below, plus every `YiCadExt_*` extension library for `test_interaction`; `YiCadShell` alone for `test_render`, which reads its reference drawings through the DXF plugin runtime), so splitting further only multiplies link time. `yicad_add_test` takes the libraries in `LINK`; list OBJECT libraries explicitly, because their object files come only from direct links. Test binaries are named `test_<subsystem>`; the install rules exclude `test_*` from the runtime package. Test executables share `tests/support/yicad_test_main.cpp`, which brings up `QApplication` and `DmSystem` (the type system registration that persistence needs) before running the suite.

Assert what the code actually does, not what a name or comment implies; when the two disagree, say so in a comment on the test. When a test uncovers a genuine defect that is out of scope to fix, keep the test with a `DISABLED_` prefix and a comment giving the file, line, mechanism, and reachability, so the defect stays visible and the test becomes the fix's acceptance check.

For changes, at minimum verify `cmake --build --preset <config>`, run `ctest`, and run the installed application.

Note that a plain build does not produce a runnable tree: shaders, `keyconfig.xml`, translations, SHX fonts and `SARibbonBar.dll` are copied by `cmake --install`, not by the build. Running `build/<config>/bin/YiCAD.exe` before installing starts the app but leaves the drawing area blank, because `GLPainterCommon` loads shaders from `<exe dir>/resources/shaders/`. Always install after building. Tests are unaffected: CTest puts the DLL directories on their `PATH`, and only `test_render` renders — it reads the shaders straight from `YiCAD/res/shaders` (environment variable `YICAD_SHADER_DIR`, which `GLPainterCommon` honours) and draws with Mesa's software OpenGL.

`test_render` draws the reference drawings in `tests/render/drawings/` (generated by `python tools/gen_render_references.py`, except the AutoCAD comparison drawing `autocad_linetype.dxf`) offscreen and compares them with the committed images in `tests/render/baseline/`. Only a software rasteriser gives the same pixels on every machine, so it always uses Mesa llvmpipe, locally and in CI: run `python tools/fetch_mesa.py` once (it downloads the pinned mesa-dist-win release, checks its SHA-256 and extracts `opengl32.dll` and `libgallium_wgl.dll` to `external/mesa/x64/`; `--archive` takes an already downloaded package). `test_render` delay-loads `opengl32.dll` and loads Mesa from there before `main`; never copy Mesa into `build/<config>/bin`, because `cmake --install` packages that whole directory. Cases that need user-provided SHX fonts skip when the fonts are missing, as in CI. On a mismatch the actual and difference images are written to `build/<config>/tests/render/output/`; when a display change is intended, check the new images and rerun with `YICAD_RENDER_UPDATE_BASELINE=1` to rewrite the baselines.

A static check also runs in CI and should be run locally before pushing:

```bash
python tools/check_layering.py               # CMake 管不到的两条：头文件不重名；application/view/ 以外不包含 application/view/
```

## Source File Collection

`YiCAD/CMakeLists.txt` collects sources with `yicad_collect_sources(<partition> <dir>...)`, one call per partition, using `file(GLOB ... CONFIGURE_DEPENDS)`. The partitions follow the target library boundaries in `doc/LAYER_RESTRUCTURE_PLAN.md` section 2, and each partition's variables feed its own `add_library`.

**The directory is the boundary.** Put a file in a directory and it joins that partition — there is no list to keep in sync. `CONFIGURE_DEPENDS` makes the build system re-collect when files are added or removed, so a new file cannot silently miss the build. `yicad_collect_sources` errors out on a directory that does not exist, which keeps dead entries from accumulating the way they had before.

Extensions are the exception to the one-call-per-partition rule: each directory under `src/extensions/<name>/` becomes its own OBJECT library `YiCadExt_<name>`, built from everything found by `GLOB_RECURSE` (sources, `.ui` forms, `.qrc`, `ts/*.ts`, and `support/` install rules), so adding or removing an extension never touches CMake; only the `#include` and `Register` lines in `src/shell/BuiltinExtensions.cpp` name it. An extension library links `YiCadUi` and sees only its own headers, its generated `ui_*.h` and Ui and below, so including another extension's header or a `shell/` header fails to compile. An extension registers its commands, Ribbon entries and settings pages through `IExtensionContext`, and every ID it registers must start with its own extension ID plus a dot (for example `ext.dim.linear`).

Dialogs belong to the extension that uses them: keep the form in the extension's `ui/`, construct it directly, and run it with `UIDialogRunner::exec()` rather than `QDialog::exec()` so `test_interaction` can intercept it (`tests/support/DialogRecorder.h`). Property dialogs are registered per entity type with `IExtensionContext::registerPropertyEditor`. Do not add business dialogs to `GuiDialogFactoryInterface`; it only carries host services (generic prompts, option-bar placement, and status and command-line feedback). Extensions reach the open drawings (new, open, save, export, all documents and views) through `IExtensionContext::documentManager()`, never through the shell's widgets. See `doc/ARCHITECTURE_EVOLUTION_PLAN.md` section 9.3.

Each layer is one library and CMake include paths enforce the direction (`doc/LAYER_RESTRUCTURE_PLAN.md` S6): `src/base/`, `src/model/`, `src/render/`, `src/application/` and `src/ui/` build the static libraries `YiCadBase`, `YiCadModel`, `YiCadRender`, `YiCadApplication` and `YiCadUi`, each seeing only its own headers and those below, so including a higher layer's header fails to compile. `src/shell/` except `Main.cpp` and `BuiltinExtensions.cpp` builds the `YiCadShell` OBJECT library, because the Ribbon icon `.qrc` is compiled into it and a static library would drop its unreferenced resource registration; if you add a `.qrc` or self-registering code under `render/`, `application/` or `ui/`, reference it explicitly (for example `Q_INIT_RESOURCE`) or make that library OBJECT. Only `ui/` compiles the precompiled header `YiCadPch.h`; Shell, the extensions and the executable reuse it. No core library references an extension. The `YiCAD` executable links `YiCadShell` and every `YiCadExt_*` library and passes `registerBuiltinExtensions` to `ApplicationWindow`; the test binaries link the libraries they need, so tests do not recompile the kernel. `main()` stays in the executable so test binaries can supply their own.

## Logging and Profiling

Use the facade in `YiCAD/src/base/debug/`, not `std::cout`:

- `YICAD_LOG(category, level) << ...` (`YiCadLog.h`) — filtered by category and level; the right-hand side is not evaluated when filtered out. Default level is Warning, so nothing prints on hot paths. Configure at runtime with `YICAD_LOG=*:warning,render:debug`.
- `YICAD_SCOPED_TIMER(counter)` (`ScopedTimer.h`) — off by default, costs one relaxed atomic read when off. Uses `steady_clock`; never `system_clock`, which can run backwards. Enable with `YICAD_PROFILE=1`, summarize with `yicad::Profiler::report()`.
- `yicad::ValueCounter` (same header) — counts quantities instead of time, sampled by the caller (for example `render.uploadBytes` and `render.drawCalls`, one sample per frame through `opengl::GLFrameStats`). With profiling on, the app prints the summary on exit.

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
