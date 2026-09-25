/// @file test_command_extensions.cpp
/// @brief 原内置命令拆成的五个扩展（业务工具化第四步）的单元测试
///
/// 覆盖：各扩展注册的命令类型与打断方式、原内置 ID 不再存在、按钮挂进宿主占位的面板
/// 且顺序与迁移前一致（多段线面板里节点按钮排到云线之后，见 9.4 节）、选项条随命令注册
/// （样条的容器高度 26）、宿主按 ID 调用的命令都在。命令各自的交互见
/// test_draw_line_commands、test_draw_curve_commands、test_modify_commands 与
/// test_select_first_commands。

#include <gtest/gtest.h>

#include <variant>

#include "CommandRegistry.h"
#include "ExtensionManager.h"
#include "UIRibbonRegistry.h"
#include "support/CommandExtensions.h"

using namespace yicad_test;

namespace
{
/// @brief 面板里按钮的命令 ID，按注册顺序
QStringList commandsOf(const UIRibbonRegistry& ribbon, const char* panelId)
{
    QStringList out;
    for (const UIRibbonEntry* entry : ribbon.entriesOf(panelId))
    {
        if (const auto* action = std::get_if<UIRibbonActionDef>(entry))
        {
            out.append(action->commandId);
        }
    }
    return out;
}

/// @brief 启动五个扩展的夹具
struct CommandExtensionsFixture : ::testing::Test
{
    CommandExtensionsScope extensions;
};
}  // namespace

TEST_F(CommandExtensionsFixture, 各扩展的命令类型与打断方式)
{
    const CommandRegistry& registry = CommandRegistry::instance();
    for (const char* id : {"ext.draw.line", "ext.draw.point", "ext.draw.ellipse_arc_axis", "ext.draw.image",
                           "ext.modify.move", "ext.modify.delete", "ext.modify.polyline_add",
                           "ext.modify.copy_to_layer", "ext.measure.dist", "ext.measure.total_length",
                           "ext.edit.copy", "ext.edit.cut", "ext.edit.paste"})
    {
        EXPECT_EQ(registry.kind(id), CommandKind::Exclusive) << id;
    }
    for (const char* id : {"ext.modify.delete_no_select", "ext.measure.selected", "ext.edit.undo", "ext.edit.redo",
                           "ext.view.zoom_in", "ext.view.zoom_out"})
    {
        EXPECT_EQ(registry.kind(id), CommandKind::Instant) << id;
    }
    EXPECT_EQ(registry.kind("ext.view.pan"), CommandKind::ViewTool);

    // 缩放不打断任何命令（原视图 Action），其余即时命令照旧结束不可打断的命令
    EXPECT_EQ(registry.instantInterrupt("ext.view.zoom_in"), InstantInterrupt::KeepAll);
    EXPECT_EQ(registry.instantInterrupt("ext.view.zoom_out"), InstantInterrupt::KeepAll);
    EXPECT_EQ(registry.instantInterrupt("ext.edit.undo"), InstantInterrupt::EndUninterruptible);
    EXPECT_EQ(registry.instantInterrupt("ext.modify.delete_no_select"), InstantInterrupt::EndUninterruptible);
}

TEST_F(CommandExtensionsFixture, 原内置ID不再存在)
{
    const CommandRegistry& registry = CommandRegistry::instance();
    for (const char* id : {"draw.line", "draw.circle", "modify.move", "modify.delete_no_select", "polyline.add",
                           "info.dist", "info.selected", "edit.copy", "edit.undo", "zoom.in", "zoom.pan"})
    {
        EXPECT_FALSE(registry.hasCommand(id)) << id;
    }
}

