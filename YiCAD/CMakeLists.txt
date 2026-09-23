# 设置cmake 最低版本
cmake_minimum_required (VERSION 3.21)

PROJECT (YiCAD)

# 加载Qt模块
# Qt 5.15 is discovered from the user's installation via CMAKE_PREFIX_PATH or Qt5_DIR.
set(YICAD_USE_QT_MODULES Widgets Gui Core PrintSupport Designer Svg Network)

find_package(Qt5 5.15 REQUIRED COMPONENTS ${YICAD_USE_QT_MODULES})

set(CMAKE_AUTOMOC ON)
SET(QT_USE_QTDESIGNER ON)

# Qt5 imported targets (Qt5::Core, Qt5::Widgets, etc.) carry their include
# directories transitively. No need for explicit include_directories().

# Dependencies come from find_package in cmake/dependencies.cmake.
# The Conan-generated *Config.cmake files provide imported targets that
# carry their own include directories.

# 设置.qrc目录
set(qrc_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/res/ribbon/ribbon_icon.qrc)

# #####################################################
# ================== 源文件清单 ========================
#
# 源文件按 doc/ARCHITECTURE_EVOLUTION_PLAN.md 6.3 节的目标库边界分成七个
# 分区收集。唯一的事实来源是目录结构：文件放进哪个目录就归哪个库。
#
# 全部加了 CONFIGURE_DEPENDS：新增或删除源文件时构建系统会自己重新收集，
# 不会出现「文件建好了但没进构建」（方案 P5）。
#
# 阶段 0 里七个分区全部喂给同一个 YiCadCore OBJECT 库，依赖方向尚未由构建
# 系统强制。阶段 3 拆库时发现 RENDER/INTERACTION/UI/APP 四个分区之间存在
# 真实的双向依赖（见下方各分区注释与 6.7 节），只有 MATH/MODEL/PERSISTENCE
# 三个分区能拆成三个独立静态库，其余四个分区仍合编进 YiCadCore。
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
# 的白名单已清空。DmCachePainter.{h,cpp} 已迁到 kernel/gui（本就是纯 GL
# 封装，放错了目录）；DmImage.{h,cpp} 对 GuiDocumentView.h 的引用也是死
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
# kernel/fileio/ 不在此列——见下方 APP 分区的说明，FileIO 对插件运行时的
# 依赖使它物理上只能归 APP。DmDocument 对具体读写的调用已改为经
# GuiDialogFactoryInterface 注入，不再直接依赖本分区。
yicad_collect_sources(PERSISTENCE
    src/kernel/persistence
    src/kernel/persistence/Meta
    src/kernel/filters)

# 对应阶段 3 的 YiCadRender。计划图未列出 kernel/printing/，按依赖方向归此。
#
# 本分区内部实现不在本方案排期内（渲染专项另行规划），此处仅作层次边界。
#
# 与方案不同：本分区在实现上与 INTERACTION 分区互相持有对方的具体类型
# （GuiDocumentView 构造并持有 ViewToolControl/PanZoomTool/LegacyActionTool，
# GuiEventHandler 直接持有 Action 栈），是阶段 1、2 已落地设计的一部分，
# 不是本次拆库引入的问题。这两个分区因此不能拆成两个独立静态库，仍与
# INTERACTION/UI/APP 一起编入 YiCadCore。详见 6.7 节。
yicad_collect_sources(RENDER
    src/kernel/painters
    src/kernel/painters/opengl
    src/kernel/gui
    src/kernel/printing)

# 对应阶段 3 的 YiCadInteraction。
#
# 计划图未列出 src/cmd/（命令行输入解析），它与 Ribbon 同为命令入口，归此。
# 阶段 1（视图解耦）与阶段 2（交互层工具化）的改动集中在本分区。
#
# src/actions/ 下 106 个具体 Action 里有 40 余个直接包含 UI（对话框、
# 文件选择器）与 APP（ApplicationWindow/MDIWindow）头文件——这是 Action
# orchestrate 用户交互的正常需要，但意味着本分区不能反过来独立于 UI/APP
# 之下。见上方 RENDER 分区说明与 6.7 节。
yicad_collect_sources(INTERACTION
    src/kernel/actions
    src/actions
    src/cmd)

# 对应阶段 3 的 YiCadUi。
#
# UI 组件里有多处（UIActionHandler、UIBottomWidget、UICurrentActivePen、
# UILineTypeBox、UITabDrawWidget 等）直接调用 ApplicationWindow/MDIWindow
# 取全局状态（当前文档、当前 MDI 标签），是与 APP 互相依赖的既有设计。
yicad_collect_sources(UI
    src/ui
    src/ui/forms)

