# tests/interaction —— 对应阶段2引入的交互层工具化
# （IViewTool / ViewToolControl / PanZoomTool），落在阶段3的 YiCadInteraction
# 分区（kernel/actions/）。

yicad_add_test(interaction
    SOURCES
        test_view_tool_control.cpp
        test_pan_zoom_tool.cpp
        test_select_tool.cpp
        test_legacy_action_tool.cpp
        test_command_registry.cpp
        test_command_dispatch.cpp
        test_extension_manager.cpp
        test_ribbon_registry.cpp
    LABELS "interaction;unit")
