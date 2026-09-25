/// @file test_modify_commands.cpp
/// @brief 修改与查询命令（业务工具化第三步第④批）的单元测试
///
/// 对照原 Action 的行为：注册类型、第一步提示、选项条、右键结束、命令行
/// 选项、拾取实体后的提示与高亮，以及迁移时修正的"预览隐藏的实体结束后仍不可见"。
/// 不执行提交（见 CommandTestFixture.h）。

#include <gtest/gtest.h>

#include <vector>

#include "CircleData.h"
#include "DmCircle.h"
#include "DmLine.h"
#include "DmPolyline.h"
#include "LineData.h"
#include "ModifyBevelCommand.h"
#include "ModifyRoundCommand.h"
#include "ModifySingleOffsetCommand.h"
#include "PolylineData.h"
#include "support/CommandTestFixture.h"

using namespace yicad_test;

namespace
{
struct FirstStep
{
    const char* id;
    const char* hint;  ///< 为空：原 Action 没有按键提示
    const char* right;
};

const FirstStep kCommands[] = {
    {"ext.measure.dist", "Specify first point of distance", "Cancel"},
    {"ext.measure.angle", "Specify first line", "Cancel"},
    {"ext.measure.area", "Specify first point of polygon", "Cancel"},
    {"ext.edit.paste", "Set reference point", "Cancel"},
    {"ext.modify.entity", "Click on entity to modify", "Cancel"},
    {"ext.modify.cut", "Specify entity to cut", "Cancel"},
    {"ext.modify.cut_2p", nullptr, nullptr},
    {"ext.modify.single_offset", "Choose the original entity", ""},
    {"ext.modify.polyline_add", "Specify polyline to add nodes", "Cancel"},
    {"ext.modify.polyline_append", "Specify the polyline somewhere near the beginning or end point",
     "Cancel"},
    {"ext.modify.polyline_del", "Specify polyline to delete node", "Cancel"},
    {"ext.modify.trim", "Select entitys", "Back"},
    {"ext.modify.bevel", "Specify first entity", "Back"},
    {"ext.modify.round", "Specify first entity", "Back"},
    {"ext.modify.extend", nullptr, nullptr},
};

struct ModifyFixture : BuiltinCommandFixture
{
    template <typename Command>
    Command* active() const
    {
        return dynamic_cast<Command*>(bus.activeCommand());
    }

    DmLine* addLine(const DmVector& p1, const DmVector& p2)
    {
        auto* line = new DmLine(nullptr, LineData(p1, p2));
        line->calculateBorders();
        EXPECT_TRUE(doc.getEntityTable()->add_direct(line));
        return line;
    }

    DmPolyline* addPolyline(const std::vector<DmVector>& pts, bool closed)
    {
        const size_t segments = closed ? pts.size() : pts.size() - 1;
        std::vector<double> bulges(segments, 0.0);
        std::vector<double> weights(segments * 2, 0.0);
        auto* poly = new DmPolyline(nullptr, PolylineData(pts, bulges, weights, closed));
        poly->update(); // 由顶点生成各段子实体，拾取按子实体计算距离
        EXPECT_TRUE(doc.getEntityTable()->add_direct(poly));
        return poly;
    }
};
}  // namespace

TEST_F(ModifyFixture, 注册为交互命令)
{
    for (const FirstStep& step : kCommands)
    {
        SCOPED_TRACE(step.id);
        EXPECT_EQ(CommandRegistry::instance().kind(step.id), CommandKind::Exclusive);
    }
}

TEST_F(ModifyFixture, 启动后给出第一步提示右键结束)
{
    for (const FirstStep& step : kCommands)
    {
        SCOPED_TRACE(step.id);
        ASSERT_TRUE(start(step.id));
        if (step.hint)
        {
            EXPECT_EQ(ui.lastHint(), QString::fromLatin1(step.hint));
            EXPECT_EQ(ui.lastRightHint(), QString::fromLatin1(step.right));
        }
        rightClick();
        EXPECT_FALSE(bus.hasActiveCommand());
    }
}

TEST_F(ModifyFixture, 倒角与圆角打开选项条结束时收起)
{
    for (const char* id : {"ext.modify.bevel", "ext.modify.round"})
    {
        SCOPED_TRACE(id);
        ASSERT_TRUE(start(id));
        ASSERT_NE(lastOptions(id), nullptr);
        EXPECT_TRUE(lastOptions(id)->on);
        EXPECT_FALSE(lastOptions(id)->update);
        endCommand();
        EXPECT_FALSE(lastOptions(id)->on);
    }
}

