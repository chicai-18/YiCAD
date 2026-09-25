/// @file test_keyconfig.cpp
/// @brief keyconfig.xml 读写（Commands）的单测
///
/// 业务工具化第四步把 keyconfig.xml 从以 DM::ActionType 的枚举名为键
/// （`action="ActionDrawLine"`）改为以命令 ID 为键（`command="ext.draw.line"`），
/// 见 doc/COMMAND_TOOL_MIGRATION_PLAN.md 9.4 节。覆盖：新格式读取、旧格式按原
/// 映射表转换、用户目录下旧格式文件的改写与备份、保存时保留其它组，以及程序
/// 目录下默认配置里的命令都已注册。

#include <gtest/gtest.h>

#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>

#include "CommandRegistry.h"
#include "Commands.h"
#include "support/CommandExtensions.h"

namespace
{
/// @brief 写一个 UTF-8 文本文件
void writeFile(const QString& path, const QString& text)
{
    QFile f(path);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text));
    QTextStream out(&f);
    out.setCodec("UTF-8");
    out << text;
}

/// @brief 读出整个文件
QString readFile(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return QString();
    }
    QTextStream in(&f);
    in.setCodec("UTF-8");
    return in.readAll();
}

/// @brief 旧格式：两组，含一条从未实现的枚举名与一条已搬进扩展的命令
const char* const kLegacyConfig = R"(<?xml version="1.0" encoding="utf-8" ?>
<groups>
	<group name="Default">
		<!--直线-->
		<item action="ActionDrawLine" description="Line" keys="line, LI"/>
		<item action="ActionDrawLineParallel" description="Parallel" keys="pa"/>
		<item action="ActionDrawText" description="Text" keys="dt"/>
		<item action="ActionSnapFree" description="Free" keys="sf"/>
	</group>
	<group name="Pinyin">
		<item action="ActionDrawLine" description="Line" keys="zx"/>
	</group>
</groups>
)";

/// @brief 在 items 里找命令 ID
const CommandKeys* find(const std::vector<CommandKeys>& items, const char* commandId)
{
    for (const CommandKeys& item : items)
    {
        if (item.commandId == QLatin1String(commandId))
        {
            return &item;
        }
    }
    return nullptr;
}
}  // namespace

TEST(KeyconfigTest, 新格式按命令ID读取指定组)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString file = dir.filePath("keyconfig.xml");
    writeFile(file, R"(<?xml version="1.0" encoding="utf-8" ?>
<groups>
	<group name="A"><item command="ext.draw.line" description="Line" keys="l"/></group>
	<group name="B"><item command="ext.draw.circle" description="Circle" keys=" C , Circle"/></group>
</groups>
)");

    QString group = "B";
    const std::vector<CommandKeys> items = Commands::readConfigFile(file, group, true);
    ASSERT_EQ(items.size(), 1u);
    EXPECT_EQ(items[0].commandId, QStringLiteral("ext.draw.circle"));
    EXPECT_EQ(items[0].description, QStringLiteral("Circle"));
    // 别名去首尾空白、转小写
    EXPECT_EQ(items[0].keys, (QStringList{"c", "circle"}));

    // 不精确匹配时找不到的组退回第一组，并把组名改成它
    group = "missing";
    const std::vector<CommandKeys> first = Commands::readConfigFile(file, group, false);
    ASSERT_EQ(first.size(), 1u);
    EXPECT_EQ(group, QStringLiteral("A"));
    group = "missing";
    EXPECT_TRUE(Commands::readConfigFile(file, group, true).empty());
}

TEST(KeyconfigTest, 旧格式按原映射表转换没有对应命令的条目丢弃)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString file = dir.filePath("keyconfig.xml");
    writeFile(file, kLegacyConfig);

    QString group = "Default";
    const std::vector<CommandKeys> items = Commands::readConfigFile(file, group, true);
    ASSERT_EQ(items.size(), 3u);
    ASSERT_NE(find(items, "ext.draw.line"), nullptr);
    EXPECT_EQ(find(items, "ext.draw.line")->keys, (QStringList{"line", "li"}));
    // 已搬进扩展的命令转成扩展的 ID；捕捉开关是宿主处理的内置命令
    EXPECT_NE(find(items, "ext.text.draw"), nullptr);
    EXPECT_NE(find(items, "snap.free"), nullptr);

    EXPECT_EQ(Commands::legacyCommandId("ActionDrawLineParallel"), QString());
    EXPECT_EQ(Commands::legacyCommandId("ActionModifyMoveNoSelect"), QStringLiteral("ext.modify.move"));
    EXPECT_EQ(Commands::legacyCommandId("ActionDimLinear"), QStringLiteral("ext.dim.linear"));
}

