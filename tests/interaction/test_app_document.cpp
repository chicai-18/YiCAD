/// @file test_app_document.cpp
/// @brief 一份打开的图纸 AppDocument：持有文档、它的存盘策略与选择集（选择集移出 Model 第 2、3 步）
///
/// 第 2 步把"持有文档、持有文档文件服务"从图纸窗口 MDIWindow 提到 Application 层的 AppDocument，
/// 见 doc/SELECTION_SET_PLAN.md 6.3 节。存盘策略本身的用例在 test_document_file_service.cpp，
/// 这里只核对 AppDocument 把它们接在一起：服务管理的是它持有的文档，宿主管理的打开图纸传给了服务，
/// 服务随它释放；选择集（第 3 步）选的是它持有的文档里的实体，选择集本身的用例在 test_selection_set.cpp。

#include <gtest/gtest.h>

#include <vector>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

#include "AppDocument.h"
#include "DmDocument.h"
#include "DmLine.h"
#include "DocumentFileService.h"
#include "EntityTable.h"
#include "MD5.h"
#include "SelectionSet.h"
#include "support/FakeDocumentManager.h"

using namespace yicad_test;

TEST(AppDocumentTest, 文档文件服务管理的是持有的文档且随图纸释放)
{
    FakeDocumentManager documents;
    const DmDocument* released = nullptr;
    {
        AppDocument app(documents);
        EXPECT_EQ(&app.fileService().document(), &app.document());
        EXPECT_EQ(DocumentFileService::find(&app.document()), &app.fileService());
        released = &app.document();
    }
    // 只比较地址、不解引用：释放后按这份文档找不到服务
    EXPECT_EQ(DocumentFileService::find(released), nullptr);
}

TEST(AppDocumentTest, 选择集选的是持有的文档里的实体)
{
    FakeDocumentManager documents;
    AppDocument app(documents);
    auto* line = new DmLine(DmVector(0.0, 0.0), DmVector(10.0, 0.0));
    line->calculateBorders();
    ASSERT_TRUE(app.document().getEntityTable()->add_direct(line));

    app.selection().selectAll();
    EXPECT_EQ(app.selection().entities(), std::vector<DmEntity*>{line});
}

TEST(AppDocumentTest, 未命名图纸的自动保存副本用宿主给的名字)
{
    // 宿主管理的打开图纸经 AppDocument 交给文档文件服务；副本路径的算法见 test_document_file_service.cpp
    const QString name = QStringLiteral("AppDocUntitled_%1").arg(QDateTime::currentMSecsSinceEpoch());
    const QString tmpDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const QString md5 = QString::fromStdString(MD5::getMD5(name.toStdString())).left(8);
    const QString copy = QDir::cleanPath(tmpDir + QDir::separator() + name + "_" + md5 + ".ycd");
    QFile::remove(copy);

    FakeDocumentManager documents;
    AppDocument app(documents);
    documents.untitledNames[&app.document()] = name;
    ASSERT_TRUE(app.fileService().save(true, true));
    EXPECT_TRUE(QFileInfo::exists(copy));
    QFile::remove(copy);
}
