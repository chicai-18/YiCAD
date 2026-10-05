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

/// @file test_custom_entity.cpp
/// @brief 进程内自定义实体经扩展接入（RENDER_PLAN.md 第 7 阶段）
///
/// 用测试用的示例扩展 ext.sample（tests/support/SampleEntityExtension.h）与它的实体"管道"：扩展登记实体类与按类名的
/// 属性编辑命令、扩展关闭时注销；命令创建、撤销重做；捕捉（端点、圆心、交点）、拾取与交叉选；夹点编辑；属性编辑分派；
/// 炸开；修改的撤销；代理实体按代理权限放行命令；另存为 DXF 时写出炸开结果。

#include <gtest/gtest.h>

#include <map>
#include <memory>

#include <QTemporaryDir>

#include "DmCustomEntityRegistry.h"
#include "DmLine.h"
#include "DmProxyEntity.h"
#include "ExtensionManager.h"
#include "IExtension.h"
#include "IExtensionContext.h"
#include "ISnapService.h"
#include "support/CommandTestFixture.h"
#include "support/DxfTestRuntime.h"
#include "support/SampleEntityExtension.h"
#include "support/SamplePipeEntity.h"

using namespace yicad_test;

namespace
{
/// @brief 启动原内置命令所在的五个扩展与示例扩展，析构时注销
struct SampleExtensionsScope
{
    FakeExtensionHost host;

    SampleExtensionsScope()
    {
        registerCommandExtensions();
        ExtensionManager::instance().Register(std::make_unique<SampleEntityExtension>());
        ExtensionManager::instance().BootAll(host);
    }
    ~SampleExtensionsScope() { ExtensionManager::instance().Shutdown(); }

    SampleExtensionsScope(const SampleExtensionsScope&) = delete;
    SampleExtensionsScope& operator=(const SampleExtensionsScope&) = delete;
};

/// @brief 有示例扩展的命令夹具；管道 (0,0)-(100,0)、管径 20，边线在 y = ±10
struct CustomEntityFixture : CommandFixture
{
    SampleExtensionsScope extensions;

    CustomEntityFixture()
    {
        SampleEntityExtension::propertyRuns = 0;
        SampleEntityExtension::lastEditedEntity = nullptr;
    }

    SamplePipeEntity* addPipe()
    {
        auto* pipe = new SamplePipeEntity({DmVector(0, 0), DmVector(100, 0)}, 20.0);
        pipe->setDocument(&doc);
        pipe->update();
        EXPECT_TRUE(doc.getEntityTable()->add_direct(pipe));
        return pipe;
    }

    /// @brief 一个代理实体：图形取同样的管道
    DmProxyEntity* addProxy(DmProxyFlags flags)
    {
        SamplePipeEntity source({DmVector(0, 0), DmVector(100, 0)}, 20.0);
        auto* proxy = new DmProxyEntity(QStringLiteral("ext.sample.Pipe"), flags);
        proxy->setDocument(&doc);
        proxy->setProxyGraphics(source.proxyGraphics());
        proxy->update();
        EXPECT_TRUE(doc.getEntityTable()->add_direct(proxy));
        return proxy;
    }

    /// @brief 只开一种捕捉
    void snapOnly(bool SnapMode::*mode)
    {
        SnapMode snapMode;
        snapMode.*mode = true;
        snapper.setSnapMode(snapMode);
    }

    DmVector snapAt(int x, int y)
    {
        QMouseEvent e = makeMouse(QEvent::MouseMove, x, y, Qt::NoButton);
        return snapper.snapPoint(&e);
    }

    /// @brief 实体表里未删除的实体按类型数
    std::map<DM::EntityType, int> liveTypes()
    {
        std::map<DM::EntityType, int> counts;
        for (DmEntity* e : *doc.getEntityTable())
        {
            if (!e->isErased())
            {
                ++counts[e->getEntityType()];
            }
        }
        return counts;
    }