# 对应阶段 3 的 YiCadApp（可执行目标）。
#
# 计划图未列出 src/ai/ 与 src/plugin_runtime/，二者均位于 UI 之上：
#   plugin_runtime/   引用 UIActionHandler.h、UICommandWidget.h
# 阶段 4 第一阶段已把 ai/AIAssistant.h 里"引用 ApplicationWindow /
# MDIWindow"这条依赖去掉了（构造期只需要一个 QWidget*），第二阶段把
# ai/ 抽成了首个 IExtension 试点扩展（见 src/ai/AIExtension.{h,cpp}）。
#
# src/main/extensions/（IExtension/IExtensionContext/ExtensionManager）
# 是新增的进程内扩展框架，物理上和 ApplicationWindow 同属 src/main，
# 只是单独开了个子目录；file(GLOB) 不递归，子目录必须显式列在这里。
#
# kernel/fileio/（FileIO 门面类）归入本分区而非 PERSISTENCE：它的
# getFilters() 之外还要 setPluginRuntime() 接入 plugin_runtime/ 的
# PluginRegistry/PluginManager/HostApi，物理上依赖 APP 分区。这是已有
# 的、通过 setPluginRuntime() 显式注入的设计，不是本次拆库引入的问题；
# 只是它决定了 Fileio.{h,cpp} 不能编进只应依赖 Model 的 YiCadPersistence。
yicad_collect_sources(APP
    src/main
    src/main/extensions
    src/ai
    src/plugin_runtime
    src/kernel/fileio)

