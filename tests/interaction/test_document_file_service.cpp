/// @file test_document_file_service.cpp
/// @brief 文档文件服务 DocumentFileService：一份文档的存盘策略（分层重组 S4c）
///
/// S4c 把 .bak 备份、外部修改检测、自动保存、打开失败的处理从 DmDocument 的 save/saveAs/open
/// 整段搬到 Application 的 DocumentFileService。"DocumentSavePolicy" 一组是 S0 为 DmDocument
/// 写的用例（原在 tests/persistence/test_persistence_document.cpp），随代码搬来，只把
/// doc.save/saveAs/open 改成经服务调用；其中两个原先依赖 R7、R8 而保留 DISABLED_ 前缀，
/// S4c 修好后启用。样本文档与文件工具见 tests/support/OcdSampleDocument.h。
///
/// 宿主服务用 FileServiceHost 代替，记录提示与命令行消息、预设确认对话框的回答；未命名文档的名字
/// 由宿主管理的打开图纸给出（S5 从宿主服务并入 IDocumentManager），用 FakeDocumentManager 代替。

#include <gtest/gtest.h>

#include <memory>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QStringList>
#include <QTemporaryDir>

#include "DmDocument.h"
#include "DocumentFileService.h"
#include "GuiDialogFactory.h"
#include "GuiDialogFactoryAdapter.h"
#include "MD5.h"
#include "support/FakeDocumentManager.h"
#include "support/OcdSampleDocument.h"

using namespace yicad_test;

namespace
{
/// @brief 测试用宿主服务：记录提示，确认对话框按预设回答
class FileServiceHost : public GuiDialogFactoryAdapter
{
public:
    bool confirmAnswer = false;  ///< 确认对话框的回答
    int confirmCount = 0;        ///< 确认对话框弹出的次数
    QStringList warnings;        ///< 警告对话框的内容
    QStringList messages;        ///< 命令行消息

    bool requestConfirmDialog(const QString&, const QString&) override
    {
        ++confirmCount;
        return confirmAnswer;
    }

    void requestWarningDialog(const QString& warning) override { warnings.append(warning); }

    void commandMessage(const QString& message) override { messages.append(message); }
};

/// @brief 用例夹具：装上 FileServiceHost，提供临时目录
struct FileServiceFixture : ::testing::Test
{
    FileServiceHost host;
    QTemporaryDir dir;

    FileServiceFixture() { GuiDialogFactory::instance()->setFactoryObject(&host); }
    ~FileServiceFixture() override { GuiDialogFactory::instance()->setFactoryObject(nullptr); }

    QString path(const QString& name) const { return dir.filePath(name); }

    /// @brief 构造样本文档
    void build(DmDocument& doc) { buildSample(doc); }
};

using DocumentSavePolicy = FileServiceFixture;
using DocumentFileServiceTest = FileServiceFixture;

/// @brief 自动保存副本的路径：临时目录下的 <名字>_<MD5 前 8 位>.ycd（与服务里的算法相同）
QString autoSaveCopy(const QString& baseName, const QString& md5Source)
{
    const QString tmpDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const QString sName = QString::fromStdString(MD5::getMD5(md5Source.toStdString())).left(8);
    return QDir::cleanPath(tmpDir + QDir::separator() + baseName + "_" + sName + ".ycd");
}
}  // namespace

// ---------------------------------------------------------------------------
// 存盘策略（S0 为 DmDocument::saveAs / save / open 写的用例，随代码搬来）
// ---------------------------------------------------------------------------

TEST_F(DocumentSavePolicy, 另存为经临时文件写出并记录文件名)
{
    DmDocument doc;
    ASSERT_NO_FATAL_FAILURE(build(doc));
    DocumentFileService files(doc);
    const QString file = path(QStringLiteral("saved.ycd"));
    ASSERT_TRUE(files.saveAs(file, kOcdFormat));
    EXPECT_EQ(doc.getFilename(), file);
    EXPECT_EQ(doc.getFormatType(), kOcdFormat);
    EXPECT_FALSE(doc.isModified());
    // 先写 <文件名>.tmp 再改名；第一次保存没有旧文件，不产生 .bak
    EXPECT_EQ(QDir(dir.path()).entryList(QDir::Files), QStringList{QStringLiteral("saved.ycd")});
    EXPECT_EQ(host.messages, QStringList{QStringLiteral("File saved: %1").arg(file)});
    // 文档与 27 个容器文件（含自定义实体的 CustomEntities.bin）
    EXPECT_EQ(readArchive(file).size(), 28u);
}

