/// @file test_render_environment.cpp
/// @brief 出图测试的前提：本进程的 OpenGL 是 Mesa llvmpipe，且版本够跑现在的着色器
///
/// 基准图像是 llvmpipe 画的，换成显卡驱动画就会逐像素对不上；这里先把前提查清楚，
/// 出错时说明怎么补（tools/fetch_mesa.py），而不是让后面每个出图用例各自报一堆像素差。

// GLEW 必须先于 Qt 拉入的 gl.h（见 GuiDocumentView.h 的说明）
#define GL_GLEXT_PROTOTYPES
#include <GL/glew.h>

#include <gtest/gtest.h>

#include <string>

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QSurfaceFormat>

#include "GLRhiDevice.h"
#include "MesaLoader.h"

namespace
{
/// @brief 与画布相同的默认格式建一个上下文，读出实现的名字与版本
struct GlInfo
{
    bool created = false;
    std::string renderer;
    std::string version;
    int major = 0;
    int minor = 0;
    GLint profileMask = 0;   ///< GL_CONTEXT_PROFILE_MASK
    GLint contextFlags = 0;  ///< GL_CONTEXT_FLAGS
};

GlInfo queryGl()
{
    GlInfo info;
    QOffscreenSurface surface;
    surface.create();
    QOpenGLContext context;
    if (!context.create() || !context.makeCurrent(&surface))
    {
        return info;
    }
    info.created = true;
    info.renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    info.version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    info.major = context.format().majorVersion();
    info.minor = context.format().minorVersion();
    glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &info.profileMask);
    glGetIntegerv(GL_CONTEXT_FLAGS, &info.contextFlags);
    context.doneCurrent();
    return info;
}
}  // namespace

TEST(RenderEnvironmentTest, 已装入Mesa)
{
    const yicad_test::MesaStatus& status = yicad_test::mesaStatus();
    EXPECT_TRUE(status.loaded) << "没有装入 " << status.path << "（" << status.error
                               << "）。先运行 python tools/fetch_mesa.py，见 doc/RENDER_PLAN.md 第 7 节 D9";
}

TEST(RenderEnvironmentTest, OpenGL是llvmpipe且不低于4点3)
{
    const GlInfo info = queryGl();
    ASSERT_TRUE(info.created) << "建不了 OpenGL 上下文";
    EXPECT_NE(info.renderer.find("llvmpipe"), std::string::npos)
        << "当前 GL 实现是 " << info.renderer << "，基准图像是 Mesa llvmpipe 画的";
    // 现有着色器写的是 #version 430 且带几何着色器（RENDER_PLAN.md P15）
    EXPECT_TRUE(info.major > 4 || (info.major == 4 && info.minor >= 3))
        << "GL 版本 " << info.version << " 低于 4.3";
}

TEST(RenderEnvironmentTest, 上下文与程序相同_4点3core且共享)
{
    // MesaLoader.cpp 在 main 之前做了与 Main.cpp 相同的设置（RENDER_PLAN.md 第 4.7.3 节）
    EXPECT_NE(QOpenGLContext::globalShareContext(), nullptr) << "没有设置 Qt::AA_ShareOpenGLContexts";
    const GlInfo info = queryGl();
    ASSERT_TRUE(info.created) << "建不了 OpenGL 上下文";
    EXPECT_TRUE(info.profileMask & GL_CONTEXT_CORE_PROFILE_BIT) << "上下文不是 core profile";
    // 旧渲染器用 glLineWidth 画宽线，forward-compatible 上下文里宽线是错误
    EXPECT_FALSE(info.contextFlags & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT) << "上下文是 forward-compatible";
    EXPECT_TRUE(GLRhiDevice::checkSupport().ok);
}
