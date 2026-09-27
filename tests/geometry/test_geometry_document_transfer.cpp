/// @file test_geometry_document_transfer.cpp
/// @brief 实体在文档之间改归（DmEntity::transferTo、DmDocumentTransfer）与剪贴板的回归测试
///
/// 复制到剪贴板时，实体连同它引用的图层、线型、文字样式、标注样式与块改归剪贴板自己的文档，
/// 来源图纸关闭后剪贴板照样可用；粘贴时同名条目用目标文档的、不改动它，目标文档没有的才复制，
/// 且只复制实体用到的。样本用 tests/support/OcdSampleDocument.h 的 buildSample：全部一等实体、
/// 自定义线型、三个图层、自定义文字样式与标注样式、带属性定义的块、带属性值的块引用。

#include <gtest/gtest.h>

#include <map>
#include <memory>
#include <vector>

#include "CmdManager.h"
#include "DmClipboard.h"
#include "DmDocumentTransfer.h"
#include "Modification.h"
#include "Transaction.h"
#include "support/OcdSampleDocument.h"

using namespace yicad_test;

namespace
{
const QString kUnusedLayer = QStringLiteral("闲置");
const QString kUnusedTextStyle = QStringLiteral("闲置样式");

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

/// @brief 构造样本，另加一个没有实体用到的图层与文字样式，把全部实体复制到剪贴板，然后关闭样本
void copySampleToClipboard()
{
    DmDocument source;
    ASSERT_NO_FATAL_FAILURE(buildSample(source));
    auto* unusedLayer = new DmLayer();
    unusedLayer->setDocument(&source);
    unusedLayer->setData(
        DmLayerData(kUnusedLayer, DmPen(DmColor(0, 0, 0), DM::Width05, DmLineTypeTable::Continuous), false, false));
    ASSERT_TRUE(source.getLayerTable()->add_direct(unusedLayer));
    auto* unusedStyle = new DmTextStyle(*source.getTextStyleTable()->find(QStringLiteral("Standard")), kUnusedTextStyle);
    unusedStyle->setDocument(&source);
    ASSERT_TRUE(source.getTextStyleTable()->add_direct(unusedStyle));

    // 复制接收显式的实体列表，隐藏线图层冻结、锁定也照样复制
    std::vector<DmEntity*> all;
    for (DmEntity* e : *source.getEntityTable())
    {
        all.push_back(e);
    }
    Modification(&source).copy(all, DmVector(0.0, 0.0), false);
}

/// @brief 把剪贴板里的实体逐个克隆、改归目标文档，按 add 放进目标文档的实体表
///        （AddWithUndo 须在事务里调用，与 EditPasteCommand::commitPaste 相同）
void pasteInto(DmDocument& target, DmDocumentTransfer::Missing missing)
{
    DmDocumentTransfer transfer(target, missing);
    for (DmEntity* src : *DMCLIPBOARD->getDocument()->getEntityTable())
    {
        DmEntity* e = src->clone();
        e->resetId();
        e->transferTo(transfer);
        if (missing == DmDocumentTransfer::Missing::AddWithUndo)
        {
            target.getEntityTable()->add(e);
        }
        else
        {
            EXPECT_TRUE(target.getEntityTable()->add_direct(e));
        }
    }
}

/// @brief 用例结束时清空剪贴板，剪贴板是全局单例
struct DocumentTransfer : ::testing::Test
{
    void TearDown() override { DMCLIPBOARD->clear(); }
};
}  // namespace

