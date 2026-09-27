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
# 源文件按 doc/LAYER_RESTRUCTURE_PLAN.md 第 2 节的目标库分成六个分区收集：
# BASE、MODEL、RENDER、APPLICATION、UI、SHELL。唯一的事实来源是目录结构：
# 文件放进哪个目录就归哪个库。
#
# 全部加了 CONFIGURE_DEPENDS：新增或删除源文件时构建系统会自己重新收集，
# 不会出现「文件建好了但没进构建」（方案 P5）。
#
# 每个分区是一个库，依赖方向由 CMake 保证：每个库只看得到自己与下层的头文件。
# RENDER 以上四个分区原先合编进 YiCadCore，分层重组 S6 拆开（库类型见 YiCadRender 之前的
# 说明）。tools/check_layering.py 只检查 CMake 管不到的两条：头文件重名，以及
# application/view/ 以外不得包含 application/view/。
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

# 对应 YiCadBase（最底层，只依赖第三方库）。
#   src/base/core/      类型系统（Type/MetaType）、序列化机制（Persistence/Stream/
#                       Reader/Writer/Archive）、压缩、编码、Uuid、Datamodel（DM 命名空间
#                       的枚举与常量）、TSingleton
#   src/base/debug/     日志与计时门面（YiCadLog/ScopedTimer/Debug），被全仓包含
#   src/base/geometry/  向量、矩形、Math2d、方程求解、凸包、KD 树、R 树
#
# 序列化机制不依赖任何实体类型，DmFlags/DmObject 自身也要用到，所以在本层而不在
# model/io/。Math2d 的表达式求值要用 muParser 与 QString，本层本就需要 Qt Core。
yicad_collect_sources(BASE
    src/base/core
    src/base/debug
    src/base/geometry)

# 对应 YiCadModel（只依赖 YiCadBase，不得包含渲染、命令机制与界面）。
#   src/model/document/     文档与全局对象（DmDocument/DmSystem/DmSettings/DmClipboard 等）
#   src/model/property/     颜色、画笔、图层、线型、填充图案
#   src/model/entity/       实体与实体工具类；dimension/ 标注，text/ 文字
#   src/model/entity_data/  实体的数据结构 *Data；dimension/、text/ 同上
#   src/model/table/        符号表、实体表、空间索引
#   src/model/history/      撤销重做（Cmd/CmdManager/MacroCmd/Transaction 与各 *Cmd）
#   src/model/edit/         Modification（选择集在 application/ 的 SelectionSet）
#   src/model/algorithm/    求交、面积、闭合区域、三角剖分
#   src/model/io/           原生格式 FilterOcdIO、格式注册表 FilterRegistry、覆盖写时的
#                           编号备份、迁移基类；meta/ 是各类型的
#                           Meta* 容器。原是独立的 YiCadPersistence 库，它本来就只依赖
#                           Model 与 Base，分层重组 S2 并入本库
#
# Model 不认识视图与宿主：文档经 DmDocumentListener 通知画布（分层重组 S3），读写文件经
# model/io/ 的格式注册表，存盘策略与宿主服务接口在 application/（S4）。原先的过渡目录
# model/host/ 已删除。
yicad_collect_sources(MODEL
    src/model/document
    src/model/property
    src/model/entity
    src/model/entity/dimension
    src/model/entity/text
    src/model/entity_data
    src/model/entity_data/dimension
    src/model/entity_data/text
    src/model/table
    src/model/history
    src/model/edit
    src/model/algorithm
    src/model/io
    src/model/io/meta)

# 对应 YiCadRender。
#   src/render/painter/  绘制抽象（Painter/PainterCreator）
#   src/render/opengl/   OpenGL 实现（GL*）
#   src/render/view/     画布 GuiDocumentView 及其事件、坐标输入、网格、预览窗、DmCachePainter，
#                        以及命令与工具眼中的视图接口 IDocumentView、捕捉接口 ISnapService，
#                        绘制时判断实体是否选中的接口 ISelectionSource、取要高亮的实体的接口
#                        IHighlightSource
#
# 画布不认识命令与视图工具；装配工具的交互视图 UIView 在 application/view/。
# 文档经 DmDocumentListener 通知画布，Model 不认识本分区。
# 本分区的内部实现不在分层重组方案的排期内（渲染专项另行规划）。
yicad_collect_sources(RENDER
    src/render/painter
    src/render/opengl
    src/render/view)