TEST_F(DocumentSavePolicy, 再次保存时旧文件备份为bak)
{
    DmDocument doc;
    ASSERT_NO_FATAL_FAILURE(build(doc));
    DocumentFileService files(doc);
    const QString file = path(QStringLiteral("drawing.ycd"));
    const QString bak = path(QStringLiteral("drawing.bak"));
    ASSERT_TRUE(files.saveAs(file, kOcdFormat));
    EXPECT_FALSE(QFileInfo::exists(bak));
    const QByteArray firstBytes = readBytes(file);

    // 未修改时 save() 直接返回成功、不写盘；force 才真正写
    ASSERT_TRUE(files.save());
    EXPECT_FALSE(QFileInfo::exists(bak));
    ASSERT_TRUE(files.save(false, true));
    ASSERT_TRUE(QFileInfo::exists(bak));
    EXPECT_EQ(readBytes(bak), firstBytes);
    EXPECT_EQ(QDir(dir.path()).entryList(QDir::Files),
              (QStringList{QStringLiteral("drawing.bak"), QStringLiteral("drawing.ycd")}));

    // 第三次保存覆盖 .bak，仍然只有一份
    const QByteArray secondBytes = readBytes(file);
    ASSERT_TRUE(files.save(false, true));
    EXPECT_EQ(readBytes(bak), secondBytes);
}

TEST_F(DocumentSavePolicy, 后缀与格式不符时拒绝保存)
{
    DmDocument doc;
    ASSERT_NO_FATAL_FAILURE(build(doc));
    DocumentFileService files(doc);
    const QString file = path(QStringLiteral("drawing.dxf"));
    EXPECT_FALSE(files.saveAs(file, kOcdFormat));
    EXPECT_FALSE(QFileInfo::exists(file));
    EXPECT_TRUE(doc.getFilename().isEmpty()) << "失败的另存为不应改动文件名";
    ASSERT_FALSE(host.messages.isEmpty());
    EXPECT_TRUE(host.messages.back().startsWith(QStringLiteral("File format mismatch.")));
}

TEST_F(DocumentSavePolicy, 外部修改过的文件拒绝保存)
{
    DmDocument doc;
    ASSERT_NO_FATAL_FAILURE(build(doc));
    DocumentFileService files(doc);
    const QString file = path(QStringLiteral("external.ycd"));
    ASSERT_TRUE(files.saveAs(file, kOcdFormat));

    // 修改时间只在 save() 的手动分支写盘成功后记录（另存为也走这条分支）
    QFile touched(file);
    ASSERT_TRUE(touched.open(QIODevice::ReadWrite));
    ASSERT_TRUE(touched.setFileTime(QDateTime::currentDateTime().addSecs(3600), QFileDevice::FileModificationTime));
    touched.close();

    host.messages.clear();
    EXPECT_FALSE(files.save(false, true));
    ASSERT_EQ(host.messages.size(), 1);
    EXPECT_TRUE(host.messages.front().startsWith(QStringLiteral("File on disk modified.")));
    EXPECT_FALSE(QFileInfo::exists(path(QStringLiteral("external.bak"))));
}

