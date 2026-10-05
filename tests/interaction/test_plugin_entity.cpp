/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/// @file test_plugin_entity.cpp
/// @brief 插件自定义实体（ABI v4，RENDER_PLAN.md 第 8 阶段）
///
/// 加载构建出来的 demo 插件（plugins/demo_plugin，实体类"管道" com.yicad.demo.Pipe）与 DXF 插件，走与程序相同的
/// 插件运行时：插件登记实体类；插件命令在事务里新建、修改管道，可撤销；插件实体的图形记成 GI 流；夹点、捕捉、
/// 变换、炸开交给插件；原生格式存读，插件不在或数据版本比插件新时读成代理；插件卸载后留下的实体不再调用插件；
/// 只读枚举交出自定义实体与实体句柄（开放多段线不中断枚举）；DXF 往返；单次 worldDraw 的输出上限。

#include <gtest/gtest.h>

#include <map>
#include <span>
#include <vector>

#include <QTemporaryDir>

#include "DmCustomEntityRegistry.h"
#include "DmDocument.h"
#include "DmHatch.h"
#include "DmLine.h"
#include "DmPluginEntity.h"
#include "DmPolyline.h"
#include "DmProxyEntity.h"
#include "DmText.h"
#include "EntityTable.h"
#include "FilterOcdIO.h"
#include "GiStream.h"
#include "IGiDrawable.h"
#include "IGiGeometry.h"
#include "IGiSubEntityTraits.h"
#include "PluginGi.h"
#include "YiCadPluginSdk.h"
#include "support/DxfTestRuntime.h"
#include "support/OcdSampleDocument.h"

#ifndef YICAD_DEMO_PLUGIN_DLL
#error "test_plugin_entity.cpp 需要 YICAD_DEMO_PLUGIN_DLL（demo 插件 DLL 的绝对路径）"
#endif

using namespace yicad_test;

namespace
{
const QString kPluginId = QStringLiteral("com.yicad.demo");
const QString kPipeClass = QStringLiteral("com.yicad.demo.Pipe");

/// @brief 管道的数据（与 plugins/demo_plugin/DemoPipe.cpp 的编码相同）
struct Pipe
{
    std::vector<DmVector> vertices;
    double diameter = 0.0;
};

Pipe decodePipe(const std::string& bytes)
{
    yicad::plugin::ByteReader reader(
        std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size()));
    Pipe pipe;
    const uint32_t count = reader.u32();
    for (uint32_t i = 0; i < count; ++i)
    {
        const YiCadPoint2d p = reader.point();
        pipe.vertices.emplace_back(p.x, p.y);
    }
    pipe.diameter = reader.f64();
    return pipe;
}

std::string encodePipe(const Pipe& pipe)
{
    yicad::plugin::ByteWriter writer;
    writer.u32(static_cast<uint32_t>(pipe.vertices.size()));
    for (const DmVector& v : pipe.vertices)
    {
        writer.point({v.x, v.y});
    }
    writer.f64(pipe.diameter);
    const auto& bytes = writer.bytes();
    return std::string(bytes.begin(), bytes.end());
}

std::vector<DmVector> points(std::initializer_list<DmVector> values)
{
    return std::vector<DmVector>(values);
}

/// @brief 装了 demo 插件与 DXF 插件的运行时和一份文档
struct PluginEntityTest : ::testing::Test
{
    DxfRuntime runtime{QStringList{QStringLiteral(YICAD_DEMO_PLUGIN_DLL)}};
    DmDocument doc;
    QTemporaryDir dir;

    void SetUp() override
    {
        ASSERT_TRUE(runtime.loaded()) << runtime.diagnostics().toStdString();
        ASSERT_TRUE(runtime.pluginActive(kPluginId)) << runtime.diagnostics().toStdString();
        runtime.openDocument(doc);
    }

    /// @brief 文档模型空间里未删除的自定义实体
    static std::vector<DmCustomEntity*> customEntities(DmDocument& document)
    {
        std::vector<DmCustomEntity*> result;
        for (DmEntity* e : *document.getEntityTable())
        {
            if (!e->isErased() && e->getEntityType() == DM::EntityCustom)
            {
                result.push_back(static_cast<DmCustomEntity*>(e));
            }
        }
        return result;
    }