# 对应 YiCadApplication（沿用 DS 的 Application 命名）：扩展赖以运行的机制。
#   src/application/            命令（IExclusiveCommand/BaseExclusiveCommand/
#                               ExclusiveCommandBus/CommandRegistry、放置命令与先选
#                               后建的基类）、视图工具框架（IViewTool/ViewToolControl/
#                               PanZoomTool）、选择层 SelectTool、捕捉器 Snapper、预览、
#                               keyconfig.xml 与命令行别名（Commands）、一份打开的图纸
#                               AppDocument（持有文档、存盘策略与选择集）、文档的选择集
#                               SelectionSet、视图的高亮集 HighlightSet、文档的存盘策略
#                               DocumentFileService（自动保存、备份、打开失败的处理）、
#                               宿主服务接口 GuiDialogFactoryInterface（实现在 shell/ 的
#                               UIDialogFactory）
#   src/application/framework/  进程内扩展框架（IExtension/IExtensionContext/
#                               IExtensionHost/ExtensionManager），对应 DS 的
#                               Application/Framework/；宿主管理的打开图纸 IDocumentManager
#                               （实现在 shell/）
#   src/application/view/       交互视图 UIView（对应 DS 的 View/UIView）：继承画布
#                               GuiDocumentView，装配 ViewToolControl、各层工具与命令总线
#
# 本分区看不到 UI/SHELL 的头文件；命令与工具只经 IDocumentView/GuiDocumentView 认识视图，
# application/view/ 以外的文件不包含 UIView（同在一个库里，由 tools/check_layering.py 检查）。
yicad_collect_sources(APPLICATION
    src/application
    src/application/framework
    src/application/view)

# 对应 YiCadUi：扩展与壳层都能用的控件与对话框，以及对话框运行器 UIDialogRunner、文件对话框
# UIFileDialog。forms/ 是 Qt Designer 表单对应的对话框，ribbon/ 是 Ribbon 注册表
# UIRibbonRegistry，内置类目与扩展共用。
#
# 本分区看不到 SHELL 的头文件。要取当前文档的控件（UICurrentActivePen、UILineTypeBox）经
# 宿主注入的 IDocumentManager 取；管理图纸与主窗口的部件在 SHELL（分层重组 S5）。
# 预编译头 YiCadPch.h 也在这里：由 YiCadUi 编译，YiCadShell、扩展与可执行目标复用。
yicad_collect_sources(UI
    src/ui
    src/ui/forms
    src/ui/ribbon)

# 对应 YiCadShell：把各层组装成程序的外壳。
#   src/shell/                 主窗口 ApplicationWindow（兼宿主侧的 IExtensionHost、
#                              IDocumentManager 实现）、图纸选项卡 UITabDrawWidget、图纸窗口
#                              MDIWindow、状态栏 UIBottomWidget 与捕捉工具栏 UISnapWidget、
#                              命令行 UICommandWidget、命令分派 UIActionHandler、宿主服务实现
#                              UIDialogFactory、Ribbon 装配器 UIRibbonManager、只有壳层用的表单
#                              （UIExitDialog、UISnapMiddleOptions），以及只编进可执行目标的
#                              Main.cpp 与扩展注册表 BuiltinExtensions.cpp（见下方）
#   src/shell/plugin_runtime/  C ABI 插件运行时。PluginUiAdapter 把插件命令与按钮登记进
#                              CommandRegistry 与 UIRibbonRegistry，PluginFormatRegistration
#                              把插件的文件格式登记进 model/io/ 的 FilterRegistry；公开头文件
#                              YiCadPluginAbi.h、YiCadPluginSdk.h 同时是插件 SDK
#
# 本分区原名 APP（目标库 YiCadApp），与 APPLICATION 分区、ApplicationWindow 容易混淆，
# 2026-09-25 更名；更名前的文档执行记录保留原名。
yicad_collect_sources(SHELL
    src/shell
    src/shell/plugin_runtime)

