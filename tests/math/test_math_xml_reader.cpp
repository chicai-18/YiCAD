/// @file test_math_xml_reader.cpp
/// @brief XMLReader 元素事件语义的单元测试
///
/// XMLReader 在 pugixml 的 DOM 上模拟 FreeCAD 基于 SAX 的读取器，调用方（FilterOcdIO 与各
/// Meta*Container）按那套语义编写：readElement 向后找下一个开始（或开始即结束）的元素，
/// readEndElement 向后找元素的结束。原实现只做先序遍历、从不回到父节点，readEndElement
/// 实际找的是下一个同名"开始"标签，readElement 还会反复读到当前节点，整份 .ycd 读不回来
/// （doc/LAYER_RESTRUCTURE_PLAN.md 4.5 节的 R2）。这里按 Reader.h 的接口说明锁住修复后的语义，
/// 不依赖 OCD 格式本身。

#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include "Reader.h"
#include "Tools.h"

namespace
{
/// @brief 用给定文本构造读取器
struct XmlText
{
    explicit XmlText(const std::string& text)
        : stream(text)
        , reader("test.xml", stream)
    {
    }

    std::istringstream stream;
    XMLReader reader;
};
}  // namespace

TEST(XMLReaderTest, 连续读同名兄弟元素依次读到每一个)
{
    XmlText xml("<Data Count=\"3\"><d v=\"1\"/><d v=\"2\"/><d v=\"3\"/></Data>");
    ASSERT_TRUE(xml.reader.isValid());
    XMLReader& reader = xml.reader;

    reader.readElement("Data");
    EXPECT_EQ(reader.getAttributeAsInteger("Count"), 3);
    for (int i = 1; i <= 3; ++i)
    {
        reader.readElement("d");
        EXPECT_EQ(reader.getAttributeAsInteger("v"), i);
    }
    reader.readEndElement("Data");
    EXPECT_STREQ(reader.localName(), "Data");
}

TEST(XMLReaderTest, 结束元素停在本元素的结束处而不是后面的同名开始标签)
{
    // 原实现在 <B v="1"> 里调 readEndElement("B") 会停到 <B v="2"> 的开始，
    // 随后的 readElement("B") 又读到同一个元素
    XmlText xml("<A><B v=\"1\"><C/><C/></B><B v=\"2\"><C/></B></A>");
    XMLReader& reader = xml.reader;

    reader.readElement("A");
    reader.readElement("B");
    EXPECT_EQ(reader.getAttributeAsInteger("v"), 1);
    reader.readElement("C");
    reader.readEndElement("B");
    reader.readElement("B");
    EXPECT_EQ(reader.getAttributeAsInteger("v"), 2);
    reader.readEndElement("B");
    reader.readEndElement("A");
    EXPECT_STREQ(reader.localName(), "A");
}

TEST(XMLReaderTest, 跳过整个元素后读到它的下一个兄弟)
{
    // MetaLineTypesContainer 跳过固定线型就是这样：读到开始，直接 readEndElement
    XmlText xml("<Root><Item n=\"skip\"><Desp/><Data Count=\"2\"><x/><x/></Data></Item>"
                "<Item n=\"keep\"><Desp/></Item></Root>");
    XMLReader& reader = xml.reader;

    reader.readElement("Root");
    reader.readElement("Item");
    EXPECT_STREQ(reader.getAttribute("n"), "skip");
    reader.readEndElement("Item");
    reader.readElement("Item");
    EXPECT_STREQ(reader.getAttribute("n"), "keep");
}

