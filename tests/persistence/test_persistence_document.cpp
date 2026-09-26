/// @file test_persistence_document.cpp
/// @brief 整文档 OCD 读写的回归测试（分层重组方案 S0）
///
/// doc/LAYER_RESTRUCTURE_PLAN.md 的 S4 要改动文档读写路径：原生格式与格式注册表下沉到
/// Model，存盘策略移出 DmDocument。本文件先把这条路径的现状锁住：
///
/// - 写出：构造一份含全部一等实体、块定义与块引用（含属性）、多个图层、线型、文字样式与
///   标注样式的文档，经 FilterOcdIO 写出，逐项检查压缩包的条目与 Document.xml；
/// - 往返：把写出的文件读回新文档，逐项比对；
/// - 异常路径：空文件、非压缩包、截断的文件、不存在的文件；
/// - 存盘策略：DmDocument::saveAs / save / open 这一层的 .bak 备份、外部修改检测与打开失败的处理。
///
/// 宿主服务用 OcdHost 代替：文件读写直接交给 FilterOcdIO（与 FileIO 对 .ycd 的分派相同，
/// 只是不查插件格式、不弹框）。
/// 读回的文档按产品的做法构造：新建 DmDocument 再导入。
///
/// ## 读回路径的缺陷
///
/// S0 查出读回路径 R1–R9 九处缺陷，编号与机理见 doc/LAYER_RESTRUCTURE_PLAN.md 4.5 节。
/// R1–R3、R5、R6、R9 已在 D8 修复步修复（同文档 4.6 节），对应用例已启用。仍未修的三处，
/// 相关用例保留 DISABLED_ 前缀，各自注明依赖哪几处，修好即可去掉前缀，用例就是修复的验收：
///
/// - R4 MetaLayers.cpp:114、MetaTextStyles.cpp:114、MetaDimensionStyles.cpp:116、
///   MetaBlockTableRecords.cpp:129：新建的 DmDocument 在各表的 setDocument 里已经放好
///   "0" 图层、"Standard" 文字样式、"ISO-25" 标注样式与 19 个标注箭头块，读入时又原样
///   add_direct 一份，同名条目各有两份；按名字查找（实体的图层、标注的样式）取到的是默认
///   那份，文件里这些条目自己的属性被忽略；箭头块每存一次、开一次多 19 个。
///   修法待定（读入时覆盖同名默认条目，还是先清空默认表），见方案 12 节 D9。
/// - R7 DmDocument.cpp:544（以及 :589、:597）：requestFileImport 没有 try/catch，而
///   FilterOcdIO 对坏文件一律抛异常（FilterOcdIO.cpp:152、:159、:165，MinizipNgArchive.cpp:324），
///   异常穿出 DmDocument::open，"是否打开备份"的询问与"Open failed, invalid file!"
///   的警告都走不到；MDIWindow::slotFileOpen、UITabDrawWidget::slotFileOpen 也不捕获。
///   并入 S4c。
/// - R8 DmDocument.cpp:589：打开失败后改开 "<文件名>.bak"，仍经 requestFileImport 按后缀选
///   过滤器，而 FilterOcdIO::canImport 只认 "ycd"（FilterOcdIO.cpp:118），.bak 没有过滤器
///   接，备份永远打不开；只有临时目录里 .ycd 后缀的自动保存副本能走通。并入 S4c。

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
#include "GuiDialogFactory.h"
#include "GuiDialogFactoryAdapter.h"
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