TEST(KeyconfigTest, 旧格式文件改写为新格式并保留备份)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString file = dir.filePath("keyconfig.xml");
    writeFile(file, kLegacyConfig);

    ASSERT_TRUE(Commands::migrateLegacyConfig(file));
    EXPECT_EQ(readFile(file + ".bak"), QString::fromUtf8(kLegacyConfig));

    const QString migrated = readFile(file);
    EXPECT_FALSE(migrated.contains("action="));
    EXPECT_TRUE(migrated.contains(R"(command="ext.draw.line")"));
    // 两组都在，内容与旧格式读出的一致
    QString group = "Pinyin";
    const std::vector<CommandKeys> pinyin = Commands::readConfigFile(file, group, true);
    ASSERT_EQ(pinyin.size(), 1u);
    EXPECT_EQ(pinyin[0].keys, QStringList{"zx"});
    group = "Default";
    EXPECT_EQ(Commands::readConfigFile(file, group, true).size(), 3u);

    // 已是新格式：不再改写
    EXPECT_FALSE(Commands::migrateLegacyConfig(file));
    // 文件不存在
    EXPECT_FALSE(Commands::migrateLegacyConfig(dir.filePath("missing.xml")));
}

TEST(KeyconfigTest, 已有备份时不覆盖)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString file = dir.filePath("keyconfig.xml");
    writeFile(file, kLegacyConfig);
    writeFile(file + ".bak", "earlier backup");

    ASSERT_TRUE(Commands::migrateLegacyConfig(file));
    EXPECT_EQ(readFile(file + ".bak"), QStringLiteral("earlier backup"));
    EXPECT_FALSE(readFile(file).contains("action="));
}

TEST(KeyconfigTest, 保存只替换一组其它组原样保留)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString file = dir.filePath("sub/keyconfig.xml");
    ASSERT_TRUE(Commands::saveToFile({CommandKeys{"ext.draw.line", "Line", {"l"}}}, "A", file));
    ASSERT_TRUE(Commands::saveToFile({CommandKeys{"ext.draw.circle", "Circle", {"c", "ci"}}}, "B", file));
    ASSERT_TRUE(Commands::saveToFile({CommandKeys{"ext.draw.arc", "Arc", {"a"}}}, "A", file));

    QString group = "A";
    const std::vector<CommandKeys> a = Commands::readConfigFile(file, group, true);
    ASSERT_EQ(a.size(), 1u);
    EXPECT_EQ(a[0].commandId, QStringLiteral("ext.draw.arc"));
    group = "B";
    const std::vector<CommandKeys> b = Commands::readConfigFile(file, group, true);
    ASSERT_EQ(b.size(), 1u);
    EXPECT_EQ(b[0].keys, (QStringList{"c", "ci"}));
}

TEST(KeyconfigTest, 默认配置里的命令都已注册)
{
    // 默认配置里的命令都在原内置命令拆成的五个扩展里（第四步）
    yicad_test::CommandExtensionsScope extensions;
    // 程序目录下的 keyconfig.xml 由 cmake --install 复制，这里直接读源码树里的那份
    for (const char* name : {"默认", "拼音简写"})
    {
        QString group = QString::fromUtf8(name);
        const std::vector<CommandKeys> items =
            Commands::readConfigFile(QStringLiteral(YICAD_DEFAULT_KEYCONFIG), group, true);
        SCOPED_TRACE(name);
        EXPECT_FALSE(items.empty());
        for (const CommandKeys& item : items)
        {
            EXPECT_TRUE(CommandRegistry::instance().hasCommand(item.commandId)) << item.commandId.toStdString();
        }
    }
}

TEST(KeyconfigTest, 命令行计算器取表达式)
{
    // 阶段 5：QRegExp 换成 QRegularExpression。QRegExp 的 \s 按 QChar::isSpace 判断，
    // 包括全角空格 U+3000，换过去要带 UseUnicodePropertiesOption 才保持一致。
    EXPECT_EQ(Commands::filterCliCal(QStringLiteral("cal 1+2")), QStringLiteral("1+2"));
    EXPECT_EQ(Commands::filterCliCal(QStringLiteral("  calculate   3*4 ")), QStringLiteral("3*4"));
    EXPECT_EQ(Commands::filterCliCal(QString::fromUtf8("cal　5-1")), QStringLiteral("5-1"));
    EXPECT_EQ(Commands::filterCliCal(QStringLiteral("cal")), QString());
    EXPECT_EQ(Commands::filterCliCal(QStringLiteral("line 1,2")), QString());
}
