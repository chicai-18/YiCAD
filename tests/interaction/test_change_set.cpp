/// @file test_change_set.cpp
/// @brief 文档的变更集（DmChangeTracker、DmChangeSet、DmDocumentListener::entitiesChanged）的单元测试
///
/// 渲染方案第 4.1 步（doc/RENDER_PLAN.md 第 4.3.6 节）：实体表与各符号表在增、删、改时向文档登记，
/// 撤销系统在提交、撤销、重做、回滚之后补登并交出，读盘结束时交出全部重建。覆盖：
/// 没有监听者时不记录；add_direct 等不走命令的路径；事务的提交、撤销、重做与回滚；释放了的实体只给地址；
/// 块内图元的改动同时登记块定义；图层表的修改；读盘的全部重建；修订号的递增；
/// 先于 documentModified() 交出。
///
/// 事务照 test_selection_set 的做法：先 start 再改，最后 commit。

#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

#include <QTemporaryDir>

#include "CmdManager.h"
#include "DmBlock.h"
#include "DmBlockTable.h"
#include "DmChangeSet.h"
#include "DmDocument.h"
#include "DmDocumentListener.h"
#include "DmLayer.h"
#include "DmLayerTable.h"
#include "DmLine.h"
#include "DocumentCmd.h"
#include "EntityTable.h"
#include "FilterOcdIO.h"
#include "Transaction.h"

namespace
{
/// @brief 记下收到的变更集与通知的先后
struct ChangeRecorder : DmDocumentListener
{
    std::vector<DmChangeSet> changes;
    std::vector<char> order;  ///< 'c' 为 entitiesChanged，'m' 为 documentModified

    void documentModified() override { order.push_back('m'); }
    void redrawRequested() override {}
    void paintContainerChanged(DmEntityContainer*) override {}
    void entitiesChanged(const DmChangeSet& c) override
    {
        changes.push_back(c);
        order.push_back('c');
    }

    /// @brief 全部收到的变更里，改动过的实体
    std::vector<DmEntity*> touched() const
    {
        std::vector<DmEntity*> result;
        for (const DmChangeSet& c : changes)
        {
            for (const DmEntityChange& e : c.entities)
            {
                result.push_back(e.entity);
            }
        }
        return result;
    }
};

bool contains(const std::vector<DmEntity*>& list, const DmEntity* e)
{
    return std::find(list.begin(), list.end(), e) != list.end();
}

struct ChangeSetFixture : ::testing::Test
{
    DmDocument doc;
    ChangeRecorder recorder;

    ChangeSetFixture() { doc.addListener(&recorder); }
    ~ChangeSetFixture() override { doc.removeListener(&recorder); }

    DmLine* makeLine(double y)
    {
        auto* line = new DmLine(DmVector(0.0, y), DmVector(10.0, y));
        line->setDocument(&doc);
        line->setLayer(QStringLiteral("0"));
        line->calculateBorders();
        return line;
    }
};
}  // namespace

TEST(ChangeSet, 没有监听者时不记录但修订号照样递增)
{
    DmDocument doc;
    auto* line = new DmLine(DmVector(0.0, 0.0), DmVector(1.0, 0.0));
    line->setDocument(&doc);
    const std::uint64_t before = line->revision();
    ASSERT_TRUE(doc.getEntityTable()->add_direct(line));
    EXPECT_GT(line->revision(), before);
    EXPECT_FALSE(doc.changeTracker().hasPendingChanges());
}

TEST_F(ChangeSetFixture, 不走命令的添加在通知修改时交出且先于documentModified)
{
    DmLine* line = makeLine(0.0);
    ASSERT_TRUE(doc.getEntityTable()->add_direct(line));
    EXPECT_TRUE(doc.changeTracker().isPending(line));
    EXPECT_TRUE(recorder.changes.empty()) << "登记后不立即交出";

    doc.notifyDocumentModified();
    ASSERT_EQ(recorder.changes.size(), 1u);
    ASSERT_EQ(recorder.changes[0].entities.size(), 1u);
    EXPECT_EQ(recorder.changes[0].entities[0].entity, line);
    EXPECT_EQ(recorder.changes[0].entities[0].ownerBlock, nullptr) << "模型空间";
    EXPECT_EQ(recorder.order, (std::vector<char>{'c', 'm'}));
    EXPECT_FALSE(doc.changeTracker().hasPendingChanges());

    doc.notifyDocumentModified();
    EXPECT_EQ(recorder.changes.size(), 1u) << "没有变更时不调用 entitiesChanged";
}

