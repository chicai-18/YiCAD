# tests/interaction —— 对应阶段2引入的交互层工具化
# （IViewTool / ViewToolControl / PanZoomTool），落在阶段3的 YiCadInteraction
# 分区（kernel/actions/）。

yicad_add_test(interaction
    SOURCES
        test_view_tool_control.cpp
        test_pan_zoom_tool.cpp
        test_zoom_pan_tool.cpp
        test_draw_line_commands.cpp
        test_draw_curve_commands.cpp
        test_modify_commands.cpp
        test_select_tool.cpp
        test_legacy_action_tool.cpp
        test_command_registry.cpp
        test_command_dispatch.cpp
        test_coordinate_input.cpp
        test_exclusive_command_bus.cpp
        test_select_first_commands.cpp
        test_extension_manager.cpp
        test_host_extensions.cpp
        test_block_extension.cpp
        test_text_extension.cpp
        test_hatch_extension.cpp
        test_dim_extension.cpp
        test_ribbon_registry.cpp
    LABELS "interaction;unit")