# src/ui 下的 Qt Designer 表单，由 QT5_WRAP_UI 生成对应的 ui_*.h。
file(GLOB YICAD_UI_FORMS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/*.ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/forms/*.ui)

# main() 必须留在可执行目标里，否则测试二进制无法提供自己的入口。
set(YICAD_MAIN_ENTRY "${CMAKE_CURRENT_SOURCE_DIR}/src/main/Main.cpp")
list(REMOVE_ITEM YICAD_APP_SOURCES "${YICAD_MAIN_ENTRY}")
if(NOT EXISTS "${YICAD_MAIN_ENTRY}")
    message(FATAL_ERROR "未找到应用入口 ${YICAD_MAIN_ENTRY}")
endif()

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
file(GLOB TS_FILES "${CMAKE_CURRENT_SOURCE_DIR}/ts/*.ts")

# 将.ui转成.h
QT5_WRAP_UI(ui_FILES ${YICAD_UI_FORMS})

# 将.qrc转成.cpp
qt5_add_resources(qrc_FILES ${qrc_DIRS})

# 附加库路径(lib路径)
find_package(Qt5LinguistTools REQUIRED)

# lupdate 扫描范围：与拆分前保持一致（七个分区的全部源文件，加上 Main.cpp 与 .rc）。
set (ALL_RSC_FILES ${YICAD_MATH_HEADERS}        ${YICAD_MATH_SOURCES}
                   ${YICAD_MODEL_HEADERS}       ${YICAD_MODEL_SOURCES}
                   ${YICAD_PERSISTENCE_HEADERS} ${YICAD_PERSISTENCE_SOURCES}
                   ${YICAD_RENDER_HEADERS}      ${YICAD_RENDER_SOURCES}
                   ${YICAD_INTERACTION_HEADERS} ${YICAD_INTERACTION_SOURCES}
                   ${YICAD_UI_SOURCES}
                   ${YICAD_APP_HEADERS}         ${YICAD_APP_SOURCES}
                   ${YICAD_MAIN_ENTRY}
                   ${MAIN_RES}
                     )

if(UPDATE_TRANSLATIONS)
    QT5_CREATE_TRANSLATION_Exclude_SubFolder(QM_FILES ${ALL_RSC_FILES} ${TS_FILES})
else()
    QT5_ADD_TRANSLATION(QM_FILES ${TS_FILES})
endif(UPDATE_TRANSLATIONS )

SET(QM_FILES ${QM_FILES} PARENT_SCOPE)
ADD_CUSTOM_TARGET (TS_YiCAD ALL DEPENDS ${QM_FILES})
set_target_properties(TS_YiCAD PROPERTIES FOLDER "CMakePredefinedTargets")

# #####################################################
# ============ YiCadMath / YiCadModel / YiCadPersistence ============
#
# 阶段 3 把 MATH/MODEL/PERSISTENCE 三个分区拆成三个独立静态库，依赖方向由
# CMake 强制：YiCadPersistence -> YiCadModel -> YiCadMath。RENDER/
# INTERACTION/UI/APP 四个分区因 6.7 节所述的既有循环依赖，继续合编进
# YiCadCore（见下）。
#
# 用 STATIC 而非 OBJECT：这三层不含任何 Qt 元对象类（Q_OBJECT），没有
# YiCadCore 注释里提到的自动注册符号丢失问题；STATIC 让它们能被
# tests/math、tests/geometry、tests/persistence 单独链接，不必拖入整个
# YiCadCore（这也是本次拆库最主要的增量构建收益来源）。
# #####################################################

add_library(YiCadMath STATIC
    ${YICAD_MATH_HEADERS} ${YICAD_MATH_SOURCES})
set_target_properties(YiCadMath PROPERTIES FOLDER "YiCAD")

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
    APPDIR="YiCAD")
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
target_link_libraries(YiCadMath PUBLIC Qt5::Core)

add_library(YiCadModel STATIC
    ${YICAD_MODEL_HEADERS} ${YICAD_MODEL_SOURCES})
set_target_properties(YiCadModel PROPERTIES FOLDER "YiCAD")

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
target_link_libraries(YiCadModel PUBLIC Qt5::Gui)

# DmSystem.cpp 在 Windows 上用 DirectWrite 枚举系统字体文件路径。
if(WIN32)
    target_link_libraries(YiCadModel PUBLIC Dwrite.lib)
endif()

add_library(YiCadPersistence STATIC
    ${YICAD_PERSISTENCE_HEADERS} ${YICAD_PERSISTENCE_SOURCES})
set_target_properties(YiCadPersistence PROPERTIES FOLDER "YiCAD")

target_include_directories(YiCadPersistence PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/persistence
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/persistence/Meta
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/filters)

target_link_libraries(YiCadPersistence PUBLIC YiCadModel)

# #####################################################
# ==================== YiCadCore ======================
#
# RENDER + INTERACTION + UI + APP 四个分区。可执行目标和测试二进制共享
# 同一批目标文件，测试因此不必重新编译这部分代码，也不必为每个测试手工
# 维护源文件子集。
#
# 选 OBJECT 而非 STATIC：静态库的符号剥离会丢掉 Qt 的静态注册（资源初始化、
# 插件注册宏等自动注册符号没有显式引用），OBJECT 库没有这个问题。
#
# 这四个分区未按方案拆成四个独立静态库：见上方 RENDER/INTERACTION/UI 分区
# 注释与 6.7 节，GuiDocumentView/GuiEventHandler（RENDER）与
# ViewToolControl/PanZoomTool/SelectTool/LegacyActionTool/ActionInterface
# （INTERACTION）互相持有对方的具体类型，UI 与 APP 之间同样存在大量
# ApplicationWindow/MDIWindow 互相调用，是阶段 1、2 与既有代码留下的真实
# 双向依赖，不是本次拆库能靠移动文件解决的问题。
# #####################################################
add_library(YiCadCore OBJECT
    ${YICAD_RENDER_HEADERS}      ${YICAD_RENDER_SOURCES}
    ${YICAD_INTERACTION_HEADERS} ${YICAD_INTERACTION_SOURCES}
    ${YICAD_UI_HEADERS}          ${YICAD_UI_SOURCES}
    ${YICAD_APP_HEADERS}         ${YICAD_APP_SOURCES}
    ${YICAD_UI_FORMS}
    ${ui_FILES}
    ${qrc_FILES})

set_target_properties(YiCadCore PROPERTIES FOLDER "YiCAD")

# 头文件搜索路径挂在目标上（PUBLIC），测试目标通过链接 YiCadCore 继承。
target_include_directories(YiCadCore PUBLIC
    # uic 生成的 ui_*.h 落在构建目录
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/actions
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cmd
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/actions
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/fileio
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/gui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/printing
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/painters
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/painters/opengl
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main/extensions
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugin_runtime
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ai
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/forms)

target_precompile_headers(YiCadCore PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/main/YiCadPch.h")

# ---- YiCadCore 的依赖 ----
# 全部用 PUBLIC：可执行目标与测试目标都需要这些库的头文件和符号。
# YiCadMath/YiCadModel/YiCadPersistence 需要的第三方库已经在各自层级声明，
# 经 PUBLIC 依赖链传播到这里，此处只声明 RENDER/INTERACTION/UI/APP
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
    target_link_libraries(YiCadCore PUBLIC Qt5::${qtmod})
endforeach(qtmod)

# #####################################################
# ================ YiCAD 可执行目标 ====================
# 只编译 main()，其余目标文件来自 YiCadCore。
# #####################################################
add_executable(${PROJECT_NAME} ${YICAD_MAIN_ENTRY}
                               ${MAIN_RES}
                               ${qrc_DIRS}
                               ${QM_FILES})

# 链接 OBJECT 库会同时带入它的目标文件和全部使用要求（include 路径、
# 编译定义、依赖库），无需再写 $<TARGET_OBJECTS:YiCadCore>。
target_link_libraries(${PROJECT_NAME} PRIVATE YiCadCore)

target_precompile_headers(${PROJECT_NAME} REUSE_FROM YiCadCore)

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

# ai
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../README.md
        DESTINATION ${CMAKE_INSTALL_BINDIR}/ai/
        COMPONENT Runtime)
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/support/ai/
        DESTINATION ${CMAKE_INSTALL_BINDIR}/ai/
        COMPONENT Runtime
        FILES_MATCHING PATTERN "*.*")

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