TEST_F(DocumentTransfer, 复制到剪贴板的实体与来源图纸无关)
{
    ASSERT_NO_FATAL_FAILURE(copySampleToClipboard());  // 来源图纸已关闭
    DmDocument* clip = DMCLIPBOARD->getDocument();
    const EntityTable& table = *clip->getEntityTable();
    EXPECT_EQ(countByType(table), kModelSpaceCounts);
    for (DmEntity* e : table)
    {
        SCOPED_TRACE(static_cast<int>(e->getEntityType()));
        EXPECT_EQ(e->getDocument(), clip);
        if (DmLayer* layer = e->getLayer(false))
        {
            EXPECT_EQ(clip->getLayerTable()->find(layer->getName()), layer) << "图层不是剪贴板文档的";
        }
    }

    // 只复制实体用到的
    EXPECT_NE(clip->getLayerTable()->find(kLayerOutline), nullptr);
    EXPECT_NE(clip->getLayerTable()->find(kLayerHidden), nullptr);
    EXPECT_EQ(clip->getLayerTable()->find(kUnusedLayer), nullptr);
    EXPECT_EQ(clip->getTextStyleTable()->find(kUnusedTextStyle), nullptr);

    // 线型：直线的画笔与轮廓图层的画笔都用自定义线型
    DmLineType* lineType = clip->getLineTypeTable()->find(kLineTypeName);
    ASSERT_NE(lineType, nullptr);
    auto* line = first<DmLine>(table, DM::EntityLine);
    ASSERT_NE(line, nullptr);
    EXPECT_EQ(line->getPen(false).getLineType(), lineType);
    EXPECT_EQ(clip->getLayerTable()->find(kLayerOutline)->getPen().getLineType(), lineType);

    // 文字样式与标注样式
    DmTextStyle* textStyle = clip->getTextStyleTable()->find(kTextStyleName);
    ASSERT_NE(textStyle, nullptr);
    auto* text = first<DmText>(table, DM::EntityText);
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->getStyle(), textStyle);
    auto* mtext = first<DmMText>(table, DM::EntityMText);
    ASSERT_NE(mtext, nullptr);
    EXPECT_EQ(mtext->getTextStyle(), textStyle);
    DmDimensionStyle* dimStyle = clip->getDimStyleTable()->find(kDimStyleName);
    ASSERT_NE(dimStyle, nullptr);
    EXPECT_EQ(dimStyle->getDataConstRef().textStyle(), textStyle);
    auto* dim = first<DmDimLinear>(table, DM::EntityDimLinear);
    ASSERT_NE(dim, nullptr);
    EXPECT_EQ(dim->getStyle(), dimStyle);
    auto* leader = first<DmLeader>(table, DM::EntityDimLeader);
    ASSERT_NE(leader, nullptr);
    EXPECT_EQ(leader->getData().pStyle, dimStyle);

    // 块定义复制进剪贴板，块内图元属于剪贴板文档
    DmBlock* block = clip->getBlockTable()->find(kBlockName);
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->getDocument(), clip);
    EXPECT_EQ(block->getEntityTable().count(), 3);
    for (DmEntity* e : block->getEntityTable())
    {
        EXPECT_EQ(e->getDocument(), clip);
        ASSERT_NE(e->getLayer(false), nullptr);
        EXPECT_EQ(clip->getLayerTable()->find(e->getLayer(false)->getName()), e->getLayer(false));
    }
    const std::list<DmAttributeDefinition*> definitions = block->getAttributeDefinitions();
    ASSERT_EQ(definitions.size(), 1u);
    EXPECT_EQ(definitions.front()->getStyle(), textStyle);

    auto* insert = first<DmBlockReference>(table, DM::EntityBlockReference);
    ASSERT_NE(insert, nullptr);
    EXPECT_EQ(insert->getBlockForInsert(), block);
    const std::list<DmAttribute*> attributes = insert->getAttributes();
    ASSERT_EQ(attributes.size(), 1u);
    EXPECT_EQ(attributes.front()->getDocument(), clip);
    EXPECT_EQ(attributes.front()->getStyle(), textStyle);
    EXPECT_EQ(attributes.front()->getLayer(false), clip->getLayerTable()->find(QStringLiteral("0")));
}