TEST_F(ModifyFixture, 倒角命令行设置长度)
{
    ASSERT_TRUE(start("ext.modify.bevel"));
    auto* bevel = active<ModifyBevelCommand>();
    ASSERT_NE(bevel, nullptr);

    EXPECT_TRUE(typeText(QStringLiteral("length1")));
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Enter length 1:"));
    EXPECT_TRUE(typeText(QStringLiteral("7")));
    EXPECT_DOUBLE_EQ(bevel->length1(), 7.0);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify first entity"));
    ASSERT_NE(lastOptions("ext.modify.bevel"), nullptr);
    EXPECT_TRUE(lastOptions("ext.modify.bevel")->update);

    // 输入无效时提示并回到原来的一步，长度不变
    EXPECT_TRUE(typeText(QStringLiteral("length1")));
    EXPECT_FALSE(typeText(QStringLiteral("abc")));
    EXPECT_EQ(ui.messages.back(), QStringLiteral("Not a valid expression"));
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify first entity"));
    EXPECT_DOUBLE_EQ(bevel->length1(), 7.0);

    // Commands::checkCommand 对 help/close/undo 以外的关键字都返回 true：
    // 排在前面的 "length1" 接住了所有文本，"length2" 与 "trim" 到不了（与原 Action 一致）
    EXPECT_TRUE(typeText(QStringLiteral("trim")));
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Enter length 1:"));
    EXPECT_TRUE(bevel->isTrimOn());
    endCommand();
}

TEST_F(ModifyFixture, 圆角命令行设置半径)
{
    ASSERT_TRUE(start("ext.modify.round"));
    auto* round = active<ModifyRoundCommand>();
    ASSERT_NE(round, nullptr);

    EXPECT_TRUE(typeText(QStringLiteral("radius")));
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Enter radius:"));
    EXPECT_TRUE(typeText(QStringLiteral("3")));
    EXPECT_DOUBLE_EQ(round->radius(), 3.0);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify first entity"));

    // 同上："radius" 接住了所有文本，"trim" 到不了
    EXPECT_TRUE(typeText(QStringLiteral("trim")));
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Enter radius:"));
    EXPECT_TRUE(round->isTrimOn());
    endCommand();
}

TEST_F(ModifyFixture, 单个偏移选中实体后按选项条距离预览)
{
    DmLine* line = addLine(DmVector(0, 0), DmVector(10, 0));
    ASSERT_TRUE(start("ext.modify.single_offset"));
    ASSERT_NE(lastOptions("ext.modify.single_offset"), nullptr);
    EXPECT_TRUE(lastOptions("ext.modify.single_offset")->on);
    auto* offsetCommand = active<ModifySingleOffsetCommand>();
    ASSERT_NE(offsetCommand, nullptr);
    EXPECT_DOUBLE_EQ(offsetCommand->distance(), 30.0);

    click(5, 0);
    EXPECT_TRUE(line->isHighlighted());
    move(5, 5);
    ASSERT_EQ(previewCount(), 1);

    // 选项条（UIModifyOffsetOptions::updateDist）改写距离，下一次移动按新距离预览
    offsetCommand->setDistance(2.0);
    move(5, 6);
    ASSERT_EQ(previewCount(), 1);
    DmEntity* offset = view.getPreviewContainer()->entityAt(0);
    ASSERT_NE(offset, nullptr);
    EXPECT_NEAR(offset->getStartpoint().y, 2.0, 1e-9);

    // 只有一步：右键结束，取消高亮并收起选项条
    rightClick();
    EXPECT_FALSE(bus.hasActiveCommand());
    EXPECT_FALSE(line->isHighlighted());
    EXPECT_FALSE(lastOptions("ext.modify.single_offset")->on);
}

TEST_F(ModifyFixture, 修剪只响应小键盘回车且不接受按键)
{
    ASSERT_TRUE(start("ext.modify.trim"));
    EXPECT_FALSE(pressKey(Qt::Key_Return));
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Select entitys"));
    EXPECT_FALSE(pressKey(Qt::Key_Enter));
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Select entity to be cut"));
    EXPECT_FALSE(pressKey(Qt::Key_Enter));
    EXPECT_FALSE(bus.hasActiveCommand());
}