    bool hasProxyMessage() const
    {
        for (const QString& message : ui.messages)
        {
            if (message.contains(QStringLiteral("proxy")))
            {
                return true;
            }
        }
        return false;
    }
};

/// @brief 只在 OnRegister 里调一个函数的扩展
class LambdaExtension : public IExtension
{
public:
    LambdaExtension(std::string id, std::function<void(IExtensionContext&)> onRegister)
        : m_id(std::move(id))
        , m_onRegister(std::move(onRegister))
    {
    }
    void OnRegister(IExtensionContext& ctx) override { m_onRegister(ctx); }
    std::string_view Id() const override { return m_id; }

private:
    std::string m_id;
    std::function<void(IExtensionContext&)> m_onRegister;
};
}  // namespace

TEST(CustomEntityExtensionTest, 扩展登记实体类关闭时注销)
{
    const QString name = QStringLiteral("ext.sample.Pipe");
    {
        FakeExtensionHost host;
        ExtensionManager::instance().Register(std::make_unique<SampleEntityExtension>());
        ExtensionManager::instance().BootAll(host);
        const DmCustomEntityClass* entityClass = DmCustomEntityRegistry::instance().find(name);
        ASSERT_NE(entityClass, nullptr);
        EXPECT_EQ(entityClass->proxyFlags, SampleEntityExtension::kProxyFlags);
        EXPECT_EQ(entityClass->version, SamplePipeEntity::kVersion);
        EXPECT_EQ(entityClass->owner, QStringLiteral("ext.sample"));

        // 按类名登记的属性编辑命令：同是自定义实体，别的类没有
        SamplePipeEntity pipe({DmVector(0, 0), DmVector(1, 0)}, 1.0);
        EXPECT_EQ(CommandRegistry::instance().propertyEditor(pipe), QStringLiteral("ext.sample.properties"));
        DmProxyEntity other(QStringLiteral("ext.other.Thing"), DmProxyFlags::None);
        EXPECT_TRUE(CommandRegistry::instance().propertyEditor(other).isEmpty());
        EXPECT_TRUE(CommandRegistry::instance().propertyEditor(DM::EntityCustom).isEmpty());
        ExtensionManager::instance().Shutdown();
    }
    EXPECT_EQ(DmCustomEntityRegistry::instance().find(name), nullptr);
    EXPECT_FALSE(CommandRegistry::instance().hasCommand(QStringLiteral("ext.sample.pipe")));
}

TEST(CustomEntityExtensionTest, 类名不在本扩展命名空间内被拒)
{
    bool classAccepted = true;
    bool editorAccepted = true;
    FakeExtensionHost host;
    ExtensionManager::instance().Register(std::make_unique<LambdaExtension>(
        "ext.other",
        [&](IExtensionContext& ctx)
        {
            // 示例实体的类名是 ext.sample.Pipe，不在 ext.other 下
            classAccepted = ctx.registerEntityClass<SamplePipeEntity>(DmProxyFlags::None);
            ctx.registerInstantCommand(QStringLiteral("ext.other.edit"), [](const CommandContext&) {}, CommandInfo{});
            editorAccepted =
                ctx.registerPropertyEditor(QStringLiteral("ext.sample.Pipe"), QStringLiteral("ext.other.edit"));
        }));
    ExtensionManager::instance().BootAll(host);
    EXPECT_FALSE(classAccepted);
    EXPECT_FALSE(editorAccepted);
    EXPECT_EQ(DmCustomEntityRegistry::instance().find(QStringLiteral("ext.sample.Pipe")), nullptr);
    ExtensionManager::instance().Shutdown();
}

TEST_F(CustomEntityFixture, 命令创建管道并可撤销重做)
{
    ASSERT_TRUE(start("ext.sample.pipe"));
    typeCoordinate(0.0, 0.0);
    move(50, 0);
    EXPECT_EQ(previewCount(), 1) << "移动鼠标时预览管道";
    typeCoordinate(100.0, 0.0);
    EXPECT_EQ(previewCount(), 0);

    SamplePipeEntity* pipe = nullptr;
    for (DmEntity* e : *doc.getEntityTable())
    {
        pipe = dynamic_cast<SamplePipeEntity*>(e);
    }
    ASSERT_NE(pipe, nullptr);
    ASSERT_EQ(pipe->vertices().size(), 2u);
    EXPECT_EQ(pipe->vertices()[1], DmVector(100.0, 0.0));
    EXPECT_DOUBLE_EQ(pipe->diameter(), SampleEntityExtension::kDiameter);
    endCommand();

    doc.getCmdManager()->undo();
    EXPECT_TRUE(pipe->isErased());
    doc.getCmdManager()->redo();
    EXPECT_FALSE(pipe->isErased());
}

