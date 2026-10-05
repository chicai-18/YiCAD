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

/// @file test_persistence_custom.cpp
/// @brief 自定义实体与代理图形的读写往返（RENDER_PLAN.md 第 7.3 步）
///
/// 用测试用的示例实体"管道"（tests/support/SamplePipeEntity.h）：类注册时读回原实体；类没注册时读成代理实体，
/// 字节原样保留、再存盘不丢，类回来后读出原实体；代理被变换过时读回的原实体补上变换；数据版本比程序新时读成代理；
/// 块定义里的自定义实体；块里有不写出的类型时个数不再错位。

#include <gtest/gtest.h>

#include <QTemporaryDir>

#include "DmBlock.h"
#include "DmBlockTable.h"
#include "DmCustomEntityRegistry.h"
#include "DmDocument.h"
#include "DmLayer.h"
#include "DmLayerTable.h"
#include "DmLine.h"
#include "DmProxyEntity.h"
#include "DmTriangle.h"
#include "EntityTable.h"
#include "FilterOcdIO.h"
#include "GiStream.h"
#include "TriangleData.h"
#include "support/OcdSampleDocument.h"
#include "support/SamplePipeEntity.h"

using namespace yicad_test;

namespace
{
constexpr double kTol = 1e-9;

struct CustomEntityPersistence : ::testing::Test
{
    QTemporaryDir dir;

    QString path(const QString& name) const { return dir.filePath(name); }

    void exportTo(DmDocument& doc, const QString& file)
    {
        FilterOcdIO filter;
        ASSERT_TRUE(filter.fileExport(doc, file, kOcdFormat));
    }

    void importFrom(DmDocument& doc, const QString& file)
    {
        FilterOcdIO filter;
        ASSERT_TRUE(filter.fileImport(doc, file));
        doc.regenerate();
    }

    /// @brief 在文档的"轮廓"图层上加一段管道：红色、线型比例 2
    SamplePipeEntity* addPipe(DmDocument& doc, const std::vector<DmVector>& vertices, double diameter)
    {
        DmLayer* layer = doc.getLayerTable()->find(QStringLiteral("轮廓"));
        if (!layer)
        {
            layer = new DmLayer(QStringLiteral("轮廓"));
            layer->setDocument(&doc);
            doc.getLayerTable()->add_direct(layer);
        }
        auto* pipe = new SamplePipeEntity(vertices, diameter);
        addTo(*doc.getEntityTable(), doc, pipe, layer,
              DmPen(DmColor(255, 0, 0), DM::Width09, doc.getLineTypeTable()->getLineTypeByLayer()));
        pipe->setLineTypeScale(2.0);
        return pipe;
    }

    template <typename T>
    static T* onlyCustom(const EntityTable& table)
    {
        T* found = nullptr;
        int count = 0;
        for (DmEntity* e : table)
        {
            if (e->getEntityType() == DM::EntityCustom)
            {
                found = dynamic_cast<T*>(e);
                ++count;
            }
        }
        EXPECT_EQ(count, 1);
        return found;
    }
};

void expectNear(const DmVector& got, const DmVector& expected)
{
    EXPECT_NEAR(got.x, expected.x, kTol);
    EXPECT_NEAR(got.y, expected.y, kTol);
}
}  // namespace