TEST_F(ModifyFixture, 修剪预览隐藏的实体在退回和结束时恢复可见)
{
    DmLine* boundary = addLine(DmVector(0, -10), DmVector(0, 10));
    DmLine* target = addLine(DmVector(-10, 0), DmVector(10, 0));
    ASSERT_TRUE(start("ext.modify.trim"));
    click(0, 5);
    EXPECT_TRUE(boundary->isHighlighted());
    pressKey(Qt::Key_Enter);

    move(5, 0);
    EXPECT_FALSE(target->isVisible());
    EXPECT_GT(previewCount(), 0);

    // 右键退回第一步：原 Action 不恢复，实体一直不可见
    rightClick();
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Select entitys"));
    EXPECT_TRUE(target->isVisible());

    pressKey(Qt::Key_Enter);
    move(5, 0);
    EXPECT_FALSE(target->isVisible());
    endCommand();
    EXPECT_TRUE(target->isVisible());
    EXPECT_FALSE(boundary->isHighlighted());
}

TEST_F(ModifyFixture, 延伸预览隐藏的实体在结束时恢复可见)
{
    DmLine* boundary = addLine(DmVector(10, -10), DmVector(10, 10));
    DmLine* target = addLine(DmVector(0, 0), DmVector(5, 0));
    // 命令开始时有选中的实体：以它们为边界
    boundary->setSelected(true);
    ASSERT_TRUE(start("ext.modify.extend"));

    move(4, 0);
    EXPECT_FALSE(target->isVisible());
    ASSERT_EQ(previewCount(), 1);
    DmEntity* extended = view.getPreviewContainer()->entityAt(0);
    ASSERT_NE(extended, nullptr);
    EXPECT_NEAR(extended->getEndpoint().x, 10.0, 1e-9);

    endCommand();
    EXPECT_TRUE(target->isVisible());
}

TEST_F(ModifyFixture, 多段线节点命令只接受多段线)
{
    addLine(DmVector(0, 0), DmVector(10, 0));
    for (const char* id : {"ext.modify.polyline_add", "ext.modify.polyline_append", "ext.modify.polyline_del"})
    {
        SCOPED_TRACE(id);
        ASSERT_TRUE(start(id));
        click(50, 50);
        EXPECT_EQ(ui.messages.back(), QStringLiteral("No Entity found."));
        click(5, 0);
        EXPECT_EQ(ui.messages.back(), QStringLiteral("Entity must be a polyline."));
        EXPECT_TRUE(bus.hasActiveCommand());
        endCommand();
    }
}

TEST_F(ModifyFixture, 追加节点从近端开始预览右键结束)
{
    DmPolyline* poly = addPolyline({DmVector(0, 0), DmVector(10, 0), DmVector(10, 10)}, false);
    ASSERT_TRUE(start("ext.modify.polyline_append"));
    click(10, 8);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify next point"));
    EXPECT_EQ(view.getRelativeZero(), DmVector(10, 10));
    move(20, 10);
    EXPECT_GT(previewCount(), 0);
    EXPECT_EQ(poly->getVertexCount(), 3);

    // 原 Action 右键在任何一步都直接结束
    rightClick();
    EXPECT_FALSE(bus.hasActiveCommand());
    EXPECT_EQ(previewCount(), 0);
}

TEST_F(ModifyFixture, 追加节点不接受闭合多段线)
{
    addPolyline({DmVector(0, 0), DmVector(10, 0), DmVector(10, 10)}, true);
    ASSERT_TRUE(start("ext.modify.polyline_append"));
    click(10, 5);
    EXPECT_EQ(ui.messages.back(), QStringLiteral("Can not append nodes in a closed polyline."));
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify the polyline somewhere near the beginning or end point"));
    endCommand();
}

TEST_F(ModifyFixture, 添加与删除节点选中多段线后高亮结束或退回时取消)
{
    DmPolyline* poly = addPolyline({DmVector(0, 0), DmVector(10, 0), DmVector(10, 10)}, false);

    ASSERT_TRUE(start("ext.modify.polyline_add"));
    click(5, 0);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify adding node's point"));
    EXPECT_TRUE(poly->isHighlighted());
    endCommand();
    EXPECT_FALSE(poly->isHighlighted());

    ASSERT_TRUE(start("ext.modify.polyline_del"));
    click(5, 0);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify deleting node's point"));
    EXPECT_TRUE(poly->isHighlighted());
    rightClick();
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify polyline to delete node"));
    EXPECT_FALSE(poly->isHighlighted());
    EXPECT_TRUE(bus.hasActiveCommand());
    endCommand();
}