TEST_F(CommandExtensionsFixture, 按钮挂进宿主占位的面板且顺序与迁移前一致)
{
    const UIRibbonRegistry& ribbon = extensions.host.ribbon;
    using namespace UIRibbonIds;
    EXPECT_EQ(commandsOf(ribbon, kPanelDraw2dLine),
              (QStringList{"ext.draw.line", "ext.draw.line_rectangle", "ext.draw.line_bisector",
                           "ext.draw.line_tangent1", "ext.draw.line_tangent2", "ext.draw.line_orth_tan",
                           "ext.draw.line_polygon_cen_cor", "ext.draw.line_polygon_cen_tan", "ext.draw.ray",
                           "ext.draw.xline"}));
    EXPECT_EQ(commandsOf(ribbon, kPanelDraw2dCurve),
              (QStringList{"ext.draw.arc", "ext.draw.arc_3p", "ext.draw.arc_tangential", "ext.draw.spline",
                           "ext.draw.spline_points", "ext.draw.line_free"}));
    // 节点按钮归修改扩展，排到绘图扩展的云线按钮之后（原先在多段线与云线之间）
    EXPECT_EQ(commandsOf(ribbon, kPanelDraw2dPolyline),
              (QStringList{"ext.draw.polyline", "ext.draw.cloud_line_rectangle", "ext.draw.cloud_line_polygon",
                           "ext.draw.cloud_line_free", "ext.modify.polyline_add", "ext.modify.polyline_append",
                           "ext.modify.polyline_del"}));
    EXPECT_EQ(commandsOf(ribbon, kPanelDraw2dCircle),
              (QStringList{"ext.draw.circle", "ext.draw.circle_2p", "ext.draw.circle_3p", "ext.draw.circle_tan2",
                           "ext.draw.circle_tan3"}));
    EXPECT_EQ(commandsOf(ribbon, kPanelDraw2dEllipse),
              (QStringList{"ext.draw.ellipse_axis", "ext.draw.ellipse_inscribe"}));
    EXPECT_EQ(commandsOf(ribbon, kPanelDraw2dOther), QStringList{"ext.draw.image"});
    EXPECT_EQ(commandsOf(ribbon, kPanelDraw2dModify),
              (QStringList{"ext.modify.copy", "ext.modify.move", "ext.modify.rotate", "ext.modify.scale",
                           "ext.modify.mirror", "ext.modify.trim", "ext.modify.extend", "ext.modify.single_offset",
                           "ext.modify.bevel", "ext.modify.round", "ext.modify.cut", "ext.modify.cut_2p",
                           "ext.modify.entity", "ext.modify.explode"}));
    EXPECT_EQ(commandsOf(ribbon, kPanelDraw2dMeasure),
              (QStringList{"ext.measure.dist", "ext.measure.angle", "ext.measure.total_length",
                           "ext.measure.area"}));
}

TEST_F(CommandExtensionsFixture, 选项条随命令注册)
{
    const CommandRegistry& registry = CommandRegistry::instance();
    for (const char* id : {"ext.draw.line", "ext.draw.polyline", "ext.draw.line_polygon_cen_cor",
                           "ext.draw.line_polygon_cen_tan", "ext.draw.line_bisector", "ext.draw.arc",
                           "ext.draw.arc_tangential", "ext.draw.circle_tan2", "ext.draw.spline",
                           "ext.draw.spline_points", "ext.draw.cloud_line_rectangle", "ext.draw.cloud_line_polygon",
                           "ext.draw.cloud_line_free", "ext.draw.image", "ext.modify.bevel", "ext.modify.round",
                           "ext.modify.single_offset"})
    {
        EXPECT_TRUE(static_cast<bool>(registry.commandOptionsFactory(id))) << id;
    }
    for (const char* id : {"ext.draw.circle", "ext.draw.ray", "ext.modify.move", "ext.measure.dist"})
    {
        EXPECT_FALSE(static_cast<bool>(registry.commandOptionsFactory(id))) << id;
    }
    // 原先对话框工厂里写死的容器高度：样条 26，其余 23
    EXPECT_EQ(registry.commandOptionsHeight("ext.draw.spline"), 26);
    EXPECT_EQ(registry.commandOptionsHeight("ext.draw.spline_points"), 26);
    EXPECT_EQ(registry.commandOptionsHeight("ext.draw.line"), 23);
    EXPECT_EQ(registry.commandOptionsHeight("ext.modify.bevel"), 23);
}

TEST(CommandExtensionsTest, 关闭扩展后命令随之注销)
{
    {
        CommandExtensionsScope extensions;
        EXPECT_TRUE(CommandRegistry::instance().hasCommand("ext.draw.line"));
    }
    // 宿主按 ID 调用的入口（Delete 键、撤销、图层面板的复制到图层）此时什么也不做
    for (const char* id : {"ext.draw.line", "ext.modify.delete_no_select", "ext.edit.undo", "ext.modify.copy_to_layer"})
    {
        EXPECT_FALSE(CommandRegistry::instance().hasCommand(id)) << id;
    }
}