TEST_F(CustomEntityPersistence, 类注册时读回原实体)
{
    SamplePipeRegistration registration;
    ASSERT_TRUE(registration.registered);

    DmDocument original;
    SamplePipeEntity* pipe = addPipe(original, {DmVector(0, 0), DmVector(10, 0), DmVector(10, 5)}, 1.5);
    const QString file = path(QStringLiteral("pipe.ycd"));
    ASSERT_NO_FATAL_FAILURE(exportTo(original, file));

    DmDocument restored;
    ASSERT_NO_FATAL_FAILURE(importFrom(restored, file));
    auto* back = onlyCustom<SamplePipeEntity>(*restored.getEntityTable());
    ASSERT_NE(back, nullptr) << "读回的不是原实体";
    ASSERT_EQ(back->vertices().size(), 3u);
    for (std::size_t i = 0; i < 3; ++i)
    {
        expectNear(back->vertices()[i], pipe->vertices()[i]);
    }
    EXPECT_DOUBLE_EQ(back->diameter(), 1.5);
    EXPECT_EQ(back->getId(), pipe->getId());
    ASSERT_NE(back->getLayer(false), nullptr);
    EXPECT_EQ(back->getLayer(false)->getName(), QStringLiteral("轮廓"));
    EXPECT_EQ(back->getPen(false).getColor(), DmColor(255, 0, 0));
    EXPECT_EQ(back->getPen(false).getWidth(), DM::Width09);
    EXPECT_DOUBLE_EQ(back->getLineTypeScale(), 2.0);
    expectNear(back->getMin(), pipe->getMin());
    expectNear(back->getMax(), pipe->getMax());
}

TEST_F(CustomEntityPersistence, 类没注册时读成代理再存盘不丢扩展回来后读出原实体)
{
    const QString first = path(QStringLiteral("first.ycd"));
    const QString second = path(QStringLiteral("second.ycd"));
    GiStream originalGraphics;
    DmVector originalMin, originalMax;
    {
        SamplePipeRegistration registration;
        DmDocument original;
        SamplePipeEntity* pipe = addPipe(original, {DmVector(0, 0), DmVector(10, 0)}, 2.0);
        originalGraphics = pipe->proxyGraphics();
        originalMin = pipe->getMin();
        originalMax = pipe->getMax();
        ASSERT_NO_FATAL_FAILURE(exportTo(original, first));
    }
    ASSERT_EQ(DmCustomEntityRegistry::instance().find(QStringLiteral("ext.sample.Pipe")), nullptr);

    // 扩展不在：代理保管类名、代理权限与数据，按代理图形显示（包围框与原实体相同）
    {
        DmDocument proxies;
        ASSERT_NO_FATAL_FAILURE(importFrom(proxies, first));
        auto* proxy = onlyCustom<DmProxyEntity>(*proxies.getEntityTable());
        ASSERT_NE(proxy, nullptr);
        EXPECT_TRUE(proxy->isProxy());
        EXPECT_EQ(proxy->className(), QStringLiteral("ext.sample.Pipe"));
        EXPECT_EQ(proxy->classVersion(), SamplePipeEntity::kVersion);
        EXPECT_EQ(proxy->proxyFlags(), DmProxyFlags::Erase | DmProxyFlags::Transform);
        EXPECT_FALSE(proxy->dataBytes().empty());
        EXPECT_FALSE(proxy->proxyGraphics().isEmpty());
        EXPECT_EQ(proxy->proxyGraphics().byteSize(), originalGraphics.byteSize());
        expectNear(proxy->getMin(), originalMin);
        expectNear(proxy->getMax(), originalMax);
        EXPECT_EQ(proxy->getLayer(false)->getName(), QStringLiteral("轮廓"));
        EXPECT_DOUBLE_EQ(proxy->getLineTypeScale(), 2.0);
        ASSERT_NO_FATAL_FAILURE(exportTo(proxies, second));
    }

    // 扩展回来：再存过的图纸读出原实体
    SamplePipeRegistration registration;
    DmDocument restored;
    ASSERT_NO_FATAL_FAILURE(importFrom(restored, second));
    auto* back = onlyCustom<SamplePipeEntity>(*restored.getEntityTable());
    ASSERT_NE(back, nullptr);
    ASSERT_EQ(back->vertices().size(), 2u);
    expectNear(back->vertices()[1], DmVector(10, 0));
    EXPECT_DOUBLE_EQ(back->diameter(), 2.0);
    EXPECT_EQ(back->getPen(false).getColor(), DmColor(255, 0, 0));
}

