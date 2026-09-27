/// @file test_modify_commands.cpp
/// @brief 修改与查询命令（业务工具化第三步第④批）的单元测试
///
/// 对照原 Action 的行为：注册类型、第一步提示、选项条、右键结束、命令行
/// 选项、拾取实体后的提示与高亮，以及迁移时修正的"预览隐藏的实体结束后仍不可见"。
/// 不执行提交（见 CommandTestFixture.h）。

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <vector>

#include "BaseExclusiveCommand.h"
#include "CircleData.h"
#include "CmdManager.h"
#include "DmBlockReference.h"
#include "DmCircle.h"
#include "DmClipboard.h"
#include "DmDimLinear.h"
#include "DmDimensionStyleTable.h"
#include "DmDocumentListener.h"
#include "DmLayer.h"
#include "DmLayerTable.h"
#include "DmLine.h"
#include "DmLineTypeTable.h"
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

/// @brief 什么也不做的交互命令，充当属性面板那样的交互式属性编辑命令
class PanelCommand : public BaseExclusiveCommand
{
protected:
    bool onActivate() override { return true; }
    void onDeactivate() override {}
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

TEST_F(ModifyFixture, 圆角提交后取消全部高亮)
{
    DmLine* first = addLine(DmVector(0, 0), DmVector(10, 0));
    DmLine* second = addLine(DmVector(0, 0), DmVector(0, 10));
    ASSERT_TRUE(start("ext.modify.round"));
    auto* round = active<ModifyRoundCommand>();
    ASSERT_NE(round, nullptr);
    round->setRadius(2.0);
    // 不裁剪：两条线都不被修改。原先第一条线的高亮靠修改命令清位，这时就留下了
    round->setTrim(false);

    move(5, 0);
    click(5, 0);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify second entity"));
    move(0, 5);
    EXPECT_TRUE(highlight().contains(first));
    EXPECT_TRUE(highlight().contains(second));

    click(0, 5);
    EXPECT_EQ(doc.getEntityTable()->count(), 3);  // 加了一段圆弧，两条线不变
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify first entity"));
    EXPECT_TRUE(highlight().entities().empty());
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
    EXPECT_TRUE(highlight().contains(line));
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
    EXPECT_TRUE(highlight().entities().empty());
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
    EXPECT_TRUE(highlight().contains(boundary));
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
    EXPECT_TRUE(highlight().entities().empty());
}

TEST_F(ModifyFixture, 修剪掉一条边界后它不再高亮)
{
    DmLine* vertical = addLine(DmVector(0, -10), DmVector(0, 10));
    DmLine* horizontal = addLine(DmVector(-10, 0), DmVector(10, 0));
    DmLine* other = addLine(DmVector(20, -10), DmVector(20, 10));
    ASSERT_TRUE(start("ext.modify.trim"));
    click(0, 5);
    click(5, 0);
    EXPECT_TRUE(highlight().contains(vertical));
    EXPECT_TRUE(highlight().contains(horizontal));

    // 选完边界时光标下还没点选的实体，进入下一步后不再高亮
    move(20, 0);
    EXPECT_TRUE(highlight().contains(other));
    pressKey(Qt::Key_Enter);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Select entity to be cut"));
    EXPECT_FALSE(highlight().contains(other));

    // 以水平线为界剪掉竖线的上半段：竖线就地修改（id 不变），Modification::trim 把它移出边界列表。
    // 原先由修改命令清位，现在工具按边界列表重设高亮集
    click(0, 5);
    EXPECT_NEAR(std::max(vertical->getStartpoint().y, vertical->getEndpoint().y), 0.0, 1e-9);
    EXPECT_EQ(doc.getEntityTable()->find(vertical->getId()), vertical);
    EXPECT_FALSE(highlight().contains(vertical));
    EXPECT_TRUE(highlight().contains(horizontal));
    EXPECT_TRUE(bus.hasActiveCommand());

    endCommand();
    EXPECT_TRUE(highlight().entities().empty());
}

TEST_F(ModifyFixture, 延伸预览隐藏的实体在结束时恢复可见)
{
    DmLine* boundary = addLine(DmVector(10, -10), DmVector(10, 10));
    DmLine* target = addLine(DmVector(0, 0), DmVector(5, 0));
    // 命令开始时有选中的实体：以它们为边界
    selection.add(boundary);
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
    EXPECT_TRUE(highlight().contains(poly));
    endCommand();
    EXPECT_FALSE(highlight().contains(poly));

    ASSERT_TRUE(start("ext.modify.polyline_del"));
    click(5, 0);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify deleting node's point"));
    EXPECT_TRUE(highlight().contains(poly));
    rightClick();
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify polyline to delete node"));
    EXPECT_FALSE(highlight().contains(poly));
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
    EXPECT_TRUE(highlight().contains(first));
    click(5, 0);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify second line"));
    click(0, 5);
    ASSERT_FALSE(ui.messages.empty());
    EXPECT_TRUE(ui.messages.back().startsWith(QStringLiteral("Angle: ")));
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify first line"));
    endCommand();
    EXPECT_TRUE(highlight().entities().empty());
}

TEST_F(ModifyFixture, 查询角度先悬停第二条线再点量完后取消高亮)
{
    DmLine* first = addLine(DmVector(0, 0), DmVector(10, 0));
    DmLine* second = addLine(DmVector(0, 0), DmVector(0, 10));
    ASSERT_TRUE(start("ext.measure.angle"));
    move(5, 0);
    click(5, 0);
    move(0, 5);
    EXPECT_TRUE(highlight().contains(first));
    EXPECT_TRUE(highlight().contains(second));

    // 原先量完回到第一步后第一条线一直高亮，结束命令也不恢复
    click(0, 5);
    EXPECT_TRUE(ui.messages.back().startsWith(QStringLiteral("Angle: ")));
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify first line"));
    EXPECT_TRUE(highlight().entities().empty());
    EXPECT_TRUE(bus.hasActiveCommand());

    // 光标还在第二条线上：不移动直接再点，它作为第一条线高亮
    click(0, 5);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify second line"));
    EXPECT_EQ(highlight().entities(), std::vector<DmEntity*>{second});
    endCommand();
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
    int changed = 0;
    QObject::connect(&highlight(), &HighlightSet::changed, [&changed]() { ++changed; });
    ASSERT_TRUE(start("ext.modify.cut"));
    // 原先只重绘、不重建缓存，选中的实体不立即变色；现在高亮集改变即通知视图（UIView 重建缓存并重绘）
    click(5, 0);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify cutting point"));
    EXPECT_EQ(highlight().entities(), std::vector<DmEntity*>{line});
    EXPECT_EQ(changed, 1);
    endCommand();
    EXPECT_TRUE(highlight().entities().empty());
    EXPECT_EQ(changed, 2);
}

TEST_F(ModifyFixture, 修改实体属性选中实体并弹出对话框)
{
    DmLine* line = addLine(DmVector(0, 0), DmVector(10, 0));
    auto* circle = new DmCircle(nullptr, CircleData(DmVector(50, 0), 5));
    circle->calculateBorders();
    ASSERT_TRUE(doc.getEntityTable()->add_direct(circle));

    ASSERT_TRUE(start("ext.modify.entity"));
    click(5, 0);
    EXPECT_TRUE(selection.contains(line));
    // 绘图扩展登记的属性编辑命令弹出直线的属性对话框；对话框是模态的，本命令留着
    EXPECT_EQ(dialogs.shown, std::vector<QString>{QStringLiteral("UIDlgLine")});
    EXPECT_TRUE(bus.hasActiveCommand());

    // 接着点下一个实体
    click(45, 0);
    EXPECT_EQ(dialogs.shown, (std::vector<QString>{QStringLiteral("UIDlgLine"), QStringLiteral("UIDlgCircle")}));
    EXPECT_TRUE(bus.hasActiveCommand());
    endCommand();
}

TEST_F(ModifyFixture, 修改实体属性由交互式属性编辑命令接替)
{
    // 多行文字的属性面板是交互命令（文字扩展不在本夹具里）：这里把直线的属性编辑换成一个交互命令
    CommandRegistry& registry = CommandRegistry::instance();
    // 用例中途失败也要注销，否则下一个用例里绘图扩展登记不上直线的属性编辑（ext.draw.properties
    // 随扩展在每个用例重新注册）
    struct Unregister
    {
        ~Unregister() { CommandRegistry::instance().unregisterCommand(QStringLiteral("test.modify.panel")); }
    } unregister;
    ASSERT_TRUE(registry.unregisterCommand(QStringLiteral("ext.draw.properties")));
    ASSERT_TRUE(registry.registerExclusiveCommand(QStringLiteral("test.modify.panel"),
                                                  [](const CommandContext&) { return std::make_unique<PanelCommand>(); }));
    ASSERT_TRUE(registry.registerPropertyEditor(DM::EntityLine, QStringLiteral("test.modify.panel")));

    DmLine* line = addLine(DmVector(0, 0), DmVector(10, 0));
    ASSERT_TRUE(start("ext.modify.entity"));
    click(5, 0);
    EXPECT_TRUE(selection.contains(line));
    EXPECT_TRUE(dialogs.shown.empty());
    // 修改实体命令让位，由属性编辑命令接替
    EXPECT_EQ(bus.activeCommandId(), QStringLiteral("test.modify.panel"));

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
    EXPECT_TRUE(registry.runInstant(QStringLiteral("ext.draw.properties"), CommandContext{&doc, &view, &selection}));
    EXPECT_TRUE(dialogs.shown.empty());
}

TEST_F(ModifyFixture, 粘贴剪贴板为空时指定参考点即结束)
{
    ASSERT_TRUE(start("ext.edit.paste"));
    typeCoordinate(0, 0);
    EXPECT_FALSE(bus.hasActiveCommand());
}

TEST_F(ModifyFixture, 粘贴别的图纸复制来的标注时标注改归本文档)
{
    // 剪贴板把放进来的实体改归自己的文档（DmClipboard::addEntity），复制来源的图纸先关闭也能粘贴。
    // 粘贴预览改归本文档：同名的图层、标注样式用本文档的；标注更新时从所属文档取箭头块（S4a），
    // 箭头也取本文档的。这里只测预览（移动即更新），提交见下一个用例。
    {
        DmDocument source;
        auto* dim = new DmDimLinear(nullptr,
                                    DmDimensionData(DmVector(20.0, -10.0), DmVector(10.0, -10.0),
                                                    EMTextVertMode::kTextVertMid, EMTextHorzMode::kTextCenter, 1.0,
                                                    QString(), 0.0, source.getDimStyleTable()->getActive()),
                                    DmDimLinearData(DmVector(0.0, 0.0), DmVector(20.0, 0.0)));
        dim->setDocument(&source);
        dim->update();
        ASSERT_TRUE(source.getEntityTable()->add_direct(dim));
        DMCLIPBOARD->clear();
        DMCLIPBOARD->addEntity(dim->clone());
    }  // 来源图纸在粘贴之前关闭

    ASSERT_TRUE(start("ext.edit.paste"));
    move(10, 10);

    DmDimLinear* pasted = nullptr;
    for (DmEntity* e : *view.getPreviewContainer())
    {
        if (e->getEntityType() == DM::EntityDimLinear)
        {
            pasted = static_cast<DmDimLinear*>(e);
        }
    }
    ASSERT_NE(pasted, nullptr);
    EXPECT_EQ(pasted->getDocument(), &doc);
    EXPECT_EQ(pasted->getLayer(false), doc.getLayerTable()->find(QStringLiteral("0"))) << "同名图层用本文档的";
    EXPECT_EQ(pasted->getStyle(), doc.getDimStyleTable()->find(QStringLiteral("ISO-25"))) << "同名标注样式用本文档的";
    // getSubEntities() 把箭头块参照展开成图元，图元的父实体才是箭头块参照
    std::vector<DmBlockReference*> arrows;
    for (DmEntity* sub : pasted->getSubEntities())
    {
        DmEntity* parent = sub->getParent();
        if (parent && parent != pasted && parent->getEntityType() == DM::EntityBlockReference &&
            std::find(arrows.begin(), arrows.end(), parent) == arrows.end())
        {
            arrows.push_back(static_cast<DmBlockReference*>(parent));
        }
    }
    ASSERT_EQ(arrows.size(), 2u);
    for (DmBlockReference* arrow : arrows)
    {
        EXPECT_EQ(arrow->getData().blockSource, doc.getDimStyleTable()->getArrowBlocks());
    }

    endCommand();
    DMCLIPBOARD->clear();
}

TEST_F(ModifyFixture, 粘贴提交时只复制用到的图层并随撤销移除)
{
    // 来源图纸有两个图层，只把其中一个图层上的直线放进剪贴板，来源图纸随后关闭。粘贴提交后本文档
    // 只多出这个图层，直线属于本文档、挂在它上面；撤销把直线与图层一起撤掉（图层经命令加入事务）。
    // 与夹具说明不同，这里走了事务：Transaction 先 start 再 add 没有问题，崩溃的是不开事务直接
    // 调 add()（CmdManager 没有当前命令，test_geometry_spatial_query 的情形）
    {
        DmDocument source;
        auto addLayer = [&source](const QString& name) {
            auto* layer = new DmLayer();
            layer->setDocument(&source);
            layer->setData(
                DmLayerData(name, DmPen(DmColor(255, 0, 0), DM::Width05, DmLineTypeTable::Continuous), false, false));
            EXPECT_TRUE(source.getLayerTable()->add_direct(layer));
            return layer;
        };
        DmLayer* used = addLayer(QStringLiteral("用到"));
        addLayer(QStringLiteral("没用到"));
        auto* line = new DmLine(DmVector(0.0, 0.0), DmVector(10.0, 0.0));
        line->setDocument(&source);
        line->setLayer(used);
        ASSERT_TRUE(source.getEntityTable()->add_direct(line));
        DMCLIPBOARD->clear();
        DMCLIPBOARD->addEntity(line->clone());
    }

    ASSERT_TRUE(start("ext.edit.paste"));
    typeCoordinate(5, 5);
    EXPECT_FALSE(bus.hasActiveCommand());

    ASSERT_EQ(doc.getEntityTable()->count(), 1);
    DmLayer* pastedLayer = doc.getLayerTable()->find(QStringLiteral("用到"));
    ASSERT_NE(pastedLayer, nullptr);
    EXPECT_EQ(pastedLayer->getPen().getColor().red(), 255);
    EXPECT_EQ(doc.getLayerTable()->find(QStringLiteral("没用到")), nullptr);
    DmEntity* pasted = *doc.getEntityTable()->begin();
    EXPECT_EQ(pasted->getDocument(), &doc);
    EXPECT_EQ(pasted->getLayer(false), pastedLayer);
    EXPECT_NEAR(pasted->getStartpoint().x, 5.0, 1e-9);
    EXPECT_NEAR(pasted->getStartpoint().y, 5.0, 1e-9);

    doc.getCmdManager()->undo();
    EXPECT_EQ(doc.getEntityTable()->count(), 0);
    EXPECT_EQ(doc.getLayerTable()->find(QStringLiteral("用到")), nullptr);
    DMCLIPBOARD->clear();
}

// 删除、移动、复制与剪切的提交把调用方收集的选中实体交给 Modification，操作后的选中状态由
// 调用方处理（doc/SELECTION_SET_PLAN.md 第 1 步）；下面几例锁定提交后的选中状态与改前一致。
// 与上一例相同，这里走了事务。

TEST_F(ModifyFixture, 删除提交只删选中实体撤销后恢复为未选中)
{
    DmLine* selected = addLine(DmVector(0.0, 0.0), DmVector(10.0, 0.0));
    DmLine* other = addLine(DmVector(0.0, 5.0), DmVector(10.0, 5.0));
    selection.add(selected);

    ASSERT_TRUE(start("ext.modify.delete"));
    // 删除总是先进入选择阶段（test_select_first_commands 的 P8），回车用已有的选择集
    pressKey(Qt::Key_Enter);
    EXPECT_FALSE(bus.hasActiveCommand());
    EXPECT_TRUE(selected->isErased());
    EXPECT_FALSE(other->isErased());

    doc.getCmdManager()->undo();
    EXPECT_FALSE(selected->isErased());
    EXPECT_FALSE(selection.contains(selected));
}

TEST_F(ModifyFixture, 移动提交只移动选中实体并取消选中)
{
    DmLine* selected = addLine(DmVector(10.0, 10.0), DmVector(50.0, 10.0));
    DmLine* other = addLine(DmVector(10.0, 30.0), DmVector(50.0, 30.0));
    selection.add(selected);

    ASSERT_TRUE(start("ext.modify.move"));
    typeCoordinate(0.0, 0.0);
    typeCoordinate(5.0, 5.0);
    EXPECT_FALSE(bus.hasActiveCommand());
    EXPECT_EQ(selected->getStartpoint(), DmVector(15.0, 15.0));
    EXPECT_EQ(other->getStartpoint(), DmVector(10.0, 30.0));
    EXPECT_FALSE(selection.contains(selected));

    doc.getCmdManager()->undo();
    EXPECT_EQ(selected->getStartpoint(), DmVector(10.0, 10.0));
    EXPECT_FALSE(selection.contains(selected));
}

TEST_F(ModifyFixture, 复制到剪贴板后取消选中并通知视图剪切还删掉实体)
{
    DmLine* line = addLine(DmVector(10.0, 10.0), DmVector(50.0, 10.0));
    addLine(DmVector(10.0, 30.0), DmVector(50.0, 30.0));
    selection.add(line);

    ASSERT_TRUE(start("ext.edit.copy"));
    // 复制不经事务，也不改文档：取消选中经选择集的 changed() 通知视图重建缓存，文档不发通知
    struct ModifiedCounter : DmDocumentListener
    {
        int modified = 0;
        void documentModified() override { ++modified; }
        void redrawRequested() override {}
        void paintContainerChanged(DmEntityContainer*) override {}
    } counter;
    int changed = 0;
    const QMetaObject::Connection connection =
        QObject::connect(&selection, &SelectionSet::changed, [&changed]() { ++changed; });
    doc.addListener(&counter);
    typeCoordinate(10.0, 10.0);
    doc.removeListener(&counter);
    QObject::disconnect(connection);
    EXPECT_FALSE(bus.hasActiveCommand());
    EXPECT_EQ(DMCLIPBOARD->count(), 1u);
    EXPECT_FALSE(selection.contains(line));
    EXPECT_FALSE(line->isErased());
    EXPECT_GT(changed, 0);
    EXPECT_EQ(counter.modified, 0);

    selection.add(line);
    ASSERT_TRUE(start("ext.edit.cut"));
    typeCoordinate(10.0, 10.0);
    EXPECT_FALSE(bus.hasActiveCommand());
    EXPECT_EQ(DMCLIPBOARD->count(), 1u);
    EXPECT_TRUE(line->isErased());
    EXPECT_EQ(doc.getEntityTable()->count(), 1);
    DMCLIPBOARD->clear();
}