TEST_F(DocumentSavePolicy, 另存为后能打开且不算修改)
{
    DmDocument original;
    ASSERT_NO_FATAL_FAILURE(build(original));
    const QString file = path(QStringLiteral("saved.ycd"));
    ASSERT_TRUE(DocumentFileService(original).saveAs(file, kOcdFormat));

    DmDocument reopened;
    ASSERT_TRUE(DocumentFileService(reopened).open(file));
    EXPECT_EQ(reopened.getFilename(), file);
    EXPECT_FALSE(reopened.isModified());
    EXPECT_NE(first<DmLine>(*reopened.getEntityTable(), DM::EntityLine), nullptr);
    EXPECT_NE(reopened.getBlockTable()->find(kBlockName), nullptr);
    EXPECT_TRUE(host.warnings.isEmpty());
    EXPECT_EQ(host.confirmCount, 0);
}

// S0 时依赖 R7（打开主文件失败时不再抛出）、R8（.bak 找得到过滤器），S4c 修复后启用。
TEST_F(DocumentSavePolicy, 打开损坏文件时询问是否打开备份)
{
    DmDocument doc;
    ASSERT_NO_FATAL_FAILURE(build(doc));
    DocumentFileService files(doc);
    const QString file = path(QStringLiteral("broken.ycd"));
    ASSERT_TRUE(files.saveAs(file, kOcdFormat));
    ASSERT_TRUE(files.save(false, true));  // 第二次保存留下 broken.bak
    ASSERT_TRUE(QFileInfo::exists(path(QStringLiteral("broken.bak"))));
    ASSERT_TRUE(writeBytes(file, QByteArray(4096, 'x')));

    host.confirmAnswer = true;
    DmDocument reopened;
    EXPECT_TRUE(DocumentFileService(reopened).open(file));
    EXPECT_EQ(host.confirmCount, 1);
    EXPECT_NE(reopened.getBlockTable()->find(kBlockName), nullptr);
}

// S0 时依赖 R7，S4c 修复后启用。
TEST_F(DocumentSavePolicy, 打开损坏文件且没有备份时警告并返回失败)
{
    const QString file = path(QStringLiteral("broken.ycd"));
    ASSERT_TRUE(writeBytes(file, QByteArray(4096, 'x')));

    DmDocument doc;
    EXPECT_FALSE(DocumentFileService(doc).open(file));
    EXPECT_EQ(host.confirmCount, 0);
    EXPECT_EQ(host.warnings, QStringList{QStringLiteral("Open failed, invalid file!")});
}

// ---------------------------------------------------------------------------
// S4c 补充
// ---------------------------------------------------------------------------

TEST_F(DocumentFileServiceTest, 打开备份后改名为带时间戳的副本)
{
    // 读到的备份复制成 <基名>_<时间戳>.ycd，文档改用这个文件名，原文件与 .bak 不动
    DmDocument doc;
    ASSERT_NO_FATAL_FAILURE(build(doc));
    DocumentFileService files(doc);
    const QString file = path(QStringLiteral("broken.ycd"));
    ASSERT_TRUE(files.saveAs(file, kOcdFormat));
    ASSERT_TRUE(files.save(false, true));
    ASSERT_TRUE(writeBytes(file, QByteArray(4096, 'x')));

    host.confirmAnswer = true;
    DmDocument reopened;
    ASSERT_TRUE(DocumentFileService(reopened).open(file));
    const QFileInfo copied(reopened.getFilename());
    EXPECT_EQ(copied.absolutePath(), QFileInfo(file).absolutePath());
    EXPECT_TRUE(copied.fileName().startsWith(QStringLiteral("broken_"))) << copied.fileName().toStdString();
    EXPECT_EQ(copied.suffix(), QStringLiteral("ycd"));
    EXPECT_TRUE(copied.exists());
    EXPECT_EQ(readBytes(copied.filePath()), readBytes(path(QStringLiteral("broken.bak"))));
    EXPECT_FALSE(reopened.isModified());
}

TEST_F(DocumentFileServiceTest, 拒绝打开备份时警告并返回失败)
{
    DmDocument doc;
    ASSERT_NO_FATAL_FAILURE(build(doc));
    DocumentFileService files(doc);
    const QString file = path(QStringLiteral("broken.ycd"));
    ASSERT_TRUE(files.saveAs(file, kOcdFormat));
    ASSERT_TRUE(files.save(false, true));
    ASSERT_TRUE(writeBytes(file, QByteArray(4096, 'x')));

    host.confirmAnswer = false;
    DmDocument reopened;
    EXPECT_FALSE(DocumentFileService(reopened).open(file));
    EXPECT_EQ(host.confirmCount, 1);
    EXPECT_EQ(host.warnings, QStringList{QStringLiteral("Open failed, invalid file!")});
}

