/// @file test_host_extensions.cpp
/// @brief 文件、图层、选项扩展（业务工具化第三步第⑤批）的单元测试
///
/// 覆盖：三个扩展注册的即时命令与打断方式、原内置 ID 不再存在、按钮挂进宿主
/// 占位的面板、没有宿主标签页或文档时命令什么也不做、Shutdown 后命令注销；图层与设置
/// 命令弹出各自的对话框（扩展直接构造，经 DialogRecorder 记录并视为取消）；图层下拉框
/// 每行按钮记着图层名。命令对文档的修改要走事务，默认构造的 DmDocument 走事务会崩溃
/// （见 CommandTestFixture.h），因此不执行修改。
///
/// ExtensionManager 与 CommandRegistry 是进程范围的单例，用例结束前 Shutdown()。

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QWidget>

#include "CommandRegistry.h"
#include "CustomComboboxItem.h"
#include "DmDocument.h"
#include "DmLayer.h"
#include "DmLayerTable.h"
#include "ExtensionManager.h"
#include "FileExtension.h"
#include "LayerExtension.h"
#include "OptionsExtension.h"
#include "UIRibbonRegistry.h"
#include "support/DialogRecorder.h"
#include "support/FakeExtensionHost.h"

using yicad_test::DialogRecorder;
using yicad_test::FakeExtensionHost;

namespace
{
const char* const kFileCommands[] = {"ext.file.new", "ext.file.open", "ext.file.save", "ext.file.save_as",
                                     "ext.file.export_image"};

const char* const kOtherCommands[] = {
    "ext.layer.activate",   "ext.layer.add",        "ext.layer.rename",   "ext.layer.color",
    "ext.layer.delete",     "ext.layer.freeze",     "ext.layer.lock",     "ext.layer.print",
    "ext.layer.freeze_all", "ext.layer.defreeze_all", "ext.layer.lock_all", "ext.layer.unlock_all",
    "ext.options.general",  "ext.options.drawing",
};

/// @brief 注册并启动三个扩展，析构时 Shutdown
struct HostExtensions
{
    FakeExtensionHost host;

    HostExtensions()
    {
        auto& manager = ExtensionManager::instance();
        EXPECT_TRUE(manager.Register(std::make_unique<FileExtension>()));
        EXPECT_TRUE(manager.Register(std::make_unique<LayerExtension>()));
        EXPECT_TRUE(manager.Register(std::make_unique<OptionsExtension>()));
        manager.BootAll(host);
    }
    ~HostExtensions() { ExtensionManager::instance().Shutdown(); }
};
}  // namespace

TEST(HostExtensionsTest, 注册即时命令文件命令先结束全部命令)
{
    HostExtensions extensions;
    const CommandRegistry& registry = CommandRegistry::instance();
    for (const char* id : kFileCommands)
    {
        SCOPED_TRACE(id);
        EXPECT_EQ(registry.kind(id), CommandKind::Instant);
        // 原 Action 是排他的（isExclusive）
        EXPECT_EQ(registry.instantInterrupt(id), InstantInterrupt::EndAll);
    }
    for (const char* id : kOtherCommands)
    {
        SCOPED_TRACE(id);
        EXPECT_EQ(registry.kind(id), CommandKind::Instant);
        EXPECT_EQ(registry.instantInterrupt(id), InstantInterrupt::EndUninterruptible);
    }
}

TEST(HostExtensionsTest, 原内置ID不再存在)
{
    HostExtensions extensions;
    const CommandRegistry& registry = CommandRegistry::instance();
    for (const char* id : {"file.new", "file.open", "file.save", "file.save_as", "file.export_image",
                           "layers.freeze", "layers.add", "layers.unlock_all", "options.general", "options.drawing"})
    {
        SCOPED_TRACE(id);
        EXPECT_FALSE(registry.hasCommand(id));
    }
}

TEST(HostExtensionsTest, 按钮挂进宿主占位的面板)
{
    HostExtensions extensions;
    using namespace UIRibbonIds;
    EXPECT_EQ(extensions.host.ribbon.entriesOf(kPanelFileFile).size(), 4u);
    EXPECT_EQ(extensions.host.ribbon.entriesOf(kPanelFileExport).size(), 1u);
    EXPECT_EQ(extensions.host.ribbon.entriesOf(kPanelOptionsSettings).size(), 2u);
}

TEST(HostExtensionsTest, 没有标签页或文档时命令什么也不做)
{
    HostExtensions extensions;
    const CommandRegistry& registry = CommandRegistry::instance();
    // 假宿主没有图纸标签页
    EXPECT_TRUE(registry.runInstant(QStringLiteral("ext.file.save"), CommandContext{}));
    EXPECT_TRUE(registry.runInstant(QStringLiteral("ext.options.drawing"), CommandContext{}));
    // 没有文档，也没有触发的按钮
    for (const char* id : {"ext.layer.freeze", "ext.layer.lock", "ext.layer.print", "ext.layer.color",
                           "ext.layer.delete", "ext.layer.activate", "ext.layer.freeze_all", "ext.layer.lock_all"})
    {
        SCOPED_TRACE(id);
        EXPECT_TRUE(registry.runInstant(QString::fromLatin1(id), CommandContext{}));
    }
}

