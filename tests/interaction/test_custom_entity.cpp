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
/// 炸开；修改的撤销；代理实体按代理权限放行命令；DXF 里照 AutoCAD 写自定义实体、类不在时读成代理，
/// AutoCAD 另存的 ACAD_PROXY_ENTITY 与别的程序的自定义实体读成代理、另存时原样写回（第 8.4 步）。

#include <gtest/gtest.h>

#include <algorithm>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include <QFile>
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

namespace
{
/// @brief 文档模型空间里的实体按类型数
std::map<DM::EntityType, int> countTypes(DmDocument& document)
{
    std::map<DM::EntityType, int> counts;
    for (DmEntity* e : *document.getEntityTable())
    {
        ++counts[e->getEntityType()];
    }
    return counts;
}

/// @brief 文档模型空间里第一个自定义实体
DmCustomEntity* firstCustom(DmDocument& document)
{
    for (DmEntity* e : *document.getEntityTable())
    {
        if (e->getEntityType() == DM::EntityCustom)
        {
            return static_cast<DmCustomEntity*>(e);
        }
    }
    return nullptr;
}

QByteArray readAll(const QString& path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

/// @brief ASCII DXF 的组码对：组码去掉空白，值去掉行尾
using GroupPairs = std::vector<std::pair<int, QByteArray>>;

GroupPairs readPairs(const QString& path)
{
    GroupPairs pairs;
    const QList<QByteArray> lines = readAll(path).split('\n');
    for (qsizetype i = 0; i + 1 < lines.size(); i += 2)
    {
        QByteArray value = lines[i + 1];
        if (value.endsWith('\r'))
        {
            value.chop(1);
        }
        pairs.emplace_back(lines[i].trimmed().toInt(), value);
    }
    return pairs;
}

bool writePairs(const QString& path, const GroupPairs& pairs)
{
    QByteArray text;
    for (const auto& [code, value] : pairs)
    {
        text += QByteArray::number(code).rightJustified(3, ' ') + "\r\n" + value + "\r\n";
    }
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(text) == text.size();
}

/// @brief 从 from 起第一个组码 0、值为 name 的记录：[开头, 下一个组码 0)；没有时两个都是 pairs.size()
std::pair<std::size_t, std::size_t> findRecord(const GroupPairs& pairs, const QByteArray& name, std::size_t from = 0)
{
    for (std::size_t i = from; i < pairs.size(); ++i)
    {
        if (pairs[i].first == 0 && pairs[i].second == name)
        {
            std::size_t end = i + 1;
            while (end < pairs.size() && pairs[end].first != 0)
            {
                ++end;
            }
            return {i, end};
        }
    }
    return {pairs.size(), pairs.size()};
}

/// @brief 实体段里第一个记录名为 name 的记录
std::pair<std::size_t, std::size_t> findEntity(const GroupPairs& pairs, const QByteArray& name)
{
    for (std::size_t i = 0; i + 1 < pairs.size(); ++i)
    {
        if (pairs[i].first == 2 && pairs[i].second == "ENTITIES")
        {
            return findRecord(pairs, name, i);
        }
    }
    return {pairs.size(), pairs.size()};
}

/// @brief 记录 [begin, end) 里第一个组码 100、值为 marker 的位置；没有时为 end
std::size_t findSubclass(const GroupPairs& pairs, std::size_t begin, std::size_t end, const QByteArray& marker)
{
    for (std::size_t i = begin; i < end; ++i)
    {
        if (pairs[i].first == 100 && pairs[i].second == marker)
        {
            return i;
        }
    }
    return end;
}

/// @brief 记录 [begin, end) 里的代理图形：160 与跟着的 310
GroupPairs graphicsPairs(const GroupPairs& pairs, std::size_t begin, std::size_t end)
{
    GroupPairs graphics;
    for (std::size_t i = begin; i < end; ++i)
    {
        if (pairs[i].first == 160)
        {
            graphics.push_back(pairs[i]);
            for (std::size_t k = i + 1; k < end && pairs[k].first == 310; ++k)
            {
                graphics.push_back(pairs[k]);
            }
            break;
        }
    }
    return graphics;
}

/// @brief [begin, end) 里组码在 codes 中的组码对，值去掉两端空白
GroupPairs pick(const GroupPairs& pairs, std::size_t begin, std::size_t end, std::initializer_list<int> codes)
{
    GroupPairs picked;
    for (std::size_t i = begin; i < end; ++i)
    {
        if (std::find(codes.begin(), codes.end(), pairs[i].first) != codes.end())
        {
            picked.emplace_back(pairs[i].first, pairs[i].second.trimmed());
        }
    }
    return picked;
}

/// @brief 在 CLASSES 段第一条之前加一条 AutoCAD 自己的类登记（之后各类的序号加一）
void prependClass(GroupPairs& pairs)
{
    const std::size_t cls = findRecord(pairs, "CLASS").first;
    const GroupPairs dictionary{{0, "CLASS"}, {1, "ACDBDICTIONARYWDFLT"}, {2, "AcDbDictionaryWithDefault"},
                                {3, "ObjectDBX Classes"}, {90, "0"}, {91, "0"}, {280, "0"}, {281, "0"}};
    pairs.insert(pairs.begin() + static_cast<std::ptrdiff_t>(cls), dictionary.begin(), dictionary.end());
}

/// @brief 把 CLASSES 段里的类换成别的程序的类 ACME_THING（类名 AcmeThing，应用 ACME，代理权限 flags）
void renameClassToAcme(GroupPairs& pairs, int flags)
{
    const auto [cls, clsEnd] = findRecord(pairs, "CLASS");
    for (std::size_t i = cls; i < clsEnd; ++i)
    {
        switch (pairs[i].first)
        {
        case 1: pairs[i].second = "ACME_THING"; break;
        case 2: pairs[i].second = "AcmeThing"; break;
        case 3: pairs[i].second = "ACME"; break;
        case 90: pairs[i].second = QByteArray::number(flags); break;
        default: break;
        }
    }
}

/// @brief 把记录 [begin, end) 里从 from 起的部分换成 replacement
void replaceTail(GroupPairs& pairs, std::size_t from, std::size_t end, const GroupPairs& replacement)
{
    pairs.erase(pairs.begin() + static_cast<std::ptrdiff_t>(from), pairs.begin() + static_cast<std::ptrdiff_t>(end));
    pairs.insert(pairs.begin() + static_cast<std::ptrdiff_t>(from), replacement.begin(), replacement.end());
}
}  // namespace

// v4 起照 AutoCAD 写：自己的类型名、CLASSES 登记、代理图形与数据（RENDER_PLAN.md 第 8.4 步；第 7 阶段写的是炸开结果）
TEST_F(CustomEntityFixture, 另存为DXF时照AutoCAD写出自定义实体)
{
    DxfRuntime runtime;
    ASSERT_TRUE(runtime.loaded()) << runtime.diagnostics().toStdString();
    addPipe();
    QTemporaryDir dir;
    const QString file = dir.filePath(QStringLiteral("pipe.dxf"));
    ASSERT_TRUE(runtime.exportFile(doc, file));
    const QByteArray text = readAll(file);
    EXPECT_TRUE(text.contains("EXT_SAMPLE_PIPE")) << "类型名由类名转大写";
    EXPECT_TRUE(text.contains("ext.sample.Pipe")) << "CLASSES 段登记类名";
    EXPECT_TRUE(text.contains("YiCadCustomEntity")) << "数据写在自己的子类段";

    DmDocument reread;
    ASSERT_TRUE(runtime.importFile(reread, file));
    const auto counts = countTypes(reread);
    EXPECT_EQ(counts.size(), 1u) << "只有那一个自定义实体，没有炸开的线";
    auto* pipe = dynamic_cast<SamplePipeEntity*>(firstCustom(reread));
    ASSERT_NE(pipe, nullptr) << "类在：读回原实体";
    EXPECT_EQ(pipe->vertices(), (std::vector<DmVector>{DmVector(0, 0), DmVector(100, 0)}));
    EXPECT_DOUBLE_EQ(pipe->diameter(), 20.0);
}

TEST_F(CustomEntityFixture, 类不在时DXF里的自定义实体读成代理且数据不丢)
{
    DxfRuntime runtime;
    ASSERT_TRUE(runtime.loaded()) << runtime.diagnostics().toStdString();
    addPipe();
    QTemporaryDir dir;
    const QString file = dir.filePath(QStringLiteral("pipe.dxf"));
    ASSERT_TRUE(runtime.exportFile(doc, file));

    // 扩展不在：读成代理，按 DXF 里的代理图形显示
    ASSERT_TRUE(DmCustomEntityRegistry::instance().unregisterClass(QStringLiteral("ext.sample.Pipe")));
    DmDocument proxyDocument;
    ASSERT_TRUE(runtime.importFile(proxyDocument, file));
    auto* proxy = dynamic_cast<DmProxyEntity*>(firstCustom(proxyDocument));
    ASSERT_NE(proxy, nullptr);
    EXPECT_EQ(proxy->className(), QStringLiteral("ext.sample.Pipe"));
    EXPECT_EQ(proxy->proxyFlags(), DmProxyFlags::Erase | DmProxyFlags::Transform | DmProxyFlags::LayerChange |
                                       DmProxyFlags::ColorChange)
        << "代理权限取 CLASSES 段的组码 90";
    EXPECT_NEAR(proxy->getMin().x, -10.0, 1e-6) << "图形含端头";
    EXPECT_NEAR(proxy->getMax().x, 110.0, 1e-6);
    std::map<DM::EntityType, int> parts;
    for (DmEntity* part : proxy->explode())
    {
        ++parts[part->getEntityType()];
        delete part;
    }
    EXPECT_EQ(parts[DM::EntityLine], 3) << "中心线与两条边线";
    EXPECT_EQ(parts[DM::EntityArc], 2) << "两端的半圆";
    EXPECT_EQ(parts[DM::EntitySolid], 2) << "箭头的两个三角形（DXF 里是填充多边形），与炸开原实体相同是 SOLID";
    EXPECT_EQ(parts[DM::EntityHatch], 0);

    // 代理再存 DXF，扩展回来后读出原实体
    const QString again = dir.filePath(QStringLiteral("again.dxf"));
    ASSERT_TRUE(runtime.exportFile(proxyDocument, again));
    ASSERT_TRUE(DmCustomEntityRegistry::instance().registerClass(DmCustomEntityRegistry::describe<SamplePipeEntity>(
        DmProxyFlags::Erase | DmProxyFlags::Transform | DmProxyFlags::LayerChange | DmProxyFlags::ColorChange,
        QStringLiteral("ext.sample"))));
    DmDocument restored;
    ASSERT_TRUE(runtime.importFile(restored, again));
    auto* pipe = dynamic_cast<SamplePipeEntity*>(firstCustom(restored));
    ASSERT_NE(pipe, nullptr);
    EXPECT_EQ(pipe->vertices(), (std::vector<DmVector>{DmVector(0, 0), DmVector(100, 0)}));
    EXPECT_DOUBLE_EQ(pipe->diameter(), 20.0);
}

TEST_F(CustomEntityFixture, 移动过的代理存DXF后读回原实体补上变换)
{
    DxfRuntime runtime;
    ASSERT_TRUE(runtime.loaded()) << runtime.diagnostics().toStdString();
    DmProxyEntity* proxy = addProxy(DmProxyFlags::Transform);
    // 代理的数据取一根真的管道，扩展回来时读得出
    SamplePipeEntity source({DmVector(0, 0), DmVector(100, 0)}, 20.0);
    ASSERT_TRUE(proxy->assignDataBytes(source.dataBytes(), SamplePipeEntity::kVersion));
    proxy->move(DmVector(5.0, 7.0));
    QTemporaryDir dir;
    const QString file = dir.filePath(QStringLiteral("moved.dxf"));
    ASSERT_TRUE(runtime.exportFile(doc, file));

    DmDocument reread;
    ASSERT_TRUE(runtime.importFile(reread, file));
    auto* pipe = dynamic_cast<SamplePipeEntity*>(firstCustom(reread));
    ASSERT_NE(pipe, nullptr);
    EXPECT_EQ(pipe->vertices(), (std::vector<DmVector>{DmVector(5, 7), DmVector(105, 7)}));
}

// 本机 AutoCAD 2026 把 2013 版 DXF 里读成代理的实体另存成 2018 版 DXF 时写成 ACAD_PROXY_ENTITY（2026-10-05 核对）：
// AcDbProxyEntity 的 91 是类的序号加 500，70 为 1，再写一份代理图形，之后是原来的组码。这里照它改写导出的文件
TEST_F(CustomEntityFixture, AutoCAD另存成ACAD_PROXY_ENTITY后读回原实体)
{
    DxfRuntime runtime;
    ASSERT_TRUE(runtime.loaded()) << runtime.diagnostics().toStdString();
    addPipe();
    QTemporaryDir dir;
    const QString file = dir.filePath(QStringLiteral("pipe.dxf"));
    ASSERT_TRUE(runtime.exportFile(doc, file));

    GroupPairs pairs = readPairs(file);
    prependClass(pairs);  // 管道的类排第二：序号 501
    const auto [begin, end] = findEntity(pairs, "EXT_SAMPLE_PIPE");
    ASSERT_LT(begin, pairs.size());
    const std::size_t data = findSubclass(pairs, begin, end, "YiCadCustomEntity");
    ASSERT_LT(data, end);
    const GroupPairs graphics = graphicsPairs(pairs, begin, data);
    ASSERT_FALSE(graphics.empty());
    GroupPairs proxy{{100, "AcDbProxyEntity"}, {90, "498"}, {91, "501"}, {71, "31"}, {97, "40"}, {70, "1"}};
    proxy.insert(proxy.end(), graphics.begin(), graphics.end());
    proxy.insert(proxy.end(), pairs.begin() + static_cast<std::ptrdiff_t>(data),
                 pairs.begin() + static_cast<std::ptrdiff_t>(end));
    replaceTail(pairs, data, end, proxy);
    pairs[begin].second = "ACAD_PROXY_ENTITY";
    const QString resaved = dir.filePath(QStringLiteral("resaved.dxf"));
    ASSERT_TRUE(writePairs(resaved, pairs));

    DmDocument reread;
    ASSERT_TRUE(runtime.importFile(reread, resaved));
    auto* pipe = dynamic_cast<SamplePipeEntity*>(firstCustom(reread));
    ASSERT_NE(pipe, nullptr) << "按组码 91 找到类，读回原实体";
    EXPECT_EQ(pipe->vertices(), (std::vector<DmVector>{DmVector(0, 0), DmVector(100, 0)}));
    EXPECT_DOUBLE_EQ(pipe->diameter(), 20.0);

    // 类不在：读成代理，类名与数据保留，另存时写回自己的记录名
    ASSERT_TRUE(DmCustomEntityRegistry::instance().unregisterClass(QStringLiteral("ext.sample.Pipe")));
    DmDocument proxyDocument;
    ASSERT_TRUE(runtime.importFile(proxyDocument, resaved));
    auto* proxyEntity = dynamic_cast<DmProxyEntity*>(firstCustom(proxyDocument));
    ASSERT_NE(proxyEntity, nullptr);
    EXPECT_EQ(proxyEntity->className(), QStringLiteral("ext.sample.Pipe"));
    EXPECT_EQ(proxyEntity->dataBytes(), SamplePipeEntity({DmVector(0, 0), DmVector(100, 0)}, 20.0).dataBytes());
    EXPECT_NEAR(proxyEntity->getMax().x, 110.0, 1e-6) << "按代理图形显示";
    const QString again = dir.filePath(QStringLiteral("again.dxf"));
    ASSERT_TRUE(runtime.exportFile(proxyDocument, again));
    const QByteArray text = readAll(again);
    EXPECT_TRUE(text.contains("EXT_SAMPLE_PIPE"));
    EXPECT_FALSE(text.contains("ACAD_PROXY_ENTITY"));
    ASSERT_TRUE(DmCustomEntityRegistry::instance().registerClass(DmCustomEntityRegistry::describe<SamplePipeEntity>(
        DmProxyFlags::Erase | DmProxyFlags::Transform | DmProxyFlags::LayerChange | DmProxyFlags::ColorChange,
        QStringLiteral("ext.sample"))));
}

TEST_F(CustomEntityFixture, 别的程序的自定义实体读成代理另存时原样写回)
{
    DxfRuntime runtime;
    ASSERT_TRUE(runtime.loaded()) << runtime.diagnostics().toStdString();
    addPipe();
    QTemporaryDir dir;
    const QString file = dir.filePath(QStringLiteral("pipe.dxf"));
    ASSERT_TRUE(runtime.exportFile(doc, file));

    // 改成别的程序的类：自己的子类段、扩展数据，以及指向别的对象的反应器
    GroupPairs pairs = readPairs(file);
    renameClassToAcme(pairs, 3);  // 删除、变换
    const auto [begin, end] = findEntity(pairs, "EXT_SAMPLE_PIPE");
    ASSERT_LT(begin, pairs.size());
    const std::size_t data = findSubclass(pairs, begin, end, "YiCadCustomEntity");
    ASSERT_LT(data, end);
    const GroupPairs acme{{100, "AcmeThingData"}, {10, "1.5"}, {20, "2.5"}, {1, "hello"},
                          {1001, "ACME"}, {1000, "extended"}, {1070, "7"}};
    replaceTail(pairs, data, end, acme);
    const GroupPairs reactors{{102, "{ACAD_REACTORS"}, {330, "ABC"}, {102, "}"}};
    pairs.insert(pairs.begin() + static_cast<std::ptrdiff_t>(begin) + 2, reactors.begin(), reactors.end());
    pairs[begin].second = "ACME_THING";
    const QString foreign = dir.filePath(QStringLiteral("acme.dxf"));
    ASSERT_TRUE(writePairs(foreign, pairs));

    DmDocument document;
    ASSERT_TRUE(runtime.importFile(document, foreign));
    auto* proxy = dynamic_cast<DmProxyEntity*>(firstCustom(document));
    ASSERT_NE(proxy, nullptr) << "读成代理";
    EXPECT_EQ(proxy->className(), QStringLiteral("AcmeThing"));
    EXPECT_EQ(proxy->proxyFlags(), DmProxyFlags::Erase | DmProxyFlags::Transform);
    EXPECT_NEAR(proxy->getMin().x, -10.0, 1e-6) << "按代理图形显示";
    EXPECT_NEAR(proxy->getMax().x, 110.0, 1e-6);

    // 常规编辑：允许变换的代理移动后另存，子类段与扩展数据原样写回，代理图形是移动后的
    proxy->move(DmVector(0.0, 50.0));
    const QString saved = dir.filePath(QStringLiteral("saved.dxf"));
    ASSERT_TRUE(runtime.exportFile(document, saved));
    const GroupPairs out = readPairs(saved);
    const auto [outBegin, outEnd] = findEntity(out, "ACME_THING");
    ASSERT_LT(outBegin, out.size()) << "记录名原样";
    const std::size_t subclass = findSubclass(out, outBegin, outEnd, "AcmeThingData");
    ASSERT_LT(subclass, outEnd);
    EXPECT_EQ(GroupPairs(out.begin() + static_cast<std::ptrdiff_t>(subclass),
                         out.begin() + static_cast<std::ptrdiff_t>(outEnd)),
              acme)
        << "子类段与扩展数据原样";
    EXPECT_TRUE(pick(out, outBegin, outEnd, {102}).empty()) << "反应器指向没保留的对象，不写";
    const auto appId = findRecord(out, "APPID");
    bool acmeRegistered = false;
    for (auto at = appId; at.first < out.size(); at = findRecord(out, "APPID", at.second))
    {
        acmeRegistered = acmeRegistered || pick(out, at.first, at.second, {2}) == GroupPairs{{2, "ACME"}};
    }
    EXPECT_TRUE(acmeRegistered) << "扩展数据的应用名登记进 APPID 表";
    const auto [cls, clsEnd] = findRecord(out, "CLASS");
    EXPECT_EQ(pick(out, cls, clsEnd, {1, 2, 3, 90}),
              (GroupPairs{{1, "ACME_THING"}, {2, "AcmeThing"}, {3, "ACME"}, {90, "3"}}));

    DmDocument reread;
    ASSERT_TRUE(runtime.importFile(reread, saved));
    auto* again = dynamic_cast<DmProxyEntity*>(firstCustom(reread));
    ASSERT_NE(again, nullptr);
    EXPECT_EQ(again->className(), QStringLiteral("AcmeThing"));
    EXPECT_NEAR(again->getMin().y, 40.0, 1e-6) << "代理图形是移动后的";
    const QString twice = dir.filePath(QStringLiteral("twice.dxf"));
    ASSERT_TRUE(runtime.exportFile(reread, twice));
    const GroupPairs second = readPairs(twice);
    const auto [secondBegin, secondEnd] = findEntity(second, "ACME_THING");
    const std::size_t secondSubclass = findSubclass(second, secondBegin, secondEnd, "AcmeThingData");
    EXPECT_EQ(GroupPairs(second.begin() + static_cast<std::ptrdiff_t>(secondSubclass),
                         second.begin() + static_cast<std::ptrdiff_t>(secondEnd)),
              acme)
        << "再存一次仍原样";
}

// 别的程序的实体经 DWG 转来的 ACAD_PROXY_ENTITY：原数据是 DWG 二进制（70 为 0）。AcDbProxyEntity 照本机 AutoCAD 2026
// 另存 2018 版 DXF 的写法（2026-10-05 核对：71、97、70、代理图形只写在这里、162、161、94）。
// 写法随 DXF 版本变、没有真实样本核对 2013 版的写法，另存 DXF 时写成图形（DxfCustomEntity.h）
TEST_F(CustomEntityFixture, 原数据是DWG格式的ACAD_PROXY_ENTITY读成代理另存DXF时写成图形)
{
    DxfRuntime runtime;
    ASSERT_TRUE(runtime.loaded()) << runtime.diagnostics().toStdString();
    addPipe();
    QTemporaryDir dir;
    const QString file = dir.filePath(QStringLiteral("pipe.dxf"));
    ASSERT_TRUE(runtime.exportFile(doc, file));

    GroupPairs pairs = readPairs(file);
    renameClassToAcme(pairs, 1);
    prependClass(pairs);  // ACME_THING 的序号 501
    const auto [begin, end] = findEntity(pairs, "EXT_SAMPLE_PIPE");
    ASSERT_LT(begin, pairs.size());
    const std::size_t data = findSubclass(pairs, begin, end, "YiCadCustomEntity");
    ASSERT_LT(data, end);
    const GroupPairs graphics = graphicsPairs(pairs, begin, data);
    ASSERT_FALSE(graphics.empty());
    GroupPairs proxyData{{100, "AcDbProxyEntity"}, {90, "498"}, {91, "501"}, {71, "31"}, {97, "2147483646"}, {70, "0"}};
    proxyData.insert(proxyData.end(), graphics.begin(), graphics.end());
    const GroupPairs tail{{162, "0"}, {161, "0"}, {94, "0"}};
    proxyData.insert(proxyData.end(), tail.begin(), tail.end());
    replaceTail(pairs, data, end, proxyData);
    // 公共属性里不再写代理图形
    std::size_t graphicsBegin = begin;
    while (pairs[graphicsBegin].first != 160)
    {
        ++graphicsBegin;
    }
    pairs.erase(pairs.begin() + static_cast<std::ptrdiff_t>(graphicsBegin),
                pairs.begin() + static_cast<std::ptrdiff_t>(graphicsBegin + graphics.size()));
    pairs[begin].second = "ACAD_PROXY_ENTITY";
    const QString foreign = dir.filePath(QStringLiteral("proxy.dxf"));
    ASSERT_TRUE(writePairs(foreign, pairs));

    DmDocument document;
    ASSERT_TRUE(runtime.importFile(document, foreign));
    auto* proxy = dynamic_cast<DmProxyEntity*>(firstCustom(document));
    ASSERT_NE(proxy, nullptr) << "按组码 91 找到类，读成代理";
    EXPECT_EQ(proxy->className(), QStringLiteral("AcmeThing"));
    EXPECT_EQ(proxy->proxyFlags(), DmProxyFlags::Erase);
    EXPECT_NEAR(proxy->getMin().x, -10.0, 1e-6) << "按 AcDbProxyEntity 里的代理图形显示";
    EXPECT_NEAR(proxy->getMax().x, 110.0, 1e-6);

    const QString saved = dir.filePath(QStringLiteral("saved.dxf"));
    ASSERT_TRUE(runtime.exportFile(document, saved));
    const GroupPairs out = readPairs(saved);
    EXPECT_EQ(findEntity(out, "ACAD_PROXY_ENTITY").first, out.size());
    EXPECT_EQ(findRecord(out, "CLASS").first, out.size()) << "不登记类";
    int arcs = 0;
    for (auto at = findEntity(out, "ARC"); at.first < out.size(); at = findRecord(out, "ARC", at.second))
    {
        ++arcs;
    }
    EXPECT_EQ(arcs, 2) << "写成图形：两端的半圆";
    bool told = false;
    for (const QString& message : runtime.messages())
    {
        told = told || message.contains(QStringLiteral("写成了图形"));
    }
    EXPECT_TRUE(told) << "告诉用户";
}
