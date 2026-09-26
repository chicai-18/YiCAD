/// @file test_persistence_document.cpp
/// @brief 整文档 OCD 读写的回归测试（分层重组方案 S0）
///
/// doc/LAYER_RESTRUCTURE_PLAN.md 的 S4 改动了文档读写路径：原生格式与格式注册表下沉到
/// Model，存盘策略移出 DmDocument。本文件锁住 Model 这一层：
///
/// - 写出：构造一份含全部一等实体、块定义与块引用（含属性）、多个图层、线型、文字样式与
///   标注样式的文档（tests/support/OcdSampleDocument.h），经 FilterOcdIO 写出，逐项检查
///   压缩包的条目与 Document.xml；
/// - 往返：把写出的文件读回新文档，逐项比对；
/// - 异常路径：空文件、非压缩包、截断的文件、不存在的文件，经 DmDocument::readFile 读；
/// - 只链接 YiCadModel、不装宿主服务，经 DmDocument::readFile、writeFile 读写整份文档（方案 8.6 节）。
///
/// 存盘策略（.bak 备份、外部修改检测、打开失败的处理）在 S4c 移到 Application 的
/// DocumentFileService，用例在 tests/interaction/test_document_file_service.cpp。
/// 读回的文档按产品的做法构造：新建 DmDocument 再导入。
///
/// ## 读回路径的缺陷
///
/// S0 查出读回路径 R1–R9 九处缺陷，编号与机理见 doc/LAYER_RESTRUCTURE_PLAN.md 4.5 节。
/// R1–R3、R5、R6、R9 已在 D8 修复步修复（同文档 4.6 节），R7、R8 在 S4c 修复（同文档 8.8 节），
/// R4 按 12 节 D9 的做法 A 修复，对应用例都已启用。
///
/// R4：新建的 DmDocument 在各表的 setDocument 里已经放好 "0" 图层、"Standard" 文字样式、
/// "ISO-25" 标注样式与 19 个标注箭头块，读入时又原样 add_direct 一份，同名条目各有两份。
/// 现在 FilterOcdIO::fileImport 读入前清空实体、块、标注样式、文字样式与图层，读完再补上
/// 文件里缺少的默认条目。

#include <gtest/gtest.h>

#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <QByteArray>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QStringList>
#include <QTemporaryDir>
#include <QXmlStreamReader>

#include "ArcData.h"
#include "AttributeData.h"
#include "AttributeDefinitionData.h"
#include "CircleData.h"
#include "DmArc.h"
#include "DmAttribute.h"
#include "DmAttributeDefinition.h"
#include "DmBlock.h"
#include "DmBlockReference.h"
#include "DmBlockTable.h"
#include "DmCircle.h"
#include "DmDimAligned.h"
#include "DmDimAngular.h"
#include "DmDimDiametric.h"
#include "DmDimLinear.h"
#include "DmDimRadial.h"
#include "DmDimensionStyle.h"
#include "DmDimensionStyleTable.h"
#include "DmDocument.h"
#include "DmEllipse.h"
#include "DmEntityContainer.h"
#include "DmHatch.h"
#include "DmLayer.h"
#include "DmLayerTable.h"
#include "DmLeader.h"
#include "DmLine.h"
#include "DmLineType.h"
#include "DmLineTypeTable.h"
#include "DmMText.h"
#include "DmPen.h"
#include "DmPoint.h"
#include "DmPolyline.h"
#include "DmRay.h"
#include "DmRegion.h"
#include "DmSolid.h"
#include "DmSpline.h"
#include "DmText.h"
#include "DmTextStyle.h"
#include "DmTextStyleTable.h"
#include "DmVector.h"
#include "DmXline.h"
#include "EllipseData.h"
#include "EntityTable.h"
#include "FilterOcdIO.h"
#include "HatchData.h"
#include "MTextData.h"
#include "MinizipNgArchive.h"
#include "PointData.h"
#include "PolylineData.h"
#include "RayData.h"
#include "RegionData.h"
#include "SolidData.h"
#include "SplineData.h"
#include "TextData.h"
#include "XLineData.h"
#include "support/OcdSampleDocument.h"

using namespace yicad_test;

namespace
{
constexpr double kTol = 1e-9;

/// 新建文档自带的标注箭头块数（DmDimensionStyleTable::initArrowBlocks）
constexpr int kArrowBlocks = 19;

/// @brief 按类型统计实体表里的实体
std::map<DM::EntityType, int> countByType(const EntityTable& table)
{
    std::map<DM::EntityType, int> counts;
    for (DmEntity* e : table)
    {
        ++counts[e->getEntityType()];
    }
    return counts;
}

/// @brief 统计表里名字为 name 的条目数
template <typename Table>
int countNamed(Table* table, const QString& name)
{
    int n = 0;
    for (auto it = table->begin(); it != table->end(); ++it)
    {
        if ((*it)->getName() == name)
        {
            ++n;
        }
    }
    return n;
}

/// @brief 统计线型表里名字为 name 的线型数（DmLineType 的名字接口与其他表不同）
int countLineTypes(DmLineTypeTable* table, const QString& name)
{
    int n = 0;
    for (auto it = table->begin(); it != table->end(); ++it)
    {
        if ((*it)->getLineTypeName() == name)
        {
            ++n;
        }
    }
    return n;
}

void expectVectorNear(const DmVector& got, const DmVector& expected, double tol = kTol)
{
    EXPECT_NEAR(got.x, expected.x, tol);
    EXPECT_NEAR(got.y, expected.y, tol);
}

/// @brief 用例夹具：提供临时目录。不装宿主服务：Model 读写文件不经宿主
struct OcdFixture : ::testing::Test
{
    QTemporaryDir dir;

    QString path(const QString& name) const { return dir.filePath(name); }

    /// @brief 构造样本文档
    void build(DmDocument& doc)
    {
        buildSample(doc);
    }

    /// @brief 经 FilterOcdIO 写出
    void exportTo(DmDocument& doc, const QString& file)
    {
        FilterOcdIO filter;
        ASSERT_TRUE(filter.canExport(kOcdFormat));
        ASSERT_TRUE(filter.fileExport(doc, file, kOcdFormat));
        ASSERT_TRUE(QFileInfo::exists(file));
    }

    /// @brief 经 FilterOcdIO 读入，读完与产品一样重新生成
    void importFrom(DmDocument& doc, const QString& file)
    {
        FilterOcdIO filter;
        ASSERT_TRUE(filter.canImport(file));
        ASSERT_TRUE(filter.fileImport(doc, file));
        doc.regenerate();
    }