TEST_F(DocumentTransfer, 粘贴时同名条目用目标文档的缺的才复制)
{
    ASSERT_NO_FATAL_FAILURE(copySampleToClipboard());

    // 目标文档已有同名的轮廓图层，颜色、线型与样本不同
    DmDocument target;
    auto* outline = new DmLayer();
    outline->setDocument(&target);
    outline->setData(
        DmLayerData(kLayerOutline, DmPen(DmColor(0, 0, 255), DM::Width05, DmLineTypeTable::Continuous), false, false));
    ASSERT_TRUE(target.getLayerTable()->add_direct(outline));

    ASSERT_NO_FATAL_FAILURE(pasteInto(target, DmDocumentTransfer::Missing::AddDirect));
    const EntityTable& table = *target.getEntityTable();
    EXPECT_EQ(countByType(table), kModelSpaceCounts);

    // 同名图层用目标文档的，不改动它
    EXPECT_EQ(target.getLayerTable()->find(kLayerOutline), outline);
    EXPECT_EQ(outline->getPen().getColor().blue(), 255);
    EXPECT_EQ(outline->getPen().getLineType(), DmLineTypeTable::Continuous);
    auto* circle = first<DmCircle>(table, DM::EntityCircle);
    ASSERT_NE(circle, nullptr);
    EXPECT_EQ(circle->getLayer(false), outline);

    // 没有的复制进来：隐藏线图层（颜色、线宽照样本）、自定义线型、文字样式、标注样式、块
    DmLayer* hidden = target.getLayerTable()->find(kLayerHidden);
    ASSERT_NE(hidden, nullptr);
    EXPECT_EQ(hidden->getPen().getColor().green(), 128);
    EXPECT_EQ(hidden->getPen().getColor().blue(), 255);
    EXPECT_EQ(hidden->getPen().getWidth(), DM::Width05);
    auto* ray = first<DmRay>(table, DM::EntityRay);
    ASSERT_NE(ray, nullptr);
    EXPECT_EQ(ray->getLayer(false), hidden);

    DmLineType* lineType = target.getLineTypeTable()->find(kLineTypeName);
    ASSERT_NE(lineType, nullptr);
    EXPECT_EQ(first<DmLine>(table, DM::EntityLine)->getPen(false).getLineType(), lineType);

    DmTextStyle* textStyle = target.getTextStyleTable()->find(kTextStyleName);
    ASSERT_NE(textStyle, nullptr);
    EXPECT_NEAR(textStyle->getData().defaultHeight, 3.5, 1e-9);
    EXPECT_EQ(first<DmText>(table, DM::EntityText)->getStyle(), textStyle);
    DmDimensionStyle* dimStyle = target.getDimStyleTable()->find(kDimStyleName);
    ASSERT_NE(dimStyle, nullptr);
    EXPECT_EQ(dimStyle->getDataConstRef().textStyle(), textStyle);
    EXPECT_EQ(first<DmDimLinear>(table, DM::EntityDimLinear)->getStyle(), dimStyle);

    DmBlock* block = target.getBlockTable()->find(kBlockName);
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->getDocument(), &target);
    for (DmEntity* e : block->getEntityTable())
    {
        EXPECT_EQ(e->getDocument(), &target);
    }
    EXPECT_EQ(first<DmBlockReference>(table, DM::EntityBlockReference)->getBlockForInsert(), block);

    // 只复制实体用到的：图层只有 "0"、轮廓、隐藏线；新文档自带的条目不重复
    EXPECT_EQ(target.getLayerTable()->count(), 3u);
    EXPECT_EQ(target.getLayerTable()->find(kUnusedLayer), nullptr);
    EXPECT_EQ(target.getTextStyleTable()->find(kUnusedTextStyle), nullptr);
    int standards = 0;
    for (auto it = target.getTextStyleTable()->begin(); it != target.getTextStyleTable()->end(); ++it)
    {
        standards += (*it)->getName() == QStringLiteral("Standard") ? 1 : 0;
    }
    EXPECT_EQ(standards, 1);
}