namespace
{
constexpr double kPi = 3.14159265358979323846;
constexpr double kTol = 1e-9;

/// 导出格式名，与 FilterOcdIO 的 EXPORTTYPE、DmDocument 的 DOCDEFAULTFORMAT 相同
const QString kOcdFormat = QString::fromLatin1(DOCDEFAULTFORMAT);

// 样本文档里的名字。图层、块、属性与文字用中文，顺带覆盖 UTF-8 的读写。
const QString kLayerOutline = QStringLiteral("轮廓");
const QString kLayerHidden = QStringLiteral("隐藏线");
const QString kLineTypeName = QStringLiteral("DASHDOT_S0");
const QString kLineTypeDesp = QStringLiteral("Dash dot (S0) __ . __");
const QString kTextStyleName = QStringLiteral("工程字");
const QString kDimStyleName = QStringLiteral("机械");
const QString kBlockName = QStringLiteral("螺栓");
const QString kAttributeTag = QStringLiteral("规格");
const QString kAttributeValue = QStringLiteral("M12");
const QString kTextValue = QStringLiteral("单行文字 Text 123");
const QString kMTextValue = QStringLiteral("多行文字");

/// @brief 测试用宿主服务：文件读写直接交给 FilterOcdIO
///
/// 与 FileIO 对原生格式的分派相同：导入按文件后缀、导出按格式名找过滤器。
/// 过滤器抛出的异常原样向外传播，FileIO 也不捕获。
class OcdHost : public GuiDialogFactoryAdapter
{
public:
    bool confirmAnswer = false;    ///< 确认对话框的回答
    int confirmCount = 0;          ///< 确认对话框弹出的次数
    QStringList warnings;          ///< 警告对话框的内容
    QStringList messages;          ///< 命令行消息

    bool requestFileExport(DmDocument& document, const QString& file, const QString& formatType) override
    {
        FilterOcdIO filter;
        return filter.canExport(formatType) && filter.fileExport(document, file, formatType);
    }

    bool requestFileImport(DmDocument& document, const QString& file) override
    {
        FilterOcdIO filter;
        return filter.canImport(file) && filter.fileImport(document, file);
    }

    bool requestConfirmDialog(const QString&, const QString&) override
    {
        ++confirmCount;
        return confirmAnswer;
    }

    void requestWarningDialog(const QString& warning) override { warnings.append(warning); }