    /// @brief 构造样本到 original、写出、读回到 restored
    void roundTrip(DmDocument& original, DmDocument& restored, const QString& name = QStringLiteral("sample.ycd"))
    {
        ASSERT_NO_FATAL_FAILURE(build(original));
        const QString file = path(name);
        ASSERT_NO_FATAL_FAILURE(exportTo(original, file));
        ASSERT_NO_FATAL_FAILURE(importFrom(restored, file));
    }
};

using OcdDocumentWrite = OcdFixture;
using OcdDocumentRoundTrip = OcdFixture;
using OcdDocumentErrorPath = OcdFixture;
using DocumentReadWrite = OcdFixture;
}  // namespace

// ---------------------------------------------------------------------------
// 写出（今天可用；S4 搬移读写代码时必须保持）
// ---------------------------------------------------------------------------

TEST_F(OcdDocumentWrite, 样本文档构造完整)
{
    // 先确认样本本身是想要的样子，否则后面的比对没有意义
    DmDocument doc;
    ASSERT_NO_FATAL_FAILURE(build(doc));
    EXPECT_EQ(countByType(*doc.getEntityTable()), kModelSpaceCounts);
    EXPECT_EQ(doc.getLayerTable()->count(), 3u);
    EXPECT_EQ(doc.getBlockTable()->count(), static_cast<unsigned int>(kArrowBlocks + 1));
    DmBlock* block = doc.getBlockTable()->find(kBlockName);
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->getEntityTable().count(), 3);
    EXPECT_EQ(block->getAttributeDefinitions().size(), 1u);
}

TEST_F(OcdDocumentWrite, 压缩包依次是文档与各类实体文件)
{
    DmDocument doc;
    ASSERT_NO_FATAL_FAILURE(build(doc));
    const QString file = path(QStringLiteral("layout.ycd"));
    ASSERT_NO_FATAL_FAILURE(exportTo(doc, file));

    const std::vector<ArchiveEntry> entries = readArchive(file);
    std::vector<std::string> names;
    std::map<std::string, size_t> sizes;
    for (const ArchiveEntry& e : entries)
    {
        names.push_back(e.name);
        sizes[e.name] = e.data.size();
    }
    // 顺序即 FilterOcdIO::saveXML 调各容器 addFile 的顺序；读回时 XMLReader::readFiles 按此顺序匹配
    const std::vector<std::string> expected = {
        "Document.xml",        "TextStyles.bin",     "DimensionStyles.bin",
        "Blocks.bin",          "Lines.bin",          "Circles.bin",
        "Arcs.bin",            "Points.bin",         "Ellipses.bin",
        "Solids.bin",          "Triangles.bin",      "Rays.bin",
        "Xlines.bin",          "Polylines.bin",      "Splines.bin",
        "BlockReferences.bin", "Texts.bin",          "MTexts.bin",
        "AttributeDefinitions.bin", "Attributes.bin", "DimLinears.bin",
        "DimAligneds.bin",     "DimAngulars.bin",    "DimRadials.bin",
        "DimDiametrics.bin",   "DimLeaders.bin",     "Hatchs.bin",
    };
    EXPECT_EQ(names, expected);

    // 有实体的类型各有数据；属性定义在块里、属性值随块引用存，顶层的这两个文件为空
    for (const char* name : {"TextStyles.bin", "DimensionStyles.bin", "Blocks.bin", "Lines.bin", "Points.bin",
                             "Rays.bin", "Xlines.bin", "BlockReferences.bin", "MTexts.bin", "DimRadials.bin",
                             "Hatchs.bin"})
    {
        EXPECT_GT(sizes[name], 0u) << name;
    }
    EXPECT_EQ(sizes["Triangles.bin"], 0u);
    EXPECT_EQ(sizes["AttributeDefinitions.bin"], 0u);
    EXPECT_EQ(sizes["Attributes.bin"], 0u);
}

TEST_F(OcdDocumentWrite, 文档XML记录各表与各类实体的数量)
{
    DmDocument doc;
    ASSERT_NO_FATAL_FAILURE(build(doc));
    const QString file = path(QStringLiteral("xml.ycd"));
    ASSERT_NO_FATAL_FAILURE(exportTo(doc, file));

    const std::vector<ArchiveEntry> entries = readArchive(file);
    ASSERT_FALSE(entries.empty());
    ASSERT_EQ(entries.front().name, "Document.xml");

    // 用 Qt 的解析器独立读一遍，不依赖被测的 XMLReader
    QXmlStreamReader xml(QByteArray::fromStdString(entries.front().data));
    std::map<QString, int> counts;
    QString root;
    QString activeTextStyle;
    QString activeDimStyle;
    QString activeLayer;
    QSet<QString> layerNames;
    QStringList lineTypes;
    const QSet<QString> countedElements = {
        QStringLiteral("LineTypes"),       QStringLiteral("Layers"),        QStringLiteral("TextStyles"),
        QStringLiteral("DimensionStyles"), QStringLiteral("Blocks"),        QStringLiteral("Entities"),
        QStringLiteral("Lines"),           QStringLiteral("Circles"),       QStringLiteral("Arcs"),
        QStringLiteral("Points"),          QStringLiteral("Ellipses"),      QStringLiteral("Solids"),
        QStringLiteral("Triangles"),       QStringLiteral("Rays"),          QStringLiteral("Xlines"),
        QStringLiteral("Polylines"),       QStringLiteral("Splines"),       QStringLiteral("BlockReferences"),
        QStringLiteral("Texts"),           QStringLiteral("MTexts"),        QStringLiteral("AttributeDefinitions"),
        QStringLiteral("Attributes"),      QStringLiteral("DimLinears"),    QStringLiteral("DimAligneds"),
        QStringLiteral("DimAngulars"),     QStringLiteral("DimRadials"),    QStringLiteral("DimDiametrics"),
        QStringLiteral("DimLeaders"),      QStringLiteral("Hatchs"),
    };
    while (!xml.atEnd())
    {
        if (xml.readNext() != QXmlStreamReader::StartElement)
        {
            continue;
        }
        const QString name = xml.name().toString();
        const QXmlStreamAttributes attrs = xml.attributes();
        if (root.isEmpty())
        {
            root = name;
        }
        if (countedElements.contains(name))
        {
            counts[name] = attrs.value(QStringLiteral("Count")).toInt();
        }
        if (name == QStringLiteral("TextStyles"))
        {
            activeTextStyle = attrs.value(QStringLiteral("active")).toString();
        }
        else if (name == QStringLiteral("DimensionStyles"))
        {
            activeDimStyle = attrs.value(QStringLiteral("active")).toString();
        }
        else if (name == QStringLiteral("Layer"))
        {
            // 图层名按 base64 写（MetaLayersContainer::saveXML 经 Persistence::encode）
            const QString layerName =
                QString::fromUtf8(QByteArray::fromBase64(attrs.value(QStringLiteral("name")).toLatin1()));
            layerNames.insert(layerName);
            if (attrs.value(QStringLiteral("active")) == QStringLiteral("1"))
            {
                activeLayer = layerName;
            }
        }
        else if (name == QStringLiteral("LineType") && attrs.hasAttribute(QStringLiteral("active")))
        {
            lineTypes.append(attrs.value(QStringLiteral("name")).toString());
        }
    }
    ASSERT_FALSE(xml.hasError()) << xml.errorString().toStdString();

    EXPECT_EQ(root, QStringLiteral("EntityContainer"));
    EXPECT_EQ(lineTypes, (QStringList{QStringLiteral("ByLayer"), QStringLiteral("ByBlock"),
                                      QStringLiteral("Continuous"), kLineTypeName}));
    EXPECT_EQ(layerNames, (QSet<QString>{QStringLiteral("0"), kLayerOutline, kLayerHidden}));
    EXPECT_EQ(activeLayer, kLayerOutline);
    EXPECT_EQ(activeTextStyle, kTextStyleName);
    EXPECT_EQ(activeDimStyle, kDimStyleName);

    const std::map<QString, int> expected = {
        {QStringLiteral("LineTypes"), 4},
        {QStringLiteral("Layers"), 3},
        {QStringLiteral("TextStyles"), 2},       // Standard + 工程字
        {QStringLiteral("DimensionStyles"), 2},  // ISO-25 + 机械
        {QStringLiteral("Blocks"), kArrowBlocks + 1},
        {QStringLiteral("Entities"), 20},
        {QStringLiteral("Lines"), 1},
        {QStringLiteral("Circles"), 1},
        {QStringLiteral("Arcs"), 1},
        {QStringLiteral("Points"), 1},
        {QStringLiteral("Ellipses"), 1},
        {QStringLiteral("Solids"), 1},
        {QStringLiteral("Triangles"), 0},
        {QStringLiteral("Rays"), 1},
        {QStringLiteral("Xlines"), 1},
        {QStringLiteral("Polylines"), 1},
        {QStringLiteral("Splines"), 1},
        {QStringLiteral("BlockReferences"), 1},
        {QStringLiteral("Texts"), 1},
        {QStringLiteral("MTexts"), 1},
        {QStringLiteral("AttributeDefinitions"), 0},
        {QStringLiteral("Attributes"), 0},
        {QStringLiteral("DimLinears"), 1},
        {QStringLiteral("DimAligneds"), 1},
        {QStringLiteral("DimAngulars"), 1},
        {QStringLiteral("DimRadials"), 1},
        {QStringLiteral("DimDiametrics"), 1},
        {QStringLiteral("DimLeaders"), 1},
        {QStringLiteral("Hatchs"), 1},
    };
    EXPECT_EQ(counts, expected);
}