TEST_F(DocumentTransfer, 粘贴预览不改动目标文档)
{
    // 预览按 KeepSource 改归：同名条目用目标文档的，没有的仍用剪贴板里的，目标文档的表不变
    ASSERT_NO_FATAL_FAILURE(copySampleToClipboard());
    DmDocument* clip = DMCLIPBOARD->getDocument();
    DmDocument target;
    const unsigned int layers = target.getLayerTable()->count();
    const unsigned int blocks = target.getBlockTable()->count();
    const unsigned int dimStyles = target.getDimStyleTable()->count();

    DmDocumentTransfer transfer(target, DmDocumentTransfer::Missing::KeepSource);
    std::vector<std::unique_ptr<DmEntity>> previews;
    for (DmEntity* src : *clip->getEntityTable())
    {
        previews.emplace_back(src->clone());
        previews.back()->transferTo(transfer);
    }
    auto find = [&previews](DM::EntityType type) -> DmEntity* {
        for (const auto& e : previews)
        {
            if (e->getEntityType() == type)
            {
                return e.get();
            }
        }
        return nullptr;
    };

    EXPECT_EQ(target.getLayerTable()->count(), layers);
    EXPECT_EQ(target.getBlockTable()->count(), blocks);
    EXPECT_EQ(target.getDimStyleTable()->count(), dimStyles);
    EXPECT_EQ(target.getTextStyleTable()->find(kTextStyleName), nullptr);
    EXPECT_EQ(target.getLineTypeTable()->find(kLineTypeName), nullptr);

    for (const auto& e : previews)
    {
        EXPECT_EQ(e->getDocument(), &target);
    }
    DmEntity* point = find(DM::EntityPoint);
    ASSERT_NE(point, nullptr);
    EXPECT_EQ(point->getLayer(false), target.getLayerTable()->find(QStringLiteral("0")));
    DmEntity* ray = find(DM::EntityRay);
    ASSERT_NE(ray, nullptr);
    EXPECT_EQ(ray->getLayer(false), clip->getLayerTable()->find(kLayerHidden));
    auto* text = static_cast<DmText*>(find(DM::EntityText));
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->getStyle(), clip->getTextStyleTable()->find(kTextStyleName));
    auto* insert = static_cast<DmBlockReference*>(find(DM::EntityBlockReference));
    ASSERT_NE(insert, nullptr);
    EXPECT_EQ(insert->getBlockForInsert(), clip->getBlockTable()->find(kBlockName));
}

TEST_F(DocumentTransfer, 粘贴复制进来的条目随事务撤销)
{
    ASSERT_NO_FATAL_FAILURE(copySampleToClipboard());
    DmDocument target;
    Transaction t("Paste", &target);
    t.start();
    ASSERT_NO_FATAL_FAILURE(pasteInto(target, DmDocumentTransfer::Missing::AddWithUndo));
    t.commit();

    EXPECT_EQ(countByType(*target.getEntityTable()), kModelSpaceCounts);
    EXPECT_NE(target.getLayerTable()->find(kLayerHidden), nullptr);
    EXPECT_NE(target.getLineTypeTable()->find(kLineTypeName), nullptr);
    EXPECT_NE(target.getTextStyleTable()->find(kTextStyleName), nullptr);
    EXPECT_NE(target.getDimStyleTable()->find(kDimStyleName), nullptr);
    EXPECT_NE(target.getBlockTable()->find(kBlockName), nullptr);

    target.getCmdManager()->undo();
    EXPECT_EQ(target.getEntityTable()->count(), 0);
    EXPECT_EQ(target.getLayerTable()->find(kLayerOutline), nullptr);
    EXPECT_EQ(target.getLayerTable()->find(kLayerHidden), nullptr);
    EXPECT_EQ(target.getLineTypeTable()->find(kLineTypeName), nullptr);
    EXPECT_EQ(target.getTextStyleTable()->find(kTextStyleName), nullptr);
    EXPECT_EQ(target.getDimStyleTable()->find(kDimStyleName), nullptr);
    EXPECT_EQ(target.getBlockTable()->find(kBlockName), nullptr);
}