    /// @brief 用插件命令建一根管道：(0,0)-(100,0)-(100,60)，管径 10
    DmPluginEntity* addPipe()
    {
        EXPECT_TRUE(runtime.runCommand(doc, kPluginId, QStringLiteral("demo.add-pipe")));
        const auto entities = customEntities(doc);
        if (entities.size() != 1)
        {
            char error[512] = {};
            yicad::plugin::Host host(runtime.host().api());
            host.document(runtime.host().documentHandle(&doc)).importLastError(error, sizeof(error));
            ADD_FAILURE() << "插件消息：" << runtime.messages().join(QStringLiteral("; ")).toStdString()
                          << "；宿主错误：" << error;
        }
        return entities.size() == 1 ? dynamic_cast<DmPluginEntity*>(entities.front()) : nullptr;
    }

    void save(DmDocument& document, const QString& file)
    {
        FilterOcdIO filter;
        ASSERT_TRUE(filter.fileExport(document, file, kOcdFormat));
    }

    void load(DmDocument& document, const QString& file)
    {
        runtime.openDocument(document);
        FilterOcdIO filter;
        ASSERT_TRUE(filter.fileImport(document, file));
    }

    /// @brief 插件不在：注销它的实体类、断开（同插件 shutdown 前）
    void detachPlugin() { runtime.registry().detachEntityClasses(kPluginId); }
};

/// @brief 只数图元的 GI 接收方
class CountingDraw final : public IGiWorldDraw, public IGiGeometry, public IGiSubEntityTraits
{
public:
    int primitives = 0;

    IGiGeometry& geometry() override { return *this; }
    IGiSubEntityTraits& traits() override { return *this; }
    GiRegenType regenType() const override { return GiRegenType::Display; }
    double deviation() const override { return 0.0; }
    bool isDragging() const override { return false; }

    void setColor(const DmColor&) override {}
    void setLayer(const DmLayer*) override {}
    void setLineType(const DmLineType*) override {}
    void setLineTypeScale(double) override {}
    void setLinePattern(const GiLinePattern&) override {}
    void setFill(const GiHatchPattern*) override {}
    void setLineWeight(DM::LineWidth) override {}
    void setTransparency(std::uint8_t) override {}
    void setSelectionMarker(std::int32_t) override {}
    void setScreenSpace(const DmVector*) override {}

    void polyline(std::span<const DmVector>, std::span<const double>, std::span<const GiSegmentWidth>,
                  GiPolylineFlags) override { ++primitives; }
    void circle(const DmVector&, double) override { ++primitives; }
    void arc(const DmVector&, double, double, double) override { ++primitives; }
    void ellipseArc(const DmVector&, const DmVector&, double, double, double) override { ++primitives; }
    void nurbs(const GiNurbs&) override { ++primitives; }
    void fill(std::span<const GiLoop>, GiFillRule) override { ++primitives; }
    void triangles(std::span<const DmVector>, std::span<const std::uint32_t>) override { ++primitives; }
    void glyphRun(const GiGlyphRun&) override { ++primitives; }
    void image(const GiImage&) override { ++primitives; }
    void point(const DmVector&) override { ++primitives; }
    void ray(const DmVector&, const DmVector&) override { ++primitives; }
    void xline(const DmVector&, const DmVector&) override { ++primitives; }
    void draw(const IGiDrawable&) override { ++primitives; }
    void drawShared(const IGiDrawable&, const GiTransform&, const GiByBlockTraits&) override { ++primitives; }
    void pushTransform(const GiTransform&) override {}
    void popTransform() override {}
};
}  // namespace

TEST_F(PluginEntityTest, 插件登记的实体类进自定义实体注册表)
{
    const DmCustomEntityClass* entityClass = DmCustomEntityRegistry::instance().find(kPipeClass);
    ASSERT_NE(entityClass, nullptr);
    EXPECT_EQ(entityClass->owner, kPluginId);
    EXPECT_EQ(entityClass->version, 1u);
    EXPECT_EQ(entityClass->proxyFlags, DmProxyFlags::Erase | DmProxyFlags::Transform | DmProxyFlags::Cloning |
                                           DmProxyFlags::LayerChange | DmProxyFlags::ColorChange);
    detachPlugin();
    EXPECT_EQ(DmCustomEntityRegistry::instance().find(kPipeClass), nullptr) << "插件关闭前注销";
}