TEST_F(OcdDocumentWrite, 覆盖已有文件时旧文件改名为编号备份)
{
    // FilterOcdIO::fileExport 自己带一层备份（BackupPolicy::Standard，保留 1 份）：
    // 目标已存在时先把它改名为 "<文件名>1"，再把临时文件改名为目标。
    DmDocument doc;
    ASSERT_NO_FATAL_FAILURE(build(doc));
    const QString file = path(QStringLiteral("overwrite.ycd"));
    ASSERT_NO_FATAL_FAILURE(exportTo(doc, file));
    const QByteArray firstBytes = readBytes(file);
    ASSERT_NO_FATAL_FAILURE(exportTo(doc, file));

    EXPECT_TRUE(QFileInfo::exists(file + QStringLiteral("1")));
    EXPECT_EQ(readBytes(file + QStringLiteral("1")), firstBytes);
    // 临时文件（<文件名>.<uuid>）不残留
    const QStringList left = QDir(dir.path()).entryList(QDir::Files);
    EXPECT_EQ(left, (QStringList{QStringLiteral("overwrite.ycd"), QStringLiteral("overwrite.ycd1")}));
}

// ---------------------------------------------------------------------------
// 往返
// ---------------------------------------------------------------------------

TEST_F(OcdDocumentRoundTrip, 基本曲线的几何不变)
{
    DmDocument original;
    DmDocument restored;
    ASSERT_NO_FATAL_FAILURE(roundTrip(original, restored));
    const EntityTable& model = *restored.getEntityTable();

    auto* line = first<DmLine>(model, DM::EntityLine);
    ASSERT_NE(line, nullptr);
    expectVectorNear(line->getStartpoint(), DmVector(1.5, -2.25));
    expectVectorNear(line->getEndpoint(), DmVector(30.75, 41.125));

    auto* circle = first<DmCircle>(model, DM::EntityCircle);
    ASSERT_NE(circle, nullptr);
    expectVectorNear(circle->getCenter(), DmVector(12.0, -34.5));
    EXPECT_NEAR(circle->getRadius(), 6.125, kTol);

    auto* arc = first<DmArc>(model, DM::EntityArc);
    ASSERT_NE(arc, nullptr);
    expectVectorNear(arc->getCenter(), DmVector(3.0, 4.0));
    EXPECT_NEAR(arc->getRadius(), 7.5, kTol);
    EXPECT_NEAR(arc->getStartAngle(), 0.25, kTol);
    EXPECT_NEAR(arc->getEndAngle(), 2.75, kTol);

    auto* ellipse = first<DmEllipse>(model, DM::EntityEllipse);
    ASSERT_NE(ellipse, nullptr);
    expectVectorNear(ellipse->getCenter(), DmVector(1.0, 2.0));
    expectVectorNear(ellipse->getMajorP(), DmVector(8.0, 0.0));
    EXPECT_NEAR(ellipse->getRatio(), 0.5, kTol);

    auto* solid = first<DmSolid>(model, DM::EntitySolid);
    ASSERT_NE(solid, nullptr);
    expectVectorNear(solid->getCorner(0), DmVector(50.0, 0.0));
    expectVectorNear(solid->getCorner(1), DmVector(60.0, 0.0));
    expectVectorNear(solid->getCorner(2), DmVector(55.0, 8.0));

    auto* polyline = first<DmPolyline>(model, DM::EntityPolyline);
    ASSERT_NE(polyline, nullptr);
    EXPECT_EQ(polyline->getVertexCount(), 4);
    EXPECT_TRUE(polyline->isClosed());
    EXPECT_NEAR(polyline->getBulgeAt(1), 0.5, kTol);

    auto* spline = first<DmSpline>(model, DM::EntitySpline);
    ASSERT_NE(spline, nullptr);
    EXPECT_EQ(spline->getDegree(), 3);
    const std::vector<DmVector> controlPoints = spline->getControlPoints();
    ASSERT_EQ(controlPoints.size(), 4u);
    expectVectorNear(controlPoints.front(), DmVector(0.0, 50.0));
    expectVectorNear(controlPoints.back(), DmVector(30.0, 50.0));
}

