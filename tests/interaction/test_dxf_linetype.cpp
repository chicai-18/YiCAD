/// @file test_dxf_linetype.cpp
/// @brief DXF 导入的随层、随块线型（doc/RENDER_PLAN.md 第 10 节阶段 5）
///
/// 与 AutoCAD/ODA 一致，随层、随块线型是文档线型表里的 ByLayer、ByBlock 保留记录：导入后实体引用的
/// 就是这两条记录（不是别的对象），块里线型为 BYBLOCK 的实体取块参照的线型。
/// 经构建出来的 DXF 插件导入（support/DxfTestRuntime.h），与 test_dxf_encoding 相同。

#include <gtest/gtest.h>

#include <string>

#include <QFile>
#include <QTemporaryDir>

#include "DmBlock.h"
#include "DmBlockReference.h"
#include "DmBlockTable.h"
#include "DmDocument.h"
#include "DmLine.h"
#include "DmLineType.h"
#include "DmLineTypeTable.h"
#include "EntityTable.h"
#include "support/DxfTestRuntime.h"

namespace
{
using yicad_test::DxfRuntime;

/// @brief R12 图纸：线型 DASHED；块 B 里一条线型 BYBLOCK 的直线；模型空间一条线型 BYLAYER 的直线、
///        一个线型为 DASHED 的块参照。R12 的线型表里没有 ByLayer、ByBlock 记录
std::string drawing()
{
    std::string dxf;
    auto group = [&dxf](const char* code, const char* value) {
        dxf += code;
        dxf += '\n';
        dxf += value;
        dxf += '\n';
    };
    auto point = [&group](const char* x, const char* y, int code) {
        const std::string cx = std::to_string(code);
        const std::string cy = std::to_string(code + 10);
        const std::string cz = std::to_string(code + 20);
        group(cx.c_str(), x);
        group(cy.c_str(), y);
        group(cz.c_str(), "0.0");
    };
    group("0", "SECTION");
    group("2", "HEADER");
    group("9", "$ACADVER");
    group("1", "AC1009");
    group("0", "ENDSEC");

    group("0", "SECTION");
    group("2", "TABLES");
    group("0", "TABLE");
    group("2", "LTYPE");
    group("70", "1");
    group("0", "LTYPE");
    group("2", "DASHED");
    group("70", "0");
    group("3", "Dashed __ __ __");
    group("72", "65");
    group("73", "2");
    group("40", "0.75");
    group("49", "0.5");
    group("49", "-0.25");
    group("0", "ENDTAB");
    group("0", "TABLE");
    group("2", "LAYER");
    group("70", "1");
    group("0", "LAYER");
    group("2", "0");
    group("70", "0");
    group("62", "7");
    group("6", "CONTINUOUS");
    group("0", "ENDTAB");
    group("0", "ENDSEC");

    group("0", "SECTION");
    group("2", "BLOCKS");
    group("0", "BLOCK");
    group("8", "0");
    group("2", "B");
    group("70", "0");
    point("0.0", "0.0", 10);
    group("3", "B");
    group("0", "LINE");
    group("8", "0");
    group("6", "BYBLOCK");
    point("0.0", "0.0", 10);
    point("10.0", "0.0", 11);
    group("0", "ENDBLK");
    group("8", "0");
    group("0", "ENDSEC");

    group("0", "SECTION");
    group("2", "ENTITIES");
    group("0", "LINE");
    group("8", "0");
    group("6", "BYLAYER");
    point("0.0", "5.0", 10);
    point("10.0", "5.0", 11);
    group("0", "INSERT");
    group("8", "0");
    group("6", "DASHED");
    group("2", "B");
    point("0.0", "-5.0", 10);
    group("0", "ENDSEC");
    group("0", "EOF");
    return dxf;
}

bool writeText(const QString& path, const std::string& text)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        return false;
    }
    return file.write(text.data(), static_cast<qint64>(text.size())) == static_cast<qint64>(text.size());
}

/// @brief 实体表里第一个指定类型的实体
template <typename T>
T* firstOf(EntityTable& table, DM::EntityType type)
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

/// @brief 读入 drawing()
struct DxfLinetypeImport : ::testing::Test
{
    DxfRuntime runtime;
    QTemporaryDir dir;
    DmDocument document;

    void SetUp() override
    {
        ASSERT_TRUE(runtime.loaded()) << runtime.diagnostics().toStdString();
        const QString source = dir.filePath(QStringLiteral("linetype_r12.dxf"));
        ASSERT_TRUE(writeText(source, drawing()));
        ASSERT_TRUE(runtime.importFile(document, source));
    }
};
}  // namespace

TEST_F(DxfLinetypeImport, BYLAYER线型导入为线型表里的随层记录)
{
    auto* line = firstOf<DmLine>(*document.getEntityTable(), DM::EntityLine);
    ASSERT_NE(line, nullptr);
    EXPECT_EQ(line->getPen(false).getLineType(), document.getLineTypeTable()->find(LineType::ByLayer));
}

TEST_F(DxfLinetypeImport, 块里BYBLOCK线型导入为线型表里的随块记录)
{
    DmBlock* block = document.getBlockTable()->find(QStringLiteral("B"));
    ASSERT_NE(block, nullptr);
    auto* line = firstOf<DmLine>(block->getEntityTable(), DM::EntityLine);
    ASSERT_NE(line, nullptr);
    DmLineType* lineType = line->getPen(false).getLineType();
    ASSERT_NE(lineType, nullptr);
    EXPECT_EQ(lineType->getLineTypeName(), LineType::ByBlock);
    EXPECT_EQ(lineType, document.getLineTypeTable()->find(LineType::ByBlock));
}

TEST_F(DxfLinetypeImport, 块里随块线型的实体取块参照的线型)
{
    auto* insert = firstOf<DmBlockReference>(*document.getEntityTable(), DM::EntityBlockReference);
    ASSERT_NE(insert, nullptr);
    const std::list<DmEntity*> subs = insert->getSubEntities();
    ASSERT_FALSE(subs.empty());
    DmLineType* lineType = subs.front()->getPen(true).getLineType();
    ASSERT_NE(lineType, nullptr);
    EXPECT_EQ(lineType->getLineTypeName(), QStringLiteral("DASHED"));
}