TEST_F(CustomEntityFixture, 捕捉端点圆心与交点)
{
    addPipe();
    snapOnly(&SnapMode::snapEndpoint);
    EXPECT_EQ(snapAt(98, 9), DmVector(100.0, 10.0));
    snapOnly(&SnapMode::snapCenter);
    EXPECT_EQ(snapAt(103, 2), DmVector(100.0, 0.0)) << "端头圆弧的圆心";
    snapOnly(&SnapMode::snapMiddle);
    EXPECT_EQ(snapAt(51, 9), DmVector(50.0, 10.0));

    auto* line = new DmLine(DmVector(60, -30), DmVector(60, 30));
    line->setDocument(&doc);
    line->calculateBorders();
    ASSERT_TRUE(doc.getEntityTable()->add_direct(line));
    snapOnly(&SnapMode::snapIntersection);
    const DmVector hit = snapAt(61, 9);
    EXPECT_NEAR(hit.x, 60.0, 1e-9);
    EXPECT_NEAR(hit.y, 10.0, 1e-9);
}

TEST_F(CustomEntityFixture, 点选与交叉选)
{
    SamplePipeEntity* pipe = addPipe();
    click(30, 10);
    EXPECT_TRUE(selection.contains(pipe)) << "点在边线上选中整个管道";

    EntityTable* table = doc.getEntityTable();
    auto crossing = table->entitiesCrossingRect(DmVector(20, 5), DmVector(30, 15), {});
    EXPECT_EQ(crossing.size(), 1u) << "交叉选：矩形只与一条边线相交";
    EXPECT_TRUE(table->entitiesInsideRect(DmVector(20, 5), DmVector(30, 15), {}).empty());
    EXPECT_EQ(table->entitiesInsideRect(DmVector(-20, -20), DmVector(120, 20), {}).size(), 1u);
    EXPECT_TRUE(table->entitiesCrossingRect(DmVector(20, 12), DmVector(30, 15), {}).empty());
}

TEST_F(CustomEntityFixture, 夹点改折点与管径并可撤销)
{
    SamplePipeEntity* pipe = addPipe();
    selection.add(pipe);
    // 终点的夹点：单击激活，再单击落位
    click(100, 0);
    click(120, 20);
    ASSERT_EQ(pipe->vertices().size(), 2u);
    EXPECT_EQ(pipe->vertices()[1], DmVector(120.0, 20.0));
    EXPECT_TRUE(selection.contains(pipe));
    doc.getCmdManager()->undo();
    EXPECT_EQ(pipe->vertices()[1], DmVector(100.0, 0.0));

    // 管径夹点在起点上方 10 处：拖到 15 处，管径变成 30
    click(0, 10);
    click(0, 15);
    EXPECT_DOUBLE_EQ(pipe->diameter(), 30.0);
}

TEST_F(CustomEntityFixture, 修改实体属性按类名分派)
{
    SamplePipeEntity* pipe = addPipe();
    ASSERT_TRUE(start("ext.modify.entity"));
    click(30, 10);
    EXPECT_EQ(SampleEntityExtension::propertyRuns, 1);
    EXPECT_EQ(SampleEntityExtension::lastEditedEntity, pipe);
    endCommand();
}

TEST_F(CustomEntityFixture, 移动后撤销回到原处)
{
    SamplePipeEntity* pipe = addPipe();
    selection.add(pipe);
    ASSERT_TRUE(start("ext.modify.move"));
    typeCoordinate(0.0, 0.0);
    typeCoordinate(5.0, 5.0);
    EXPECT_EQ(pipe->vertices()[0], DmVector(5.0, 5.0));
    EXPECT_NEAR(pipe->getMin().x, -5.0, 1e-9) << "包围框随之更新";
    doc.getCmdManager()->undo();
    EXPECT_EQ(pipe->vertices()[0], DmVector(0.0, 0.0));
    EXPECT_NEAR(pipe->getMin().x, -10.0, 1e-9);
}

