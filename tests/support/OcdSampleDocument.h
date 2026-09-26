/// @file OcdSampleDocument.h
/// @brief 整文档读写用例共用的样本文档与文件工具
///
/// 分层重组 S0 在 tests/persistence/test_persistence_document.cpp 里建立；S4c 把存盘策略从
/// DmDocument 移到 Application 的 DocumentFileService，相应用例搬到 tests/interaction，
/// 两处共用这份样本，于是移到这里，内容不变。样本含全部一等实体、块定义与块引用（含属性）、
/// 多个图层、线型、文字样式与标注样式。

#ifndef YICAD_TEST_OCD_SAMPLE_DOCUMENT_H
#define YICAD_TEST_OCD_SAMPLE_DOCUMENT_H

#include <gtest/gtest.h>

#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <QByteArray>
#include <QFile>
#include <QString>

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

namespace yicad_test
{
inline constexpr double kPi = 3.14159265358979323846;
/// 导出格式名，与 FilterOcdIO 的 EXPORTTYPE、DmDocument 的 DOCDEFAULTFORMAT 相同
inline const QString kOcdFormat = QString::fromLatin1(DOCDEFAULTFORMAT);

// 样本文档里的名字。图层、块、属性与文字用中文，顺带覆盖 UTF-8 的读写。
inline const QString kLayerOutline = QStringLiteral("轮廓");
inline const QString kLayerHidden = QStringLiteral("隐藏线");
inline const QString kLineTypeName = QStringLiteral("DASHDOT_S0");
inline const QString kLineTypeDesp = QStringLiteral("Dash dot (S0) __ . __");
inline const QString kTextStyleName = QStringLiteral("工程字");
inline const QString kDimStyleName = QStringLiteral("机械");
inline const QString kBlockName = QStringLiteral("螺栓");
inline const QString kAttributeTag = QStringLiteral("规格");
inline const QString kAttributeValue = QStringLiteral("M12");
inline const QString kTextValue = QStringLiteral("单行文字 Text 123");
inline const QString kMTextValue = QStringLiteral("多行文字");

/// @brief 按图层取画笔（新实体的默认画笔）
inline DmPen byLayerPen()
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
inline const std::map<DM::EntityType, int> kModelSpaceCounts = {
    {DM::EntityLine, 1},         {DM::EntityCircle, 1},       {DM::EntityArc, 1},
    {DM::EntityEllipse, 1},      {DM::EntityPoint, 1},        {DM::EntityRay, 1},
    {DM::EntityXline, 1},        {DM::EntitySolid, 1},        {DM::EntityPolyline, 1},
    {DM::EntitySpline, 1},       {DM::EntityText, 1},         {DM::EntityMText, 1},
    {DM::EntityDimLinear, 1},    {DM::EntityDimAligned, 1},   {DM::EntityDimAngular, 1},
    {DM::EntityDimRadial, 1},    {DM::EntityDimDiametric, 1}, {DM::EntityDimLeader, 1},
    {DM::EntityHatch, 1},        {DM::EntityBlockReference, 1},
};

/// @brief 构造样本文档
///
/// 表：自定义线型 kLineTypeName；图层 "0"、kLayerOutline（红色、自定义线型、当前图层）、
/// kLayerHidden（冻结、锁定、不打印）；文字样式 kTextStyleName 与标注样式 kDimStyleName，
/// 两者都设为当前样式；当前线型保持默认的 ByLayer。
/// 块 kBlockName：一个圆、一条直线、一个属性定义；模型空间一个块引用带一个属性值。
/// 其余模型空间实体见 kModelSpaceCounts。
inline void buildSample(DmDocument& doc)
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

/// @brief 写一个文件，内容为 bytes
inline bool writeBytes(const QString& path, const QByteArray& bytes)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        return false;
    }
    return file.write(bytes) == bytes.size();
}

inline QByteArray readBytes(const QString& path)
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
inline std::vector<ArchiveEntry> readArchive(const QString& path)
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

}  // namespace yicad_test

#endif  // YICAD_TEST_OCD_SAMPLE_DOCUMENT_H
