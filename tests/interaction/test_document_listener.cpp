/// @file test_document_listener.cpp
/// @brief 文档监听接口 DmDocumentListener 的单元测试
///
/// 分层重组 S3 让文档不再持有视图，改为通知任意多个监听者（doc/LAYER_RESTRUCTURE_PLAN.md
/// 7.2 节）。覆盖：注册与注销；选择集 SelectionSet 修改后监听者收到"已修改"与"重绘"（方案第 4 步
/// 改为经选择集自己的信号通知，doc/SELECTION_SET_PLAN.md）；Modification 复制
/// 不改选中状态，也不通知；块编辑进入与退出时收到绘制容器切换；画布 GuiDocumentView 关联文档时注册，
/// 换文档或析构时注销。
///
/// 实体用 EntityTable::add_direct 放进表，块编辑直接执行 BlockEditEnterCmd/BlockEditExitCmd，
/// 都不经事务：默认构造的 DmDocument 走事务会崩溃（CommandTestFixture.h 的说明）。

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "BlockEditCmd.h"
#include "DmBlock.h"
#include "DmBlockTable.h"
#include "DmClipboard.h"
#include "DmDocument.h"
#include "DmDocumentListener.h"
#include "DmEntityContainer.h"
#include "DmLine.h"
#include "EntityTable.h"
#include "GuiDocumentView.h"
#include "Modification.h"
#include "SelectionSet.h"

namespace
{
/// @brief 记录收到的通知
struct RecordingListener : DmDocumentListener
{
    int modifiedCount = 0;
    int redrawCount = 0;
    std::vector<DmEntityContainer*> containers;

    void documentModified() override { ++modifiedCount; }
    void redrawRequested() override { ++redrawCount; }
    void paintContainerChanged(DmEntityContainer* container) override { containers.push_back(container); }
};

/// @brief 一份空文档，注册了一个记录通知的监听者
struct DocumentListenerFixture : ::testing::Test
{
    DmDocument doc;
    RecordingListener listener;

    DocumentListenerFixture() { doc.addListener(&listener); }
    ~DocumentListenerFixture() override { doc.removeListener(&listener); }

    /// @brief 在 "0" 图层上加一条直线
    DmLine* addLine(const DmVector& a, const DmVector& b)
    {
        auto* line = new DmLine(a, b);
        line->setDocument(&doc);
        line->setLayer(QStringLiteral("0"));
        line->calculateBorders();
        EXPECT_TRUE(doc.getEntityTable()->add_direct(line));
        return line;
    }
};

/// @brief 数重绘次数的画布：文档的重绘通知经 redraw() 到达
class CountingView : public GuiDocumentView
{
public:
    using GuiDocumentView::GuiDocumentView;

    int redrawCount = 0;
    void redraw() override { ++redrawCount; }
};
}  // namespace

TEST_F(DocumentListenerFixture, 重新生成通知全部监听者修改并重绘)
{
    RecordingListener second;
    doc.addListener(&second);

    doc.regenerate();
    EXPECT_EQ(listener.modifiedCount, 1);
    EXPECT_EQ(listener.redrawCount, 1);
    EXPECT_EQ(second.modifiedCount, 1);
    EXPECT_EQ(second.redrawCount, 1);

    doc.removeListener(&second);
}

TEST_F(DocumentListenerFixture, 画笔与实体修改只通知已修改)
{
    doc.specifyPenModified();
    DmLine* line = addLine(DmVector(0.0, 0.0), DmVector(10.0, 0.0));
    doc.specifyModifiedEntity(line);

    EXPECT_EQ(listener.modifiedCount, 2);
    EXPECT_EQ(listener.redrawCount, 0);
}

TEST_F(DocumentListenerFixture, 重复注册只通知一次注销后不再通知)
{
    doc.addListener(&listener);
    doc.addListener(nullptr);
    doc.regenerate();
    EXPECT_EQ(listener.modifiedCount, 1);

    doc.removeListener(&listener);
    doc.regenerate();
    EXPECT_EQ(listener.modifiedCount, 1);
    EXPECT_EQ(listener.redrawCount, 1);
}