TEST_F(CustomEntityPersistence, 代理被变换过扩展回来后原实体补上变换)
{
    const QString first = path(QStringLiteral("first.ycd"));
    const QString moved = path(QStringLiteral("moved.ycd"));
    {
        SamplePipeRegistration registration;
        DmDocument original;
        addPipe(original, {DmVector(0, 0), DmVector(10, 0)}, 2.0);
        ASSERT_NO_FATAL_FAILURE(exportTo(original, first));
    }
    {
        DmDocument proxies;
        ASSERT_NO_FATAL_FAILURE(importFrom(proxies, first));
        auto* proxy = onlyCustom<DmProxyEntity>(*proxies.getEntityTable());
        ASSERT_NE(proxy, nullptr);
        const DmVector minBefore = proxy->getMin();
        // 允许变换：记下累计变换，图形跟着动（包围框随之移动），数据不动
        const std::string data = proxy->dataBytes();
        proxy->rotate(DmVector(0, 0), DmVector(M_PI / 2.0));
        proxy->move(DmVector(100, 0));
        EXPECT_EQ(proxy->dataBytes(), data);
        EXPECT_FALSE(proxy->transform().isIdentity());
        EXPECT_GT(proxy->getMin().x, minBefore.x + 90.0);
        ASSERT_NO_FATAL_FAILURE(exportTo(proxies, moved));
    }
    SamplePipeRegistration registration;
    DmDocument restored;
    ASSERT_NO_FATAL_FAILURE(importFrom(restored, moved));
    auto* back = onlyCustom<SamplePipeEntity>(*restored.getEntityTable());
    ASSERT_NE(back, nullptr);
    ASSERT_EQ(back->vertices().size(), 2u);
    // 先绕原点转 90°，再右移 100
    expectNear(back->vertices()[0], DmVector(100, 0));
    expectNear(back->vertices()[1], DmVector(100, 10));
}

TEST_F(CustomEntityPersistence, 代理不允许变换时移动不起作用)
{
    const QString file = path(QStringLiteral("locked.ycd"));
    {
        SamplePipeRegistration registration(DmProxyFlags::Erase);
        DmDocument original;
        addPipe(original, {DmVector(0, 0), DmVector(10, 0)}, 2.0);
        ASSERT_NO_FATAL_FAILURE(exportTo(original, file));
    }
    DmDocument proxies;
    ASSERT_NO_FATAL_FAILURE(importFrom(proxies, file));
    auto* proxy = onlyCustom<DmProxyEntity>(*proxies.getEntityTable());
    ASSERT_NE(proxy, nullptr);
    const DmVector minBefore = proxy->getMin();
    proxy->move(DmVector(100, 0));
    EXPECT_TRUE(proxy->transform().isIdentity());
    expectNear(proxy->getMin(), minBefore);
    EXPECT_TRUE(DmProxyEntity::allows(proxy, DmProxyFlags::Erase));
    EXPECT_FALSE(DmProxyEntity::allows(proxy, DmProxyFlags::Transform));
    EXPECT_FALSE(DmProxyEntity::allows(proxy, DmProxyFlags::Erase | DmProxyFlags::Cloning));
}

TEST_F(CustomEntityPersistence, 数据版本比程序新时读成代理)
{
    // 存盘时类登记的版本是 2，比示例实体能读的 1 新：restoreData 返回 false，宿主改建代理
    const QString file = path(QStringLiteral("newer.ycd"));
    DmCustomEntityClass newer = DmCustomEntityRegistry::describe<SamplePipeEntity>(DmProxyFlags::Erase,
                                                                                   QStringLiteral("ext.sample"));
    newer.version = 2;
    {
        ASSERT_TRUE(DmCustomEntityRegistry::instance().registerClass(newer));
        DmDocument original;
        addPipe(original, {DmVector(0, 0), DmVector(10, 0)}, 2.0);
        ASSERT_NO_FATAL_FAILURE(exportTo(original, file));
        DmCustomEntityRegistry::instance().unregisterClass(newer.name);
    }
    SamplePipeRegistration registration;
    DmDocument restored;
    ASSERT_NO_FATAL_FAILURE(importFrom(restored, file));
    auto* proxy = onlyCustom<DmProxyEntity>(*restored.getEntityTable());
    ASSERT_NE(proxy, nullptr);
    EXPECT_EQ(proxy->classVersion(), 2u);
    EXPECT_FALSE(proxy->proxyGraphics().isEmpty());
}