TEST_F(OcdDocumentRoundTrip, 点射线与构造线不变)
{
    DmDocument original;
    DmDocument restored;
    ASSERT_NO_FATAL_FAILURE(roundTrip(original, restored));
    std::map<DM::EntityType, int> counts = countByType(*restored.getEntityTable());
    EXPECT_EQ(counts[DM::EntityPoint], 1);
    EXPECT_EQ(counts[DM::EntityRay], 1);
    EXPECT_EQ(counts[DM::EntityXline], 1);
    const EntityTable& model = *restored.getEntityTable();

    auto* point = first<DmPoint>(model, DM::EntityPoint);
    ASSERT_NE(point, nullptr);
    expectVectorNear(point->getPos(), DmVector(-7.0, 9.0));
    ASSERT_NE(point->getLayer(), nullptr);
    EXPECT_EQ(point->getLayer()->getName(), QStringLiteral("0"));

    auto* ray = first<DmRay>(model, DM::EntityRay);
    ASSERT_NE(ray, nullptr);
    expectVectorNear(ray->getBasePoint(), DmVector(0.0, 100.0));
    ASSERT_NE(ray->getLayer(), nullptr);
    EXPECT_EQ(ray->getLayer()->getName(), kLayerHidden);

    auto* xline = first<DmXline>(model, DM::EntityXline);
    ASSERT_NE(xline, nullptr);
    expectVectorNear(xline->getBasePoint(), DmVector(0.0, -100.0));
    ASSERT_NE(xline->getLayer(), nullptr);
    EXPECT_EQ(xline->getLayer()->getName(), kLayerHidden);
}

TEST_F(OcdDocumentRoundTrip, 各类实体数量与类型不变)
{
    DmDocument original;
    DmDocument restored;
    ASSERT_NO_FATAL_FAILURE(roundTrip(original, restored));
    EXPECT_EQ(countByType(*restored.getEntityTable()), kModelSpaceCounts);
}

TEST_F(OcdDocumentRoundTrip, 实体的图层与画笔不变)
{
    DmDocument original;
    DmDocument restored;
    ASSERT_NO_FATAL_FAILURE(roundTrip(original, restored));
    const EntityTable& model = *restored.getEntityTable();

    auto* line = first<DmLine>(model, DM::EntityLine);
    ASSERT_NE(line, nullptr);
    ASSERT_NE(line->getLayer(), nullptr);
    EXPECT_EQ(line->getLayer()->getName(), kLayerOutline);
    const DmPen pen = line->getPen(false);
    EXPECT_EQ(pen.getColor().red(), 0);
    EXPECT_EQ(pen.getColor().green(), 255);
    EXPECT_EQ(pen.getColor().blue(), 0);
    EXPECT_EQ(pen.getWidth(), DM::Width07);
    ASSERT_NE(pen.getLineType(), nullptr);
    EXPECT_EQ(pen.getLineType()->getLineTypeName(), kLineTypeName);

    auto* ellipse = first<DmEllipse>(model, DM::EntityEllipse);
    ASSERT_NE(ellipse, nullptr);
    ASSERT_NE(ellipse->getLayer(), nullptr);
    EXPECT_EQ(ellipse->getLayer()->getName(), QStringLiteral("0"));
    EXPECT_TRUE(ellipse->getPen(false).getColor().isByLayer());

    auto* hatch = first<DmHatch>(model, DM::EntityHatch);
    ASSERT_NE(hatch, nullptr);
    ASSERT_NE(hatch->getLayer(), nullptr);
    EXPECT_EQ(hatch->getLayer()->getName(), kLayerOutline);
}

TEST_F(OcdDocumentRoundTrip, 文字与多行文字不变)
{
    DmDocument original;
    DmDocument restored;
    ASSERT_NO_FATAL_FAILURE(roundTrip(original, restored));
    const EntityTable& model = *restored.getEntityTable();

    auto* text = first<DmText>(model, DM::EntityText);
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->getText(), kTextValue);
    EXPECT_NEAR(text->getHeight(), 3.5, kTol);
    ASSERT_NE(text->getStyle(), nullptr);
    EXPECT_EQ(text->getStyle()->getName(), kTextStyleName);

    auto* mtext = first<DmMText>(model, DM::EntityMText);
    ASSERT_NE(mtext, nullptr);
    EXPECT_EQ(mtext->getContent(), kMTextValue);
    // getHeight() 是排版框的高度（DmMText.cpp 取 defineHeight），字高是 getCharHeight()
    EXPECT_NEAR(mtext->getCharHeight(), 2.5, kTol);
    ASSERT_NE(mtext->getStyle(), nullptr);
    EXPECT_EQ(mtext->getStyle()->getName(), kTextStyleName);
}

TEST_F(OcdDocumentRoundTrip, 标注与引线不变)
{
    DmDocument original;
    DmDocument restored;
    ASSERT_NO_FATAL_FAILURE(roundTrip(original, restored));
    const EntityTable& before = *original.getEntityTable();
    const EntityTable& model = *restored.getEntityTable();

    auto* linear = first<DmDimLinear>(model, DM::EntityDimLinear);
    ASSERT_NE(linear, nullptr);
    expectVectorNear(linear->getExtensionPoint1(), DmVector(0.0, 0.0));
    expectVectorNear(linear->getExtensionPoint2(), DmVector(20.0, 0.0));
    expectVectorNear(linear->getDefinitionPoint(), DmVector(20.0, -10.0));

    auto* aligned = first<DmDimAligned>(model, DM::EntityDimAligned);
    ASSERT_NE(aligned, nullptr);
    expectVectorNear(aligned->getExtensionPoint1(), DmVector(0.0, 10.0));
    expectVectorNear(aligned->getExtensionPoint2(), DmVector(20.0, 10.0));

    auto* angular = first<DmDimAngular>(model, DM::EntityDimAngular);
    ASSERT_NE(angular, nullptr);
    const DmDimAngularData angularData = angular->getEData();
    expectVectorNear(angularData.line1EndPt, DmVector(10.0, 0.0));
    expectVectorNear(angularData.line2EndPt, DmVector(0.0, 10.0));

    // 半径与直径标注在构造、更新之后定义点已经变成箭头点（与读写无关），所以与写出前的文档比
    auto* radial = first<DmDimRadial>(model, DM::EntityDimRadial);
    auto* radialBefore = first<DmDimRadial>(before, DM::EntityDimRadial);
    ASSERT_NE(radial, nullptr);
    ASSERT_NE(radialBefore, nullptr);
    expectVectorNear(radial->getDefinitionPoint(), radialBefore->getDefinitionPoint());
    expectVectorNear(radial->getEData().endPoint, radialBefore->getEData().endPoint);
    EXPECT_NEAR(radial->getEData().leader, radialBefore->getEData().leader, kTol);

    auto* diametric = first<DmDimDiametric>(model, DM::EntityDimDiametric);
    auto* diametricBefore = first<DmDimDiametric>(before, DM::EntityDimDiametric);
    ASSERT_NE(diametric, nullptr);
    ASSERT_NE(diametricBefore, nullptr);
    expectVectorNear(diametric->getDefinitionPoint(), diametricBefore->getDefinitionPoint());
    expectVectorNear(diametric->getEData().endPoint, diametricBefore->getEData().endPoint);

    // 五种标注都挂在读回文档的同名标注样式上
    for (DmEntity* e : model)
    {
        if (auto* dim = dynamic_cast<DmDimension*>(e))
        {
            SCOPED_TRACE(static_cast<int>(e->getEntityType()));
            ASSERT_NE(dim->getStyle(), nullptr);
            EXPECT_EQ(dim->getStyle()->getName(), kDimStyleName);
            EXPECT_NE(restored.getDimStyleTable()->find(dim->getStyle()->getId()), nullptr)
                << "标注样式不是读回文档自己的";
        }
    }

    auto* leader = first<DmLeader>(model, DM::EntityDimLeader);
    ASSERT_NE(leader, nullptr);
    const DmLeaderData leaderData = leader->getData();
    ASSERT_EQ(leaderData.vertextes.size(), 3u);
    expectVectorNear(leaderData.vertextes[0], DmVector(70.0, 0.0));
    expectVectorNear(leaderData.vertextes[2], DmVector(90.0, 10.0));
}

