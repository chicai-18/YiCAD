# 设置cmake 最低版本
cmake_minimum_required (VERSION 3.21)

PROJECT (YiCAD)

# 加载Qt模块
# Qt 6.8 is discovered from the user's installation via CMAKE_PREFIX_PATH or Qt6_DIR.
# QOpenGLWidget 在 Qt 6 里移到了 OpenGLWidgets 模块；QDomDocument 在 Xml 模块。
set(YICAD_USE_QT_MODULES Widgets Gui Core OpenGL OpenGLWidgets Xml Svg Network)

find_package(Qt6 6.8 REQUIRED COMPONENTS ${YICAD_USE_QT_MODULES} LinguistTools)

set(CMAKE_AUTOMOC ON)

# Qt6 imported targets (Qt6::Core, Qt6::Widgets, etc.) carry their include
# directories transitively. No need for explicit include_directories().

# Dependencies come from find_package in cmake/dependencies.cmake.
# The Conan-generated *Config.cmake files provide imported targets that
# carry their own include directories.

# 设置.qrc目录
set(qrc_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/res/ribbon/ribbon_icon.qrc)

# #####################################################
# ================== 源文件清单 ========================
#
# 源文件按 doc/ARCHITECTURE_EVOLUTION_PLAN.md 6.3 节的目标库边界分成八个
# 分区收集（APPLICATION 是业务工具化第四步新增的，见
# doc/COMMAND_TOOL_MIGRATION_PLAN.md 9.4 节）。唯一的事实来源是目录结构：文件放进哪个目录就归哪个库。
#
# 全部加了 CONFIGURE_DEPENDS：新增或删除源文件时构建系统会自己重新收集，
# 不会出现「文件建好了但没进构建」（方案 P5）。
#
# 阶段 0 里全部分区喂给同一个 YiCadCore OBJECT 库，依赖方向尚未由构建
# 系统强制。阶段 3 拆库时发现 RENDER/INTERACTION/UI/SHELL 四个分区之间存在
# 真实的双向依赖（见下方各分区注释与 6.7 节），只有 MATH/MODEL/PERSISTENCE
# 三个分区能拆成三个独立静态库，其余分区仍合编进 YiCadCore。
# #####################################################
set(YICAD_SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

# ---------------------------------------------------------------------------
# yicad_collect_sources(<分区名> <目录> [<目录> ...])
#
# 在给定目录（不递归）下收集 .h 与 .cpp，产出
# YICAD_<分区名>_HEADERS 与 YICAD_<分区名>_SOURCES 两个变量。
# ---------------------------------------------------------------------------
function(yicad_collect_sources partition)
    set(_headers "")
    set(_sources "")
    foreach(_dir IN LISTS ARGN)
        if(NOT IS_DIRECTORY "${YICAD_SRC_ROOT}/${_dir}")
            message(FATAL_ERROR "分区 ${partition} 引用了不存在的目录: ${_dir}")
        endif()
        file(GLOB _h CONFIGURE_DEPENDS "${YICAD_SRC_ROOT}/${_dir}/*.h")
        file(GLOB _c CONFIGURE_DEPENDS "${YICAD_SRC_ROOT}/${_dir}/*.cpp")
        list(APPEND _headers ${_h})
        list(APPEND _sources ${_c})
    endforeach()
    set(YICAD_${partition}_HEADERS ${_headers} PARENT_SCOPE)
    set(YICAD_${partition}_SOURCES ${_sources} PARENT_SCOPE)
endfunction()

# 对应阶段 3 的 YiCadMath（最底层，不依赖任何上层）。
#
# 计划图未列出 kernel/debug/：日志与调试门面被 143 处包含，必须位于最底层。
#
# 阶段 3 实际拆库时发现 kernel/persistence/ 里混杂着两类文件：Persistence
# 类型系统本身（Persistence/Stream/Reader/Writer/Archive/Tools/Base64/Swap/
# TimeInfo/Uuid/gzstream/MinizipNgArchive/FileInfo 与 Meta/MetaType、
# Meta/Type）不依赖任何实体类型，是 DmFlags/DmObject 自身也要用到的底层
# 设施；其余（Meta/ 下各 MetaXxx、MigratorBase、filters/、fileio/）才是
# 真正依赖 Model 的具体持久化实现。前者已随本次拆库迁入本分区（物理文件
# 从 kernel/persistence 移到 kernel/math，见 6.7 节「与方案的偏差」）。
# 这也顺带使 Math2d 的 QString 依赖（eval()/doubleToString()，muParser
# 表达式求值）不必再搬去 utility——本层反正已经需要 Qt Core。
yicad_collect_sources(MATH
    src/kernel/math
    src/kernel/utility
    src/kernel/debug)

# 对应阶段 3 的 YiCadModel（不得依赖 YiCadUi 或 YiCadRender）。
#
# 计划图未列出的 solver/、generators/ 按依赖方向归此。
#
# 改用逐分区 GLOB 时发现，原先的 file(GLOB) 里还列着四个早已不存在的目录：
# builder_model/array、temp/entity_builders、temp/entity_data、temp/geometry。
# GLOB 对不存在的目录静默返回空，所以一直没人察觉。这里已经删掉，
# yicad_collect_sources 也加了目录存在性检查，避免再次积累死目录。
#
# 阶段 3 任务 6.4.1 的三处已知 P7 违规（DmDocument.cpp -> UITabDrawWidget.h、
# DmEntityContainer.cpp/DmHatch.cpp -> UIDialogFactory.h）均已修复：前者
# 经 GuiDialogFactoryInterface 新增的 requestUntitledDocumentName() 接口注入
# 解决；后两处的 include 本身就是死代码，直接删除。tools/check_layering.py
# 的白名单已清空。DmCachePainter.{h,cpp} 已迁到 kernel/gui（现名 kernel/view；
# 本就是纯 GL 封装，放错了目录）；DmImage.{h,cpp} 对 GuiDocumentView.h 的引用也是死
# include，一并删除。详见 doc/ARCHITECTURE_EVOLUTION_PLAN.md 6.7 节。
yicad_collect_sources(MODEL
    src/kernel/builder_model
    src/kernel/builder_model/dimension
    src/kernel/builder_model/text
    src/kernel/data_model
    src/kernel/data_model/dimension
    src/kernel/data_model/text
    src/kernel/history
    src/kernel/modification
    src/kernel/information
    src/kernel/solver
    src/kernel/generators)

# 对应阶段 3 的 YiCadPersistence（依赖 YiCadModel）。
#
# kernel/fileio/ 不在此列——见下方 SHELL 分区的说明，FileIO 对插件运行时的
# 依赖使它物理上只能归 SHELL。DmDocument 对具体读写的调用已改为经
# GuiDialogFactoryInterface 注入，不再直接依赖本分区。
yicad_collect_sources(PERSISTENCE
    src/kernel/persistence
    src/kernel/persistence/Meta
    src/kernel/filters)

# 对应阶段 3 的 YiCadRender。
#
# 本分区内部实现不在本方案排期内（渲染专项另行规划），此处仅作层次边界。
# 原先的 kernel/printing/ 只有一个没有调用方的纸张格式转换，随没有入口的打印桩代码
# 一并删除；打印要有 QPainter 之类的绘制后端，归渲染专项（主计划 7.11 节）。
#
# kernel/view/（原 kernel/gui/）是画布 GuiDocumentView 及其事件、网格等，
# 不认识交互层的工具；装配工具的交互视图 UIView 在 INTERACTION 分区的
# kernel/interaction/。
#
# 业务工具化第四步删除旧版 Action 栈 GuiEventHandler 后，本分区不再包含
# INTERACTION 分区的头文件（doc/COMMAND_TOOL_MIGRATION_PLAN.md 9.4 节），主计划
# 6.7 节记录的这处双向依赖解开；目前仍与 INTERACTION/UI/SHELL 一起编入 YiCadCore。
yicad_collect_sources(RENDER
    src/kernel/painters
    src/kernel/painters/opengl
    src/kernel/view)

# 对应 DS 的 Application/：扩展赖以运行的机制，业务工具化第四步从 kernel/actions/
# 与 kernel/extension/ 移来（doc/COMMAND_TOOL_MIGRATION_PLAN.md 9.4 节）。
#   src/application/            命令（IExclusiveCommand/BaseExclusiveCommand/
#                               ExclusiveCommandBus/CommandRegistry、放置命令与先选
#                               后建的基类）、视图工具框架（IViewTool/ViewToolControl/
#                               PanZoomTool）、选择层 SelectTool、捕捉器 Snapper、预览
#   src/application/framework/  进程内扩展框架（IExtension/IExtensionContext/
#                               IExtensionHost/ExtensionManager），对应 DS 的
#                               Application/Framework/
#   src/cmd/                    keyconfig.xml 与命令行别名（Commands）；计划图未列出，
#                               工具要调用 Commands::checkCommand，归此
#
# 本分区不包含 UI/SHELL 的头文件，也不包含交互视图 UIView（tools/check_layering.py
# 检查）：命令与工具只经 IDocumentView/GuiDocumentView 认识视图。
yicad_collect_sources(APPLICATION
    src/application
    src/application/framework
    src/cmd)

# 对应阶段 3 的 YiCadInteraction：交互视图 UIView（kernel/interaction/，对应 DS 的
# View/UIView），继承 RENDER 分区的画布 GuiDocumentView，装配 APPLICATION 分区的
# ViewToolControl、各层工具与命令总线。原先本分区的 src/actions/（内置命令）在业务
# 工具化第四步拆进了 src/extensions/（ext.draw、ext.modify、ext.measure、ext.edit、
# ext.view）。
yicad_collect_sources(INTERACTION
    src/kernel/interaction)

# 对应阶段 3 的 YiCadUi。
#
# UI 组件里有多处（UIActionHandler、UIBottomWidget、UICurrentActivePen、
# UILineTypeBox、UITabDrawWidget 等）直接调用 ApplicationWindow/MDIWindow
# 取全局状态（当前文档、当前 MDI 标签），是与 SHELL 互相依赖的既有设计。
#
# src/ui/ribbon/ 是 Ribbon 注册表与装配器（UIRibbonRegistry/UIRibbonManager），
# 内置类目与扩展共用。
yicad_collect_sources(UI
    src/ui
    src/ui/forms
    src/ui/ribbon)

# 对应阶段 3 的 YiCadShell（可执行目标）：把各层组装成程序的外壳。
#
# 本分区原名 APP（目标库 YiCadApp），与 APPLICATION 分区、ApplicationWindow 容易混淆，
# 2026-09-25 更名；更名前的文档执行记录保留原名。
#
# 计划图未列出 src/plugin_runtime/，它位于 UI 之上：
#   plugin_runtime/   PluginUiAdapter 把插件命令与按钮登记进 CommandRegistry 与
#                     UIRibbonRegistry（主计划 7.11 节）
#
# 宿主侧的 IExtensionHost 实现在 ApplicationWindow 里；扩展框架本身在
# APPLICATION 分区（src/application/framework/），各扩展是独立的库，见下方
# src/extensions/ 的规则。src/main/BuiltinExtensions.cpp（扩展注册表）与 Main.cpp
# 一样只编进可执行文件。
#
# kernel/fileio/（FileIO 门面类）归入本分区而非 PERSISTENCE：它的
# getFilters() 之外还要 setPluginRuntime() 接入 plugin_runtime/ 的
# PluginRegistry/PluginManager/HostApi，物理上依赖 SHELL 分区。这是已有
# 的、通过 setPluginRuntime() 显式注入的设计，不是本次拆库引入的问题；
# 只是它决定了 Fileio.{h,cpp} 不能编进只应依赖 Model 的 YiCadPersistence。
yicad_collect_sources(SHELL
    src/main
    src/plugin_runtime
    src/kernel/fileio)

# 进程内扩展：src/extensions/ 下每个子目录是一个扩展，各自建成 OBJECT 库
# YiCadExt_<扩展>（见下方 YiCadCore 之后），构建系统里没有逐个扩展的登记。目录约定：
#   <扩展>/**/*.h、*.cpp   源码，递归收集；含头文件的每个目录自动成为该扩展的 include 路径
#   <扩展>/**/*.qrc        Qt 资源
#   <扩展>/**/*.ui         Qt Designer 表单，uic 生成的 ui_*.h 放在该扩展自己的目录里
#   <扩展>/ts/<扩展>_<语言>.ts
#                          扩展自己的翻译，lupdate 只扫本扩展的源码与表单，编译成
#                          qm 与主程序翻译包装在一起，运行期由扩展调用
#                          DmSystem::loadExtensionTranslation("<扩展>") 加载
#   <扩展>/support/        运行期文件，安装到 bin/<扩展>/
# 移除一个扩展 = 删除其目录 + 删除 src/main/BuiltinExtensions.cpp 里对应的
# #include 与 Register 两行；CONFIGURE_DEPENDS 保证增删目录后自动重新配置。
set(YICAD_EXTENSIONS_ROOT "${YICAD_SRC_ROOT}/src/extensions")
file(GLOB _yicad_extension_entries LIST_DIRECTORIES true CONFIGURE_DEPENDS "${YICAD_EXTENSIONS_ROOT}/*")
set(YICAD_EXTENSION_DIRS "")
foreach(_entry IN LISTS _yicad_extension_entries)
    if(IS_DIRECTORY "${_entry}")
        list(APPEND YICAD_EXTENSION_DIRS "${_entry}")
    endif()
endforeach()

# src/ui 下的 Qt Designer 表单，由 qt6_wrap_ui 生成对应的 ui_*.h（扩展的表单见下方
# yicad_wrap_ui）。
file(GLOB YICAD_UI_FORMS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/*.ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/forms/*.ui)

# main() 必须留在可执行目标里，否则测试二进制无法提供自己的入口。
# 扩展注册表 BuiltinExtensions.cpp 同样只编进可执行目标：它引用全部扩展，而扩展
# 各自是独立的库，只有可执行目标（与需要它们的测试）链接。
set(YICAD_MAIN_ENTRY "${CMAKE_CURRENT_SOURCE_DIR}/src/main/Main.cpp")
set(YICAD_BUILTIN_EXTENSIONS "${CMAKE_CURRENT_SOURCE_DIR}/src/main/BuiltinExtensions.cpp")
foreach(_exe_only IN ITEMS "${YICAD_MAIN_ENTRY}" "${YICAD_BUILTIN_EXTENSIONS}")
    if(NOT EXISTS "${_exe_only}")
        message(FATAL_ERROR "未找到只编进可执行目标的源文件 ${_exe_only}")
    endif()
    list(REMOVE_ITEM YICAD_SHELL_SOURCES "${_exe_only}")
endforeach()

# #####################################################
# ======================main===========================
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/main/YiCAD.rc.cmake ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/YiCAD.rc)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/main/icon.ico DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(MAIN_RES ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/YiCAD.rc)

# #####################################################
# =================plugin SDK==========================
set(PLUGIN_SDK_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugin_runtime/YiCadPluginAbi.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugin_runtime/YiCadPluginSdk.h)

# SDK 仅传播公开头文件、C++ 标准和插件导出宏，不链接 YiCAD 或 Qt 内部库。
add_library(YiCadPluginSdk INTERFACE)
add_library(YiCAD::PluginSdk ALIAS YiCadPluginSdk)
# 安装导出时与 YiCAD:: 命名空间组合为 YiCAD::PluginSdk。
set_target_properties(YiCadPluginSdk PROPERTIES EXPORT_NAME PluginSdk)
# 构建树使用源码目录，安装后使用稳定的公开头文件目录。
target_include_directories(YiCadPluginSdk INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/plugin_runtime>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/YiCAD/plugin-sdk>")
target_compile_features(YiCadPluginSdk INTERFACE cxx_std_23)
target_compile_definitions(YiCadPluginSdk INTERFACE YICAD_PLUGIN_BUILD)
target_sources(YiCadPluginSdk INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/plugin_runtime/YiCadPluginAbi.h>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/plugin_runtime/YiCadPluginSdk.h>")

# PluginSDK 是安装组件名称，与 find_package 的 COMPONENTS 参数无关。
install(TARGETS YiCadPluginSdk
        EXPORT YiCADPluginSdkTargets
        COMPONENT PluginSDK)
install(FILES ${PLUGIN_SDK_HEADERS}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/YiCAD/plugin-sdk
        COMPONENT PluginSDK)

# #####################################################
# 翻译文件
#   ts/YiCAD_<语言>.ts   主程序的翻译，lupdate 按下方 YICAD_LUPDATE_SOURCES 更新
#   ts/qtbase_<语言>.ts  Qt 自带对话框的翻译，项目自己维护（Qt 只随附繁体的 qtbase_zh_TW），
#                        只编译不扫描：它的词条来自 Qt 源码，拿本项目源码跑 lupdate 会全部作废
file(GLOB YICAD_TS_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ts/YiCAD_*.ts")
file(GLOB YICAD_QT_TS_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ts/qtbase_*.ts")

# 将.ui转成.h（src/ui 的表单；扩展的表单由各扩展库用 yicad_wrap_ui 生成）
# Qt 6 的 uic 默认按函数指针生成表单里的信号槽连接，槽要在表单根控件的类
# （QDialog、QWidget）上才能编译；本项目的 .ui 连接的是子类里声明的槽，
# 所以沿用 Qt 5 的字符串连接（SIGNAL/SLOT），运行期行为与原来一致。
set(YICAD_UIC_OPTIONS --connections string)
qt6_wrap_ui(ui_FILES ${YICAD_UI_FORMS} OPTIONS ${YICAD_UIC_OPTIONS})

# 将.qrc转成.cpp（主程序的资源；扩展的资源编进各扩展库）
qt6_add_resources(qrc_FILES ${qrc_DIRS})

# ---------------------------------------------------------------------------
# yicad_wrap_ui(<输出变量> <输出目录> <表单> [<表单> ...])
#
# 与 qt6_wrap_ui 相同，但 ui_*.h 生成到给定目录：每个扩展的表单放在自己的目录里，
# 别的扩展看不到。
# ---------------------------------------------------------------------------
function(yicad_wrap_ui out_var out_dir)
    set(_headers "")
    foreach(_form IN LISTS ARGN)
        get_filename_component(_name "${_form}" NAME_WE)
        set(_header "${out_dir}/ui_${_name}.h")
        add_custom_command(OUTPUT "${_header}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${out_dir}"
            COMMAND Qt6::uic ${YICAD_UIC_OPTIONS} -o "${_header}" "${_form}"
            MAIN_DEPENDENCY "${_form}"
            VERBATIM)
        set_source_files_properties("${_form}" PROPERTIES SKIP_AUTOMOC ON SKIP_AUTOUIC ON)
        set_source_files_properties("${_header}" PROPERTIES SKIP_AUTOMOC ON SKIP_AUTOUIC ON)
        list(APPEND _headers "${_header}")
    endforeach()
    set(${out_var} ${_headers} PARENT_SCOPE)
endfunction()

# 翻译的构建与更新
#
# 构建只把 .ts 编译成 .qm（安装时拷到 bin/resources/qm/），从不改写源码树里的 .ts。
# 源码里的字符串有增删时，手动构建 update_translations 目标更新全部 .ts，再用 Qt Linguist
# 填写译文：
#   cmake --build --preset Release --target update_translations
# 源码里已不存在的字符串，lupdate 标为 vanished 并保留译文：字符串挪进别的 .ts（例如从主程序
# 挪进扩展）时，可以从这里找回译文；确认无用后在 Linguist 里删除。
#
# update_translations 是一个目标，里面对主程序与每个扩展各跑一次 lupdate（每份 .ts 只扫
# 自己的源码）。不用 Qt 的 qt_add_lupdate：它每调用一次建一个目标，每个扩展都会多出一个
# 项目。

# yicad_add_lupdate_job(<名字> TS_FILES <ts>... SOURCES <源文件>...)
#
# 把一次 lupdate 调用追加到 YICAD_LUPDATE_COMMANDS。源文件清单写进构建目录里的列表文件，
# 免得命令行过长。
function(yicad_add_lupdate_job name)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "" "TS_FILES;SOURCES")
    set(_lst "${CMAKE_CURRENT_BINARY_DIR}/lupdate/${name}.lst")
    list(JOIN arg_SOURCES "\n" _content)
    file(WRITE "${_lst}" "${_content}\n")
    set(YICAD_LUPDATE_COMMANDS ${YICAD_LUPDATE_COMMANDS}
        COMMAND "$<TARGET_FILE:Qt6::lupdate>" "@${_lst}" -ts ${arg_TS_FILES}
        PARENT_SCOPE)
endfunction()

set(YICAD_LUPDATE_COMMANDS "")

# 主程序的扫描范围：各分区的全部源文件、src/ui 的表单，加上 Main.cpp。src/extensions/ 下
# 的源码不在此列，由各扩展自己的 ts 覆盖（见下方）。
set(YICAD_LUPDATE_SOURCES
    ${YICAD_MATH_HEADERS}        ${YICAD_MATH_SOURCES}
    ${YICAD_MODEL_HEADERS}       ${YICAD_MODEL_SOURCES}
    ${YICAD_PERSISTENCE_HEADERS} ${YICAD_PERSISTENCE_SOURCES}
    ${YICAD_RENDER_HEADERS}      ${YICAD_RENDER_SOURCES}
    ${YICAD_APPLICATION_HEADERS} ${YICAD_APPLICATION_SOURCES}
    ${YICAD_INTERACTION_HEADERS} ${YICAD_INTERACTION_SOURCES}
    ${YICAD_UI_HEADERS}          ${YICAD_UI_SOURCES}          ${YICAD_UI_FORMS}
    ${YICAD_SHELL_HEADERS}       ${YICAD_SHELL_SOURCES}
    ${YICAD_MAIN_ENTRY})

yicad_add_lupdate_job(YiCAD TS_FILES ${YICAD_TS_FILES} SOURCES ${YICAD_LUPDATE_SOURCES})
qt6_add_translation(QM_FILES ${YICAD_TS_FILES} ${YICAD_QT_TS_FILES})

# 扩展各自的翻译包：lupdate 只扫本扩展的源码与表单，编译与安装和主程序的翻译包相同。
foreach(_ext_dir IN LISTS YICAD_EXTENSION_DIRS)
    file(GLOB _ext_ts CONFIGURE_DEPENDS "${_ext_dir}/ts/*.ts")
    if(NOT _ext_ts)
        continue()
    endif()
    get_filename_component(_ext_name "${_ext_dir}" NAME)
    file(GLOB_RECURSE _ext_lupdate_src CONFIGURE_DEPENDS
        "${_ext_dir}/*.h" "${_ext_dir}/*.cpp" "${_ext_dir}/*.ui")
    yicad_add_lupdate_job(${_ext_name} TS_FILES ${_ext_ts} SOURCES ${_ext_lupdate_src})
    qt6_add_translation(_ext_qm ${_ext_ts})
    list(APPEND QM_FILES ${_ext_qm})
endforeach()

# 不属于默认构建，只在显式构建时运行
add_custom_target(update_translations ${YICAD_LUPDATE_COMMANDS} VERBATIM)

SET(QM_FILES ${QM_FILES} PARENT_SCOPE)
ADD_CUSTOM_TARGET (TS_YiCAD ALL DEPENDS ${QM_FILES})
set_target_properties(TS_YiCAD PROPERTIES FOLDER "CMakePredefinedTargets")

# #####################################################
# ============ YiCadMath / YiCadModel / YiCadPersistence ============
#
# 阶段 3 把 MATH/MODEL/PERSISTENCE 三个分区拆成三个独立静态库，依赖方向由
# CMake 强制：YiCadPersistence -> YiCadModel -> YiCadMath。RENDER/
# INTERACTION/UI/SHELL 四个分区因 6.7 节所述的既有循环依赖，继续合编进
# YiCadCore（见下）。
#
# 用 STATIC 而非 OBJECT：这三层不含任何 Qt 元对象类（Q_OBJECT），没有
# YiCadCore 注释里提到的自动注册符号丢失问题；STATIC 让它们能被
# tests/math、tests/geometry、tests/persistence 单独链接，不必拖入整个
# YiCadCore（这也是本次拆库最主要的增量构建收益来源）。
# #####################################################

add_library(YiCadMath STATIC
    ${YICAD_MATH_HEADERS} ${YICAD_MATH_SOURCES})
# QT_NO_UNICODE_DEFINES：Qt 6 的 Qt6::Platform 在 Windows 上给每个使用方加 UNICODE 与
# _UNICODE（Qt 5 的 CMake 目标不加）。这会把 muParser 的头文件切到 std::wstring 接口，
# 与 Conan 按窄字符构建的 muparser 链接不上，也会把 Win32 的 TCHAR 宏切到 W 版本。
# 本项目沿用 Qt 5 下的行为，所有链接 Qt 的目标都设置这一属性（该属性按使用方目标读取，
# 不会沿依赖传播，扩展、可执行目标与测试目标各自再设一次）。
set_target_properties(YiCadMath PROPERTIES FOLDER "YiCAD" QT_NO_UNICODE_DEFINES TRUE)

target_include_directories(YiCadMath PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/math
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/utility
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/debug)

# 公共编译定义与选项在最底层设置一次，经 PUBLIC 依赖链自动传播到
# YiCadModel -> YiCadPersistence -> YiCadCore -> 可执行目标/测试目标，
# 不必在每一层重复声明。
target_compile_definitions(YiCadMath PUBLIC
    _USE_MATH_DEFINES
    _ENABLE_EXTENDED_ALIGNED_STORAGE
    APPDIR="YiCAD"
    # 屏蔽 Qt 6.0 及以前废弃的 API，用到即编译失败，防止旧写法回流
    QT_DISABLE_DEPRECATED_UP_TO=0x060000)
remove_definitions(-D_HAS_STD_BYTE)

# MSVC: 强制源文件和执行字符集为 UTF-8，避免中文乱码。
if(MSVC)
    target_compile_options(YiCadMath PUBLIC /utf-8)
endif()

# Tools.h/Reader.h/Writer.h/gzstream.h/MinizipNgArchive.h 等原 persistence
# 基础设施随本次拆库迁入本层（见上方分区注释），因此本层需要 Qt Core、
# pugixml、minizip、zlib；muParser 用于 Math2d::eval() 的表达式求值。
if(TARGET nlohmann_json::nlohmann_json)
    target_link_libraries(YiCadMath PUBLIC nlohmann_json::nlohmann_json)
endif()
if(TARGET Eigen3::Eigen)
    target_link_libraries(YiCadMath PUBLIC Eigen3::Eigen)
endif()
if(TARGET glm::glm)
    target_link_libraries(YiCadMath PUBLIC glm::glm)
endif()
if(TARGET Boost::boost)
    target_link_libraries(YiCadMath PUBLIC Boost::boost)
endif()
if(TARGET ZLIB::ZLIB)
    target_link_libraries(YiCadMath PUBLIC ZLIB::ZLIB)
endif()
if(TARGET pugixml::pugixml)
    target_link_libraries(YiCadMath PUBLIC pugixml::pugixml)
endif()
if(TARGET MINIZIP::minizip)
    target_link_libraries(YiCadMath PUBLIC MINIZIP::minizip)
endif()
if(TARGET muparser::muparser)
    target_link_libraries(YiCadMath PUBLIC muparser::muparser)
else()
    message(FATAL_ERROR "muparser::muparser target not found. "
        "Run 'conan install' with --build=never before configuring CMake.")
endif()
target_link_libraries(YiCadMath PUBLIC Qt6::Core)

add_library(YiCadModel STATIC
    ${YICAD_MODEL_HEADERS} ${YICAD_MODEL_SOURCES})
set_target_properties(YiCadModel PROPERTIES FOLDER "YiCAD" QT_NO_UNICODE_DEFINES TRUE)

target_include_directories(YiCadModel PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/builder_model
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/builder_model/dimension
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/builder_model/text
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/data_model
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/data_model/dimension
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/data_model/text
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/history
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/modification
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/information
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/solver
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/generators)

target_link_libraries(YiCadModel PUBLIC YiCadMath)

# ConstrainedDelaunayTriangulation（kernel/information，阶段 3 从 kernel/math
# 迁入，见上）需要 CDT；DmPen/DmMText 等少量类型用到 QColor/QFont，需要 Qt Gui；
# DmFont（builder_model/text/）直接用 FreeType 量取字形轮廓，需要 Freetype。
if(TARGET CDT::CDT)
    target_link_libraries(YiCadModel PUBLIC CDT::CDT)
endif()
if(TARGET Freetype::Freetype)
    target_link_libraries(YiCadModel PUBLIC Freetype::Freetype)
endif()
target_link_libraries(YiCadModel PUBLIC Qt6::Gui)

# DmSystem.cpp 在 Windows 上用 DirectWrite 枚举系统字体文件路径。
if(WIN32)
    target_link_libraries(YiCadModel PUBLIC Dwrite.lib)
endif()

add_library(YiCadPersistence STATIC
    ${YICAD_PERSISTENCE_HEADERS} ${YICAD_PERSISTENCE_SOURCES})
set_target_properties(YiCadPersistence PROPERTIES FOLDER "YiCAD" QT_NO_UNICODE_DEFINES TRUE)

target_include_directories(YiCadPersistence PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/persistence
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/persistence/Meta
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/filters)

target_link_libraries(YiCadPersistence PUBLIC YiCadModel)

# #####################################################
# ==================== YiCadCore ======================
#
# RENDER + APPLICATION + INTERACTION + UI + SHELL 五个分区。可执行目标和测试二进制
# 共享同一批目标文件，测试因此不必重新编译这部分代码，也不必为每个测试手工
# 维护源文件子集。进程内扩展不在其中，各自是 OBJECT 库 YiCadExt_<扩展>（见下方），
# YiCadCore 不引用任何扩展。
#
# 选 OBJECT 而非 STATIC：静态库的符号剥离会丢掉 Qt 的静态注册（资源初始化、
# 插件注册宏等自动注册符号没有显式引用），OBJECT 库没有这个问题。
#
# 这四个分区未按方案拆成四个独立静态库：见上方 RENDER/INTERACTION/UI 分区
# 注释与 6.7 节，GuiDocumentView/GuiEventHandler（RENDER）与
# ViewToolControl/PanZoomTool/SelectTool/LegacyActionTool/ActionInterface
# （INTERACTION）互相持有对方的具体类型，UI 与 SHELL 之间同样存在大量
# ApplicationWindow/MDIWindow 互相调用，是阶段 1、2 与既有代码留下的真实
# 双向依赖，不是本次拆库能靠移动文件解决的问题。
# #####################################################
add_library(YiCadCore OBJECT
    ${YICAD_RENDER_HEADERS}      ${YICAD_RENDER_SOURCES}
    ${YICAD_APPLICATION_HEADERS} ${YICAD_APPLICATION_SOURCES}
    ${YICAD_INTERACTION_HEADERS} ${YICAD_INTERACTION_SOURCES}
    ${YICAD_UI_HEADERS}          ${YICAD_UI_SOURCES}
    ${YICAD_SHELL_HEADERS}       ${YICAD_SHELL_SOURCES}
    ${YICAD_UI_FORMS}
    ${ui_FILES}
    ${qrc_FILES})

set_target_properties(YiCadCore PROPERTIES FOLDER "YiCAD" QT_NO_UNICODE_DEFINES TRUE)

# 头文件搜索路径挂在目标上（PUBLIC），测试目标通过链接 YiCadCore 继承。
target_include_directories(YiCadCore PUBLIC
    # uic 生成的 ui_*.h 落在构建目录
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/application
    ${CMAKE_CURRENT_SOURCE_DIR}/src/application/framework
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cmd
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/fileio
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/interaction
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/view
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/painters
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/painters/opengl
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugin_runtime
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/forms
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ribbon)

target_precompile_headers(YiCadCore PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/main/YiCadPch.h")

# ---- YiCadCore 的依赖 ----
# 全部用 PUBLIC：可执行目标与测试目标都需要这些库的头文件和符号。
# YiCadMath/YiCadModel/YiCadPersistence 需要的第三方库已经在各自层级声明，
# 经 PUBLIC 依赖链传播到这里，此处只声明 RENDER/INTERACTION/UI/SHELL
# 自己新增的那些。

target_link_libraries(YiCadCore PUBLIC YiCadPersistence)

if(TARGET GLEW::GLEW)
    target_link_libraries(YiCadCore PUBLIC GLEW::GLEW)
endif()
if(TARGET OpenGL::GL)
    target_link_libraries(YiCadCore PUBLIC OpenGL::GL OpenGL::GLU)
endif()
if(TARGET Freetype::Freetype)
    target_link_libraries(YiCadCore PUBLIC Freetype::Freetype)
endif()

# Windows system libraries
if(WIN32)
    target_link_libraries(YiCadCore PUBLIC Dwrite.lib crypt32.lib)
endif()

# SARibbonBar: imported target from find_package (step 8)
target_link_libraries(YiCadCore PUBLIC SARibbonBar::SARibbonBar)

#add QT one by one
foreach(qtmod ${YICAD_USE_QT_MODULES})
    target_link_libraries(YiCadCore PUBLIC Qt6::${qtmod})
endforeach(qtmod)

# #####################################################
# ============== 进程内扩展 YiCadExt_<扩展> ==============
#
# src/extensions/ 下每个子目录建一个 OBJECT 库（与 YiCadCore 同样选 OBJECT：扩展里有
# Qt 资源与元对象类，静态库会丢掉没有显式引用的自动注册符号）。扩展依赖 YiCadCore
# （内核、命令机制、界面），YiCadCore 不依赖任何扩展；扩展只看得到自己的头文件与
# uic 生成的 ui_*.h，扩展之间互相包含会直接编译失败（主计划 7.11 节）。
#
# 头文件目录是 PUBLIC 的：链接扩展的只有可执行目标（BuiltinExtensions.cpp 要包含
# 扩展入口的头文件）与测试。
#
# 预编译头沿用 YiCadCore 的：拆出来之前扩展的源码一直带着 YiCadPch.h 编译。
#
# 全部扩展目标记在全局属性 YICAD_EXTENSION_TARGETS 里，供 tests/ 链接。
# #####################################################
set(YICAD_EXTENSION_TARGETS "")
foreach(_ext_dir IN LISTS YICAD_EXTENSION_DIRS)
    get_filename_component(_ext_name "${_ext_dir}" NAME)
    set(_ext_target "YiCadExt_${_ext_name}")

    file(GLOB_RECURSE _ext_headers CONFIGURE_DEPENDS "${_ext_dir}/*.h")
    file(GLOB_RECURSE _ext_sources CONFIGURE_DEPENDS "${_ext_dir}/*.cpp")
    file(GLOB_RECURSE _ext_forms CONFIGURE_DEPENDS "${_ext_dir}/*.ui")
    file(GLOB_RECURSE _ext_qrcs CONFIGURE_DEPENDS "${_ext_dir}/*.qrc")
    if(NOT _ext_sources)
        message(FATAL_ERROR "扩展目录 ${_ext_dir} 里没有源文件")
    endif()

    set(_ext_gen_dir "${CMAKE_CURRENT_BINARY_DIR}/extensions/${_ext_name}")
    yicad_wrap_ui(_ext_ui_headers "${_ext_gen_dir}" ${_ext_forms})
    set(_ext_qrc_sources "")
    if(_ext_qrcs)
        qt6_add_resources(_ext_qrc_sources ${_ext_qrcs})
    endif()

    add_library(${_ext_target} OBJECT
        ${_ext_headers} ${_ext_sources}
        ${_ext_forms} ${_ext_ui_headers}
        ${_ext_qrcs} ${_ext_qrc_sources})

    set(_ext_include_dirs "${_ext_gen_dir}")
    foreach(_header IN LISTS _ext_headers)
        get_filename_component(_header_dir "${_header}" DIRECTORY)
        list(APPEND _ext_include_dirs "${_header_dir}")
    endforeach()
    list(REMOVE_DUPLICATES _ext_include_dirs)
    target_include_directories(${_ext_target} PUBLIC ${_ext_include_dirs})

    target_link_libraries(${_ext_target} PUBLIC YiCadCore)
    target_precompile_headers(${_ext_target} REUSE_FROM YiCadCore)
    set_target_properties(${_ext_target} PROPERTIES FOLDER "YiCAD/extensions" QT_NO_UNICODE_DEFINES TRUE)
    GroupSourcesByFolder(${_ext_target})

    list(APPEND YICAD_EXTENSION_TARGETS ${_ext_target})
endforeach()
set_property(GLOBAL PROPERTY YICAD_EXTENSION_TARGETS ${YICAD_EXTENSION_TARGETS})

# #####################################################
# ================ YiCAD 可执行目标 ====================
# 只编译 main() 与扩展注册表，其余目标文件来自 YiCadCore 与各扩展库。
# #####################################################
add_executable(${PROJECT_NAME} ${YICAD_MAIN_ENTRY}
                               ${YICAD_BUILTIN_EXTENSIONS}
                               ${MAIN_RES}
                               ${qrc_DIRS}
                               ${QM_FILES})

# 链接 OBJECT 库会同时带入它的目标文件和全部使用要求（include 路径、
# 编译定义、依赖库），无需再写 $<TARGET_OBJECTS:YiCadCore>。目标文件只从直接链接的
# OBJECT 库带入，所以扩展库要逐个直接链接。
target_link_libraries(${PROJECT_NAME} PRIVATE YiCadCore ${YICAD_EXTENSION_TARGETS})

target_precompile_headers(${PROJECT_NAME} REUSE_FROM YiCadCore)
set_target_properties(${PROJECT_NAME} PROPERTIES QT_NO_UNICODE_DEFINES TRUE)

# 支持linux相对路径库加载，参考：https://stackoverflow.com/questions/43330165/how-to-link-a-shared-library-with-cmake-with-relative-path/69707790#69707790
# https://gitlab.kitware.com/cmake/community/-/wikis/doc/cmake/RPATH-handling
if(NOT WIN32)
set_target_properties(${PROJECT_NAME} PROPERTIES
    LINK_FLAGS "-Wl,-rpath,$ORIGIN")
endif()

# 根据选项
use_visual_leak_detector(${PROJECT_NAME})

if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
    set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_DEFINITIONS_DEBUG "_CONSOLE")
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:WINDOWS")
    set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "/SUBSYSTEM:WINDOWS")
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS")
endif()

#copy all third party files to target folder
if(WIN32)

    #copy all qt related files
    windeployqt(${PROJECT_NAME})

    # DLL deployment from Conan packages.
    # Conan-generated DLLs are found via the imported targets
    # and deployed by the build system automatically for MSVC generators.

endif()

set(_yicad_install_guard_code [=[
set(_yicad_runtime_output_dir "@YICAD_OUTPUT_DIRECTORY@")
set(_yicad_install_bindir "@CMAKE_INSTALL_BINDIR@")
if(NOT IS_ABSOLUTE "${_yicad_install_bindir}")
    set(_yicad_install_bindir "${CMAKE_INSTALL_PREFIX}/${_yicad_install_bindir}")
endif()

cmake_path(ABSOLUTE_PATH _yicad_runtime_output_dir
           NORMALIZE
           OUTPUT_VARIABLE _yicad_runtime_output_dir_abs)
cmake_path(ABSOLUTE_PATH _yicad_install_bindir
           NORMALIZE
           OUTPUT_VARIABLE _yicad_install_bindir_abs)

set(_yicad_runtime_output_dir_cmp "${_yicad_runtime_output_dir_abs}")
set(_yicad_install_bindir_cmp "${_yicad_install_bindir_abs}")
if(WIN32)
    string(TOLOWER "${_yicad_runtime_output_dir_cmp}" _yicad_runtime_output_dir_cmp)
    string(TOLOWER "${_yicad_install_bindir_cmp}" _yicad_install_bindir_cmp)
endif()
string(REGEX REPLACE "/+$" "" _yicad_runtime_output_dir_cmp "${_yicad_runtime_output_dir_cmp}")
string(REGEX REPLACE "/+$" "" _yicad_install_bindir_cmp "${_yicad_install_bindir_cmp}")

if(NOT _yicad_install_bindir_cmp STREQUAL _yicad_runtime_output_dir_cmp)
    string(FIND "${_yicad_install_bindir_cmp}/"
                "${_yicad_runtime_output_dir_cmp}/"
                _yicad_nested_install_bindir)
    if(_yicad_nested_install_bindir EQUAL 0)
        message(FATAL_ERROR
            "YiCAD install destination is inside the runtime output directory.\n"
            "  runtime output: ${_yicad_runtime_output_dir_abs}\n"
            "  install bindir: ${_yicad_install_bindir_abs}\n"
            "Choose a CMAKE_INSTALL_PREFIX outside the runtime output tree.")
    endif()
endif()
]=])
string(CONFIGURE "${_yicad_install_guard_code}" _yicad_install_guard_code @ONLY)
install(CODE "${_yicad_install_guard_code}"
        COMPONENT Runtime)
unset(_yicad_install_guard_code)

#install all resources
INSTALL(FILES ${QM_FILES}
        DESTINATION ${CMAKE_INSTALL_BINDIR}/resources/qm
        COMPONENT Runtime)
# User-provided resource directories: install matching files only (ignore .gitkeep, README.md).
# These directories are empty by default; users place their own PAT/LIN/SHX files here.
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/support/patterns/
        DESTINATION ${CMAKE_INSTALL_BINDIR}/resources/patterns
        COMPONENT Runtime
        FILES_MATCHING PATTERN "*.pat")
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/support/linetypes/
        DESTINATION ${CMAKE_INSTALL_BINDIR}/resources/linetypes
        COMPONENT Runtime
        FILES_MATCHING PATTERN "*.lin")
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/support/fonts/
        DESTINATION ${CMAKE_INSTALL_BINDIR}/resources/fonts
        COMPONENT Runtime
        FILES_MATCHING PATTERN "*.shx")
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/res/shaders/
        DESTINATION ${CMAKE_INSTALL_BINDIR}/resources/shaders
        COMPONENT Runtime
        FILES_MATCHING PATTERN "*.*")

#install config file
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/support/config/
        DESTINATION ${CMAKE_INSTALL_BINDIR}/
        COMPONENT Runtime
        FILES_MATCHING PATTERN "*.*")

INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../README.md
        DESTINATION ${CMAKE_INSTALL_BINDIR}/
        COMPONENT Runtime)

# 扩展的运行期文件：src/extensions/<扩展>/support/ -> bin/<扩展>/
foreach(_ext_dir IN LISTS YICAD_EXTENSION_DIRS)
    if(IS_DIRECTORY "${_ext_dir}/support")
        get_filename_component(_ext_name "${_ext_dir}" NAME)
        INSTALL(DIRECTORY "${_ext_dir}/support/"
                DESTINATION ${CMAKE_INSTALL_BINDIR}/${_ext_name}/
                COMPONENT Runtime
                FILES_MATCHING PATTERN "*.*")
    endif()
endforeach()

# Install the deployed runtime bundle produced by the build. On Windows this
# includes the files copied by windeployqt.
# 测试二进制统一以 test_ 开头，不进安装包。
INSTALL(DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT Runtime
        PATTERN "test_*" EXCLUDE)

#structure files
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src/ui" PREFIX "UIs" FILES ${YICAD_UI_FORMS})
GroupSourcesByFolder(YiCadCore)
GroupSourcesByFolder(YiCadMath)
GroupSourcesByFolder(YiCadModel)
GroupSourcesByFolder(YiCadPersistence)