    void commandMessage(const QString& message) override { messages.append(message); }
};

/// @brief 按图层取画笔（新实体的默认画笔）
DmPen byLayerPen()
{
    return DmPen(DmColor(DM::FlagByLayer), DM::WidthByLayer, DmLineTypeTable::ByLayer);
}

/// @brief 设好文档、图层与画笔，更新后直接放进实体表（不经事务）
template <typename T>
T* addTo(EntityTable& table, DmDocument& doc, T* entity, DmLayer* layer, const DmPen& pen = byLayerPen())
{
    entity->setDocument(&doc);
    entity->setLayer(layer);
    entity->setPen(pen);
    entity->update();
    entity->calculateBorders();
    EXPECT_TRUE(table.add_direct(entity));
    return entity;
}

/// @brief 样本文档里各类实体的数量（模型空间）
const std::map<DM::EntityType, int> kModelSpaceCounts = {
    {DM::EntityLine, 1},         {DM::EntityCircle, 1},       {DM::EntityArc, 1},
    {DM::EntityEllipse, 1},      {DM::EntityPoint, 1},        {DM::EntityRay, 1},
    {DM::EntityXline, 1},        {DM::EntitySolid, 1},        {DM::EntityPolyline, 1},
    {DM::EntitySpline, 1},       {DM::EntityText, 1},         {DM::EntityMText, 1},
    {DM::EntityDimLinear, 1},    {DM::EntityDimAligned, 1},   {DM::EntityDimAngular, 1},
    {DM::EntityDimRadial, 1},    {DM::EntityDimDiametric, 1}, {DM::EntityDimLeader, 1},
    {DM::EntityHatch, 1},        {DM::EntityBlockReference, 1},
};

/// 新建文档自带的标注箭头块数（DmDimensionStyleTable::initArrowBlocks）
constexpr int kArrowBlocks = 19;

/// @brief 构造样本文档
///
/// 表：自定义线型 kLineTypeName；图层 "0"、kLayerOutline（红色、自定义线型、当前图层）、
/// kLayerHidden（冻结、锁定、不打印）；文字样式 kTextStyleName 与标注样式 kDimStyleName，
/// 两者都设为当前样式；当前线型保持默认的 ByLayer。
/// 块 kBlockName：一个圆、一条直线、一个属性定义；模型空间一个块引用带一个属性值。
/// 其余模型空间实体见 kModelSpaceCounts。
void buildSample(DmDocument& doc)
{
    // 线型
    auto* lineType = new DmLineType(kLineTypeName);
    lineType->setDocument(&doc);
    lineType->setLineTypeDesp(kLineTypeDesp);
    lineType->setLineTypeData(std::vector<double>{0.5, -0.25, 0.0, -0.25});
    ASSERT_TRUE(doc.getLineTypeTable()->add_direct(lineType));

    // 图层
    DmLayer* layer0 = doc.getLayerTable()->find(QStringLiteral("0"));
    ASSERT_NE(layer0, nullptr);

    auto* outline = new DmLayer();
    outline->setDocument(&doc);
    outline->setData(DmLayerData(kLayerOutline, DmPen(DmColor(255, 0, 0), DM::Width09, lineType), false, false));
    ASSERT_TRUE(doc.getLayerTable()->add_direct(outline));

    auto* hidden = new DmLayer();
    hidden->setDocument(&doc);
    DmLayerData hiddenData(kLayerHidden, DmPen(DmColor(0, 128, 255), DM::Width05, DmLineTypeTable::Continuous), true,
                           true);
    hiddenData.print = false;
    hidden->setData(hiddenData);
    ASSERT_TRUE(doc.getLayerTable()->add_direct(hidden));
    doc.getLayerTable()->activate_direct(outline);

    // 文字样式：以 Standard 为模板，改高度与宽度因子
    DmTextStyle* standardText = doc.getTextStyleTable()->find(QStringLiteral("Standard"));
    ASSERT_NE(standardText, nullptr);
    auto* textStyle = new DmTextStyle(*standardText, kTextStyleName);
    DmTextStyleData textStyleData = textStyle->getData();
    textStyleData.defaultHeight = 3.5;
    textStyleData.widhFactor = 0.7;
    textStyle->setData(textStyleData);
    textStyle->setDocument(&doc);
    ASSERT_TRUE(doc.getTextStyleTable()->add_direct(textStyle));
    doc.getTextStyleTable()->activate_direct(textStyle);

    // 标注样式
    auto* dimStyle = new DmDimensionStyle(kDimStyleName, textStyle);
    dimStyle->setDocument(&doc);
    dimStyle->getDataRef().setArrowSize(3.0);
    dimStyle->getDataRef().setTextHeight(3.5);
    ASSERT_TRUE(doc.getDimStyleTable()->add_direct(dimStyle));
    doc.getDimStyleTable()->activate_direct(dimStyle);

    // 块定义：圆、直线、属性定义
    auto* block = new DmBlock(&doc, DmBlockData(kBlockName, DmVector(0.0, 0.0), false));
    doc.getIdManager()->assignID(block);
    doc.getBlockTable()->add_direct(block);
    EntityTable& blockTable = block->getEntityTable();
    addTo(blockTable, doc, new DmCircle(nullptr, CircleData(DmVector(0.0, 0.0), 5.0)), layer0);
    addTo(blockTable, doc, new DmLine(DmVector(-5.0, 0.0), DmVector(5.0, 0.0)), layer0);
    TextData attDefText(DmVector(0.0, -8.0), 2.5, ETextVertMode::kTextBase, ETextHorzMode::kTextLeft,
                        QStringLiteral("M10"), textStyle, 0.0, EUpdateMode::NoUpdate);
    addTo(blockTable, doc,
          new DmAttributeDefinition(nullptr, attDefText,
                                    AttributeDefinitionData(kAttributeTag, QStringLiteral("输入规格"))),
          layer0);

    EntityTable& model = *doc.getEntityTable();

    // 基本曲线
    addTo(model, doc, new DmLine(DmVector(1.5, -2.25), DmVector(30.75, 41.125)), outline,
          DmPen(DmColor(0, 255, 0), DM::Width07, lineType));
    addTo(model, doc, new DmCircle(nullptr, CircleData(DmVector(12.0, -34.5), 6.125)), outline);
    addTo(model, doc,
          new DmArc(nullptr, ArcData(DmVector(3.0, 4.0), DmVector(0.0, 0.0, 1.0), 7.5, 0.25, 2.75)), outline);
    addTo(model, doc,
          new DmEllipse(nullptr,
                        EllipseData(DmVector(1.0, 2.0), DmVector(8.0, 0.0), DmVector(0.0, 0.0, 1.0), 0.5, true,
                                    0.0, 2.0 * kPi)),
          layer0);
    addTo(model, doc, new DmPoint(nullptr, PointData(DmVector(-7.0, 9.0))), layer0);
    addTo(model, doc, new DmRay(nullptr, RayData(DmVector(0.0, 100.0), DmVector(1.0, 0.0))), hidden);
    addTo(model, doc, new DmXline(nullptr, XLineData(DmVector(0.0, -100.0), DmVector(0.0, 1.0))), hidden);
    addTo(model, doc,
          new DmSolid(nullptr, SolidData({DmVector(50.0, 0.0), DmVector(60.0, 0.0), DmVector(55.0, 8.0)})),
          layer0);

    // 多段线：四个顶点，第二段带凸度，闭合
    std::vector<double> widths(8, 0.0);
    addTo(model, doc,
          new DmPolyline(nullptr,
                         PolylineData({DmVector(0.0, 0.0), DmVector(20.0, 0.0), DmVector(20.0, 10.0),
                                       DmVector(0.0, 10.0)},
                                      {0.0, 0.5, 0.0, 0.0}, widths, true)),
          outline);

    // 三次样条：四个控制点，钳位节点向量
    SplineData splineData(3, false, ESplineType::eControlPoints);
    splineData.setControlPoints({DmVector(0.0, 50.0), DmVector(10.0, 60.0), DmVector(20.0, 40.0),
                                 DmVector(30.0, 50.0)});
    splineData.setKnots({0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0});
    addTo(model, doc, new DmSpline(nullptr, splineData), layer0);

    // 文字
    addTo(model, doc,
          new DmText(nullptr, TextData(DmVector(40.0, 40.0), 3.5, ETextVertMode::kTextBase,
                                       ETextHorzMode::kTextLeft, kTextValue, textStyle, 0.0,
                                       EUpdateMode::NoUpdate)),
          layer0);
    addTo(model, doc,
          new DmMText(nullptr, MTextData(DmVector(40.0, 60.0), 2.5, EMTextVertMode::kTextTop,
                                         EMTextHorzMode::kTextLeft, 2.5 * 1.6, 50.0, kMTextValue, textStyle, 0.0,
                                         EUpdateMode::NoUpdate)),
          layer0);

    // 五种标注
    auto dimCommon = [dimStyle](const DmVector& definitionPoint, const DmVector& textPos) {
        return DmDimensionData(definitionPoint, textPos, EMTextVertMode::kTextVertMid, EMTextHorzMode::kTextCenter,
                               1.0, QString(), 0.0, dimStyle);
    };
    addTo(model, doc,
          new DmDimLinear(nullptr, dimCommon(DmVector(20.0, -10.0), DmVector(10.0, -10.0)),
                          DmDimLinearData(DmVector(0.0, 0.0), DmVector(20.0, 0.0))),
          layer0);
    addTo(model, doc,
          new DmDimAligned(nullptr, dimCommon(DmVector(24.0, 13.0), DmVector(17.0, 16.0)),
                           DmDimAlignedData(DmVector(0.0, 10.0), DmVector(20.0, 10.0))),
          layer0);
    addTo(model, doc,
          new DmDimAngular(nullptr, dimCommon(DmVector(0.0, 0.0), DmVector(6.0, 3.0)),
                           DmDimAngularData(DmVector(0.0, 0.0), DmVector(10.0, 0.0), DmVector(0.0, 0.0),
                                            DmVector(0.0, 10.0), DmVector(5.0, 5.0))),
          layer0);
    addTo(model, doc,
          new DmDimRadial(nullptr, dimCommon(DmVector(12.0, -34.5), DmVector(15.0, -30.0)),
                          DmDimRadialData(DmVector(18.125, -34.5), 5.0)),
          layer0);
    addTo(model, doc,
          new DmDimDiametric(nullptr, dimCommon(DmVector(5.875, -34.5), DmVector(12.0, -34.5)),
                             DmDimDiametricData(DmVector(18.125, -34.5), 5.0)),
          layer0);

    // 引线
    addTo(model, doc,
          new DmLeader(nullptr, DmLeaderData(dimStyle, {DmVector(70.0, 0.0), DmVector(80.0, 10.0),
                                                        DmVector(90.0, 10.0)})),
          layer0);

    // 实心填充：一个矩形外环
    auto boundary = std::make_shared<DmEntityContainer>(nullptr);
    std::vector<double> hatchWidths(8, 0.0);
    auto* edge = new DmPolyline(boundary.get(),
                                PolylineData({DmVector(100.0, 0.0), DmVector(120.0, 0.0), DmVector(120.0, 15.0),
                                              DmVector(100.0, 15.0)},
                                             {0.0, 0.0, 0.0, 0.0}, hatchWidths, true));
    boundary->addEntity(edge);
    HatchData hatchData(true, 1.0, 0.0, std::wstring(L"SOLID"));
    hatchData.setBoundary(std::make_shared<DmRegion>(nullptr, RegionData(boundary, {})));
    addTo(model, doc, new DmHatch(nullptr, hatchData), outline);

    // 块引用与属性值
    auto* insert = new DmBlockReference(nullptr,
                                        DmBlockReferenceData(kBlockName, DmVector(100.0, 50.0), DmVector(2.0, 2.0),
                                                             0.5, 1, 1, DmVector(0.0, 0.0), doc.getBlockTable(),
                                                             DM::NoUpdate));
    insert->setBlock(block);
    auto* attribute = new DmAttribute(nullptr,
                                      TextData(DmVector(100.0, 34.0), 2.5, ETextVertMode::kTextBase,
                                               ETextHorzMode::kTextLeft, kAttributeValue, textStyle, 0.0,
                                               EUpdateMode::NoUpdate),
                                      AttributeData(kAttributeTag));
    attribute->setDocument(&doc);
    attribute->setLayer(layer0);
    attribute->setPen(byLayerPen());
    attribute->update();
    insert->addAttributes({attribute});
    addTo(model, doc, insert, outline);
}

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

/// @brief 取实体表里第一个指定类型的实体
template <typename T>
T* first(const EntityTable& table, DM::EntityType type)
{
    for (DmEntity* e : table)
    {
        if (e->getEntityType() == type)
        {
            return static_cast<T*>(e);
        }
    }
    return nullptr;
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

/// @brief 写一个文件，内容为 bytes
bool writeBytes(const QString& path, const QByteArray& bytes)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        return false;
    }
    return file.write(bytes) == bytes.size();
}

