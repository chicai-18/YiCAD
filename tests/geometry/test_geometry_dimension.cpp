/// @file test_geometry_dimension.cpp
/// @brief 标注与引线从自己所属的文档取箭头块（分层重组 S4a）
///
/// 标注与引线更新时生成箭头：按标注样式里的箭头名，到文档标注样式表的箭头块表
/// （DmDimensionStyleTable::getArrowBlocks）里找块，生成指向这张表的块参照。
/// 线性、角度、半径、直径标注与引线原先从宿主的"当前活动文档"取这张表
/// （GUIDIALOGFACTORY->requestActiveDocument()，doc/LAYER_RESTRUCTURE_PLAN.md 1.2 节 L3），
/// 不是当前文档里的标注会引用别的文档的箭头块；对齐标注一直取自己的文档。S4a 统一改为
/// 取自己的文档。
///
/// 用例里宿主的当前文档故意设成另一份文档，两份文档各有自己的标注样式；箭头块参照的
/// 来源表必须是标注自己文档的那张。

#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

#include "DmBlockReference.h"
#include "DmBlockTable.h"
#include "DmDimAligned.h"
#include "DmDimAngular.h"
#include "DmDimDiametric.h"
#include "DmDimLinear.h"
#include "DmDimRadial.h"
#include "DmDimensionStyle.h"
#include "DmDimensionStyleTable.h"
#include "DmDocument.h"
#include "DmLeader.h"
#include "DmTextStyleTable.h"
#include "DmVector.h"
#include "EntityTable.h"
#include "GuiDialogFactory.h"
#include "GuiDialogFactoryAdapter.h"

namespace
{
/// @brief 宿主的当前文档由用例指定
class ActiveDocumentHost : public GuiDialogFactoryAdapter
{
public:
    DmDocument* active = nullptr;

    DmDocument* requestActiveDocument() override { return active; }
};

struct DimensionDocumentFixture : ::testing::Test
{
    ActiveDocumentHost host;
    DmDocument own;    ///< 标注所在的文档
    DmDocument other;  ///< 宿主的当前文档

    DimensionDocumentFixture()
    {
        GuiDialogFactory::instance()->setFactoryObject(&host);
        host.active = &other;
    }
    ~DimensionDocumentFixture() override { GuiDialogFactory::instance()->setFactoryObject(nullptr); }

    /// @brief 在文档里建一个标注样式并设为当前，两份文档的样式互不相同
    DmDimensionStyle* addStyle(DmDocument& doc, const QString& name, double arrowSize)
    {
        auto* style = new DmDimensionStyle(name, doc.getTextStyleTable()->getActive());
        style->setDocument(&doc);
        style->getDataRef().setArrowSize(arrowSize);
        EXPECT_TRUE(doc.getDimStyleTable()->add_direct(style));
        doc.getDimStyleTable()->activate_direct(style);
        return style;
    }

    /// @brief 标注放进 own，更新后返回（文档析构时释放）
    template <typename T>
    T* addToOwn(T* entity)
    {
        entity->setDocument(&own);
        entity->update();
        EXPECT_TRUE(own.getEntityTable()->add_direct(entity));
        return entity;
    }

    /// @brief 标注与引线的箭头块参照
    ///
    /// getSubEntities() 把块参照展开成它的图元，箭头块参照本身不在结果里；图元的父实体
    /// 就是箭头块参照（DmBlockReference::update），由此取回。
    static std::vector<DmBlockReference*> arrowsOf(const DmEntity& entity)
    {
        std::vector<DmBlockReference*> arrows;
        for (DmEntity* sub : entity.getSubEntities())
        {
            DmEntity* parent = sub->getParent();
            if (parent && parent != &entity && parent->getEntityType() == DM::EntityBlockReference &&
                std::find(arrows.begin(), arrows.end(), parent) == arrows.end())
            {
                arrows.push_back(static_cast<DmBlockReference*>(parent));
            }
        }
        return arrows;
    }