TEST_F(ChangeSetFixture, 事务的提交撤销重做都交出改动的实体)
{
    DmLine* line = makeLine(0.0);
    {
        Transaction t("Add", &doc);
        t.start();
        doc.getEntityTable()->add(line);
        t.commit();
    }
    ASSERT_FALSE(recorder.changes.empty());
    EXPECT_TRUE(contains(recorder.touched(), line));
    EXPECT_FALSE(line->isErased());

    recorder.changes.clear();
    doc.undo();
    EXPECT_TRUE(contains(recorder.touched(), line)) << "撤销添加：实体被删除，接收方按 isErased 判断";
    EXPECT_TRUE(line->isErased());

    recorder.changes.clear();
    doc.redo();
    EXPECT_TRUE(contains(recorder.touched(), line));
    EXPECT_FALSE(line->isErased());
}

TEST_F(ChangeSetFixture, 修改与删除经事务登记修改递增修订号)
{
    DmLine* line = makeLine(0.0);
    ASSERT_TRUE(doc.getEntityTable()->add_direct(line));
    doc.flushChanges();
    recorder.changes.clear();

    const std::uint64_t before = line->revision();
    {
        Transaction t("Move", &doc);
        t.start();
        doc.getEntityTable()->startModify(line);
        line->move(DmVector(0.0, 5.0));
        t.commit();
    }
    EXPECT_TRUE(contains(recorder.touched(), line));
    EXPECT_GT(line->revision(), before);

    recorder.changes.clear();
    {
        Transaction t("Delete", &doc);
        t.start();
        doc.getEntityTable()->remove(line);
        t.commit();
    }
    EXPECT_TRUE(contains(recorder.touched(), line));
    EXPECT_TRUE(line->isErased());
}

TEST_F(ChangeSetFixture, 回滚交出被撤回的实体)
{
    DmLine* line = makeLine(0.0);
    Transaction t("Add", &doc);
    t.start();
    doc.getEntityTable()->add(line);
    t.rollback();
    EXPECT_TRUE(contains(recorder.touched(), line));
    EXPECT_TRUE(line->isErased());
}

TEST_F(ChangeSetFixture, 释放的实体只给地址且作废之前的登记)
{
    DmLine* line = makeLine(0.0);
    ASSERT_TRUE(doc.getEntityTable()->add_direct(line));
    const void* address = line;
    doc.getEntityTable()->remove_direct(line);  // 释放
    doc.flushChanges();

    ASSERT_EQ(recorder.changes.size(), 1u);
    EXPECT_TRUE(recorder.changes[0].entities.empty()) << "释放前的登记作废，不能再解引用";
    ASSERT_EQ(recorder.changes[0].destroyedEntities.size(), 1u);
    EXPECT_EQ(recorder.changes[0].destroyedEntities[0], address);
}

TEST_F(ChangeSetFixture, 块内图元的改动同时登记块定义)
{
    auto* block = new DmBlock(&doc, DmBlockData(QStringLiteral("B"), DmVector(0.0, 0.0), false));
    doc.getBlockTable()->add_direct(block);
    doc.flushChanges();
    recorder.changes.clear();

    DmLine* line = makeLine(0.0);
    ASSERT_TRUE(block->getEntityTable().add_direct(line));
    doc.flushChanges();

    ASSERT_EQ(recorder.changes.size(), 1u);
    const DmChangeSet& c = recorder.changes[0];
    ASSERT_EQ(c.entities.size(), 1u);
    EXPECT_EQ(c.entities[0].entity, line);
    EXPECT_EQ(c.entities[0].ownerBlock, block);
    EXPECT_EQ(c.blocks, (std::vector<const DmBlock*>{block}));
}