TEST_F(DocumentFileServiceTest, 未命名文档自动保存到临时目录的副本)
{
    // 未命名文档用宿主管理的打开图纸给的名字（标签页名）；只写原生格式，不改文档的文件名与"已修改"状态
    const QString name = QStringLiteral("S4cUntitled_%1").arg(QDateTime::currentMSecsSinceEpoch());
    const QString copy = autoSaveCopy(name, name);
    QFile::remove(copy);

    DmDocument doc;
    ASSERT_NO_FATAL_FAILURE(build(doc));
    FakeDocumentManager documents;
    documents.untitledNames[&doc] = name;
    DocumentFileService files(doc, &documents);
    ASSERT_TRUE(files.save(true, true));
    EXPECT_TRUE(QFileInfo::exists(copy));
    EXPECT_FALSE(QFileInfo::exists(copy + QStringLiteral(".tmp")));
    EXPECT_EQ(host.messages, (QStringList{QStringLiteral("Auto saving file: %1").arg(copy),
                                          QStringLiteral("File saved: %1").arg(copy)}));
    EXPECT_TRUE(doc.getFilename().isEmpty());
    EXPECT_EQ(readArchive(copy).size(), 28u);
    QFile::remove(copy);
}

TEST_F(DocumentFileServiceTest, 每份文档只自动保存一次)
{
    // 交互清单 W5 记录的既有行为：autoSave() 之后 hasAutoSaved() 为真，此后的自动保存直接返回成功。
    // 样本文档不经撤销栈构造，isModified() 为假，第一次 autoSave() 也不写盘
    const QString name = QStringLiteral("S4cOnce_%1").arg(QDateTime::currentMSecsSinceEpoch());
    const QString copy = autoSaveCopy(name, name);

    DmDocument doc;
    ASSERT_NO_FATAL_FAILURE(build(doc));
    FakeDocumentManager documents;
    documents.untitledNames[&doc] = name;
    DocumentFileService files(doc, &documents);
    EXPECT_FALSE(files.hasAutoSaved());
    files.autoSave();
    EXPECT_TRUE(files.hasAutoSaved());
    EXPECT_TRUE(host.messages.isEmpty());

    EXPECT_TRUE(files.save(true, true));
    EXPECT_TRUE(host.messages.isEmpty());
    EXPECT_FALSE(QFileInfo::exists(copy));
}

TEST_F(DocumentFileServiceTest, 没有打开的图纸时未命名文档的副本名为空)
{
    // 块命令读写的临时文档不交给宿主管理（BlockFileCommands 构造服务时不传），自动保存时名字为空，
    // 副本是 <临时目录>/_<空串的 MD5 前 8 位>.ycd。原先宿主服务的空实现同样返回空名字
    const QString copy = autoSaveCopy(QString(), QString());
    QFile::remove(copy);

    DmDocument doc;
    ASSERT_NO_FATAL_FAILURE(build(doc));
    DocumentFileService files(doc);
    ASSERT_TRUE(files.save(true, true));
    EXPECT_TRUE(QFileInfo::exists(copy));
    EXPECT_EQ(host.messages.value(0), QStringLiteral("Auto saving file: %1").arg(copy));
    QFile::remove(copy);
}

TEST_F(DocumentFileServiceTest, 按文档找到服务析构后找不到)
{
    DmDocument doc;
    DmDocument other;
    {
        DocumentFileService files(doc);
        EXPECT_EQ(DocumentFileService::find(&doc), &files);
        EXPECT_EQ(&files.document(), &doc);
        EXPECT_EQ(DocumentFileService::find(&other), nullptr);
    }
    EXPECT_EQ(DocumentFileService::find(&doc), nullptr);
}
