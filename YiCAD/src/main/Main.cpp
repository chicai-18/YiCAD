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

/// @file Main.cpp
/// @brief YiCAD 应用程序主入口实现文件
/// @details 实现应用程序的启动、初始化和主循环逻辑

#include "Main.h"
#ifdef Q_OS_WIN32
#include "qt_windows.h"
#endif

#include <clocale>

#include <QDebug>
#include <QApplication>
#include <QSplashScreen>
#include <QSettings>
#include <QMessageBox>
#include <QFileInfo>
#include <qmainwindow.h>

#include "DmFontList.h"
#include "DmTextStyleTable.h"
#include "DmPatternList.h"
#include "DmSettings.h"
#include "DmSystem.h"
#include "LLMSettingsService.h"
#include "DmLineTypeTable.h"

#include "ApplicationWindow.h"

#include "Debug.h"

// 取消以下注释来引入VLD探测内存泄漏
//#include "vld.h"

// 启动画面尺寸常量（像素）
const int SPLASH_IMAGE_SIZE = 700;

// 注册表配置键名常量
const char* const REG_KEY_SHOW_SPLASH = "/ShowSplash";
const char* const REG_GROUP_STARTUP = "/Startup";
const char* const REG_GROUP_DEFAULTS = "/Defaults";

/// @brief 创建并运行应用程序主窗体
/// @param [in] argc 命令行参数数量
/// @param [in] argv 命令行参数数组
/// @return 应用程序退出码，成功返回0，失败返回EXIT_FAILURE
int App_Run(int argc, char* argv[])
{
    DEBUG->print(Debug::D_INFORMATIONAL, "YiCAD Start!");

    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    QApplication::setAttribute(
        Qt::AA_DisableWindowContextHelpButton);  // 禁用对话框的帮助按钮
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("YiCAD");
    QCoreApplication::setOrganizationDomain("YiCAD");
    QCoreApplication::setApplicationName("YiApp");

    // 初始化相对路径和翻译资源等
    QFileInfo prgInfo(QFile::decodeName(argv[0]));
    QString prgDir(prgInfo.absolutePath());
    DMSETTINGS->init(
        app.organizationName(), app.applicationName());  // 初始应用程序设置

    // 初始化 LLM 配置服务（用于 AI 功能）
    LLMSettingsService::instance()->init(
        app.organizationName(), app.applicationName());
    DMSYSTEM->init(app.applicationName(),
                   app.applicationVersion(),
                   XSTR(APPDIR),
                   prgDir);  // 初始化系统路径

    // 设置语言 默认中文
    QString lang = "zh_CN";  // settings.value("Language", "zh_CN").toString();
    QSettings settings;
    settings.endGroup();
    DMSYSTEM->loadTranslation(lang);  // 加载语言包
    DMPATTERNLIST->init();            // 初始化填充

    /// 读取注册表是否加载动画以及主题颜色数据
    DMSETTINGS->beginGroup(REG_GROUP_STARTUP);
    bool show_splash = DMSETTINGS->readNumEntry(REG_KEY_SHOW_SPLASH, 1) != 0;
    DMSETTINGS->endGroup();

    // 启动界面
    auto* splash = new QSplashScreen;
    if (show_splash)
    {
        // 显示cad启动图片
        QIcon splashIcon(":/ribbon/startup_image.png");
        QPixmap pixmap = splashIcon.pixmap(QSize(SPLASH_IMAGE_SIZE, SPLASH_IMAGE_SIZE));
        splash->setPixmap(pixmap);
        splash->setAttribute(Qt::WA_DeleteOnClose);
        splash->show();
        // splash->showMessage(QObject::tr("YiCAD is Loading.."), Qt::AlignHCenter | Qt::AlignBottom, Qt::black);
        app.processEvents();
    }

    ApplicationWindow w;
    app.installEventFilter(&w);
    w.setWindowTitle(QObject::tr("YiCAD"));
    w.setWindowIcon(QIcon(":/ribbon/logo.png"));
    w.show();

    if (show_splash)
    {
        splash->finish(&w);
    }
    else
    {
        delete splash;
        splash = nullptr;
    }

    DEBUG->print(Debug::D_INFORMATIONAL, "YiCAD End!");
    return app.exec();
}

#ifdef Q_OS_WIN32
#ifndef NDEBUG

/// @brief Windows调试模式下的主函数入口
/// @param [in] argc 命令行参数数量
/// @param [in] argv 命令行参数数组
/// @return 应用程序退出码
int main(int argc, char* argv[])
{
    return App_Run(argc, argv);
}
#else

/// @brief Windows发布模式下的主函数入口（使用WinMain）
/// @param [in] hInstance 当前实例句柄
/// @param [in] hPrevInstance 前一个实例句柄（总是NULL）
/// @param [in] lpCmdLine 命令行参数字符串
/// @param [in] nCmdShow 窗口显示方式
/// @return 应用程序退出码
int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
{
    return App_Run(__argc, __argv);
}
#endif
#else

/// @brief 非Windows平台的主函数入口
/// @param [in] argc 命令行参数数量
/// @param [in] argv 命令行参数数组
/// @return 应用程序退出码
int main(int argc, char* argv[])
{
    return App_Run(argc, argv);
}
#endif
