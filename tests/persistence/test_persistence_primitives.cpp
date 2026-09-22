/// @file test_persistence_primitives.cpp
/// @brief 持久化层基础设施的单元测试
///
/// Base64、转义、字符串宽窄转换是序列化链路上的底层环节。阶段 5 迁移
/// Qt 6 时 QTextCodec 会被移除，这些与编码相关的函数是回归风险的集中区，
/// 提前把语义固定下来。

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include <QString>

#include "Base64.h"
#include "Tools.h"
#include "Uuid.h"

namespace
{
std::string encode(const std::string& raw)
{
    return PersistentTools::base64_encode(
        reinterpret_cast<const unsigned char*>(raw.data()),
        static_cast<unsigned int>(raw.size()));
}

std::string decode(const std::string& encoded)
{
    return PersistentTools::base64_decode(encoded);
}
}  // namespace

TEST(PersistenceBase64, 空串往返)
{
    EXPECT_EQ(decode(encode("")), "");
}

TEST(PersistenceBase64, ASCII往返)
{
    for (const std::string& s : {std::string("a"),
                                 std::string("ab"),
                                 std::string("abc"),
                                 std::string("abcd"),
                                 std::string("Hello, YiCAD!")})
    {
        EXPECT_EQ(decode(encode(s)), s) << "原文: " << s;
    }
}

TEST(PersistenceBase64, 补位长度符合规范)
{
    // 每 3 字节编码为 4 字符，不足 3 字节补 '='
    EXPECT_EQ(encode("a").size(), 4u);
    EXPECT_EQ(encode("ab").size(), 4u);
    EXPECT_EQ(encode("abc").size(), 4u);
    EXPECT_EQ(encode("abcd").size(), 8u);

    EXPECT_EQ(encode("a").substr(2), "==");
    EXPECT_EQ(encode("ab").substr(3), "=");
}

TEST(PersistenceBase64, 二进制数据往返)
{
    // 覆盖全部 256 个字节值，含 0x00
    std::string raw;
    raw.reserve(256);
    for (int i = 0; i < 256; ++i)
    {
        raw.push_back(static_cast<char>(i));
    }

    const std::string decoded = decode(encode(raw));

    ASSERT_EQ(decoded.size(), raw.size());
    EXPECT_EQ(decoded, raw);
}

TEST(PersistenceBase64, UTF8中文往返)
{
    const std::string raw = "图层名称：中心线";  // 源文件 UTF-8 + /utf-8
    EXPECT_EQ(decode(encode(raw)), raw);
}

TEST(PersistenceTools, QString与std_string往返保留中文)
{
    const QString original = QString::fromUtf8("标注样式 ISO-25 中文");

    const std::string asStd = Tools::toStdString(original);
    const QString back = Tools::fromStdString(asStd);

    EXPECT_EQ(back, original);
}

TEST(PersistenceTools, 宽窄字符串往返保留中文)
{
    const std::string raw = "块参照";  // 源文件 UTF-8 + /utf-8

    const std::wstring wide = Tools::widen(raw);
    const std::string narrow = Tools::narrow(wide);

    EXPECT_EQ(narrow, raw);
}

TEST(PersistenceTools, escapeEncodeString转义的是引号与反斜杠而非XML实体)
{
    // 名字里的 "escape" 容易被误解成 XML 转义。实际实现只处理三个字符：
    // 反斜杠、双引号、单引号，各自前置一个反斜杠——是 C/JSON 风格的字符串
    // 转义，不碰 < > &。
    //
    // 这一点值得钉住：如果有人以为它能防止 XML 结构被破坏而用在属性值上，
    // 尖括号会原样写出去。
    const std::string escaped = Tools::escapeEncodeString(std::string("<tag> & \"quoted\""));

    EXPECT_EQ(escaped, "<tag> & \\\"quoted\\\"");

    // 引号被转义
    EXPECT_NE(escaped.find("\\\""), std::string::npos) << escaped;
    // 尖括号与 & 原样保留
    EXPECT_NE(escaped.find('<'), std::string::npos) << escaped;
    EXPECT_NE(escaped.find('&'), std::string::npos) << escaped;
    EXPECT_EQ(escaped.find("&lt;"), std::string::npos) << escaped;
}

TEST(PersistenceTools, escapeEncodeString转义反斜杠与单引号)
{
    // 反斜杠变成两个反斜杠
    EXPECT_EQ(Tools::escapeEncodeString(std::string("a\\b")), "a\\\\b");
    // 单引号前置一个反斜杠
    EXPECT_EQ(Tools::escapeEncodeString(std::string("it's")), "it\\'s");
    // 其余字符原样通过
    EXPECT_EQ(Tools::escapeEncodeString(std::string("普通文本 123")), "普通文本 123");
}

TEST(PersistenceTools, getUniqueName在冲突时追加编号)
{
    const std::vector<std::string> existing = {"Layer", "Layer1", "Layer2"};

    const std::string name = Tools::getUniqueName("Layer", existing);

    EXPECT_FALSE(name.empty());
    for (const std::string& e : existing)
    {
        EXPECT_NE(name, e) << "返回了已存在的名字: " << name;
    }
}

TEST(PersistenceTools, getUniqueName总是追加编号)
{
    // 它不是「冲突时才改名」，而是始终在名字后面追加一个编号：
    // 先在已有名字里找出共享前缀的最大数字后缀，再加一。
    // 无冲突时后缀为空，increment("") 得到 "1"。
    const std::vector<std::string> existing = {"Layer", "Layer1"};

    EXPECT_EQ(Tools::getUniqueName("Dimension", existing), "Dimension1");
    EXPECT_EQ(Tools::getUniqueName("Layer", existing), "Layer2");
}

TEST(PersistenceTools, getUniqueName的宽度参数补零)
{
    const std::vector<std::string> existing;

    EXPECT_EQ(Tools::getUniqueName("Part", existing, 3), "Part001");
}

TEST(PersistenceTools, split_string按分隔符切分)
{
    const std::vector<std::string> parts = Tools::split_string("a,b,c", ",");

    ASSERT_EQ(parts.size(), 3u);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[1], "b");
    EXPECT_EQ(parts[2], "c");
}

TEST(PersistenceUuid, 生成的UUID互不相同且长度固定)
{
    const std::string a = Uuid::createUuid();
    const std::string b = Uuid::createUuid();

    EXPECT_FALSE(a.empty());
    EXPECT_NE(a, b);
    EXPECT_EQ(a.size(), b.size());
}

TEST(PersistenceUuid, setValue与getValue往返)
{
    Uuid id;
    const std::string generated = Uuid::createUuid();

    id.setValue(generated);

    EXPECT_EQ(id.getValue(), generated);
}