TEST_F(PluginEntityTest, 插件命令在事务里建管道可撤销)
{
    DmPluginEntity* pipe = addPipe();
    ASSERT_NE(pipe, nullptr);
    EXPECT_EQ(pipe->className(), kPipeClass);
    const Pipe data = decodePipe(pipe->data());
    EXPECT_EQ(data.vertices, points({DmVector(0, 0), DmVector(100, 0), DmVector(100, 60)}));
    EXPECT_DOUBLE_EQ(data.diameter, 10.0);
    // 包围框是插件给的：折点外扩 max(2.6×5, "DN10" 4 字 × 4) = 16
    EXPECT_NEAR(pipe->getMin().x, -16.0, 1e-9);
    EXPECT_NEAR(pipe->getMax().y, 76.0, 1e-9);

    doc.getCmdManager()->undo();
    EXPECT_TRUE(customEntities(doc).empty());
    doc.getCmdManager()->redo();
    EXPECT_EQ(customEntities(doc).size(), 1u);
}

TEST_F(PluginEntityTest, 插件实体的图形记成GI流按它拾取)
{
    DmPluginEntity* pipe = addPipe();
    ASSERT_NE(pipe, nullptr);
    EXPECT_FALSE(pipe->proxyGraphics().isEmpty()) << "数据变化时调一次插件、记下 GI 流";
    CountingDraw counting;
    EXPECT_TRUE(pipe->entityClass()->worldDraw(pipe->data(), nullptr, counting, pipe)) << "插件的 worldDraw 成功";
    // 边线 4、端头 2、中心线 1、箭头 1、文字（字形串，有字体时）
    EXPECT_GE(counting.primitives, 8);
    // 默认的拾取按 GI 流：边线上的点
    EXPECT_NEAR(pipe->getDistanceToPoint(DmVector(50, 5)), 0.0, 1e-9);
    EXPECT_GT(pipe->getDistanceToPoint(DmVector(50, 30)), 1.0);
}

TEST_F(PluginEntityTest, 炸开交给插件)
{
    DmPluginEntity* pipe = addPipe();
    ASSERT_NE(pipe, nullptr);
    std::map<DM::EntityType, int> counts;
    QString text;
    bool redFill = false;
    for (DmEntity* part : pipe->explode())
    {
        ++counts[part->getEntityType()];
        EXPECT_EQ(part->getDocument(), &doc);
        EXPECT_EQ(part->getLayer(false), pipe->getLayer(false)) << "没给图层：取被炸开的实体的";
        if (auto* t = dynamic_cast<DmText*>(part))
        {
            text = t->getText();
        }
        if (dynamic_cast<DmHatch*>(part))
        {
            redFill = part->getPen(false).getColor() == DmColor(255, 0, 0);
        }
        delete part;
    }
    EXPECT_EQ(counts[DM::EntityPolyline], 1) << "中心线";
    EXPECT_EQ(counts[DM::EntityLine], 4) << "两段各两条边线";
    EXPECT_EQ(counts[DM::EntityArc], 2) << "两端的半圆";
    EXPECT_EQ(counts[DM::EntityHatch], 1) << "箭头";
    EXPECT_TRUE(redFill);
    EXPECT_EQ(text, QStringLiteral("DN10")) << "插件的炸开给出文字，不是笔画";
    // 炸开会话由宿主结束：之后插件命令照常能开事务
    EXPECT_TRUE(runtime.runCommand(doc, kPluginId, QStringLiteral("demo.pipe-grow")));
    EXPECT_DOUBLE_EQ(decodePipe(pipe->data()).diameter, 20.0);
}

TEST_F(PluginEntityTest, 夹点交给插件)
{
    DmPluginEntity* pipe = addPipe();
    ASSERT_NE(pipe, nullptr);
    const DmVectorSolutions grips = pipe->getRefPoints();
    ASSERT_EQ(grips.getNumber(), 4u);
    EXPECT_EQ(grips.get(2), DmVector(100, 60));
    EXPECT_EQ(grips.get(3), DmVector(0, 5)) << "改管径的夹点";

    pipe->moveRef(DmVector(100, 60), DmVector(10, 0));
    EXPECT_EQ(decodePipe(pipe->data()).vertices.back(), DmVector(110, 60));
    pipe->moveRef(DmVector(0, 5), DmVector(0, 5));
    EXPECT_DOUBLE_EQ(decodePipe(pipe->data()).diameter, 20.0);
}

