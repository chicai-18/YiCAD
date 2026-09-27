/// @file test_dxf_encoding.cpp
/// @brief DXF 导入导出的中文编码测试（阶段 5 验收）
///
/// 加载构建目录里真实的 YiCadDxfPlugin.dll，走与程序相同的插件运行时
/// （support/DxfTestRuntime.h），导入导出与程序一样经格式注册表找过滤器。覆盖两条路径：
/// - R2000 文件按 $DWGCODEPAGE（ANSI_936）用 GBK 存中文，导入后图层名与文字正确；
/// - 导入后再导出（插件固定写 R2013，字符串为 UTF-8），文件里是中文的 UTF-8 字节，
///   再导入得到同样的图层与文字。
///
/// 代码页转换在插件内置的 libdxfrw 里完成，不经过 Qt；宿主一侧经手的只是插件 ABI 上的
/// UTF-8 字符串与 QString 的互转。迁移 Qt 6（移除 QTextCodec）后由本用例兜底。
///
/// 另有一个基准图纸用例（doc/BASELINE.md 的三份图纸）：设置环境变量 YICAD_BENCHMARK_DIR
/// 指向 tools/gen_benchmark_drawings.py 的输出目录时运行，否则跳过。它逐类型比对 DXF 的
/// ENTITIES 段与导入后模型里的顶层实体数。

#include <gtest/gtest.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include "DmDocument.h"
#include "DmEntity.h"
#include "DmLayer.h"
#include "DmText.h"
#include "EntityTable.h"
#include "FilterInterface.h"
#include "FilterRegistry.h"
#include "support/DxfTestRuntime.h"

namespace
{
using yicad_test::DxfRuntime;

// GBK 编码的「图层甲」与「中文文字」
const std::string GBK_LAYER = "\xCD\xBC\xB2\xE3\xBC\xD7";
const std::string GBK_TEXT = "\xD6\xD0\xCE\xC4\xCE\xC4\xD7\xD6";

/// @brief 一个 R2000 图纸：代码页 ANSI_936，一个中文名的图层，图层上一行中文单行文字
std::string gbkDrawing()
{
    std::string dxf;
    auto group = [&dxf](const char* code, const std::string& value) {
        dxf += code;
        dxf += '\n';
        dxf += value;
        dxf += '\n';
    };
    group("0", "SECTION");
    group("2", "HEADER");
    group("9", "$ACADVER");
    group("1", "AC1015");
    group("9", "$DWGCODEPAGE");
    group("3", "ANSI_936");
    group("0", "ENDSEC");
    group("0", "SECTION");
    group("2", "TABLES");
    group("0", "TABLE");
    group("2", "LAYER");
    group("70", "1");
    group("0", "LAYER");
    group("2", GBK_LAYER);
    group("70", "0");
    group("62", "7");
    group("6", "CONTINUOUS");
    group("0", "ENDTAB");
    group("0", "ENDSEC");
    group("0", "SECTION");
    group("2", "ENTITIES");
    group("0", "TEXT");
    group("8", GBK_LAYER);
    group("10", "0.0");
    group("20", "0.0");
    group("30", "0.0");
    group("40", "2.5");
    group("1", GBK_TEXT);
    group("0", "ENDSEC");
    group("0", "EOF");
    return dxf;
}

/// @brief 以二进制写文件（DXF 里的 GBK 字节原样落盘）
bool writeBytes(const QString& path, const std::string& bytes)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        return false;
    }
    return file.write(bytes.data(), static_cast<qint64>(bytes.size())) == static_cast<qint64>(bytes.size());
}

/// @brief 文档里全部单行文字
std::vector<DmText*> texts(DmDocument& document)
{
    std::vector<DmText*> result;
    for (DmEntity* entity : *document.getEntityTable())
    {
        if (auto* text = dynamic_cast<DmText*>(entity))
        {
            result.push_back(text);
        }
    }
    return result;
}