TEST_F(OcdDocumentRoundTrip, 填充不变)
{
    DmDocument original;
    DmDocument restored;
    ASSERT_NO_FATAL_FAILURE(roundTrip(original, restored));

    auto* hatch = first<DmHatch>(*restored.getEntityTable(), DM::EntityHatch);
    ASSERT_NE(hatch, nullptr);
    EXPECT_TRUE(hatch->isSolid());
    // 边界是 20×15 的矩形
    expectVectorNear(hatch->getMin(), DmVector(100.0, 0.0), 1e-6);
    expectVectorNear(hatch->getMax(), DmVector(120.0, 15.0), 1e-6);
}

TEST_F(OcdDocumentRoundTrip, 块定义与块引用不变)
{
    DmDocument original;
    DmDocument restored;
    ASSERT_NO_FATAL_FAILURE(roundTrip(original, restored));

    DmBlock* block = restored.getBlockTable()->find(kBlockName);
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(countByType(block->getEntityTable()),
              (std::map<DM::EntityType, int>{
                  {DM::EntityCircle, 1}, {DM::EntityLine, 1}, {DM::EntityAttributeDefinition, 1}}));
    const std::list<DmAttributeDefinition*> definitions = block->getAttributeDefinitions();
    ASSERT_EQ(definitions.size(), 1u);
    EXPECT_EQ(definitions.front()->getTag(), kAttributeTag);
    EXPECT_EQ(definitions.front()->getPrompt(), QStringLiteral("输入规格"));

    auto* insert = first<DmBlockReference>(*restored.getEntityTable(), DM::EntityBlockReference);
    ASSERT_NE(insert, nullptr);
    EXPECT_EQ(insert->getName(), kBlockName);
    EXPECT_EQ(insert->getBlockForInsert(), block) << "块引用没有指向读回文档里的块";
    expectVectorNear(insert->getInsertionPoint(), DmVector(100.0, 50.0));
    expectVectorNear(insert->getScale(), DmVector(2.0, 2.0));
    EXPECT_NEAR(insert->getAngle(), 0.5, kTol);

    const std::list<DmAttribute*> attributes = insert->getAttributes();
    ASSERT_EQ(attributes.size(), 1u);
    EXPECT_EQ(attributes.front()->getTag(), kAttributeTag);
    EXPECT_EQ(attributes.front()->getText(), kAttributeValue);
}

TEST_F(OcdDocumentRoundTrip, 图层表内容不变)
{
    DmDocument original;
    DmDocument restored;
    ASSERT_NO_FATAL_FAILURE(roundTrip(original, restored));
    DmLayerTable* layers = restored.getLayerTable();

    DmLayer* outline = layers->find(kLayerOutline);
    ASSERT_NE(outline, nullptr);
    EXPECT_FALSE(outline->isFrozen());
    EXPECT_FALSE(outline->isLocked());
    EXPECT_TRUE(outline->isPrint());
    const DmPen outlinePen = outline->getPen();
    EXPECT_EQ(outlinePen.getColor().red(), 255);
    EXPECT_EQ(outlinePen.getColor().green(), 0);
    EXPECT_EQ(outlinePen.getColor().blue(), 0);
    EXPECT_EQ(outlinePen.getWidth(), DM::Width09);
    ASSERT_NE(outlinePen.getLineType(), nullptr);
    EXPECT_EQ(outlinePen.getLineType()->getLineTypeName(), kLineTypeName);

    DmLayer* hidden = layers->find(kLayerHidden);
    ASSERT_NE(hidden, nullptr);
    EXPECT_TRUE(hidden->isFrozen());
    EXPECT_TRUE(hidden->isLocked());
    EXPECT_FALSE(hidden->isPrint());
    EXPECT_EQ(hidden->getPen().getColor().blue(), 255);
    EXPECT_EQ(hidden->getPen().getWidth(), DM::Width05);

    ASSERT_NE(layers->getActive(), nullptr);
    EXPECT_EQ(layers->getActive()->getName(), kLayerOutline);
}

TEST_F(OcdDocumentRoundTrip, 线型数据不变)
{
    DmDocument original;
    DmDocument restored;
    ASSERT_NO_FATAL_FAILURE(roundTrip(original, restored));

    DmLineType* lineType = restored.getLineTypeTable()->find(kLineTypeName);
    ASSERT_NE(lineType, nullptr);
    EXPECT_EQ(lineType->getLineTypeData(), (std::vector<double>{0.5, -0.25, 0.0, -0.25}));
    // 固定线型只保留一份：读入时已存在则跳过（MetaLineTypes.cpp）
    EXPECT_EQ(countLineTypes(restored.getLineTypeTable(), QStringLiteral("ByLayer")), 1);
    EXPECT_EQ(countLineTypes(restored.getLineTypeTable(), QStringLiteral("Continuous")), 1);
}

TEST_F(OcdDocumentRoundTrip, 当前线型不变)
{
    DmDocument original;
    DmDocument restored;
    ASSERT_NO_FATAL_FAILURE(roundTrip(original, restored));
    ASSERT_NE(restored.getLineTypeTable()->getActive(), nullptr);
    EXPECT_EQ(restored.getLineTypeTable()->getActive()->getLineTypeName(),
              original.getLineTypeTable()->getActive()->getLineTypeName());
}