TEST_F(ModifyFixture, 查询距离输出结果后回到第一步)
{
    ASSERT_TRUE(start("ext.measure.dist"));
    typeCoordinate(0, 0);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify second point of distance"));
    move(3, 4);
    EXPECT_EQ(previewCount(), 1);
    typeCoordinate(3, 4);
    ASSERT_FALSE(ui.messages.empty());
    EXPECT_TRUE(ui.messages.back().startsWith(QStringLiteral("Distance: 5")));
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify first point of distance"));
    EXPECT_EQ(previewCount(), 0);
    EXPECT_TRUE(bus.hasActiveCommand());
    endCommand();
}

TEST_F(ModifyFixture, 查询角度选两条线后输出夹角)
{
    DmLine* first = addLine(DmVector(0, 0), DmVector(10, 0));
    addLine(DmVector(0, 0), DmVector(0, 10));
    ASSERT_TRUE(start("ext.measure.angle"));
    move(5, 0);
    EXPECT_TRUE(first->isHighlighted());
    click(5, 0);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify second line"));
    click(0, 5);
    ASSERT_FALSE(ui.messages.empty());
    EXPECT_TRUE(ui.messages.back().startsWith(QStringLiteral("Angle: ")));
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify first line"));
    endCommand();
    EXPECT_FALSE(first->isHighlighted());
}

TEST_F(ModifyFixture, 查询面积回到已有的点时闭合并回到第一步)
{
    ASSERT_TRUE(start("ext.measure.area"));
    typeCoordinate(0, 0);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify next point of polygon"));
    typeCoordinate(10, 0);
    typeCoordinate(10, 10);
    typeCoordinate(0, 0);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify first point of polygon"));
    bool closing = false;
    bool area = false;
    for (const QString& m : ui.messages)
    {
        closing = closing || m.startsWith(QStringLiteral("Closing Point: "));
        area = area || m.startsWith(QStringLiteral("Area: 50"));
    }
    EXPECT_TRUE(closing);
    EXPECT_TRUE(area);
    endCommand();
}

TEST_F(ModifyFixture, 打断选中实体后高亮结束时取消)
{
    DmLine* line = addLine(DmVector(0, 0), DmVector(10, 0));
    ASSERT_TRUE(start("ext.modify.cut"));
    click(5, 0);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify cutting point"));
    EXPECT_TRUE(line->isHighlighted());
    endCommand();
    EXPECT_FALSE(line->isHighlighted());
}

TEST_F(ModifyFixture, 修改实体属性选中实体并弹出对话框)
{
    DmLine* line = addLine(DmVector(0, 0), DmVector(10, 0));
    auto* circle = new DmCircle(nullptr, CircleData(DmVector(50, 0), 5));
    circle->calculateBorders();
    ASSERT_TRUE(doc.getEntityTable()->add_direct(circle));

    ASSERT_TRUE(start("ext.modify.entity"));
    click(5, 0);
    EXPECT_TRUE(line->isSelected());
    // 绘图扩展登记的属性编辑命令弹出直线的属性对话框；对话框是模态的，本命令留着
    EXPECT_EQ(dialogs.shown, std::vector<QString>{QStringLiteral("UIDlgLine")});
    EXPECT_TRUE(bus.hasActiveCommand());

    // 接着点下一个实体
    click(45, 0);
    EXPECT_EQ(dialogs.shown, (std::vector<QString>{QStringLiteral("UIDlgLine"), QStringLiteral("UIDlgCircle")}));
    EXPECT_TRUE(bus.hasActiveCommand());
    endCommand();
}

TEST_F(ModifyFixture, 绘图扩展登记几类实体的属性对话框)
{
    const CommandRegistry& registry = CommandRegistry::instance();
    EXPECT_EQ(registry.kind(QStringLiteral("ext.draw.properties")), CommandKind::Instant);
    EXPECT_EQ(registry.instantInterrupt(QStringLiteral("ext.draw.properties")), InstantInterrupt::KeepAll);
    for (DM::EntityType type : {DM::EntityPoint, DM::EntityLine, DM::EntityArc, DM::EntityCircle, DM::EntityEllipse,
                                DM::EntitySpline, DM::EntityPolyline, DM::EntityImage})
    {
        EXPECT_EQ(registry.propertyEditor(type), QStringLiteral("ext.draw.properties")) << type;
    }

    // 没有实体时什么也不做
    EXPECT_TRUE(registry.runInstant(QStringLiteral("ext.draw.properties"), CommandContext{&doc, &view}));
    EXPECT_TRUE(dialogs.shown.empty());
}

TEST_F(ModifyFixture, 粘贴剪贴板为空时指定参考点即结束)
{
    ASSERT_TRUE(start("ext.edit.paste"));
    typeCoordinate(0, 0);
    EXPECT_FALSE(bus.hasActiveCommand());
}
