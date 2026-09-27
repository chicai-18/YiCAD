# tests/interaction —— 交互层：命令与视图工具的机制（src/application/，含扩展框架
# src/application/framework/ 与 keyconfig.xml 的解析 Commands）、内置命令与各扩展的命令，
# 插件命令与按钮接入宿主注册表（src/shell/plugin_runtime/PluginUiAdapter），文档经
# DmDocumentListener 通知画布（分层重组 S3），文档的存盘策略 DocumentFileService（S4c），
# 持有文档与存盘策略的每文档对象 AppDocument（选择集方案第 2 步），以及文档的选择集
# SelectionSet（第 3 步）。
# 业务工具化第四步之前这些机制在 kernel/actions/。

yicad_add_test(interaction
    SOURCES
        test_environment.cpp
        test_view_tool_control.cpp
        test_pan_zoom_tool.cpp
        test_draw_line_commands.cpp
        test_draw_curve_commands.cpp
        test_modify_commands.cpp
        test_select_tool.cpp
        test_edit_tool.cpp
        test_document_listener.cpp
        test_document_file_service.cpp
        test_app_document.cpp
        test_selection_set.cpp
        test_command_registry.cpp
        test_command_dispatch.cpp
        test_keyconfig.cpp
        test_coordinate_input.cpp
        test_exclusive_command_bus.cpp
        test_select_first_commands.cpp
        test_extension_manager.cpp
        test_host_extensions.cpp
        test_block_extension.cpp
        test_text_extension.cpp
        test_hatch_extension.cpp
        test_dim_extension.cpp
        test_command_extensions.cpp
        test_ribbon_registry.cpp
        test_plugin_ui_adapter.cpp
        test_dxf_encoding.cpp
    # 用例用到壳层：UIActionHandler（test_command_dispatch、test_command_registry）、
    # UIRibbonManager（test_ribbon_registry、test_plugin_ui_adapter）与插件运行时
    # （test_plugin_ui_adapter、test_dxf_encoding）。YiCadUi 及以下是静态库，随它带入。
    LINK YiCadShell
    LABELS "interaction;unit")

# 各扩展是独立的 OBJECT 库（YiCAD/CMakeLists.txt 的 YiCadExt_<扩展>），用例要启动扩展、
# 包含扩展的命令头文件。OBJECT 库的目标文件只从直接链接的库带入，所以逐个链接。
get_property(_yicad_extension_targets GLOBAL PROPERTY YICAD_EXTENSION_TARGETS)
target_link_libraries(test_interaction PRIVATE ${_yicad_extension_targets})

# test_keyconfig 读源码树里的默认 keyconfig.xml（程序目录下的那份由 cmake --install 复制）
target_compile_definitions(test_interaction PRIVATE
    YICAD_DEFAULT_KEYCONFIG="${CMAKE_CURRENT_SOURCE_DIR}/../../YiCAD/support/config/keyconfig.xml")

# test_dxf_encoding 加载构建出来的 DXF 插件，走与程序相同的插件运行时；插件先于测试构建
add_dependencies(test_interaction YiCadDxfPlugin)
target_compile_definitions(test_interaction PRIVATE
    YICAD_DXF_PLUGIN_DLL="$<TARGET_FILE:YiCadDxfPlugin>")