TEST_F(CustomEntityPersistence, 块定义里的自定义实体读回与读成代理)
{
    const QString file = path(QStringLiteral("block.ycd"));
    {
        SamplePipeRegistration registration;
        DmDocument original;
        auto* block = new DmBlock(&original, DmBlockData(QStringLiteral("管道块"), DmVector(0, 0), false));
        original.getBlockTable()->add_direct(block);
        auto* pipe = new SamplePipeEntity({DmVector(0, 0), DmVector(4, 0)}, 1.0);
        addTo(block->getEntityTable(), original, pipe, original.getLayerTable()->find(QStringLiteral("0")));
        addTo(block->getEntityTable(), original, new DmLine(DmVector(0, 1), DmVector(4, 1)),
              original.getLayerTable()->find(QStringLiteral("0")));
        ASSERT_NO_FATAL_FAILURE(exportTo(original, file));
    }
    {
        SamplePipeRegistration registration;
        DmDocument restored;
        ASSERT_NO_FATAL_FAILURE(importFrom(restored, file));
        DmBlock* block = restored.getBlockTable()->find(QStringLiteral("管道块"));
        ASSERT_NE(block, nullptr);
        EXPECT_EQ(block->getEntityTable().count(), 2);
        EXPECT_NE(onlyCustom<SamplePipeEntity>(block->getEntityTable()), nullptr);
    }
    DmDocument proxies;
    ASSERT_NO_FATAL_FAILURE(importFrom(proxies, file));
    DmBlock* block = proxies.getBlockTable()->find(QStringLiteral("管道块"));
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->getEntityTable().count(), 2);
    EXPECT_NE(onlyCustom<DmProxyEntity>(block->getEntityTable()), nullptr);
}

TEST_F(CustomEntityPersistence, 块里有不写出的类型时读回不错位)
{
    // 块存盘原先先写全部实体的个数、只写出认得的类型：块里有三角形时读盘多读一项，后面的块读不出来
    const QString file = path(QStringLiteral("triangle.ycd"));
    {
        DmDocument original;
        DmLayer* layer0 = original.getLayerTable()->find(QStringLiteral("0"));
        auto* first = new DmBlock(&original, DmBlockData(QStringLiteral("A"), DmVector(0, 0), false));
        original.getBlockTable()->add_direct(first);
        addTo(first->getEntityTable(), original,
              new DmTriangle(nullptr, TriangleData(DmVector(0, 0), DmVector(1, 0), DmVector(0, 1))), layer0);
        addTo(first->getEntityTable(), original, new DmLine(DmVector(0, 0), DmVector(2, 0)), layer0);
        auto* second = new DmBlock(&original, DmBlockData(QStringLiteral("B"), DmVector(0, 0), false));
        original.getBlockTable()->add_direct(second);
        addTo(second->getEntityTable(), original, new DmLine(DmVector(0, 0), DmVector(3, 0)), layer0);
        ASSERT_NO_FATAL_FAILURE(exportTo(original, file));
    }
    DmDocument restored;
    ASSERT_NO_FATAL_FAILURE(importFrom(restored, file));
    DmBlock* a = restored.getBlockTable()->find(QStringLiteral("A"));
    DmBlock* b = restored.getBlockTable()->find(QStringLiteral("B"));
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(a->getEntityTable().count(), 1) << "三角形不写出，只剩直线";
    ASSERT_EQ(b->getEntityTable().count(), 1);
    auto* line = dynamic_cast<DmLine*>(*b->getEntityTable().begin());
    ASSERT_NE(line, nullptr);
    expectNear(line->getEndpoint(), DmVector(3, 0));
}
