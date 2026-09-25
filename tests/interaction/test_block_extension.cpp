/// @file test_block_extension.cpp
/// @brief 块扩展（业务工具化第三步第⑥批）的单元测试
///
/// 覆盖：注册的命令类型与打断方式、原内置 ID 不再存在、按钮挂进宿主占位的
/// "绘图/块"面板；插入块的两个阶段（选块、放置）、选项条只在放置阶段出现、命令行改选项、
/// 右键回到选块、选块阶段单击画布结束；定义属性取消对话框时启动失败。创建块、编辑块与
/// 块编辑模式见 test_select_first_commands。不执行提交（见 CommandTestFixture.h）。

#include <gtest/gtest.h>

#include <memory>

#include "BlockExtension.h"
#include "BlockInsertCommand.h"
#include "DmBlock.h"
#include "DmBlockTable.h"
#include "ExtensionManager.h"
#include "Math2d.h"
#include "UIRibbonRegistry.h"
#include "support/CommandTestFixture.h"
#include "support/FakeExtensionHost.h"

using namespace yicad_test;

namespace
{
/// @brief 启动块扩展的命令夹具
struct BlockFixture : CommandFixture
{
    FakeExtensionHost extensionHost;

    BlockFixture()
    {
        ExtensionManager::instance().Register(std::make_unique<BlockExtension>());
        ExtensionManager::instance().BootAll(extensionHost);
    }
    ~BlockFixture() override { ExtensionManager::instance().Shutdown(); }

    BlockInsertCommand* insertCommand() const { return dynamic_cast<BlockInsertCommand*>(bus.activeCommand()); }

    /// @brief 在文档的块表里加一个空块
    DmBlock* addBlock(const QString& name)
    {
        DmBlockData data;
        data.name = name;
        auto* block = new DmBlock(&doc, data);
        doc.getBlockTable()->add_direct(block);
        return block;
    }
};
}  // namespace

TEST_F(BlockFixture, 注册的命令类型与打断方式)
{
    const CommandRegistry& registry = CommandRegistry::instance();
    for (const char* id : {"ext.block.create", "ext.block.insert", "ext.block.edit", "ext.block.define_attributes"})
    {
        EXPECT_EQ(registry.kind(id), CommandKind::Exclusive) << id;
    }
    for (const char* id : {"ext.block.delete", "ext.block.save", "ext.block.import"})
    {
        EXPECT_EQ(registry.kind(id), CommandKind::Instant) << id;
        EXPECT_EQ(registry.instantInterrupt(id), InstantInterrupt::EndUninterruptible) << id;
    }
    // 原 ActionBlocksSaveAs 是排他的
    EXPECT_EQ(registry.instantInterrupt("ext.block.save_as"), InstantInterrupt::EndAll);
    // 宿主撤销/重做后恢复块编辑的钩子不打断任何命令
    EXPECT_EQ(registry.instantInterrupt("ext.block.reenter_edit"), InstantInterrupt::KeepAll);
}

TEST_F(BlockFixture, 原内置ID不再存在)
{
    const CommandRegistry& registry = CommandRegistry::instance();
    for (const char* id : {"blocks.create", "blocks.edit", "blocks.insert", "blocks.insert_prepare", "blocks.save",
                           "blocks.save_as", "blocks.delete", "blocks.import", "blocks.define_attributes"})
    {
        EXPECT_FALSE(registry.hasCommand(id)) << id;
    }
}

TEST_F(BlockFixture, 按钮挂进宿主占位的块面板)
{
    EXPECT_EQ(extensionHost.ribbon.entriesOf(UIRibbonIds::kPanelDraw2dBlock).size(), 7u);
}

TEST_F(BlockFixture, 插入块先选块再放置选项条只在放置阶段)
{
    DmBlock* block = addBlock(QStringLiteral("door"));
    ASSERT_TRUE(start("ext.block.insert"));
    BlockInsertCommand* insert = insertCommand();
    ASSERT_NE(insert, nullptr);
    EXPECT_EQ(insert->block(), nullptr);
    // 选块阶段没有选项条，提示清空
    EXPECT_EQ(lastOptions("ext.block.insert"), nullptr);
    EXPECT_TRUE(ui.lastHint().isEmpty());
    move(5, 5);
    EXPECT_EQ(previewCount(), 0);

    // 在块列表里点了一个块
    {
        ExclusiveCommandBus::DispatchScope scope(&bus);
        insert->chooseBlock(block);
    }
    EXPECT_EQ(insert->block(), block);
    EXPECT_EQ(doc.getBlockTable()->getActive(), block);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify reference point"));
    ASSERT_NE(lastOptions("ext.block.insert"), nullptr);
    EXPECT_TRUE(lastOptions("ext.block.insert")->on);
    move(10, 10);
    EXPECT_EQ(previewCount(), 1);

    // 右键回到选块：选项条收起，预览清除，块列表仍在
    rightClick();
    EXPECT_TRUE(bus.hasActiveCommand());
    EXPECT_EQ(insert->block(), nullptr);
    EXPECT_FALSE(lastOptions("ext.block.insert")->on);
    EXPECT_EQ(previewCount(), 0);

    // 选块阶段在画布上单击结束命令
    click(5, 5);
    EXPECT_FALSE(bus.hasActiveCommand());
}

TEST_F(BlockFixture, 插入块命令行改角度并刷新选项条)
{
    DmBlock* block = addBlock(QStringLiteral("window"));
    ASSERT_TRUE(start("ext.block.insert"));
    BlockInsertCommand* insert = insertCommand();
    ASSERT_NE(insert, nullptr);
    {
        ExclusiveCommandBus::DispatchScope scope(&bus);
        insert->chooseBlock(block);
    }

    // 原 Action 不接受这些文本（随后还会被当作新命令解析），这里只看状态
    typeText(QStringLiteral("angle"));
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Enter angle:"));
    typeText(QStringLiteral("30"));
    EXPECT_NEAR(insert->angle(), Math2d::deg2rad(30.0), 1e-9);
    EXPECT_EQ(ui.lastHint(), QStringLiteral("Specify reference point"));
    ASSERT_NE(lastOptions("ext.block.insert"), nullptr);
    EXPECT_TRUE(lastOptions("ext.block.insert")->update);

    // 换一个块重新开始放置：选项复位（随后由选项条按保存的设置写入）
    DmBlock* other = addBlock(QStringLiteral("table"));
    {
        ExclusiveCommandBus::DispatchScope scope(&bus);
        insert->chooseBlock(other);
    }
    EXPECT_EQ(insert->block(), other);
    EXPECT_DOUBLE_EQ(insert->angle(), 0.0);
    endCommand();
}

TEST_F(BlockFixture, 定义属性取消对话框时启动失败)
{
    // 对话框工厂的默认实现返回"取消"
    EXPECT_FALSE(start("ext.block.define_attributes"));
    EXPECT_FALSE(bus.hasActiveCommand());
}

TEST_F(BlockFixture, 没有文档时即时命令什么也不做)
{
    const CommandRegistry& registry = CommandRegistry::instance();
    for (const char* id : {"ext.block.delete", "ext.block.save", "ext.block.save_as", "ext.block.import",
                           "ext.block.reenter_edit"})
    {
        EXPECT_TRUE(registry.runInstant(QString::fromLatin1(id), CommandContext{})) << id;
    }
    // 有文档但不在块编辑中：不进入编辑模式
    EXPECT_TRUE(registry.runInstant(QStringLiteral("ext.block.reenter_edit"), CommandContext{&doc, &view}));
    EXPECT_EQ(bus.editMode(), nullptr);
}
