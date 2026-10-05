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

/// @file test_render_custom.cpp
/// @brief 自定义实体与代理实体的出图（RENDER_PLAN.md 第 7、8 阶段）
///
/// 渲染层不按实体类型分支：自定义实体只要实现 worldDraw 就能画；扩展不在时读成的代理实体按存下的代理图形画，
/// 与原实体逐像素相同；允许变换的代理移动后与同样移动的原实体相同。
/// 插件实体（第 8 阶段，demo 插件的管道）同样：按插件的 worldDraw 画，与它的代理逐像素相同，插件改数据后画面跟着变。

#include <gtest/gtest.h>

#include <functional>
#include <vector>

#include <QImage>

#include "DmDocument.h"
#include "DmLayerTable.h"
#include "DmPluginEntity.h"
#include "DmProxyEntity.h"
#include "EntityTable.h"
#include "GuiDocumentView.h"
#include "RenderHarness.h"
#include "Transaction.h"
#include "support/DxfTestRuntime.h"
#include "support/SamplePipeEntity.h"

namespace
{
using yicad_test::RenderRequest;
using yicad_test::RenderScene;

void inTransaction(DmDocument& document, const std::function<void()>& change)
{
    Transaction t("test", &document);
    t.start();
    change();
    t.commit();
}

/// @brief 读一张参考图纸后删掉它的全部实体，取景到 (-20,-40)-(140,80)
struct EmptyScene
{
    RenderScene scene{[] {
        RenderRequest r;
        r.drawing = QStringLiteral("colors.dxf");
        return r;
    }()};

    EmptyScene()
    {
        DmDocument& document = scene.document();
        std::vector<DmEntity*> all;
        for (DmEntity* e : *document.getEntityTable())
        {
            all.push_back(e);
        }
        inTransaction(document, [&]() {
            for (DmEntity* e : all)
            {
                document.getEntityTable()->remove(e);
            }
        });
        scene.view().setView(DmVector(60.0, 20.0), 0.25);
    }

    void add(DmEntity* entity)
    {
        inTransaction(scene.document(), [&]() { scene.document().getEntityTable()->add(entity); });
    }

    void remove(DmEntity* entity)
    {
        inTransaction(scene.document(), [&]() { scene.document().getEntityTable()->remove(entity); });
    }

    /// @brief 一段管道：绿色（箭头自己是红色）
    SamplePipeEntity* makePipe(const std::vector<DmVector>& vertices)
    {
        auto* pipe = new SamplePipeEntity(vertices, 12.0);
        pipe->setDocument(&scene.document());
        pipe->setPen(DmPen(DmColor(0, 255, 0), DM::Width00, scene.document().getLineTypeTable()->getLineTypeByLayer()));
        pipe->update();
        return pipe;
    }

    /// @brief 与 pipe 显示相同的代理：同样的公共属性与代理图形
    DmProxyEntity* makeProxy(const DmCustomEntity& pipe, DmProxyFlags flags)
    {
        auto* proxy = new DmProxyEntity(pipe.className(), flags);
        proxy->setDocument(&scene.document());
        proxy->setLayer(pipe.getLayer(false));
        proxy->setPen(pipe.getPen(false));
        proxy->setProxyGraphics(pipe.proxyGraphics());
        proxy->update();
        return proxy;
    }
};

/// @brief 执行 demo 插件的命令（文档作为当前文档）
bool runDemoCommand(DmDocument& document, const char* command)
{
    yicad_test::DxfRuntime* runtime = yicad_test::pluginRuntime();
    return runtime != nullptr && runtime->runCommand(document, QStringLiteral("com.yicad.demo"), QString::fromLatin1(command));
}

/// @brief 文档里的插件实体
DmPluginEntity* firstPluginEntity(DmDocument& document)
{
    for (DmEntity* e : *document.getEntityTable())
    {
        if (auto* pipe = dynamic_cast<DmPluginEntity*>(e))
        {
            return pipe;
        }
    }
    return nullptr;
}

/// @brief 图里不是背景色的像素数
int drawnPixels(const QImage& image)
{
    const QRgb background = image.pixel(0, 0);
    int count = 0;
    for (int y = 0; y < image.height(); ++y)
    {
        for (int x = 0; x < image.width(); ++x)
        {
            count += image.pixel(x, y) != background ? 1 : 0;
        }
    }
    return count;
}
}  // namespace