/// @brief 检查文档里有中文名的图层，且那行中文文字在该图层上
void expectChineseContent(DmDocument& document)
{
    const QString layerName = QStringLiteral("图层甲");
    EXPECT_NE(document.getLayerTable()->find(layerName), nullptr);

    const std::vector<DmText*> found = texts(document);
    ASSERT_EQ(found.size(), 1u);
    EXPECT_EQ(found.front()->getText(), QStringLiteral("中文文字"));
    ASSERT_NE(found.front()->getLayer(), nullptr);
    EXPECT_EQ(found.front()->getLayer()->getName(), layerName);
}
} // namespace

TEST(DxfEncodingTest, 插件格式加载后登记进格式注册表卸载前注销)
{
    const QStringList importsBefore = FilterRegistry::instance().importNameFilters();
    const QStringList exportsBefore = FilterRegistry::instance().exportNameFilters();
    {
        DxfRuntime runtime;
        ASSERT_TRUE(runtime.loaded()) << runtime.diagnostics().toStdString();

        // 插件格式排在原生格式之后，文件对话框的过滤串补上后缀
        const QStringList imports = FilterRegistry::instance().importNameFilters();
        ASSERT_EQ(imports.size(), importsBefore.size() + runtime.importFormatCount());
        EXPECT_EQ(imports.mid(0, importsBefore.size()), importsBefore);
        EXPECT_TRUE(imports.back().endsWith(QStringLiteral("(*.dxf)"))) << imports.back().toStdString();

        const QStringList exports = FilterRegistry::instance().exportNameFilters();
        ASSERT_EQ(exports.size(), exportsBefore.size() + runtime.exportFormatCount());
        EXPECT_EQ(exports.mid(0, exportsBefore.size()), exportsBefore);
        EXPECT_TRUE(exports.back().endsWith(QStringLiteral("(*.dxf)"))) << exports.back().toStdString();
        // 保存对话框选中插件的过滤串，得到插件规范格式名 "pluginId/formatId"
        EXPECT_EQ(FilterRegistry::instance().exportFormatType(exports.back()), runtime.exportFormat());

        EXPECT_NE(FilterRegistry::instance().importFilter(QStringLiteral("drawing.dxf")), nullptr);
        EXPECT_NE(FilterRegistry::instance().exportFilter(runtime.exportFormat()), nullptr);
    }
    EXPECT_EQ(FilterRegistry::instance().importNameFilters(), importsBefore);
    EXPECT_EQ(FilterRegistry::instance().exportNameFilters(), exportsBefore);
    EXPECT_EQ(FilterRegistry::instance().importFilter(QStringLiteral("drawing.dxf")), nullptr);
}

TEST(DxfEncodingTest, 按代码页导入GBK中文)
{
    DxfRuntime runtime;
    ASSERT_TRUE(runtime.loaded()) << runtime.diagnostics().toStdString();

    QTemporaryDir dir;
    const QString source = dir.filePath(QStringLiteral("gbk_r2000.dxf"));
    ASSERT_TRUE(writeBytes(source, gbkDrawing()));

    DmDocument document;
    ASSERT_TRUE(runtime.importFile(document, source));
    expectChineseContent(document);
}

TEST(DxfEncodingTest, 导出再导入保持中文)
{
    DxfRuntime runtime;
    ASSERT_TRUE(runtime.loaded()) << runtime.diagnostics().toStdString();

    QTemporaryDir dir;
    const QString source = dir.filePath(QStringLiteral("gbk_r2000.dxf"));
    ASSERT_TRUE(writeBytes(source, gbkDrawing()));

    DmDocument imported;
    ASSERT_TRUE(runtime.importFile(imported, source));

    // 路径本身也带中文，顺带覆盖插件 ABI 上 UTF-8 路径的传递
    const QString exported = dir.filePath(QStringLiteral("导出_r2013.dxf"));
    ASSERT_TRUE(runtime.exportFile(imported, exported));

    // 插件导出 R2013，字符串按 UTF-8 写入
    QFile file(exported);
    ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    const QByteArray bytes = file.readAll();
    EXPECT_TRUE(bytes.contains(QStringLiteral("中文文字").toUtf8()));
    EXPECT_TRUE(bytes.contains(QStringLiteral("图层甲").toUtf8()));
    EXPECT_FALSE(bytes.contains(QByteArray::fromStdString(GBK_TEXT)));
    file.close();

    DmDocument reimported;
    ASSERT_TRUE(runtime.importFile(reimported, exported));
    expectChineseContent(reimported);
}