TEST_F(PluginEntityTest, 捕捉交给插件)
{
    DmPluginEntity* pipe = addPipe();
    ASSERT_NE(pipe, nullptr);
    EXPECT_EQ(pipe->getNearestEndpoint(DmVector(98, 2)), DmVector(100, 0));
    EXPECT_EQ(pipe->getNearestMiddle(DmVector(52, 3)), DmVector(50, 0));
    EXPECT_EQ(pipe->getNearestPointOnEntity(DmVector(40, 7)), DmVector(40, 0)) << "中心线上最近的点";
    EXPECT_FALSE(pipe->getNearestCenter(DmVector(0, 0)).valid) << "管道没有圆心";
}

TEST_F(PluginEntityTest, 变换交给插件)
{
    DmPluginEntity* pipe = addPipe();
    ASSERT_NE(pipe, nullptr);
    pipe->move(DmVector(10, 20));
    EXPECT_EQ(decodePipe(pipe->data()).vertices.front(), DmVector(10, 20));
    pipe->scale(DmVector(10, 20), DmVector(2, 2));
    EXPECT_DOUBLE_EQ(decodePipe(pipe->data()).diameter, 20.0);
    EXPECT_EQ(decodePipe(pipe->data()).vertices[1], DmVector(210, 20));
    pipe->mirror(DmVector(0, 0), DmVector(1, 0));
    EXPECT_EQ(decodePipe(pipe->data()).vertices.front(), DmVector(10, -20));
}

TEST_F(PluginEntityTest, 插件命令在事务里改数据可撤销)
{
    DmPluginEntity* pipe = addPipe();
    ASSERT_NE(pipe, nullptr);
    ASSERT_TRUE(runtime.runCommand(doc, kPluginId, QStringLiteral("demo.pipe-grow")));
    EXPECT_DOUBLE_EQ(decodePipe(pipe->data()).diameter, 20.0);
    // 管径 20：外扩 max(2.6×10, "DN20" 4 字 × 8) = 32
    EXPECT_NEAR(pipe->getMin().x, -32.0, 1e-9) << "包围框随数据重算";
    doc.getCmdManager()->undo();
    EXPECT_DOUBLE_EQ(decodePipe(pipe->data()).diameter, 10.0) << "撤销换回原来的字节";
}

TEST_F(PluginEntityTest, 只读枚举交出自定义实体与实体句柄)
{
    DmPluginEntity* pipe = addPipe();
    ASSERT_NE(pipe, nullptr);
    yicad::plugin::Host host(runtime.host().api());
    const auto document = host.document(runtime.host().documentHandle(&doc));
    ASSERT_TRUE(document);
    auto entities = document.entities();
    yicad::plugin::EntityData value;
    int count = 0;
    while (entities.next(value))
    {
        const auto* custom = std::get_if<yicad::plugin::CustomEntityData>(&value);
        ASSERT_NE(custom, nullptr);
        ++count;
        EXPECT_EQ(custom->className(), kPipeClass.toStdString());
        EXPECT_EQ(custom->classVersion(), 1u);
        EXPECT_FALSE(custom->isProxy());
        EXPECT_TRUE(custom->entity());
        EXPECT_EQ(std::string(custom->data().begin(), custom->data().end()), pipe->data());
        // 图形做成基本实体：中心线、边线、端头、箭头、文字的笔画
        auto graphics = entities.graphics();
        yicad::plugin::EntityData part;
        int parts = 0;
        while (graphics.next(part))
        {
            ++parts;
        }
        EXPECT_GE(parts, 8);
    }
    EXPECT_EQ(count, 1);
}