TEST(RenderCustomTest, 代理按代理图形画与原实体逐像素相同)
{
    SamplePipeRegistration registration;
    EmptyScene s;
    ASSERT_TRUE(s.scene.error().isEmpty()) << s.scene.error().toStdString();
    const QImage empty = s.scene.grab();

    SamplePipeEntity* first = s.makePipe({DmVector(0, 0), DmVector(100, 0), DmVector(120, 40)});
    SamplePipeEntity* second = s.makePipe({DmVector(10, 60), DmVector(90, 30)});
    s.add(first);
    s.add(second);
    const QImage original = s.scene.grab();
    EXPECT_GT(drawnPixels(original), drawnPixels(empty) + 1000) << "管道画出来了";

    DmProxyEntity* firstProxy = s.makeProxy(*first, DmProxyFlags::None);
    DmProxyEntity* secondProxy = s.makeProxy(*second, DmProxyFlags::None);
    s.remove(first);
    s.remove(second);
    s.add(firstProxy);
    s.add(secondProxy);
    yicad_test::expectIdenticalImage(QStringLiteral("custom_proxy"), original, s.scene.grab());
}

TEST(RenderCustomTest, 允许变换的代理移动后与同样移动的原实体相同)
{
    SamplePipeRegistration registration;
    EmptyScene s;
    ASSERT_TRUE(s.scene.error().isEmpty()) << s.scene.error().toStdString();

    SamplePipeEntity* pipe = s.makePipe({DmVector(0, 0), DmVector(100, 0)});
    DmProxyEntity* proxy = s.makeProxy(*pipe, DmProxyFlags::Transform);
    s.add(pipe);
    inTransaction(s.scene.document(), [&]() {
        s.scene.document().getEntityTable()->startModify(pipe);
        pipe->rotate(DmVector(50, 0), DmVector(0.5));
        pipe->move(DmVector(0, 20));
    });
    const QImage moved = s.scene.grab();

    s.remove(pipe);
    s.add(proxy);
    inTransaction(s.scene.document(), [&]() {
        s.scene.document().getEntityTable()->startModify(proxy);
        proxy->rotate(DmVector(50, 0), DmVector(0.5));
        proxy->move(DmVector(0, 20));
    });
    yicad_test::expectIdenticalImage(QStringLiteral("custom_proxy_moved"), moved, s.scene.grab());
}

TEST(RenderCustomTest, 插件实体按插件的图形画代理与它逐像素相同)
{
    EmptyScene s;
    ASSERT_TRUE(s.scene.error().isEmpty()) << s.scene.error().toStdString();
    const QImage empty = s.scene.grab();

    ASSERT_TRUE(runDemoCommand(s.scene.document(), "demo.add-pipe")) << "demo 插件建管道";
    DmPluginEntity* pipe = firstPluginEntity(s.scene.document());
    ASSERT_NE(pipe, nullptr);
    const QImage drawn = s.scene.grab();
    EXPECT_GT(drawnPixels(drawn), drawnPixels(empty) + 1000) << "管道画出来了";

    DmProxyEntity* proxy = s.makeProxy(*pipe, DmProxyFlags::None);
    s.remove(pipe);
    s.add(proxy);
    yicad_test::expectIdenticalImage(QStringLiteral("plugin_proxy"), drawn, s.scene.grab());
}

TEST(RenderCustomTest, 插件命令改数据后画面跟着变)
{
    EmptyScene s;
    ASSERT_TRUE(s.scene.error().isEmpty()) << s.scene.error().toStdString();
    ASSERT_TRUE(runDemoCommand(s.scene.document(), "demo.add-pipe"));
    DmPluginEntity* pipe = firstPluginEntity(s.scene.document());
    ASSERT_NE(pipe, nullptr);
    const QImage before = s.scene.grab();

    // 管径加倍：插件在事务里换数据，图形系统按变更集重新编这个实体
    ASSERT_TRUE(runDemoCommand(s.scene.document(), "demo.pipe-grow"));
    const QImage after = s.scene.grab();
    EXPECT_GT(drawnPixels(after), drawnPixels(before) + 500) << "管子变粗了";

    // 按新数据另记一份代理图形：画面与之相同，说明画的不是旧的图形
    DmProxyEntity* proxy = s.makeProxy(*pipe, DmProxyFlags::None);
    s.remove(pipe);
    s.add(proxy);
    yicad_test::expectIdenticalImage(QStringLiteral("plugin_grown"), after, s.scene.grab());
}