TEST_F(OcdDocumentRoundTrip, 当前线型为自定义或固定线型时都不变)
{
    // 固定线型（ByLayer、ByBlock、Continuous）读入时跳过、不重复添加，"是否为当前线型"仍要恢复
    for (const QString& name : {kLineTypeName, QStringLiteral("Continuous"), QStringLiteral("ByBlock")})
    {
        SCOPED_TRACE(name.toStdString());
        DmDocument original;
        ASSERT_NO_FATAL_FAILURE(build(original));
        DmLineType* lineType = original.getLineTypeTable()->find(name);
        ASSERT_NE(lineType, nullptr);
        original.getLineTypeTable()->activate_direct(lineType);

        const QString file = path(QStringLiteral("active_%1.ycd").arg(name));
        ASSERT_NO_FATAL_FAILURE(exportTo(original, file));
        DmDocument restored;
        ASSERT_NO_FATAL_FAILURE(importFrom(restored, file));
        ASSERT_NE(restored.getLineTypeTable()->getActive(), nullptr);
        EXPECT_EQ(restored.getLineTypeTable()->getActive()->getLineTypeName(), name);
    }
}

TEST_F(OcdDocumentRoundTrip, 线型说明不变)
{
    DmDocument original;
    DmDocument restored;
    ASSERT_NO_FATAL_FAILURE(roundTrip(original, restored));
    DmLineType* lineType = restored.getLineTypeTable()->find(kLineTypeName);
    ASSERT_NE(lineType, nullptr);
    EXPECT_EQ(lineType->getLineTypeDesp(), kLineTypeDesp);
}

TEST_F(OcdDocumentRoundTrip, 文字样式与标注样式表内容不变)
{
    DmDocument original;
    DmDocument restored;
    ASSERT_NO_FATAL_FAILURE(roundTrip(original, restored));

    DmTextStyle* textStyle = restored.getTextStyleTable()->find(kTextStyleName);
    ASSERT_NE(textStyle, nullptr);
    EXPECT_NEAR(textStyle->getData().defaultHeight, 3.5, kTol);
    EXPECT_NEAR(textStyle->getData().widhFactor, 0.7, kTol);
    ASSERT_NE(restored.getTextStyleTable()->getActive(), nullptr);
    EXPECT_EQ(restored.getTextStyleTable()->getActive()->getName(), kTextStyleName);

    DmDimensionStyle* dimStyle = restored.getDimStyleTable()->find(kDimStyleName);
    ASSERT_NE(dimStyle, nullptr);
    EXPECT_NEAR(dimStyle->getDataConstRef().arrowSize(), 3.0, kTol);
    EXPECT_NEAR(dimStyle->getDataConstRef().textHeight(), 3.5, kTol);
    ASSERT_NE(dimStyle->getDataConstRef().textStyle(), nullptr);
    EXPECT_EQ(dimStyle->getDataConstRef().textStyle()->getName(), kTextStyleName);
    ASSERT_NE(restored.getDimStyleTable()->getActive(), nullptr);
    EXPECT_EQ(restored.getDimStyleTable()->getActive()->getName(), kDimStyleName);
}

TEST_F(OcdDocumentRoundTrip, 读回不重复新文档自带的条目)
{
    DmDocument original;
    DmDocument restored;
    ASSERT_NO_FATAL_FAILURE(roundTrip(original, restored));
    EXPECT_EQ(restored.getLayerTable()->count(), original.getLayerTable()->count());
    EXPECT_EQ(countNamed(restored.getLayerTable(), QStringLiteral("0")), 1);
    EXPECT_EQ(countNamed(restored.getTextStyleTable(), QStringLiteral("Standard")), 1);
    EXPECT_EQ(countNamed(restored.getDimStyleTable(), QStringLiteral("ISO-25")), 1);
    EXPECT_EQ(restored.getBlockTable()->count(), original.getBlockTable()->count());
}

TEST_F(OcdDocumentRoundTrip, 中文路径往返)
{
    DmDocument original;
    DmDocument restored;
    ASSERT_NO_FATAL_FAILURE(roundTrip(original, restored, QStringLiteral("图纸 副本.ycd")));
    EXPECT_NE(restored.getLayerTable()->find(kLayerOutline), nullptr);
    EXPECT_NE(restored.getBlockTable()->find(kBlockName), nullptr);
    auto* text = first<DmText>(*restored.getEntityTable(), DM::EntityText);
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->getText(), kTextValue);
}

TEST_F(OcdDocumentRoundTrip, 写出读回再写出保持稳定)
{
    // 读回的文档再存、再读，内容必须与第一次读回的相同，否则说明读写不对称
    DmDocument original;
    DmDocument once;
    ASSERT_NO_FATAL_FAILURE(roundTrip(original, once));
    const QString again = path(QStringLiteral("again.ycd"));
    ASSERT_NO_FATAL_FAILURE(exportTo(once, again));
    DmDocument twice;
    ASSERT_NO_FATAL_FAILURE(importFrom(twice, again));
    EXPECT_EQ(countByType(*twice.getEntityTable()), kModelSpaceCounts);
    EXPECT_EQ(twice.getLayerTable()->count(), original.getLayerTable()->count());
    EXPECT_EQ(twice.getBlockTable()->count(), original.getBlockTable()->count());
    DmBlock* block = twice.getBlockTable()->find(kBlockName);
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->getEntityTable().count(), 3);
}

TEST_F(OcdDocumentRoundTrip, 默认条目按文件里的属性读回)
{
    // R4 修复前实体取到的是新文档自带的 "0" 图层、"Standard"、"ISO-25"，文件里改过的属性不生效
    DmDocument original;
    ASSERT_NO_FATAL_FAILURE(build(original));
    DmLayer* layer0 = original.getLayerTable()->find(QStringLiteral("0"));
    ASSERT_NE(layer0, nullptr);
    layer0->setPen(DmPen(DmColor(0, 255, 0), DM::Width05, DmLineTypeTable::Continuous));
    layer0->lock(true);
    DmTextStyle* standard = original.getTextStyleTable()->find(QStringLiteral("Standard"));
    ASSERT_NE(standard, nullptr);
    DmTextStyleData standardData = standard->getData();
    standardData.defaultHeight = 5.0;
    standard->setData(standardData);
    DmDimensionStyle* iso = original.getDimStyleTable()->find(QStringLiteral("ISO-25"));
    ASSERT_NE(iso, nullptr);
    iso->getDataRef().setArrowSize(4.0);
    const QString file = path(QStringLiteral("defaults.ycd"));
    ASSERT_NO_FATAL_FAILURE(exportTo(original, file));

    DmDocument restored;
    const DmId builtinLayer0 = restored.getLayerTable()->find(QStringLiteral("0"))->getId();
    ASSERT_NO_FATAL_FAILURE(importFrom(restored, file));

    DmLayer* restoredLayer0 = restored.getLayerTable()->find(QStringLiteral("0"));
    ASSERT_NE(restoredLayer0, nullptr);
    EXPECT_EQ(restoredLayer0->getId().asString(), layer0->getId().asString()) << "应是文件里的 \"0\" 图层";
    EXPECT_EQ(restored.findObject(builtinLayer0), nullptr) << "新文档自带的 \"0\" 图层应已删除并注销 id";
    EXPECT_EQ(restoredLayer0->getPen().getColor().green(), 255);
    EXPECT_EQ(restoredLayer0->getPen().getColor().red(), 0);
    EXPECT_TRUE(restoredLayer0->isLocked());
    auto* point = first<DmPoint>(*restored.getEntityTable(), DM::EntityPoint);
    ASSERT_NE(point, nullptr);
    EXPECT_EQ(point->getLayer(false), restoredLayer0) << "样本里的点在 \"0\" 图层上";

    DmTextStyle* restoredStandard = restored.getTextStyleTable()->find(QStringLiteral("Standard"));
    ASSERT_NE(restoredStandard, nullptr);
    EXPECT_NEAR(restoredStandard->getData().defaultHeight, 5.0, kTol);
    DmDimensionStyle* restoredIso = restored.getDimStyleTable()->find(QStringLiteral("ISO-25"));
    ASSERT_NE(restoredIso, nullptr);
    EXPECT_NEAR(restoredIso->getDataConstRef().arrowSize(), 4.0, kTol);
    EXPECT_EQ(restoredIso->getDataConstRef().textStyle(), restoredStandard);
}