TEST_F(ChangeSetFixture, 释放块定义时作废块内图元的登记)
{
    auto* block = new DmBlock(&doc, DmBlockData(QStringLiteral("B"), DmVector(0.0, 0.0), false));
    DmLine* line = makeLine(0.0);
    ASSERT_TRUE(block->getEntityTable().add_direct(line));
    const void* address = block;
    delete block;  // 块没进块表，图元随实体表析构直接释放
    doc.flushChanges();

    ASSERT_EQ(recorder.changes.size(), 1u);
    EXPECT_TRUE(recorder.changes[0].entities.empty());
    EXPECT_TRUE(recorder.changes[0].blocks.empty());
    EXPECT_EQ(recorder.changes[0].destroyedBlocks, (std::vector<const void*>{address}));
}

TEST_F(ChangeSetFixture, 图层的增改登记图层表)
{
    {
        Transaction t("Layer", &doc);
        t.start();
        auto* layer = new DmLayer(QStringLiteral("L1"));
        layer->setDocument(&doc);
        doc.getLayerTable()->add(layer);
        t.commit();
    }
    ASSERT_FALSE(recorder.changes.empty());
    EXPECT_TRUE(recorder.changes.back().layersChanged);

    recorder.changes.clear();
    doc.undo();
    ASSERT_FALSE(recorder.changes.empty()) << "撤销时由撤销系统按命令类型补登";
    EXPECT_TRUE(recorder.changes.back().layersChanged);
}

TEST_F(ChangeSetFixture, 读盘交出全部重建而不逐个列出实体)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString file = dir.filePath(QStringLiteral("a.ycd"));
    {
        DmDocument source;
        for (int i = 0; i < 3; ++i)
        {
            auto* line = new DmLine(DmVector(0.0, i), DmVector(1.0, i));
            line->setDocument(&source);
            line->setLayer(QStringLiteral("0"));
            ASSERT_TRUE(source.getEntityTable()->add_direct(line));
        }
        FilterOcdIO filter;
        ASSERT_TRUE(filter.fileExport(source, file, QString()));
    }

    ASSERT_TRUE(doc.readNativeFile(file).ok());
    ASSERT_EQ(recorder.changes.size(), 1u);
    EXPECT_TRUE(recorder.changes[0].fullRebuild);
    EXPECT_TRUE(recorder.changes[0].entities.empty());
    EXPECT_EQ(doc.getEntityTable()->count(), 3);
}

TEST_F(ChangeSetFixture, 文档变量的修改登记)
{
    doc.setGridOn(false);
    doc.flushChanges();
    ASSERT_EQ(recorder.changes.size(), 1u);
    EXPECT_TRUE(recorder.changes[0].variablesChanged);
}

TEST_F(ChangeSetFixture, 经命令修改文档变量时提交撤销重做都登记)
{
    // 线型管理器改 LTSCALE 走 ModifyDocVariablesCmd（可撤销）；它直接改变量字典，要自己登记
    {
        Transaction t("LTSCALE", &doc);
        t.start();
        QHash<QString, DmVariable> variables;
        variables.insert(QStringLiteral("$LTSCALE"), DmVariable(2.0, 40));
        doc.getCmdManager()->addAndExecuteCmd(new ModifyDocVariablesCmd(&doc, variables));
        t.commit();
    }
    ASSERT_FALSE(recorder.changes.empty());
    EXPECT_TRUE(recorder.changes.back().variablesChanged);
    EXPECT_DOUBLE_EQ(doc.getVariableDouble(QStringLiteral("$LTSCALE"), 1.0), 2.0);

    recorder.changes.clear();
    doc.undo();
    ASSERT_FALSE(recorder.changes.empty());
    EXPECT_TRUE(recorder.changes.back().variablesChanged);
    EXPECT_DOUBLE_EQ(doc.getVariableDouble(QStringLiteral("$LTSCALE"), 1.0), 1.0);

    recorder.changes.clear();
    doc.redo();
    ASSERT_FALSE(recorder.changes.empty());
    EXPECT_TRUE(recorder.changes.back().variablesChanged);
}
