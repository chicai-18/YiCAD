/// @file MesaLoader.cpp
/// @brief 在 main 之前装入 Mesa 的软件 OpenGL，使本进程的全部 GL 调用都走 llvmpipe
///
/// 出图测试要和基准图像逐像素比对，只有软件渲染在不同机器上结果一样（RENDER_PLAN.md 第 7 节 D9）。
/// 做法：
/// - test_render 以 /DELAYLOAD:opengl32.dll 链接（tests/render/CMakeLists.txt），进程启动时不装入系统的 opengl32.dll；
/// - 本文件的静态对象在 main 之前按完整路径装入 external/mesa/x64/opengl32.dll（tools/fetch_mesa.py 取得）；
/// - Windows 对不带路径的 LoadLibrary("opengl32.dll") 先看同名模块是否已装入，所以之后延迟加载的导入、
///   GLEW 与 Qt 的 QWindowsOpengl32DLL 拿到的都是这一个 Mesa。Qt 的 DLL 不静态导入 opengl32.dll。
///
/// Mesa 不放进 build/<cfg>/bin：那个目录会被 cmake --install 整个装进发布包。
///
/// 另外几件必须在 QApplication 与画布构造之前做的事也放在这里（外部已设置的以外部为准）：
/// - GALLIUM_DRIVER=llvmpipe：mesa-dist-win 的 opengl32.dll 在有 D3D12 的机器上默认走 d3d12 驱动（又回到了显卡）；
/// - QT_ENABLE_HIGHDPI_SCALING=0：与 Main.cpp 相同，否则系统缩放不是 100% 时帧缓冲尺寸随机器而变；
/// - YICAD_SHADER_DIR：图形系统的着色器从构建目录读（GsDevice），bin/resources/shaders 里的是 cmake --install 复制的。
/// - YICAD_GL_DEBUG=1：GL 上下文是调试上下文，RHI 一致性测试按 KHR_debug 数 GL 错误；
/// - Qt::AA_ShareOpenGLContexts 与默认格式 GLRhiDevice::surfaceFormat()（4.3 core）：与 Main.cpp 相同。

#include "MesaLoader.h"

// GLEW 必须先于 Qt 拉入的 gl.h
#include "GLRhiDevice.h"

#include <string>

#include <QByteArray>
#include <QCoreApplication>
#include <QSurfaceFormat>
#include <QtGlobal>

#ifdef _WIN32
#include <windows.h>
#endif

#ifndef YICAD_MESA_DIR
#error "test_render 需要 YICAD_MESA_DIR（Mesa 的 opengl32.dll 所在目录）"
#endif
#ifndef YICAD_SHADER_DIR
#error "test_render 需要 YICAD_SHADER_DIR（图形系统着色器的构建输出目录）"
#endif

namespace yicad_test
{
namespace
{

/// @brief 环境变量未设置时才设置，便于调试时从外面覆盖
/// @details qputenv 在 MSVC 上走 _putenv_s，CRT 与进程环境一起改，Mesa 与 Qt 都读得到
void setDefaultEnv(const char* name, const char* value)
{
    if (qEnvironmentVariableIsEmpty(name))
    {
        qputenv(name, QByteArray(value));
    }
}

MesaStatus load()
{
    MesaStatus status;
    setDefaultEnv("GALLIUM_DRIVER", "llvmpipe");
    setDefaultEnv("QT_ENABLE_HIGHDPI_SCALING", "0");
    setDefaultEnv("YICAD_SHADER_DIR", YICAD_SHADER_DIR);
    // 调试上下文：RHI 一致性测试按 KHR_debug 报告的 GL 错误数判断（GLRhiDevice::debugErrorCount）
    setDefaultEnv("YICAD_GL_DEBUG", "1");

    // 与 Main.cpp 相同：上下文共享、4.3 core，都要在建 QApplication（yicad_test_main.cpp）之前设置
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QSurfaceFormat::setDefaultFormat(GLRhiDevice::surfaceFormat());

    // YICAD_MESA_DIR 环境变量优先，便于指向别的 Mesa 版本比对
    std::string dir = YICAD_MESA_DIR;
    if (!qEnvironmentVariableIsEmpty("YICAD_MESA_DIR"))
    {
        dir = qgetenv("YICAD_MESA_DIR").toStdString();
    }
    status.path = dir + "\\opengl32.dll";
    // LOAD_WITH_ALTERED_SEARCH_PATH 要求反斜杠，CMake 给的是正斜杠
    for (char& c : status.path)
    {
        if (c == '/')
        {
            c = '\\';
        }
    }

#ifdef _WIN32
    // LOAD_WITH_ALTERED_SEARCH_PATH：它依赖的 libgallium_wgl.dll 从同一目录找
    const HMODULE module = LoadLibraryExA(status.path.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (module == nullptr)
    {
        status.error = "LoadLibraryEx 失败，错误码 " + std::to_string(GetLastError());
        return status;
    }
    status.loaded = true;
#else
    status.error = "只支持 Windows";
#endif
    return status;
}

const MesaStatus g_status = load();

}  // namespace

const MesaStatus& mesaStatus()
{
    return g_status;
}

}  // namespace yicad_test