# 进程内扩展：src/extensions/ 下每个子目录是一个扩展，各自建成 OBJECT 库
# YiCadExt_<扩展>（见下方 YiCadShell 之后），构建系统里没有逐个扩展的登记。目录约定：
#   <扩展>/**/*.h、*.cpp   源码，递归收集；含头文件的每个目录自动成为该扩展的 include 路径
#   <扩展>/**/*.qrc        Qt 资源
#   <扩展>/**/*.ui         Qt Designer 表单，uic 生成的 ui_*.h 放在该扩展自己的目录里
#   <扩展>/ts/<扩展>_<语言>.ts
#                          扩展自己的翻译，lupdate 只扫本扩展的源码与表单，编译成
#                          qm 与主程序翻译包装在一起，运行期由扩展调用
#                          DmSystem::loadExtensionTranslation("<扩展>") 加载
#   <扩展>/support/        运行期文件，安装到 bin/<扩展>/
# 移除一个扩展 = 删除其目录 + 删除 src/shell/BuiltinExtensions.cpp 里对应的
# #include 与 Register 两行；CONFIGURE_DEPENDS 保证增删目录后自动重新配置。
set(YICAD_EXTENSIONS_ROOT "${YICAD_SRC_ROOT}/src/extensions")
file(GLOB _yicad_extension_entries LIST_DIRECTORIES true CONFIGURE_DEPENDS "${YICAD_EXTENSIONS_ROOT}/*")
set(YICAD_EXTENSION_DIRS "")
foreach(_entry IN LISTS _yicad_extension_entries)
    if(IS_DIRECTORY "${_entry}")
        list(APPEND YICAD_EXTENSION_DIRS "${_entry}")
    endif()
endforeach()