// 开放多段线的凸度、宽度按段存，比顶点少一段；原先按顶点取越界，枚举在它这里中断，后面的实体都交不出去
// （DXF 导出因此丢掉开放多段线与它后面的实体；阶段 8 查代理图形时发现，HostApi::readEntityData）
TEST_F(PluginEntityTest, 只读枚举交出开放多段线且不中断)
{
    std::vector<double> widths{0.0, 0.0, 1.0, 2.0};
    auto* polyline = new DmPolyline(
        nullptr, PolylineData({DmVector(0, 0), DmVector(10, 0), DmVector(10, 10)}, {0.5, 0.0}, widths, false));
    polyline->setDocument(&doc);
    polyline->update();
    ASSERT_TRUE(doc.getEntityTable()->add_direct(polyline));
    auto* line = new DmLine(nullptr, LineData(DmVector(0, 20), DmVector(10, 20)));
    line->setDocument(&doc);
    line->update();
    ASSERT_TRUE(doc.getEntityTable()->add_direct(line));

    yicad::plugin::Host host(runtime.host().api());
    const auto document = host.document(runtime.host().documentHandle(&doc));
    ASSERT_TRUE(document);
    auto entities = document.entities();
    yicad::plugin::EntityData value;
    ASSERT_TRUE(entities.next(value));
    const auto* read = std::get_if<yicad::plugin::PolylineData>(&value);
    ASSERT_NE(read, nullptr);
    ASSERT_EQ(read->vertices().size(), 3u);
    EXPECT_DOUBLE_EQ(read->vertices()[0].bulge, 0.5);
    EXPECT_DOUBLE_EQ(read->vertices()[1].startWidth, 1.0);
    EXPECT_DOUBLE_EQ(read->vertices()[1].endWidth, 2.0);
    EXPECT_DOUBLE_EQ(read->vertices()[2].bulge, 0.0) << "末顶点没有段，交 0";
    EXPECT_FALSE(read->closed());
    ASSERT_TRUE(entities.next(value)) << "后面的实体照常交出";
    EXPECT_NE(std::get_if<yicad::plugin::LineData>(&value), nullptr);
    EXPECT_FALSE(entities.next(value));
}

TEST_F(PluginEntityTest, 原生格式存读)
{
    addPipe();
    const QString file = dir.filePath(QStringLiteral("pipe.ycd"));
    save(doc, file);
    DmDocument reread;
    load(reread, file);
    const auto entities = customEntities(reread);
    ASSERT_EQ(entities.size(), 1u);
    auto* pipe = dynamic_cast<DmPluginEntity*>(entities.front());
    ASSERT_NE(pipe, nullptr) << "插件在：读回插件实体";
    EXPECT_EQ(decodePipe(pipe->data()).vertices.back(), DmVector(100, 60));
}

TEST_F(PluginEntityTest, 插件不在时读成代理按代理图形显示)
{
    DmPluginEntity* pipe = addPipe();
    ASSERT_NE(pipe, nullptr);
    const DmVector minCorner = pipe->getMin();
    const QString file = dir.filePath(QStringLiteral("pipe.ycd"));
    save(doc, file);

    detachPlugin();
    DmDocument reread;
    load(reread, file);
    const auto entities = customEntities(reread);
    ASSERT_EQ(entities.size(), 1u);
    auto* proxy = dynamic_cast<DmProxyEntity*>(entities.front());
    ASSERT_NE(proxy, nullptr);
    EXPECT_EQ(proxy->className(), kPipeClass);
    EXPECT_EQ(proxy->classVersion(), 1u);
    EXPECT_TRUE(hasFlag(proxy->proxyFlags(), DmProxyFlags::Cloning)) << "代理权限随记录存";
    EXPECT_FALSE(proxy->proxyGraphics().isEmpty());
    EXPECT_GT(proxy->getMax().x, 100.0) << "按代理图形算包围框";
    EXPECT_LE(proxy->getMin().x, -5.0 + 1e-9);
    EXPECT_GE(proxy->getMin().x, minCorner.x - 1e-9) << "代理图形不超出插件给的包围框";
    EXPECT_EQ(proxy->dataBytes(), pipe->data()) << "字节原样保留";
}

TEST_F(PluginEntityTest, 插件卸载后留下的实体只画记下的图形)
{
    DmPluginEntity* pipe = addPipe();
    ASSERT_NE(pipe, nullptr);
    const GiStream before = GiStreamRecorder::record(*pipe);
    const std::string data = pipe->data();
    detachPlugin();
    EXPECT_FALSE(pipe->entityClass()->alive());
    EXPECT_EQ(GiStreamRecorder::record(*pipe), before) << "照样画记下的图形";
    EXPECT_EQ(pipe->getRefPoints().getNumber(), 0u) << "不再调用插件";
    pipe->move(DmVector(5, 5));
    EXPECT_EQ(pipe->data(), data);
}

