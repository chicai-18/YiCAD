/// @file yicad_test_main.cpp
/// @brief 全部测试二进制共用的入口
///
/// YiCAD 的类型系统（MetaType / Persistence 的 TYPESYSTEM 注册）是在
/// DmSystem::init 里建立的，持久化的读回依赖它；DmSettings 又依赖
/// QCoreApplication 的组织名与应用名。测试因此不能直接用
/// GTest::gtest_main，而要先把这条最小启动链跑一遍。
///
/// 这里刻意只做 App_Run 里与数据模型有关的那几步，不建主窗口、不加载
/// 翻译、不读填充图案——测试不应依赖界面资源是否就位。

#include <gtest/gtest.h>

#include <QApplication>
#include <QFileInfo>
#include <QString>

#include "DmSettings.h"
#include "DmSystem.h"

namespace
{
/// @brief 组织名与应用名。与产品使用的键分开，避免测试写脏开发机的注册表。
const char* const kOrganization = "YiCAD";
const char* const kApplication = "YiAppTest";
}  // namespace

int main(int argc, char* argv[])
{
    // 用 QApplication 而非 QCoreApplication：内核里有依赖 QWidget/QPixmap
    // 的代码路径（DmImage、DmCachePainter），构造 QCoreApplication 时
    // 一旦被触达就会断言失败。
    //
    // 这意味着测试需要一个可用的窗口站。GitHub Actions 的 windows 运行器
    // 满足这个条件。不要改用 QT_QPA_PLATFORM=offscreen——windeployqt 只
    // 部署了 platforms/qwindows.dll，没有 qoffscreen.dll。
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(kOrganization);
    QCoreApplication::setOrganizationDomain(kOrganization);
    QCoreApplication::setApplicationName(kApplication);

    const QFileInfo prgInfo(QFile::decodeName(argv[0]));
    const QString prgDir = prgInfo.absolutePath();

    DMSETTINGS->init(QCoreApplication::organizationName(),
                     QCoreApplication::applicationName());

    // 注册全部实体类型；持久化读回时按类型名查表构造，缺了这一步
    // restoreStream 会找不到类型。
    DMSYSTEM->init(QCoreApplication::applicationName(),
                   QCoreApplication::applicationVersion(),
                   "YiCAD",
                   prgDir);

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
