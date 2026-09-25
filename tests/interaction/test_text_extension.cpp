/// @file test_text_extension.cpp
/// @brief 文字扩展（业务工具化第三步第⑦批）与它用到的内核机制的单元测试
///
/// 覆盖：注册的命令类型、打断方式与别名、多行文字的双击编辑登记（Shutdown 后随命令注销）、
/// 原内置 ID 不再存在、按钮挂进宿主占位的"绘图/文字"面板；单行文字取消对话框
/// 时启动失败；多行文字拉编辑框的提示与预览（假视图没有画布，进入编辑时直接结束）；
/// 属性面板不可打断、单击取消选中并结束；选择层双击实体按登记的编辑命令经视图启动，没有
/// 登记时弹出属性对话框。编辑框本身要真正的画布，不在单测范围内。

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "DmLine.h"
#include "DmMText.h"
#include "DrawMTextCommand.h"
#include "ExtensionManager.h"
#include "LineData.h"
#include "ModifyMTextCommand.h"
#include "MTextData.h"
#include "TextExtension.h"
#include "UIRibbonRegistry.h"
#include "support/CommandTestFixture.h"
#include "support/FakeExtensionHost.h"

using namespace yicad_test;

namespace
{
/// @brief 启动文字扩展的命令夹具
struct TextFixture : CommandFixture
{
    FakeExtensionHost extensionHost;

    TextFixture()
    {
        ExtensionManager::instance().Register(std::make_unique<TextExtension>());
        ExtensionManager::instance().BootAll(extensionHost);
    }
    ~TextFixture() override { ExtensionManager::instance().Shutdown(); }
};

/// @brief 选择层双击的夹具（不启动扩展）
struct SelectEditorFixture : CommandFixture
{
};

/// @brief 选择层启动命令的记录
struct StartRecord
{
    QString commandId;
    DmEntity* entity = nullptr;
    DmVector point;
};
}  // namespace

TEST_F(TextFixture, 注册的命令类型打断方式与别名)
{
    const CommandRegistry& registry = CommandRegistry::instance();
    for (const char* id : {"ext.text.draw", "ext.text.mtext", "ext.text.edit_mtext", "ext.text.modify_mtext"})
    {
        EXPECT_EQ(registry.kind(id), CommandKind::Exclusive) << id;
    }
    EXPECT_EQ(registry.kind("ext.text.style"), CommandKind::Instant);
    EXPECT_EQ(registry.instantInterrupt("ext.text.style"), InstantInterrupt::EndUninterruptible);
    // 宿主在选择变化时运行的监听者不打断任何命令
    EXPECT_EQ(registry.instantInterrupt("ext.text.selection_changed"), InstantInterrupt::KeepAll);
    // 别名取原 keyconfig.xml 两组的并集；dhwz 原先按单行文字解析
    EXPECT_EQ(registry.commandForAlias("txt"), QStringLiteral("ext.text.draw"));
    EXPECT_EQ(registry.commandForAlias("dhwz"), QStringLiteral("ext.text.draw"));
    EXPECT_EQ(registry.commandForAlias("mtxt"), QStringLiteral("ext.text.mtext"));
}

TEST_F(TextFixture, 多行文字登记为双击编辑且Shutdown后注销)
{
    EXPECT_EQ(CommandRegistry::instance().entityEditor(DM::EntityMText), QStringLiteral("ext.text.edit_mtext"));
    ExtensionManager::instance().Shutdown();
    EXPECT_TRUE(CommandRegistry::instance().entityEditor(DM::EntityMText).isEmpty());
}

TEST_F(TextFixture, 原内置ID不再存在)
{
    const CommandRegistry& registry = CommandRegistry::instance();
    for (const char* id : {"draw.text", "draw.mtext", "text.style", "select.selection_changed"})
    {
        EXPECT_FALSE(registry.hasCommand(id)) << id;
    }
}

TEST_F(TextFixture, 按钮挂进宿主占位的文字面板)
{
    EXPECT_EQ(extensionHost.ribbon.entriesOf(UIRibbonIds::kPanelDraw2dText).size(), 3u);
}

TEST_F(TextFixture, 单行文字取消对话框时启动失败)
{
    // 对话框工厂的默认实现返回"取消"
    EXPECT_FALSE(start("ext.text.draw"));
    EXPECT_FALSE(bus.hasActiveCommand());
}

TEST_F(TextFixture, 多行文字拉编辑框)
{
    ASSERT_TRUE(start("ext.text.mtext"));
    auto* mtext = dynamic_cast<DrawMTextCommand*>(bus.activeCommand());
    ASSERT_NE(mtext, nullptr);
    EXPECT_TRUE(mtext->isUninterruptible());
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify first point of edit box"));

    typeCoordinate(0, 0);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify second point of edit box"));
    move(20, -10);
    EXPECT_EQ(previewCount(), 1);

    // 假视图没有画布，进入编辑时直接结束
    typeCoordinate(20, -10);
    EXPECT_FALSE(bus.hasActiveCommand());
    EXPECT_EQ(previewCount(), 0);
}