TEST_F(CustomEntityFixture, 炸开成基本实体并可撤销)
{
    SamplePipeEntity* pipe = addPipe();
    selection.add(pipe);
    ASSERT_TRUE(start("ext.modify.explode"));
    EXPECT_TRUE(pipe->isErased());
    auto counts = liveTypes();
    EXPECT_EQ(counts[DM::EntityCustom], 0);
    EXPECT_EQ(counts[DM::EntityLine], 3);
    EXPECT_EQ(counts[DM::EntityArc], 2);
    EXPECT_EQ(counts[DM::EntitySolid], 2);

    doc.getCmdManager()->undo();
    EXPECT_FALSE(pipe->isErased());
    counts = liveTypes();
    EXPECT_EQ(counts[DM::EntityCustom], 1);
    EXPECT_EQ(counts[DM::EntityLine], 0);
}

TEST_F(CustomEntityFixture, 代理按代理权限放行命令)
{
    // 只允许删除：移动、复制跳过它并在命令行说明，删除照常
    DmProxyEntity* proxy = addProxy(DmProxyFlags::Erase);
    selection.add(proxy);
    ASSERT_TRUE(start("ext.modify.move"));
    typeCoordinate(0.0, 0.0);
    typeCoordinate(5.0, 5.0);
    EXPECT_TRUE(proxy->transform().isIdentity());
    EXPECT_TRUE(hasProxyMessage());

    ui.messages.clear();
    selection.add(proxy);
    ASSERT_TRUE(start("ext.modify.copy"));
    typeCoordinate(0.0, 0.0);
    typeCoordinate(0.0, 50.0);
    endCommand();
    EXPECT_EQ(liveTypes()[DM::EntityCustom], 1) << "没有复制出新的";
    EXPECT_TRUE(hasProxyMessage());

    selection.add(proxy);
    ASSERT_TRUE(start("ext.modify.delete"));
    pressKey(Qt::Key_Enter);
    EXPECT_TRUE(proxy->isErased());
}

TEST_F(CustomEntityFixture, 允许变换的代理移动时记下变换)
{
    DmProxyEntity* proxy = addProxy(DmProxyFlags::Transform);
    selection.add(proxy);
    ASSERT_TRUE(start("ext.modify.move"));
    typeCoordinate(0.0, 0.0);
    typeCoordinate(5.0, 5.0);
    EXPECT_EQ(proxy->transform(), GiTransform::translation(DmVector(5.0, 5.0)));
    EXPECT_NEAR(proxy->getMin().x, -5.0, 1e-9);
    doc.getCmdManager()->undo();
    EXPECT_TRUE(proxy->transform().isIdentity()) << "累计的变换随实体存取，撤销回到原样";
}

TEST_F(CustomEntityFixture, 不允许删除的代理炸开后原对象留着)
{
    DmProxyEntity* proxy = addProxy(DmProxyFlags::None);
    selection.add(proxy);
    ASSERT_TRUE(start("ext.modify.explode"));
    EXPECT_FALSE(proxy->isErased());
    EXPECT_EQ(liveTypes()[DM::EntityLine], 3) << "按代理图形炸开";
    EXPECT_TRUE(hasProxyMessage());
}

TEST_F(CustomEntityFixture, 另存为DXF时写出炸开结果)
{
    DxfRuntime runtime;
    ASSERT_TRUE(runtime.loaded()) << runtime.diagnostics().toStdString();
    addPipe();
    QTemporaryDir dir;
    const QString file = dir.filePath(QStringLiteral("pipe.dxf"));
    ASSERT_TRUE(runtime.exportFile(doc, file));

    DmDocument reread;
    ASSERT_TRUE(runtime.importFile(reread, file));
    std::map<DM::EntityType, int> counts;
    for (DmEntity* e : *reread.getEntityTable())
    {
        ++counts[e->getEntityType()];
    }
    EXPECT_EQ(counts[DM::EntityCustom], 0);
    EXPECT_EQ(counts[DM::EntityLine], 3);
    EXPECT_EQ(counts[DM::EntityArc], 2);
    EXPECT_EQ(counts[DM::EntitySolid], 2);
}