TEST_F(OcdDocumentRoundTrip, 文件缺少的默认条目读完补上)
{
    // 默认条目改名、删去一个箭头块后写出，文件里就没有 "0"、"Standard"、"ISO-25" 与那个箭头块
    DmDocument original;
    ASSERT_NO_FATAL_FAILURE(build(original));
    original.getLayerTable()->find(QStringLiteral("0"))->setName(QStringLiteral("底图"));
    original.getTextStyleTable()->find(QStringLiteral("Standard"))->setName(QStringLiteral("仿宋"));
    original.getDimStyleTable()->find(QStringLiteral("ISO-25"))->setName(QStringLiteral("国标"));
    const QString dotName = DmDimensionStyle::getArrowBlockName(DM::ArrowType::Dot);
    DmBlock* dot = original.getBlockTable()->find(dotName);
    ASSERT_NE(dot, nullptr);
    original.getBlockTable()->remove_direct(dot);  // 只从表里拿掉，不删除对象
    dot->getEntityTable().clear_direct();
    original.getIdManager()->removeID(dot->getId());
    delete dot;
    const QString file = path(QStringLiteral("no_defaults.ycd"));
    ASSERT_NO_FATAL_FAILURE(exportTo(original, file));

    DmDocument restored;
    ASSERT_NO_FATAL_FAILURE(importFrom(restored, file));

    // 文件里的条目都在，缺的默认条目各补一个
    EXPECT_NE(restored.getLayerTable()->find(QStringLiteral("底图")), nullptr);
    EXPECT_EQ(countNamed(restored.getLayerTable(), QStringLiteral("0")), 1);
    EXPECT_NE(restored.getTextStyleTable()->find(QStringLiteral("仿宋")), nullptr);
    EXPECT_EQ(countNamed(restored.getTextStyleTable(), QStringLiteral("Standard")), 1);
    EXPECT_NE(restored.getDimStyleTable()->find(QStringLiteral("国标")), nullptr);
    EXPECT_EQ(countNamed(restored.getDimStyleTable(), QStringLiteral("ISO-25")), 1);
    EXPECT_NE(restored.getBlockTable()->find(dotName), nullptr);
    EXPECT_EQ(restored.getBlockTable()->count(), static_cast<unsigned int>(kArrowBlocks + 1));

    // 补上的 "ISO-25" 用补上的 "Standard"
    DmDimensionStyle* iso = restored.getDimStyleTable()->find(QStringLiteral("ISO-25"));
    ASSERT_NE(iso, nullptr);
    EXPECT_EQ(iso->getDataConstRef().textStyle(), restored.getTextStyleTable()->find(QStringLiteral("Standard")));

    // 文件有当前项，补默认条目不改动它们
    ASSERT_NE(restored.getLayerTable()->getActive(), nullptr);
    EXPECT_EQ(restored.getLayerTable()->getActive()->getName(), kLayerOutline);
    ASSERT_NE(restored.getTextStyleTable()->getActive(), nullptr);
    EXPECT_EQ(restored.getTextStyleTable()->getActive()->getName(), kTextStyleName);
    ASSERT_NE(restored.getDimStyleTable()->getActive(), nullptr);
    EXPECT_EQ(restored.getDimStyleTable()->getActive()->getName(), kDimStyleName);
}

TEST_F(OcdDocumentRoundTrip, 读进已有内容的文档时以文件为准)
{
    // DocumentFileService::open 读失败后把备份读进同一份文档，文档里可能还有读了一半的内容。
    // 读入前清空实体、块、标注样式、文字样式与图层，读完只剩文件里的
    DmDocument sample;
    ASSERT_NO_FATAL_FAILURE(build(sample));
    const QString sampleFile = path(QStringLiteral("sample.ycd"));
    ASSERT_NO_FATAL_FAILURE(exportTo(sample, sampleFile));

    DmDocument simple;
    auto* onlyLayer = new DmLayer();
    onlyLayer->setDocument(&simple);
    onlyLayer->setData(DmLayerData(QStringLiteral("唯一"), DmPen(DmColor(0, 0, 255), DM::Width05,
                                                                  DmLineTypeTable::Continuous),
                                   false, false));
    ASSERT_TRUE(simple.getLayerTable()->add_direct(onlyLayer));
    addTo(*simple.getEntityTable(), simple, new DmLine(DmVector(0.0, 0.0), DmVector(10.0, 0.0)), onlyLayer);
    const QString simpleFile = path(QStringLiteral("simple.ycd"));
    ASSERT_NO_FATAL_FAILURE(exportTo(simple, simpleFile));

    DmDocument doc;
    ASSERT_NO_FATAL_FAILURE(importFrom(doc, sampleFile));
    const DmId hiddenId = doc.getLayerTable()->find(kLayerHidden)->getId();
    ASSERT_NO_FATAL_FAILURE(importFrom(doc, simpleFile));

    EXPECT_EQ(countByType(*doc.getEntityTable()), (std::map<DM::EntityType, int>{{DM::EntityLine, 1}}));
    EXPECT_EQ(doc.getLayerTable()->count(), 2u);
    EXPECT_EQ(doc.getLayerTable()->find(kLayerOutline), nullptr);
    EXPECT_EQ(doc.findObject(hiddenId), nullptr) << "清掉的图层应已注销 id";
    EXPECT_EQ(doc.getTextStyleTable()->find(kTextStyleName), nullptr);
    EXPECT_EQ(doc.getDimStyleTable()->find(kDimStyleName), nullptr);
    EXPECT_EQ(doc.getBlockTable()->find(kBlockName), nullptr);
    EXPECT_EQ(doc.getBlockTable()->count(), static_cast<unsigned int>(kArrowBlocks));
    auto* line = first<DmLine>(*doc.getEntityTable(), DM::EntityLine);
    ASSERT_NE(line, nullptr);
    EXPECT_EQ(line->getLayer(false), doc.getLayerTable()->find(QStringLiteral("唯一")));
    ASSERT_NE(doc.getLayerTable()->getActive(), nullptr);
    EXPECT_EQ(doc.getLayerTable()->getActive()->getName(), QStringLiteral("0"));
    // 线型表读入前不清空（固定线型读入时已存在则跳过），第一次读入的自定义线型留着
    EXPECT_NE(doc.getLineTypeTable()->find(kLineTypeName), nullptr);
}