TEST(XMLReaderTest, 结束一组元素后读到父元素的下一个兄弟)
{
    // 与 FilterOcdIO::restoreXML 的结构相同：线型表读完接着读图层表
    XmlText xml("<EntityContainer>"
                "<LineTypes Count=\"1\"><LineType name=\"L\"><LineTypeData Count=\"1\"><data value=\"0.5\"/>"
                "</LineTypeData></LineType></LineTypes>"
                "<Layers Count=\"1\"><Layer name=\"0\"><Color r=\"1\"/><LineType name=\"L\"/></Layer></Layers>"
                "</EntityContainer>");
    XMLReader& reader = xml.reader;

    reader.readElement("EntityContainer");
    reader.readElement("LineTypes");
    reader.readElement("LineType");
    reader.readElement("LineTypeData");
    reader.readElement("data");
    EXPECT_DOUBLE_EQ(reader.getAttributeAsFloat("value"), 0.5);
    reader.readEndElement("LineTypeData");
    reader.readEndElement("LineType");
    reader.readEndElement("LineTypes");

    reader.readElement("Layers");
    EXPECT_EQ(reader.getAttributeAsInteger("Count"), 1);
    reader.readElement("Layer");
    reader.readElement("Color");
    EXPECT_EQ(reader.getAttributeAsInteger("r"), 1);
    reader.readElement("LineType");
    EXPECT_STREQ(reader.getAttribute("name"), "L");
    reader.readEndElement("Layer");
    reader.readEndElement("Layers");
    reader.readEndElement("EntityContainer");
    EXPECT_STREQ(reader.localName(), "EntityContainer");
}

TEST(XMLReaderTest, 开始即结束的元素上结束元素立即返回)
{
    XmlText xml("<A><B/><C/></A>");
    XMLReader& reader = xml.reader;

    reader.readElement("B");
    reader.readEndElement("B");
    EXPECT_STREQ(reader.localName(), "B");
    reader.readElement("C");
    EXPECT_STREQ(reader.localName(), "C");
}

TEST(XMLReaderTest, 只有文本或空白的元素按开始即结束处理)
{
    // 写出的 <LineTypeData Count="0"> 与 </LineTypeData> 之间只有换行
    XmlText xml("<A><Empty Count=\"0\">\n</Empty><Text>abc</Text><Next/></A>");
    XMLReader& reader = xml.reader;

    reader.readElement("Empty");
    reader.readEndElement("Empty");
    reader.readElement("Text");
    reader.readEndElement("Text");
    reader.readElement("Next");
    EXPECT_STREQ(reader.localName(), "Next");
}

TEST(XMLReaderTest, 层级按接口说明变化)
{
    // Reader.h：开始元素加一，开始即结束的元素不变，结束元素减一
    XmlText xml("<A><B><C/></B></A>");
    XMLReader& reader = xml.reader;

    EXPECT_EQ(reader.level(), 0);
    reader.readElement("A");
    EXPECT_EQ(reader.level(), 1);
    reader.readElement("B");
    EXPECT_EQ(reader.level(), 2);
    reader.readElement("C");
    EXPECT_EQ(reader.level(), 2);
    reader.readEndElement("B");
    EXPECT_EQ(reader.level(), 1);
    reader.readEndElement("A");
    EXPECT_EQ(reader.level(), 0);
}

TEST(XMLReaderTest, 找不到的元素停在调用时所在元素的结束处)
{
    XmlText xml("<Root><A><B/></A><Z/></Root>");
    XMLReader& reader = xml.reader;

    reader.readElement("A");
    reader.readElement("Z");  // A 里没有 Z：读到 A 的结束就停，不越过去读后面的 <Z/>
    EXPECT_STREQ(reader.localName(), "A");
    reader.readElement("Z");
    EXPECT_STREQ(reader.localName(), "Z");
}

TEST(XMLReaderTest, 读过文档末尾抛异常)
{
    XmlText xml("<A><B/></A>");
    XMLReader& reader = xml.reader;

    reader.readElement("A");
    reader.readEndElement("A");
    EXPECT_THROW(reader.readElement("B"), OneException);
    EXPECT_THROW(reader.readEndElement("A"), OneException);
}

TEST(XMLReaderTest, 注释与处理指令不产生事件)
{
    XmlText xml("<?xml version='1.0' encoding='utf-8'?>\n<!-- 注释 -->\n<A><!-- 注释 --><B v=\"1\"/></A>");
    ASSERT_TRUE(xml.reader.isValid());
    XMLReader& reader = xml.reader;

    reader.readElement();
    EXPECT_STREQ(reader.localName(), "A");
    reader.readElement();
    EXPECT_STREQ(reader.localName(), "B");
    EXPECT_EQ(reader.getAttributeAsInteger("v"), 1);
}

TEST(XMLReaderTest, 不是XML的输入无效)
{
    XmlText xml("not xml at all");
    EXPECT_FALSE(xml.reader.isValid());
}