TEST(HostExtensionsTest, 新建与修改图层弹出图层对话框取消时图层表不变)
{
    HostExtensions extensions;
    DialogRecorder dialogs;
    std::vector<QString> names; // 对话框里的图层名
    dialogs.onShow = [&names](QDialog& dialog)
    {
        auto* name = dialog.findChild<QLineEdit*>(QStringLiteral("leName"));
        names.push_back(name ? name->text() : QString());
    };

    DmDocument doc;
    DmLayerTable* layers = doc.getLayerTable();
    ASSERT_TRUE(layers->add_direct(new DmLayer(QStringLiteral("墙体09"))));
    layers->activate_direct(QStringLiteral("墙体09"));
    const unsigned layerCount = layers->count();

    const CommandRegistry& registry = CommandRegistry::instance();
    EXPECT_TRUE(registry.runInstant(QStringLiteral("ext.layer.add"), CommandContext{&doc}));
    EXPECT_TRUE(registry.runInstant(QStringLiteral("ext.layer.rename"), CommandContext{&doc}));

    EXPECT_EQ(dialogs.shown, (std::vector<QString>{QStringLiteral("UILayerDialog"), QStringLiteral("UILayerDialog")}));
    // 新建时预填当前图层名末尾的数字加一（位数不变），修改时是当前图层名
    EXPECT_EQ(names, (std::vector<QString>{QStringLiteral("墙体10"), QStringLiteral("墙体09")}));
    EXPECT_EQ(layers->count(), layerCount);
    EXPECT_EQ(layers->getActive()->getName(), QStringLiteral("墙体09"));
}

TEST(HostExtensionsTest, 设置命令弹出设置对话框)
{
    HostExtensions extensions;
    DialogRecorder dialogs;
    DmDocument doc;
    const CommandRegistry& registry = CommandRegistry::instance();
    EXPECT_TRUE(registry.runInstant(QStringLiteral("ext.options.general"), CommandContext{}));
    EXPECT_TRUE(registry.runInstant(QStringLiteral("ext.options.drawing"), CommandContext{&doc}));
    EXPECT_EQ(dialogs.shown,
              (std::vector<QString>{QStringLiteral("UIDlgOptionsGeneral"), QStringLiteral("UIDlgOptionsDrawing")}));
}

TEST(HostExtensionsTest, Shutdown后命令注销)
{
    {
        HostExtensions extensions;
        EXPECT_TRUE(CommandRegistry::instance().hasCommand(QStringLiteral("ext.file.new")));
    }
    for (const char* id : kFileCommands)
    {
        EXPECT_FALSE(CommandRegistry::instance().hasCommand(id)) << id;
    }
    for (const char* id : kOtherCommands)
    {
        EXPECT_FALSE(CommandRegistry::instance().hasCommand(id)) << id;
    }
}

TEST(LayerComboboxTest, 按钮记着所在行的图层名)
{
    QWidget row;
    ComboBoxData data;
    data.btnOn = new QToolButton(&row);
    data.btnLock = new QToolButton(&row);
    data.btnPrint = new QToolButton(&row);
    data.btnColor = new QToolButton(&row);
    data.labelName = new QPushButton(&row);
    data.setLayerName(QStringLiteral("墙体"));
    for (QObject* button : {static_cast<QObject*>(data.btnOn), static_cast<QObject*>(data.btnLock),
                            static_cast<QObject*>(data.btnPrint), static_cast<QObject*>(data.btnColor),
                            static_cast<QObject*>(data.labelName)})
    {
        EXPECT_EQ(ComboBoxData::layerNameOf(button), QStringLiteral("墙体"));
    }

    // 删除按钮建在图层名之后（宿主的做法）：要再记一次
    data.btnDelete = new QToolButton(&row);
    EXPECT_TRUE(ComboBoxData::layerNameOf(data.btnDelete).isEmpty());
    data.tagButtons();
    EXPECT_EQ(ComboBoxData::layerNameOf(data.btnDelete), QStringLiteral("墙体"));

    // 改名（下拉框刷新时经 setByData 改名）后各按钮跟着改
    data.setLayerName(QStringLiteral("门窗"));
    EXPECT_EQ(ComboBoxData::layerNameOf(data.btnOn), QStringLiteral("门窗"));
    EXPECT_EQ(ComboBoxData::layerNameOf(data.btnDelete), QStringLiteral("门窗"));

    // 不是图层行的对象
    EXPECT_TRUE(ComboBoxData::layerNameOf(nullptr).isEmpty());
    EXPECT_TRUE(ComboBoxData::layerNameOf(&row).isEmpty());
}