    /// @brief 箭头都引用 own 的箭头块表，且都找得到块
    void expectOwnArrows(const DmEntity& entity, size_t count)
    {
        DmBlockTable* ownArrows = own.getDimStyleTable()->getArrowBlocks();
        DmBlockTable* otherArrows = other.getDimStyleTable()->getArrowBlocks();
        ASSERT_NE(ownArrows, otherArrows);
        const std::vector<DmBlockReference*> arrows = arrowsOf(entity);
        ASSERT_EQ(arrows.size(), count);
        for (DmBlockReference* arrow : arrows)
        {
            EXPECT_EQ(arrow->getData().blockSource, ownArrows);
            EXPECT_NE(arrow->getData().blockSource, otherArrows);
            DmBlock* block = arrow->getBlockForInsert();
            ASSERT_NE(block, nullptr);
            EXPECT_EQ(ownArrows->find(block->getName()), block);
        }
    }

    DmDimensionData dimData(DmDimensionStyle* style, const DmVector& definitionPoint, const DmVector& textPos)
    {
        return DmDimensionData(definitionPoint, textPos, EMTextVertMode::kTextVertMid, EMTextHorzMode::kTextCenter,
                               1.0, QString(), 0.0, style);
    }
};
}  // namespace

TEST_F(DimensionDocumentFixture, 不在当前文档里的标注取自己文档的箭头块)
{
    DmDimensionStyle* ownStyle = addStyle(own, QStringLiteral("自身样式"), 3.0);
    addStyle(other, QStringLiteral("当前文档样式"), 5.0);

    auto* linear = addToOwn(new DmDimLinear(nullptr, dimData(ownStyle, DmVector(20.0, -10.0), DmVector(10.0, -10.0)),
                                            DmDimLinearData(DmVector(0.0, 0.0), DmVector(20.0, 0.0))));
    auto* aligned =
        addToOwn(new DmDimAligned(nullptr, dimData(ownStyle, DmVector(24.0, 13.0), DmVector(17.0, 16.0)),
                                  DmDimAlignedData(DmVector(0.0, 10.0), DmVector(20.0, 10.0))));
    auto* angular = addToOwn(new DmDimAngular(nullptr, dimData(ownStyle, DmVector(0.0, 0.0), DmVector(6.0, 3.0)),
                                              DmDimAngularData(DmVector(0.0, 0.0), DmVector(10.0, 0.0),
                                                               DmVector(0.0, 0.0), DmVector(0.0, 10.0),
                                                               DmVector(5.0, 5.0))));
    auto* radial = addToOwn(new DmDimRadial(nullptr, dimData(ownStyle, DmVector(12.0, -34.5), DmVector(15.0, -30.0)),
                                            DmDimRadialData(DmVector(18.125, -34.5), 5.0)));
    auto* diametric =
        addToOwn(new DmDimDiametric(nullptr, dimData(ownStyle, DmVector(5.875, -34.5), DmVector(12.0, -34.5)),
                                    DmDimDiametricData(DmVector(18.125, -34.5), 5.0)));
    auto* leader = addToOwn(new DmLeader(
        nullptr, DmLeaderData(ownStyle, {DmVector(70.0, 0.0), DmVector(80.0, 10.0), DmVector(90.0, 10.0)})));

    {
        SCOPED_TRACE("线性");
        expectOwnArrows(*linear, 2);
    }
    {
        SCOPED_TRACE("对齐");
        expectOwnArrows(*aligned, 2);
    }
    {
        SCOPED_TRACE("角度");
        expectOwnArrows(*angular, 2);
    }
    {
        SCOPED_TRACE("半径");
        expectOwnArrows(*radial, 1);
    }
    {
        SCOPED_TRACE("直径");
        expectOwnArrows(*diametric, 2);
    }
    {
        SCOPED_TRACE("引线");
        expectOwnArrows(*leader, 1);
    }
}

TEST_F(DimensionDocumentFixture, 宿主没有当前文档时标注照常生成箭头)
{
    // 无界面时宿主返回空；原先四种标注与引线在这里解空指针
    host.active = nullptr;
    DmDimensionStyle* ownStyle = addStyle(own, QStringLiteral("自身样式"), 3.0);
    auto* linear = addToOwn(new DmDimLinear(nullptr, dimData(ownStyle, DmVector(20.0, -10.0), DmVector(10.0, -10.0)),
                                            DmDimLinearData(DmVector(0.0, 0.0), DmVector(20.0, 0.0))));
    auto* leader = addToOwn(new DmLeader(
        nullptr, DmLeaderData(ownStyle, {DmVector(70.0, 0.0), DmVector(80.0, 10.0), DmVector(90.0, 10.0)})));
    expectOwnArrows(*linear, 2);
    expectOwnArrows(*leader, 1);
}