QByteArray readBytes(const QString& path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

/// @brief 压缩包里的一个条目
struct ArchiveEntry
{
    std::string name;
    std::string data;
};

/// @brief 按 Archive.h 的约定（先 nextEntry() 再读）逐个读出压缩包的条目
std::vector<ArchiveEntry> readArchive(const QString& path)
{
    std::vector<ArchiveEntry> entries;
    MinizipNgArchiveReader archive(QFile::encodeName(path).toStdString());
    while (archive.nextEntry())
    {
        std::string data((std::istreambuf_iterator<char>(archive.stream())), std::istreambuf_iterator<char>());
        entries.push_back({archive.entryName(), std::move(data)});
    }
    return entries;
}

/// @brief 用例夹具：装上 OcdHost，提供临时目录
struct OcdFixture : ::testing::Test
{
    OcdHost host;
    QTemporaryDir dir;

    OcdFixture() { GuiDialogFactory::instance()->setFactoryObject(&host); }
    ~OcdFixture() override { GuiDialogFactory::instance()->setFactoryObject(nullptr); }

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
using DocumentSavePolicy = OcdFixture;
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

// 依赖 R4。
TEST_F(OcdDocumentRoundTrip, DISABLED_读回不重复新文档自带的条目)
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

// 依赖 R4。
TEST_F(OcdDocumentRoundTrip, DISABLED_写出读回再写出保持稳定)
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

// ---------------------------------------------------------------------------
// 异常路径
// ---------------------------------------------------------------------------
//
// FilterInterface::fileImport 的签名返回 bool，但 FilterOcdIO 对坏文件一律抛异常
// （OneException 或 MinizipNgArchiveReader 的 std::runtime_error），调用链上的 FileIO 与
// DmDocument::open 都不捕获（R7）。这里断言过滤器这一层的现状；S4c 让 Model 返回结果码
// 之后，这几个用例随之改写。

TEST_F(OcdDocumentErrorPath, 空文件抛异常)
{
    const QString file = path(QStringLiteral("empty.ycd"));
    ASSERT_TRUE(writeBytes(file, QByteArray()));
    DmDocument doc;
    FilterOcdIO filter;
    EXPECT_ANY_THROW(filter.fileImport(doc, file));
    EXPECT_EQ(doc.getEntityTable()->count(), 0);
}

TEST_F(OcdDocumentErrorPath, 不是压缩包的文件抛异常)
{
    const QString file = path(QStringLiteral("garbage.ycd"));
    ASSERT_TRUE(writeBytes(file, QByteArray(4096, 'x')));
    DmDocument doc;
    FilterOcdIO filter;
    EXPECT_ANY_THROW(filter.fileImport(doc, file));
    EXPECT_EQ(doc.getEntityTable()->count(), 0);
}

TEST_F(OcdDocumentErrorPath, 截断的文件抛异常)
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
    FilterOcdIO filter;
    EXPECT_ANY_THROW(filter.fileImport(doc, truncated));
}

TEST_F(OcdDocumentErrorPath, 不存在的文件抛异常)
{
    DmDocument doc;
    FilterOcdIO filter;
    EXPECT_ANY_THROW(filter.fileImport(doc, path(QStringLiteral("missing.ycd"))));
}

// ---------------------------------------------------------------------------
// 存盘策略（DmDocument::saveAs / save / open）
// ---------------------------------------------------------------------------

TEST_F(DocumentSavePolicy, 另存为经临时文件写出并记录文件名)
{
    DmDocument doc;
    ASSERT_NO_FATAL_FAILURE(build(doc));
    const QString file = path(QStringLiteral("saved.ycd"));
    ASSERT_TRUE(doc.saveAs(file, kOcdFormat));
    EXPECT_EQ(doc.getFilename(), file);
    EXPECT_EQ(doc.getFormatType(), kOcdFormat);
    EXPECT_FALSE(doc.isModified());
    // 先写 <文件名>.tmp 再改名；第一次保存没有旧文件，不产生 .bak
    EXPECT_EQ(QDir(dir.path()).entryList(QDir::Files), QStringList{QStringLiteral("saved.ycd")});
    EXPECT_EQ(host.messages, QStringList{QStringLiteral("File saved: %1").arg(file)});
    EXPECT_EQ(readArchive(file).size(), 27u);
}

TEST_F(DocumentSavePolicy, 再次保存时旧文件备份为bak)
{
    DmDocument doc;
    ASSERT_NO_FATAL_FAILURE(build(doc));
    const QString file = path(QStringLiteral("drawing.ycd"));
    const QString bak = path(QStringLiteral("drawing.bak"));
    ASSERT_TRUE(doc.saveAs(file, kOcdFormat));
    EXPECT_FALSE(QFileInfo::exists(bak));
    const QByteArray firstBytes = readBytes(file);

    // 未修改时 save() 直接返回成功、不写盘；force 才真正写
    ASSERT_TRUE(doc.save());
    EXPECT_FALSE(QFileInfo::exists(bak));
    ASSERT_TRUE(doc.save(false, true));
    ASSERT_TRUE(QFileInfo::exists(bak));
    EXPECT_EQ(readBytes(bak), firstBytes);
    EXPECT_EQ(QDir(dir.path()).entryList(QDir::Files),
              (QStringList{QStringLiteral("drawing.bak"), QStringLiteral("drawing.ycd")}));

    // 第三次保存覆盖 .bak，仍然只有一份
    const QByteArray secondBytes = readBytes(file);
    ASSERT_TRUE(doc.save(false, true));
    EXPECT_EQ(readBytes(bak), secondBytes);
}

TEST_F(DocumentSavePolicy, 后缀与格式不符时拒绝保存)
{
    DmDocument doc;
    ASSERT_NO_FATAL_FAILURE(build(doc));
    const QString file = path(QStringLiteral("drawing.dxf"));
    EXPECT_FALSE(doc.saveAs(file, kOcdFormat));
    EXPECT_FALSE(QFileInfo::exists(file));
    EXPECT_TRUE(doc.getFilename().isEmpty()) << "失败的另存为不应改动文件名";
    ASSERT_FALSE(host.messages.isEmpty());
    EXPECT_TRUE(host.messages.back().startsWith(QStringLiteral("File format mismatch.")));
}

TEST_F(DocumentSavePolicy, 外部修改过的文件拒绝保存)
{
    DmDocument doc;
    ASSERT_NO_FATAL_FAILURE(build(doc));
    const QString file = path(QStringLiteral("external.ycd"));
    ASSERT_TRUE(doc.saveAs(file, kOcdFormat));

    // 修改时间只在 save() 的手动分支写盘成功后记录（另存为也走这条分支）
    QFile touched(file);
    ASSERT_TRUE(touched.open(QIODevice::ReadWrite));
    ASSERT_TRUE(touched.setFileTime(QDateTime::currentDateTime().addSecs(3600), QFileDevice::FileModificationTime));
    touched.close();

    host.messages.clear();
    EXPECT_FALSE(doc.save(false, true));
    ASSERT_EQ(host.messages.size(), 1);
    EXPECT_TRUE(host.messages.front().startsWith(QStringLiteral("File on disk modified.")));
    EXPECT_FALSE(QFileInfo::exists(path(QStringLiteral("external.bak"))));
}

TEST_F(DocumentSavePolicy, 另存为后能打开且不算修改)
{
    DmDocument original;
    ASSERT_NO_FATAL_FAILURE(build(original));
    const QString file = path(QStringLiteral("saved.ycd"));
    ASSERT_TRUE(original.saveAs(file, kOcdFormat));

    DmDocument reopened;
    ASSERT_TRUE(reopened.open(file));
    EXPECT_EQ(reopened.getFilename(), file);
    EXPECT_FALSE(reopened.isModified());
    EXPECT_NE(first<DmLine>(*reopened.getEntityTable(), DM::EntityLine), nullptr);
    EXPECT_NE(reopened.getBlockTable()->find(kBlockName), nullptr);
    EXPECT_TRUE(host.warnings.isEmpty());
    EXPECT_EQ(host.confirmCount, 0);
}

// 依赖 R7（打开主文件失败时不再抛出）、R8（.bak 找得到过滤器）。
TEST_F(DocumentSavePolicy, DISABLED_打开损坏文件时询问是否打开备份)
{
    DmDocument doc;
    ASSERT_NO_FATAL_FAILURE(build(doc));
    const QString file = path(QStringLiteral("broken.ycd"));
    ASSERT_TRUE(doc.saveAs(file, kOcdFormat));
    ASSERT_TRUE(doc.save(false, true));  // 第二次保存留下 broken.bak
    ASSERT_TRUE(QFileInfo::exists(path(QStringLiteral("broken.bak"))));
    ASSERT_TRUE(writeBytes(file, QByteArray(4096, 'x')));

    host.confirmAnswer = true;
    DmDocument reopened;
    EXPECT_TRUE(reopened.open(file));
    EXPECT_EQ(host.confirmCount, 1);
    EXPECT_NE(reopened.getBlockTable()->find(kBlockName), nullptr);
}

// 依赖 R7。
TEST_F(DocumentSavePolicy, DISABLED_打开损坏文件且没有备份时警告并返回失败)
{
    const QString file = path(QStringLiteral("broken.ycd"));
    ASSERT_TRUE(writeBytes(file, QByteArray(4096, 'x')));

    DmDocument doc;
    EXPECT_FALSE(doc.open(file));
    EXPECT_EQ(host.confirmCount, 0);
    EXPECT_EQ(host.warnings, QStringList{QStringLiteral("Open failed, invalid file!")});
}