TEST_F(DocumentListenerFixture, 选择集的各种修改都通知修改并重绘)
{
    DmLine* line = addLine(DmVector(0.0, 0.0), DmVector(10.0, 0.0));
    SelectionSet selection(doc);

    selection.toggle(line);
    EXPECT_TRUE(selection.contains(line));
    EXPECT_EQ(listener.modifiedCount, 1);
    EXPECT_EQ(listener.redrawCount, 1);

    selection.clear();
    EXPECT_FALSE(selection.contains(line));
    EXPECT_EQ(listener.modifiedCount, 2);
    EXPECT_EQ(listener.redrawCount, 2);

    selection.selectWindow(DmVector(-1.0, -1.0), DmVector(11.0, 1.0));
    EXPECT_TRUE(selection.contains(line));
    EXPECT_EQ(listener.modifiedCount, 3);
    EXPECT_EQ(listener.redrawCount, 3);

    selection.selectLayer(QStringLiteral("0"), false);
    EXPECT_FALSE(selection.contains(line));
    EXPECT_EQ(listener.modifiedCount, 4);
    EXPECT_EQ(listener.redrawCount, 4);

    selection.add(line);
    EXPECT_TRUE(selection.contains(line));
    EXPECT_EQ(listener.modifiedCount, 5);
    EXPECT_EQ(listener.redrawCount, 5);

    selection.remove(line);
    EXPECT_FALSE(selection.contains(line));
    EXPECT_EQ(listener.modifiedCount, 6);
    EXPECT_EQ(listener.redrawCount, 6);

    selection.selectAll();
    EXPECT_TRUE(selection.contains(line));
    EXPECT_EQ(listener.modifiedCount, 7);
    EXPECT_EQ(listener.redrawCount, 7);
}

TEST_F(DocumentListenerFixture, Modification复制不改选中状态也不通知)
{
    // 复制接收显式的实体列表，取消选中由调用方决定（doc/SELECTION_SET_PLAN.md 3.1 节）；
    // 复制到剪贴板不改文档，所以也不通知。
    DmLine* line = addLine(DmVector(0.0, 0.0), DmVector(10.0, 0.0));
    SelectionSet selection(doc);
    selection.add(line);
    // 选中本身也通知，只数复制的
    listener.modifiedCount = 0;
    listener.redrawCount = 0;

    Modification modification(&doc);
    modification.copy({line}, DmVector(0.0, 0.0), false);

    EXPECT_EQ(DMCLIPBOARD->count(), 1u);
    EXPECT_TRUE(selection.contains(line));
    EXPECT_EQ(listener.modifiedCount, 0);
    EXPECT_EQ(listener.redrawCount, 0);

    DMCLIPBOARD->clear();
}

TEST_F(DocumentListenerFixture, 块编辑进入与退出时切换绘制容器)
{
    DmBlockData data;
    data.name = QStringLiteral("B");
    auto* block = new DmBlock(&doc, data);
    doc.getBlockTable()->add_direct(block);

    BlockEditEnterCmd enter(&doc, QStringLiteral("B"));
    enter.execute();
    EXPECT_EQ(doc.getEditingBlock(), block);
    ASSERT_EQ(listener.containers.size(), 1u);
    EXPECT_EQ(listener.containers[0], block->getEntityTable().getEntityContainer());
    // 进入后随即重新生成
    EXPECT_EQ(listener.modifiedCount, 1);
    EXPECT_EQ(listener.redrawCount, 1);

    BlockEditExitCmd exit(&doc, QStringLiteral("B"), false);
    exit.execute();
    EXPECT_EQ(doc.getEditingBlock(), nullptr);
    ASSERT_EQ(listener.containers.size(), 2u);
    EXPECT_EQ(listener.containers[1], doc.getDocumentEntityTable()->getEntityContainer());
}

TEST_F(DocumentListenerFixture, 编辑块不变时不通知绘制容器切换)
{
    doc.setEditBlock(nullptr);
    EXPECT_TRUE(listener.containers.empty());
}

TEST(DocumentViewListenerTest, 画布关联文档时注册换文档时注销)
{
    DmDocument doc;
    DmDocument other;
    CountingView view(nullptr, Qt::WindowFlags(), &doc);

    doc.regenerate();
    EXPECT_EQ(view.redrawCount, 1);

    view.setDocument(&other);
    doc.regenerate();
    EXPECT_EQ(view.redrawCount, 1);
    other.regenerate();
    EXPECT_EQ(view.redrawCount, 2);

    view.setDocument(nullptr);
    other.regenerate();
    EXPECT_EQ(view.redrawCount, 2);
}

TEST(DocumentViewListenerTest, 画布析构时从文档注销)
{
    DmDocument doc;
    RecordingListener listener;
    doc.addListener(&listener);

    auto view = std::make_unique<CountingView>(nullptr, Qt::WindowFlags(), &doc);
    view.reset();

    // 画布漏注销时这里会调用已释放的对象：Debug 堆把释放的内存填成 0xDD，必然崩溃；
    // Release 下不保证能暴露
    doc.regenerate();
    EXPECT_EQ(listener.redrawCount, 1);

    doc.removeListener(&listener);
}