namespace
{
/// @brief 统计 DXF 文件 ENTITIES 段里各类型的顶层实体数
std::map<QString, int> countDxfEntities(const QString& path)
{
    std::map<QString, int> counts;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        return counts;
    }
    bool inEntities = false;
    QByteArray previousValue;
    while (!file.atEnd())
    {
        const QByteArray code = file.readLine().trimmed();
        const QByteArray value = file.readLine().trimmed();
        if (code == "2" && previousValue == "SECTION")
        {
            inEntities = (value == "ENTITIES");
        }
        else if (code == "0")
        {
            if (value == "ENDSEC")
            {
                inEntities = false;
            }
            else if (inEntities)
            {
                ++counts[QString::fromLatin1(value)];
            }
        }
        previousValue = value;
    }
    return counts;
}

/// @brief DXF 实体类型对应的模型实体类型
DM::EntityType modelType(const QString& dxfType)
{
    static const std::map<QString, DM::EntityType> types = {
        {QStringLiteral("LINE"), DM::EntityLine},
        {QStringLiteral("CIRCLE"), DM::EntityCircle},
        {QStringLiteral("ARC"), DM::EntityArc},
        {QStringLiteral("LWPOLYLINE"), DM::EntityPolyline},
        {QStringLiteral("SPLINE"), DM::EntitySpline},
        {QStringLiteral("HATCH"), DM::EntityHatch},
        {QStringLiteral("INSERT"), DM::EntityBlockReference},
        {QStringLiteral("TEXT"), DM::EntityText},
        {QStringLiteral("MTEXT"), DM::EntityMText},
    };
    const auto it = types.find(dxfType);
    return it == types.end() ? DM::EntityUnknown : it->second;
}
} // namespace

TEST(DxfEncodingTest, 基准图纸逐类型完整导入)
{
    const QString dir = qEnvironmentVariable("YICAD_BENCHMARK_DIR");
    if (dir.isEmpty())
    {
        GTEST_SKIP() << "未设置 YICAD_BENCHMARK_DIR（tools/gen_benchmark_drawings.py 的输出目录）";
    }

    DxfRuntime runtime;
    ASSERT_TRUE(runtime.loaded()) << runtime.diagnostics().toStdString();

    for (const char* name : {"benchmark_small.dxf", "benchmark_medium.dxf", "benchmark_large.dxf"})
    {
        SCOPED_TRACE(name);
        const QString path = QDir(dir).filePath(QString::fromLatin1(name));
        ASSERT_TRUE(QFile::exists(path)) << path.toStdString();

        const std::map<QString, int> expected = countDxfEntities(path);
        ASSERT_FALSE(expected.empty());

        DmDocument document;
        ASSERT_TRUE(runtime.importFile(document, path));

        std::map<DM::EntityType, int> imported;
        for (DmEntity* entity : *document.getEntityTable())
        {
            ++imported[entity->getEntityType()];
        }
        int expectedTotal = 0;
        for (const auto& [dxfType, count] : expected)
        {
            const DM::EntityType type = modelType(dxfType);
            ASSERT_NE(type, DM::EntityUnknown) << "未覆盖的 DXF 实体类型 " << dxfType.toStdString();
            EXPECT_EQ(imported[type], count) << dxfType.toStdString();
            expectedTotal += count;
        }
        int importedTotal = 0;
        for (const auto& [type, count] : imported)
        {
            importedTotal += count;
        }
        EXPECT_EQ(importedTotal, expectedTotal);
    }
}