TEST_F(TextFixture, 多行文字拉编辑框时右键结束)
{
    ASSERT_TRUE(start("ext.text.mtext"));
    typeCoordinate(0, 0);
    rightClick();
    EXPECT_FALSE(bus.hasActiveCommand());
}

TEST_F(TextFixture, 编辑与属性面板命令只接受多行文字)
{
    const CommandRegistry& registry = CommandRegistry::instance();
    auto line = std::make_unique<DmLine>(nullptr, LineData(DmVector(0, 0), DmVector(1, 0)));
    for (const char* id : {"ext.text.edit_mtext", "ext.text.modify_mtext"})
    {
        SCOPED_TRACE(id);
        EXPECT_EQ(registry.createCommand(id, CommandContext{&doc, &view}), nullptr);
        EXPECT_EQ(registry.createCommand(id, CommandContext{&doc, &view, nullptr, line.get()}), nullptr);
    }
    // 编辑命令构造时要复制文字，默认构造的 DmMText 没有样式复制不了，这里只看属性面板
    auto text = std::make_unique<DmMText>(nullptr, MTextData());
    EXPECT_NE(registry.createCommand(QStringLiteral("ext.text.modify_mtext"),
                                     CommandContext{&doc, &view, nullptr, text.get()}),
              nullptr);
}

TEST_F(TextFixture, 属性面板不可打断单击取消选中并结束)
{
    auto text = std::make_unique<DmMText>(nullptr, MTextData());
    text->setSelected(true);
    std::unique_ptr<IExclusiveCommand> command = CommandRegistry::instance().createCommand(
        QStringLiteral("ext.text.modify_mtext"), CommandContext{&doc, &view, nullptr, text.get()});
    ASSERT_NE(command, nullptr);
    EXPECT_TRUE(command->isUninterruptible());
    ASSERT_TRUE(bus.start(std::move(command)));

    click(5, 5);
    EXPECT_FALSE(text->isSelected());
    EXPECT_FALSE(bus.hasActiveCommand());
}

TEST_F(TextFixture, 没有视图时选择变化的监听者什么也不做)
{
    EXPECT_TRUE(CommandRegistry::instance().runInstant(QStringLiteral("ext.text.selection_changed"), CommandContext{}));
    // 假视图不是 UIView：不启动属性面板
    EXPECT_TRUE(CommandRegistry::instance().runInstant(QStringLiteral("ext.text.selection_changed"),
                                                       CommandContext{&doc, &view}));
    EXPECT_FALSE(bus.hasActiveCommand());
}

TEST_F(SelectEditorFixture, 选择层双击按登记的编辑命令启动没有登记时弹出属性对话框)
{
    auto* line = new DmLine(nullptr, LineData(DmVector(0, 0), DmVector(10, 0)));
    line->calculateBorders();
    ASSERT_TRUE(doc.getEntityTable()->add_direct(line));

    std::vector<StartRecord> started;
    selectTool.setCommandStarter([&started](const QString& id, DmEntity* entity, const DmVector& point)
                                 {
                                     started.push_back({id, entity, point});
                                     return true;
                                 });
    auto doubleClick = [this](int x, int y)
    {
        QMouseEvent e = makeMouse(QEvent::MouseButtonDblClick, x, y, Qt::LeftButton);
        dispatch([&] { return control.mouseDoubleClickEvent(&e); });
    };

    // 没有登记：弹出属性对话框
    doubleClick(5, 0);
    EXPECT_TRUE(started.empty());
    ASSERT_EQ(ui.entityDialogs.size(), 1u);
    EXPECT_EQ(ui.entityDialogs.front(), line);

    // 登记了直线的编辑命令：经视图启动，带上双击的实体与位置
    ASSERT_TRUE(CommandRegistry::instance().registerExclusiveCommand(
        QStringLiteral("test.text.line_editor"),
        [](const CommandContext&) -> std::unique_ptr<IExclusiveCommand> { return nullptr; }));
    ASSERT_TRUE(CommandRegistry::instance().registerEntityEditor(DM::EntityLine, QStringLiteral("test.text.line_editor")));
    line->setSelected(false);
    doubleClick(5, 0);
    ASSERT_EQ(started.size(), 1u);
    EXPECT_EQ(started.front().commandId, QStringLiteral("test.text.line_editor"));
    EXPECT_EQ(started.front().entity, line);
    EXPECT_EQ(started.front().point, DmVector(5, 0));
    EXPECT_EQ(ui.entityDialogs.size(), 1u);

    CommandRegistry::instance().unregisterCommand(QStringLiteral("test.text.line_editor"));
    EXPECT_TRUE(CommandRegistry::instance().entityEditor(DM::EntityLine).isEmpty());
}