TEST_F(PluginEntityTest, 数据版本比插件新时读成代理)
{
    yicad::plugin::Host host(runtime.host().api());
    const auto document = host.document(runtime.host().documentHandle(&doc));
    auto session = document.beginImport();
    ASSERT_TRUE(session);
    yicad::plugin::ImportContainer modelSpace;
    ASSERT_EQ(session.modelSpace(modelSpace), YICAD_IMPORT_SUCCESS);
    const std::string bytes = encodePipe({points({DmVector(0, 0), DmVector(10, 0)}), 2.0});
    yicad::plugin::CustomEntityData newer(kPipeClass.toStdString(), 2, std::vector<uint8_t>(bytes.begin(), bytes.end()));
    newer.setProxyFlags(YICAD_PROXY_ERASE);
    yicad::plugin::CustomEntityData current(kPipeClass.toStdString(), 1, std::vector<uint8_t>(bytes.begin(), bytes.end()));
    ASSERT_EQ(modelSpace.createCustomEntity(newer), YICAD_IMPORT_SUCCESS);
    ASSERT_EQ(modelSpace.createCustomEntity(current), YICAD_IMPORT_SUCCESS);
    ASSERT_EQ(session.commit(), YICAD_IMPORT_SUCCESS);

    const auto entities = customEntities(doc);
    ASSERT_EQ(entities.size(), 2u);
    auto* proxy = dynamic_cast<DmProxyEntity*>(entities[0]);
    ASSERT_NE(proxy, nullptr) << "版本 2 比插件的 1 新：读不了";
    EXPECT_EQ(proxy->classVersion(), 2u);
    EXPECT_EQ(proxy->proxyFlags(), DmProxyFlags::Erase);
    EXPECT_NE(dynamic_cast<DmPluginEntity*>(entities[1]), nullptr);
}

TEST_F(PluginEntityTest, DXF往返)
{
    addPipe();
    const QString file = dir.filePath(QStringLiteral("pipe.dxf"));
    ASSERT_TRUE(runtime.exportFile(doc, file));

    DmDocument reread;
    ASSERT_TRUE(runtime.importFile(reread, file));
    auto entities = customEntities(reread);
    ASSERT_EQ(entities.size(), 1u);
    auto* pipe = dynamic_cast<DmPluginEntity*>(entities.front());
    ASSERT_NE(pipe, nullptr);
    EXPECT_EQ(decodePipe(pipe->data()).vertices.back(), DmVector(100, 60));

    // 插件不在：按 DXF 里的代理图形显示
    detachPlugin();
    DmDocument proxyDocument;
    ASSERT_TRUE(runtime.importFile(proxyDocument, file));
    entities = customEntities(proxyDocument);
    ASSERT_EQ(entities.size(), 1u);
    auto* proxy = dynamic_cast<DmProxyEntity*>(entities.front());
    ASSERT_NE(proxy, nullptr);
    std::map<DM::EntityType, int> counts;
    for (DmEntity* part : proxy->explode())
    {
        ++counts[part->getEntityType()];
        delete part;
    }
    // 代理图形里文字是笔画（TrueType 字形的三角形读回成 SOLID，SHX 字形是线），只数确定的几项
    EXPECT_EQ(counts[DM::EntityArc], 2) << "两端的半圆";
    EXPECT_GE(counts[DM::EntityLine], 4) << "四条边线";
    EXPECT_GE(counts[DM::EntityPolyline], 1) << "中心线";
    EXPECT_EQ(counts[DM::EntityHatch], 1) << "箭头";
}

TEST(PluginGiTest, 单次worldDraw的输出超过上限后丢弃)
{
    CountingDraw draw;
    {
        PluginGiContext context(draw, nullptr, "test.Limit");
        const YiCadGiApiV4& gi = pluginGiApi();
        std::vector<YiCadPoint2d> many(PluginGiContext::kMaxElements, YiCadPoint2d{1.0, 2.0});
        const YiCadPoint2dArrayView view{many.data(), static_cast<uint32_t>(many.size())};
        EXPECT_EQ(gi.polyline(&context, &view, nullptr, nullptr, 0), YICAD_SUCCESS);
        EXPECT_EQ(gi.point(&context, {0.0, 0.0}), YICAD_SUCCESS) << "超过上限：接受但丢弃";
        EXPECT_EQ(gi.circle(&context, {0.0, 0.0}, -1.0), YICAD_FAILURE) << "参数无效";
    }
    EXPECT_EQ(draw.primitives, 1);
}