# src/ui、src/shell 下的 Qt Designer 表单，由 qt6_wrap_ui 生成对应的 ui_*.h（扩展的表单见下方
# yicad_wrap_ui）。两个分区各一个变量，S6 拆库时分别交给 YiCadUi、YiCadShell。
file(GLOB YICAD_UI_FORMS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/*.ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/forms/*.ui)
file(GLOB YICAD_SHELL_FORMS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shell/*.ui)

# main() 必须留在可执行目标里，否则测试二进制无法提供自己的入口。
# 扩展注册表 BuiltinExtensions.cpp 同样只编进可执行目标：它引用全部扩展，而扩展
# 各自是独立的库，只有可执行目标（与需要它们的测试）链接。
set(YICAD_MAIN_ENTRY "${CMAKE_CURRENT_SOURCE_DIR}/src/shell/Main.cpp")
set(YICAD_BUILTIN_EXTENSIONS "${CMAKE_CURRENT_SOURCE_DIR}/src/shell/BuiltinExtensions.cpp")
foreach(_exe_only IN ITEMS "${YICAD_MAIN_ENTRY}" "${YICAD_BUILTIN_EXTENSIONS}")
    if(NOT EXISTS "${_exe_only}")
        message(FATAL_ERROR "未找到只编进可执行目标的源文件 ${_exe_only}")
    endif()
    list(REMOVE_ITEM YICAD_SHELL_SOURCES "${_exe_only}")
endforeach()

# #####################################################
# ======================main===========================
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/shell/YiCAD.rc.cmake ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/YiCAD.rc)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/shell/icon.ico DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(MAIN_RES ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/YiCAD.rc)

# #####################################################
# =================plugin SDK==========================
set(PLUGIN_SDK_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shell/plugin_runtime/YiCadPluginAbi.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shell/plugin_runtime/YiCadPluginSdk.h)

# SDK 仅传播公开头文件、C++ 标准和插件导出宏，不链接 YiCAD 或 Qt 内部库。
add_library(YiCadPluginSdk INTERFACE)
add_library(YiCAD::PluginSdk ALIAS YiCadPluginSdk)
# 安装导出时与 YiCAD:: 命名空间组合为 YiCAD::PluginSdk。
set_target_properties(YiCadPluginSdk PROPERTIES EXPORT_NAME PluginSdk)
# 构建树使用源码目录，安装后使用稳定的公开头文件目录。
target_include_directories(YiCadPluginSdk INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/shell/plugin_runtime>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/YiCAD/plugin-sdk>")
target_compile_features(YiCadPluginSdk INTERFACE cxx_std_23)
target_compile_definitions(YiCadPluginSdk INTERFACE YICAD_PLUGIN_BUILD)
target_sources(YiCadPluginSdk INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/shell/plugin_runtime/YiCadPluginAbi.h>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/shell/plugin_runtime/YiCadPluginSdk.h>")

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

# 将.ui转成.h：src/ui、src/shell 与各扩展的表单都用下方的 yicad_wrap_ui 生成。
# Qt 6 的 uic 默认按函数指针生成表单里的信号槽连接，槽要在表单根控件的类
# （QDialog、QWidget）上才能编译；本项目的 .ui 连接的是子类里声明的槽，
# 所以沿用 Qt 5 的字符串连接（SIGNAL/SLOT），运行期行为与原来一致。
set(YICAD_UIC_OPTIONS --connections string)

# 将.qrc转成.cpp（主程序的资源，编进 YiCadShell；扩展的资源编进各扩展库）
qt6_add_resources(qrc_FILES ${qrc_DIRS})

# ---------------------------------------------------------------------------
# yicad_wrap_ui(<输出变量> <输出目录> <表单> [<表单> ...])
#
# 与 qt6_wrap_ui 相同，但 ui_*.h 生成到给定目录：YiCadUi、YiCadShell 与每个扩展的表单
# 各放在自己的目录里，别的库与扩展看不到。
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

# 主程序的扫描范围：各分区的全部源文件、src/ui 与 src/shell 的表单，加上 Main.cpp。src/extensions/ 下
# 的源码不在此列，由各扩展自己的 ts 覆盖（见下方）。
set(YICAD_LUPDATE_SOURCES
    ${YICAD_BASE_HEADERS}        ${YICAD_BASE_SOURCES}
    ${YICAD_MODEL_HEADERS}       ${YICAD_MODEL_SOURCES}
    ${YICAD_RENDER_HEADERS}      ${YICAD_RENDER_SOURCES}
    ${YICAD_APPLICATION_HEADERS} ${YICAD_APPLICATION_SOURCES}
    ${YICAD_UI_HEADERS}          ${YICAD_UI_SOURCES}          ${YICAD_UI_FORMS}
    ${YICAD_SHELL_HEADERS}       ${YICAD_SHELL_SOURCES}       ${YICAD_SHELL_FORMS}
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
# ================ YiCadBase / YiCadModel ================
#
# BASE、MODEL 两个分区各自是静态库，依赖方向由 CMake 强制：YiCadModel -> YiCadBase。
# 分层重组 S2 之前还有一层 YiCadPersistence（现 model/io/），它只依赖 Model，已并入
# YiCadModel；YiCadBase 原名 YiCadMath。
#
# 用 STATIC 而非 OBJECT：这两层没有 .qrc，也没有靠全局对象自注册的代码，不会有
# YiCadShell 注释里说的目标文件被丢掉的问题；STATIC 让它们能被
# tests/math、tests/geometry、tests/persistence 单独链接，不必拖入上层。
# #####################################################

add_library(YiCadBase STATIC
    ${YICAD_BASE_HEADERS} ${YICAD_BASE_SOURCES})
# QT_NO_UNICODE_DEFINES：Qt 6 的 Qt6::Platform 在 Windows 上给每个使用方加 UNICODE 与
# _UNICODE（Qt 5 的 CMake 目标不加）。这会把 muParser 的头文件切到 std::wstring 接口，
# 与 Conan 按窄字符构建的 muparser 链接不上，也会把 Win32 的 TCHAR 宏切到 W 版本。
# 本项目沿用 Qt 5 下的行为，所有链接 Qt 的目标都设置这一属性（该属性按使用方目标读取，
# 不会沿依赖传播，扩展、可执行目标与测试目标各自再设一次）。
set_target_properties(YiCadBase PROPERTIES FOLDER "YiCAD" QT_NO_UNICODE_DEFINES TRUE)

target_include_directories(YiCadBase PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/base/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/base/debug
    ${CMAKE_CURRENT_SOURCE_DIR}/src/base/geometry)

# 公共编译定义与选项在最底层设置一次，经 PUBLIC 依赖链自动传播到
# 上层库 -> 可执行目标/测试目标，不必在每一层重复声明。
target_compile_definitions(YiCadBase PUBLIC
    _USE_MATH_DEFINES
    _ENABLE_EXTENDED_ALIGNED_STORAGE
    APPDIR="YiCAD"
    # 屏蔽 Qt 6.0 及以前废弃的 API，用到即编译失败，防止旧写法回流
    QT_DISABLE_DEPRECATED_UP_TO=0x060000)
remove_definitions(-D_HAS_STD_BYTE)

# MSVC: 强制源文件和执行字符集为 UTF-8，避免中文乱码。
if(MSVC)
    target_compile_options(YiCadBase PUBLIC /utf-8)
endif()

# base/core 的序列化机制（Tools.h/Reader.h/Writer.h/gzstream.h/MinizipNgArchive.h 等）
# 需要 Qt Core、pugixml、minizip、zlib；muParser 用于 Math2d::eval() 的表达式求值。
if(TARGET nlohmann_json::nlohmann_json)
    target_link_libraries(YiCadBase PUBLIC nlohmann_json::nlohmann_json)
endif()
if(TARGET Eigen3::Eigen)
    target_link_libraries(YiCadBase PUBLIC Eigen3::Eigen)
endif()
if(TARGET glm::glm)
    target_link_libraries(YiCadBase PUBLIC glm::glm)
endif()
if(TARGET Boost::boost)
    target_link_libraries(YiCadBase PUBLIC Boost::boost)
endif()
if(TARGET ZLIB::ZLIB)
    target_link_libraries(YiCadBase PUBLIC ZLIB::ZLIB)
endif()
if(TARGET pugixml::pugixml)
    target_link_libraries(YiCadBase PUBLIC pugixml::pugixml)
endif()
if(TARGET MINIZIP::minizip)
    target_link_libraries(YiCadBase PUBLIC MINIZIP::minizip)
endif()
if(TARGET muparser::muparser)
    target_link_libraries(YiCadBase PUBLIC muparser::muparser)
else()
    message(FATAL_ERROR "muparser::muparser target not found. "
        "Run 'conan install' with --build=never before configuring CMake.")
endif()
target_link_libraries(YiCadBase PUBLIC Qt6::Core)

add_library(YiCadModel STATIC
    ${YICAD_MODEL_HEADERS} ${YICAD_MODEL_SOURCES})
set_target_properties(YiCadModel PROPERTIES FOLDER "YiCAD" QT_NO_UNICODE_DEFINES TRUE)

target_include_directories(YiCadModel PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/document
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/property
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/entity
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/entity/dimension
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/entity/text
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/entity_data
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/entity_data/dimension
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/entity_data/text
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/table
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/history
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/edit
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/algorithm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/io
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/io/meta)

target_link_libraries(YiCadModel PUBLIC YiCadBase)

# ConstrainedDelaunayTriangulation（model/algorithm/）需要 CDT；DmPen/DmMText 等少量
# 类型用到 QColor/QFont，需要 Qt Gui；DmFont（model/entity/text/）直接用 FreeType
# 量取字形轮廓，需要 Freetype。
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

# #####################################################
# ======= YiCadRender / YiCadApplication / YiCadUi / YiCadShell =======
#
# 分层重组 S6 把原先合编的 YiCadCore 拆成这四个库，依赖方向由 CMake 保证：每个库只看得到
# 自己与下层的头文件，在下层包含上层的头文件直接编译失败（C1083）。
#
# 库类型（分层重组 D3）：Render、Application、Ui 用 STATIC，依赖沿链接传递，使用方只链接
# 最上面的那一个，链接器只取用到的目标文件。静态库的代价是：没人引用、靠全局对象的构造
# 函数自己生效的目标文件会被链接器丢掉而且不报错。这三个库里没有这样的代码（没有 .qrc，
# 没有自注册；uic 生成的是头文件里的普通代码，不受影响）。以后往这三个目录加 .qrc 或
# 自注册代码时，要改为显式引用（如 Q_INIT_RESOURCE），或把库改为 OBJECT。
# Shell 用 OBJECT，原因见 YiCadShell。
#
# 后续阶段做 C++ SDK 时，除 Shell 外改为 SHARED（LAYER_RESTRUCTURE_PLAN.md 11.1 节）。
# #####################################################

# ---- YiCadRender：画布与 OpenGL 绘制，只依赖 YiCadModel ----
add_library(YiCadRender STATIC
    ${YICAD_RENDER_HEADERS} ${YICAD_RENDER_SOURCES})
set_target_properties(YiCadRender PROPERTIES FOLDER "YiCAD" QT_NO_UNICODE_DEFINES TRUE)

target_include_directories(YiCadRender PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/render/painter
    ${CMAKE_CURRENT_SOURCE_DIR}/src/render/opengl
    ${CMAKE_CURRENT_SOURCE_DIR}/src/render/view)

# 画布 GuiDocumentView 是 QOpenGLWidget；GL* 直接调 GLEW 与 OpenGL。
target_link_libraries(YiCadRender PUBLIC
    YiCadModel Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets)
if(TARGET GLEW::GLEW)
    target_link_libraries(YiCadRender PUBLIC GLEW::GLEW)
endif()
if(TARGET OpenGL::GL)
    target_link_libraries(YiCadRender PUBLIC OpenGL::GL OpenGL::GLU)
endif()

# ---- YiCadApplication：命令与视图工具机制、扩展框架、交互视图 UIView，只依赖 YiCadRender 及以下 ----
# application/view/ 以外的文件不得包含 application/view/：同在一个库里，CMake 管不到，
# 由 tools/check_layering.py 检查。
add_library(YiCadApplication STATIC
    ${YICAD_APPLICATION_HEADERS} ${YICAD_APPLICATION_SOURCES})
set_target_properties(YiCadApplication PROPERTIES FOLDER "YiCAD" QT_NO_UNICODE_DEFINES TRUE)

target_include_directories(YiCadApplication PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/application
    ${CMAKE_CURRENT_SOURCE_DIR}/src/application/framework
    ${CMAKE_CURRENT_SOURCE_DIR}/src/application/view)

# Commands.cpp 用 QDomDocument 读 keyconfig.xml。
target_link_libraries(YiCadApplication PUBLIC YiCadRender Qt6::Xml)

# ---- YiCadUi：扩展与壳层共用的控件与对话框，只依赖 YiCadApplication 及以下 ----
# 扩展链接本库，看不到 shell/。
#
# 表单生成的 ui_*.h 放在 <构建目录>/ui，公开给使用方：UIWidgetPen.h 等公开头文件直接包含它们。
#
# 预编译头 YiCadPch.h 由本库编译，YiCadShell、各扩展与可执行目标 REUSE_FROM 本库。
# Render、Application 不用预编译头（分层重组 S6，见 LAYER_RESTRUCTURE_PLAN.md 10.5 节的测量）。
yicad_wrap_ui(YICAD_UI_FORM_HEADERS "${CMAKE_CURRENT_BINARY_DIR}/ui" ${YICAD_UI_FORMS})

add_library(YiCadUi STATIC
    ${YICAD_UI_HEADERS} ${YICAD_UI_SOURCES}
    ${YICAD_UI_FORMS} ${YICAD_UI_FORM_HEADERS})
set_target_properties(YiCadUi PROPERTIES FOLDER "YiCAD" QT_NO_UNICODE_DEFINES TRUE)

target_include_directories(YiCadUi PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/forms
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ribbon)

target_link_libraries(YiCadUi PUBLIC YiCadApplication)

target_precompile_headers(YiCadUi PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/YiCadPch.h")

# ---- YiCadShell：把各层组装成程序的外壳 ----
# 主窗口、图纸选项卡、命令行、宿主服务实现、Ribbon 装配器、C ABI 插件运行时。依赖全部
# 核心库，不引用任何扩展（扩展注册表 BuiltinExtensions.cpp 只编进可执行目标）。
#
# 用 OBJECT：Ribbon 图标的 ribbon_icon.qrc 编进本库，rcc 生成的 qrc_*.cpp 靠全局对象的
# 构造函数注册资源，全仓没有 Q_INIT_RESOURCE 显式引用它，静态库会把它丢掉而不报错
# （Ribbon 图标变空白）。OBJECT 库的目标文件只从直接链接处带入、不沿依赖传递，
# 可执行目标与 test_interaction 直接链接本库。
#
# 表单生成的 ui_*.h 放在 <构建目录>/shell，只有本库看得到。
yicad_wrap_ui(YICAD_SHELL_FORM_HEADERS "${CMAKE_CURRENT_BINARY_DIR}/shell" ${YICAD_SHELL_FORMS})

add_library(YiCadShell OBJECT
    ${YICAD_SHELL_HEADERS} ${YICAD_SHELL_SOURCES}
    ${YICAD_SHELL_FORMS} ${YICAD_SHELL_FORM_HEADERS}
    ${qrc_FILES})
set_target_properties(YiCadShell PROPERTIES FOLDER "YiCAD" QT_NO_UNICODE_DEFINES TRUE)

# 可执行目标（Main.cpp 包含 ApplicationWindow.h）与 test_interaction 要看到 shell/ 的头文件。
target_include_directories(YiCadShell
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/shell
        ${CMAKE_CURRENT_SOURCE_DIR}/src/shell/plugin_runtime
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/shell)

# 主窗口与 Ribbon 装配器用 SARibbonBar；UITabDrawWidget 用 QSvgGenerator 导出 SVG。
target_link_libraries(YiCadShell PUBLIC YiCadUi SARibbonBar::SARibbonBar Qt6::Svg)

target_precompile_headers(YiCadShell REUSE_FROM YiCadUi)

# #####################################################
# ============== 进程内扩展 YiCadExt_<扩展> ==============
#
# src/extensions/ 下每个子目录建一个 OBJECT 库（与 YiCadShell 同样选 OBJECT：扩展里有
# Qt 资源，静态库会丢掉没有显式引用的资源注册）。扩展链接 YiCadUi（内核、命令机制、
# 界面控件），看不到 shell/ 的头文件，核心库不依赖任何扩展；扩展只看得到自己的头文件与
# uic 生成的 ui_*.h，扩展之间互相包含、包含 shell/ 都会直接编译失败（主计划 7.11 节，
# 分层重组 S6）。
#
# 除 YiCadUi 带来的之外，扩展还可以用 Qt Svg、Qt Network 与 Windows 的 crypt32（AI 扩展
# 调模型接口、加密保存密钥，文字扩展渲染 SVG），原先由合编的 YiCadCore 一并提供。
#
# 头文件目录是 PUBLIC 的：链接扩展的只有可执行目标（BuiltinExtensions.cpp 要包含
# 扩展入口的头文件）与测试。
#
# 预编译头复用 YiCadUi 的 YiCadPch.h：拆出来之前扩展的源码一直带着它编译。
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

    target_link_libraries(${_ext_target} PUBLIC YiCadUi Qt6::Svg Qt6::Network)
    if(WIN32)
        target_link_libraries(${_ext_target} PUBLIC crypt32.lib)
    endif()
    target_precompile_headers(${_ext_target} REUSE_FROM YiCadUi)
    set_target_properties(${_ext_target} PROPERTIES FOLDER "YiCAD/extensions" QT_NO_UNICODE_DEFINES TRUE)
    GroupSourcesByFolder(${_ext_target})

    list(APPEND YICAD_EXTENSION_TARGETS ${_ext_target})
endforeach()
set_property(GLOBAL PROPERTY YICAD_EXTENSION_TARGETS ${YICAD_EXTENSION_TARGETS})

# #####################################################
# ================ YiCAD 可执行目标 ====================
# 只编译 main() 与扩展注册表，其余目标文件来自 YiCadShell、各扩展库与三个核心静态库。
# #####################################################
add_executable(${PROJECT_NAME} ${YICAD_MAIN_ENTRY}
                               ${YICAD_BUILTIN_EXTENSIONS}
                               ${MAIN_RES}
                               ${qrc_DIRS}
                               ${QM_FILES})

# 链接 OBJECT 库会同时带入它的目标文件和全部使用要求（include 路径、
# 编译定义、依赖库），无需再写 $<TARGET_OBJECTS:YiCadShell>。目标文件只从直接链接的
# OBJECT 库带入，所以 YiCadShell 与扩展库要逐个直接链接；YiCadUi 及以下是静态库，
# 沿依赖传递过来。
target_link_libraries(${PROJECT_NAME} PRIVATE YiCadShell ${YICAD_EXTENSION_TARGETS})

target_precompile_headers(${PROJECT_NAME} REUSE_FROM YiCadUi)
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
# 该目录同时是 ARCHIVE 输出目录，静态库和 DLL/插件的导入库 .lib、导出文件 .exp
# 都落在这里，运行期用不到；按扩展名排除，复用的构建目录里残留的旧库也一并挡掉。
INSTALL(DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT Runtime
        PATTERN "test_*" EXCLUDE
        PATTERN "*.lib" EXCLUDE
        PATTERN "*.exp" EXCLUDE)

#structure files
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src/ui" PREFIX "UIs" FILES ${YICAD_UI_FORMS})
GroupSourcesByFolder(YiCadBase)
GroupSourcesByFolder(YiCadModel)
GroupSourcesByFolder(YiCadRender)
GroupSourcesByFolder(YiCadApplication)
GroupSourcesByFolder(YiCadUi)
GroupSourcesByFolder(YiCadShell)