// ---------------------------------------------------------------------------
// 异常路径
// ---------------------------------------------------------------------------
//
// FilterInterface::fileImport 的签名返回 bool，但 FilterOcdIO 对坏文件一律抛异常
// （OneException 或 MinizipNgArchiveReader 的 std::runtime_error）。S4c 起 DmDocument::readFile
// 把异常转成 Failed，异常信息放进结果（R7）；这里经它读。

TEST_F(OcdDocumentErrorPath, 空文件读失败并带回原因)
{
    const QString file = path(QStringLiteral("empty.ycd"));
    ASSERT_TRUE(writeBytes(file, QByteArray()));
    DmDocument doc;
    const DmFileResult result = doc.readFile(file);
    EXPECT_EQ(result.status, DmFileStatus::Failed);
    // OneException 自己保存消息、覆盖 std::exception::what()，按 std::exception 捕获也能取到
    EXPECT_EQ(result.message, QStringLiteral("Invalid file"));
    EXPECT_EQ(doc.getEntityTable()->count(), 0);
}

TEST_F(OcdDocumentErrorPath, 不是压缩包的文件读失败)
{
    const QString file = path(QStringLiteral("garbage.ycd"));
    ASSERT_TRUE(writeBytes(file, QByteArray(4096, 'x')));
    DmDocument doc;
    const DmFileResult result = doc.readFile(file);
    EXPECT_EQ(result.status, DmFileStatus::Failed);
    EXPECT_FALSE(result.message.isEmpty());
    EXPECT_EQ(doc.getEntityTable()->count(), 0);
}

TEST_F(OcdDocumentErrorPath, 截断的文件读失败)
{
    DmDocument original;
    ASSERT_NO_FATAL_FAILURE(build(original));
    const QString good = path(QStringLiteral("good.ycd"));
    ASSERT_NO_FATAL_FAILURE(exportTo(original, good));
    const QByteArray bytes = readBytes(good);
    ASSERT_GT(bytes.size(), 64);

    const QString truncated = path(QStringLiteral("truncated.ycd"));
    ASSERT_TRUE(writeBytes(truncated, bytes.left(bytes.size() / 2)));
    DmDocument doc;
    const DmFileResult result = doc.readFile(truncated);
    EXPECT_EQ(result.status, DmFileStatus::Failed);
    EXPECT_FALSE(result.message.isEmpty());
}

TEST_F(OcdDocumentErrorPath, 不存在的文件读失败)
{
    DmDocument doc;
    const DmFileResult result = doc.readFile(path(QStringLiteral("missing.ycd")));
    EXPECT_EQ(result.status, DmFileStatus::Failed);
    EXPECT_FALSE(result.message.isEmpty());
}

TEST_F(OcdDocumentErrorPath, 过滤器本身仍然抛异常)
{
    // 转结果码是 DmDocument 这一层做的，FilterOcdIO 本身不变
    const QString file = path(QStringLiteral("empty.ycd"));
    ASSERT_TRUE(writeBytes(file, QByteArray()));
    DmDocument doc;
    FilterOcdIO filter;
    EXPECT_ANY_THROW(filter.fileImport(doc, file));
}

// ---------------------------------------------------------------------------
// 只链接 YiCadModel 读写整份文档（方案 8.6 节：L1 已解决）
// ---------------------------------------------------------------------------
//
// 本二进制只链接 YiCadModel，看不到宿主服务接口（它在 Application）：DmDocument 读写文件只经格式注册表
// （原生格式在 DmSystem::init 时登记），不弹框、不输出命令行消息、不改文件名。

TEST_F(DocumentReadWrite, 不起界面写出再读回整份文档)
{
    DmDocument original;
    ASSERT_NO_FATAL_FAILURE(build(original));
    const QString file = path(QStringLiteral("model_only.ycd"));
    const DmFileResult written = original.writeFile(file, kOcdFormat);
    ASSERT_TRUE(written.ok()) << written.message.toStdString();
    EXPECT_TRUE(written.message.isEmpty());
    EXPECT_TRUE(original.getFilename().isEmpty()) << "writeFile 不改文件名";
    EXPECT_EQ(QDir(dir.path()).entryList(QDir::Files), QStringList{QStringLiteral("model_only.ycd")});

    DmDocument restored;
    const DmFileResult read = restored.readFile(file);
    ASSERT_TRUE(read.ok()) << read.message.toStdString();
    EXPECT_TRUE(restored.getFilename().isEmpty()) << "readFile 不改文件名";
    EXPECT_FALSE(restored.isModified());
    EXPECT_EQ(countByType(*restored.getEntityTable()), kModelSpaceCounts);
    EXPECT_NE(restored.getBlockTable()->find(kBlockName), nullptr);
}

TEST_F(DocumentReadWrite, 没有过滤器时返回NoFilter)
{
    DmDocument doc;
    ASSERT_NO_FATAL_FAILURE(build(doc));
    const QString file = path(QStringLiteral("drawing.unknown"));
    EXPECT_EQ(doc.writeFile(file, QStringLiteral("Unknown format (*.unknown)")).status, DmFileStatus::NoFilter);
    EXPECT_FALSE(QFileInfo::exists(file));

    ASSERT_TRUE(writeBytes(file, QByteArray("not a drawing")));
    DmDocument other;
    const DmFileResult result = other.readFile(file);
    EXPECT_EQ(result.status, DmFileStatus::NoFilter);
    EXPECT_TRUE(result.message.isEmpty());
}

TEST_F(DocumentReadWrite, 按原生格式读不看后缀)
{
    // .bak 备份是原生格式，后缀却没有过滤器接（R8）：readFile 找不到过滤器，readNativeFile 读得了
    DmDocument original;
    ASSERT_NO_FATAL_FAILURE(build(original));
    const QString ycd = path(QStringLiteral("drawing.ycd"));
    ASSERT_NO_FATAL_FAILURE(exportTo(original, ycd));
    const QString bak = path(QStringLiteral("drawing.bak"));
    ASSERT_TRUE(QFile::copy(ycd, bak));

    DmDocument bySuffix;
    EXPECT_EQ(bySuffix.readFile(bak).status, DmFileStatus::NoFilter);

    DmDocument native;
    const DmFileResult result = native.readNativeFile(bak);
    ASSERT_TRUE(result.ok()) << result.message.toStdString();
    EXPECT_FALSE(native.isModified());
    EXPECT_NE(native.getBlockTable()->find(kBlockName), nullptr);
}
