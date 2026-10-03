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

/// @file GLRhiDevice.h
/// @brief RHI 的 OpenGL 4.3 core 实现：设备（RENDER_PLAN.md 第 4.7.3 节）

#ifndef GLRHIDEVICE_H
#define GLRHIDEVICE_H

#include <GL/glew.h>

#include <atomic>
#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <QMetaObject>
#include <QSurfaceFormat>

#include "RhiDevice.h"

class QOpenGLContext;
class QOffscreenSurface;
class QSurface;
class GLRhiBuffer;
class GLRhiTexture;
class GLRhiCommandList;
class GLRhiSurface;
class GLRhiUploadRing;

/// @brief 一个 GL 上下文里不能共享的对象（VAO、FBO 是容器对象，不随共享组共享）与调试输出的状态
struct GLRhiContextState
{
    QOpenGLContext* context = nullptr;
    std::unordered_map<std::uint64_t, GLuint> vertexArrays;  ///< 管线 id -> VAO
    std::unordered_map<std::uint64_t, GLuint> framebuffers;  ///< 渲染目标 id -> FBO
    GLuint scratchReadFramebuffer = 0;   ///< 读回、解析时临时挂纹理用
    GLuint scratchDrawFramebuffer = 0;
    std::vector<GLuint> pendingVertexArrayDeletes;  ///< 管线销毁时本上下文不是当前的，等它下次成为当前再删
    std::vector<GLuint> pendingFramebufferDeletes;
    QMetaObject::Connection destroyConnection;
};

/// @brief OpenGL 设备：一组共享的上下文（第 4.7.3 节）
/// @details 设备自带一个离屏上下文，与 Qt 的全局共享上下文（Qt::AA_ShareOpenGLContexts）共享，在没有当前上下文时
///          用它建资源；画到窗口时用 QOpenGLWidget 自己的上下文。缓冲、纹理、程序、采样器在共享组里共享，
///          VAO、FBO 按上下文惰性创建并缓存。
///
///          上传：upload() 先把数据复制到 CPU 侧，下一次 beginFrame 时写进上传环形缓冲（有 ARB_buffer_storage 时持久映射，
///          否则非同步映射），再在帧的上下文里复制到目标，所以对目标的修改都在使用它的上下文里发生。
///          每帧结束插一个 fence；资源句柄释放后进延迟释放队列，等它可能被用到的那一帧的 fence 通过才销毁；
///          环形缓冲的区段同样按帧回收。换了上下文画下一帧时，先在 GPU 上等上一帧的 fence（共享对象跨上下文的可见性）
class GLRhiDevice final : public RhiDevice
{
public:
    struct Options
    {
        /// @brief 不用持久映射，走 glMapBufferRange 的退路（测试两条路径用）
        bool disablePersistentMapping = false;
        /// @brief 上传环形缓冲的初始大小；一帧的上传放不下时翻倍
        std::size_t uploadRingSize = 8u * 1024u * 1024u;
    };

    /// @brief 启动检查的结果
    struct Support
    {
        bool ok = false;               ///< 拿到了 4.3 及以上的 core profile
        bool contextCreated = false;
        int majorVersion = 0;
        int minorVersion = 0;
        bool coreProfile = false;
        std::string version;           ///< GL_VERSION
        std::string renderer;          ///< GL_RENDERER
    };

    /// @brief 程序里全部 GL 上下文的格式：4.3 core profile
    /// @details 不带 forward-compatible 标志：旧渲染器还在用宽线（glLineWidth 大于 1），
    ///          forward-compatible 上下文里那是错误。调试构建或环境变量 YICAD_GL_DEBUG=1 时为调试上下文（KHR_debug）
    static QSurfaceFormat surfaceFormat();

    /// @brief 按 surfaceFormat() 建一个上下文，看驱动给出的版本与 profile；要有 QGuiApplication
    static Support checkSupport();

    /// @brief 建设备；要求已设置 Qt::AA_ShareOpenGLContexts 且驱动支持 4.3 core，失败返回空并记日志
    static std::unique_ptr<GLRhiDevice> create(const Options& options);
    static std::unique_ptr<GLRhiDevice> create() { return create(Options{}); }

    ~GLRhiDevice() override;
    GLRhiDevice(const GLRhiDevice&) = delete;
    GLRhiDevice& operator=(const GLRhiDevice&) = delete;

    // ---- RhiDevice ----
    const RhiCaps& caps() const override { return m_caps; }
    RhiBufferPtr createBuffer(const RhiBufferDesc& desc) override;
    RhiTexturePtr createTexture(const RhiTextureDesc& desc) override;
    RhiSamplerPtr createSampler(const RhiSamplerDesc& desc) override;
    RhiShaderPtr createShader(const RhiShaderDesc& desc) override;
    RhiBindGroupLayoutPtr createBindGroupLayout(const RhiBindGroupLayoutDesc& desc) override;
    RhiBindGroupPtr createBindGroup(const RhiBindGroupDesc& desc) override;
    RhiPipelinePtr createPipeline(const RhiPipelineDesc& desc) override;
    RhiRenderTargetPtr createRenderTarget(const RhiRenderTargetDesc& desc) override;
    void upload(RhiBuffer& dst, std::size_t offset, std::span<const std::byte> data) override;
    void upload(RhiTexture& dst, const RhiTextureRegion& region, std::span<const std::byte> data) override;
    RhiCommandList& beginFrame(RhiSurface& surface) override;
    RhiCommandList& beginOffscreenFrame() override;
    void endFrame() override;
    void waitIdle() override;
    bool readBuffer(const RhiBuffer& buffer, std::size_t offset, std::span<std::byte> out) override;
    const glm::mat4& clipSpaceCorrection() const override;

    // ---- 诊断（测试用） ----
    /// @brief 已开始的帧数；第 n 帧的序号是 n
    std::uint64_t frameSerial() const { return m_frameSerial; }
    /// @brief 已确认在 GPU 上完成的最大帧序号（只在 beginFrame、endFrame、waitIdle 时更新）
    std::uint64_t completedFrameSerial() const { return m_completedSerial; }
    /// @brief 等待销毁的资源数
    std::size_t pendingReleaseCount() const { return m_releaseQueue.size(); }
    /// @brief 还没销毁的资源数（含等待销毁的）
    std::size_t liveResourceCount() const { return m_liveResources; }
    /// @brief 调试输出报告的 GL 错误数（仅调试上下文）
    std::size_t debugErrorCount() const { return m_debugErrorCount.load(); }
    /// @brief 上传环形缓冲当前的大小
    std::size_t uploadRingCapacity() const;
    /// @brief 设备自己的离屏上下文
    QOpenGLContext* internalContext() const { return m_context.get(); }

    // ---- 后端内部：资源与命令列表用 ----
    /// @brief 句柄的删除器调用：放进延迟释放队列
    void deferRelease(RhiResource* resource);
    /// @brief GL 资源析构时调用
    void onResourceDestroyed() { --m_liveResources; }
    /// @brief 缓冲、纹理析构时丢弃还没执行的上传
    void dropPendingUploads(const void* target);
    /// @brief 管线销毁：各上下文里的 VAO 当场或下次成为当前时删除
    void forgetPipeline(std::uint64_t id);
    /// @brief 渲染目标销毁：各上下文里的 FBO 同上
    void forgetRenderTarget(std::uint64_t id);
    /// @brief 当前上下文的状态；当前上下文必须属于本设备的共享组
    GLRhiContextState& currentContextState();
    /// @brief 新资源的序号（VAO、FBO 缓存的键，不会因地址复用而撞上）
    std::uint64_t nextObjectId() { return ++m_lastObjectId; }
    /// @brief 等第 serial 帧在 GPU 上完成
    void waitForFrame(std::uint64_t serial);
    /// @brief 正在录制或最后提交的帧的序号（环形缓冲区段、延迟释放用）
    std::uint64_t currentSerial() const { return m_frameSerial; }
    /// @brief 环形缓冲扩容后旧缓冲按帧回收
    void retireBuffer(GLuint buffer);
    /// @brief 调试输出的回调转到这里
    void onDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, const char* message);
    /// @brief 是否支持给对象设调试名（KHR_debug）
    void setObjectLabel(GLenum identifier, GLuint name, const std::string& label) const;

    /// @brief 保证当前上下文属于本设备的共享组：不属于时换成设备自己的上下文，析构时换回去
    class ContextGuard
    {
    public:
        explicit ContextGuard(GLRhiDevice& device);
        ~ContextGuard();
        ContextGuard(const ContextGuard&) = delete;
        ContextGuard& operator=(const ContextGuard&) = delete;

    private:
        GLRhiDevice& m_device;
        QOpenGLContext* m_previousContext = nullptr;
        QSurface* m_previousSurface = nullptr;
        bool m_switched = false;
    };

private:
    explicit GLRhiDevice(const Options& options);
    bool initialize();
    void queryCaps();

    /// @brief 把句柄交给 shared_ptr，删除器把对象放进延迟释放队列（设备已销毁时直接删除，不碰 GL）
    template <typename T>
    std::shared_ptr<T> adopt(T* resource);

    bool isInShareGroup(QOpenGLContext* context) const;
    GLRhiContextState& contextState(QOpenGLContext* context);
    void flushPendingContainerDeletes(GLRhiContextState& state);
    RhiCommandList& startFrame(QOpenGLContext* context, GLRhiSurface* surface);
    void resetGlState();
    void flushUploads();
    /// @brief 取一块待上传数据的暂存（复用用过的，没有就是空的）
    std::vector<std::byte> takeUploadStorage();
    void pollCompletedFrames();
    void releaseCompleted();

    struct PendingUpload
    {
        GLRhiBuffer* buffer = nullptr;
        GLRhiTexture* texture = nullptr;
        std::size_t offset = 0;
        RhiTextureRegion region;
        std::vector<std::byte> data;
    };

    struct InFlightFrame
    {
        std::uint64_t serial = 0;
        GLsync fence = nullptr;
    };

    struct PendingRelease
    {
        std::uint64_t serial = 0;
        std::unique_ptr<RhiResource> resource;
        GLuint retiredBuffer = 0;  ///< 非 0 时这一项是扩容前的环形缓冲
    };

    /// @brief 删除器借它判断设备是否还在
    struct Link
    {
        GLRhiDevice* device = nullptr;
    };

    Options m_options;
    RhiCaps m_caps;
    std::shared_ptr<Link> m_link;
    std::unique_ptr<QOffscreenSurface> m_offscreenSurface;
    std::unique_ptr<QOpenGLContext> m_context;
    std::unordered_map<QOpenGLContext*, std::unique_ptr<GLRhiContextState>> m_contextStates;
    std::unique_ptr<GLRhiCommandList> m_commandList;
    std::unique_ptr<GLRhiUploadRing> m_uploadRing;
    std::vector<PendingUpload> m_pendingUploads;
    std::vector<std::vector<std::byte>> m_uploadStorage;   ///< 待上传数据用过的暂存，复用其容量
    std::deque<InFlightFrame> m_inFlight;
    std::deque<PendingRelease> m_releaseQueue;
    std::uint64_t m_frameSerial = 0;
    std::uint64_t m_completedSerial = 0;
    std::uint64_t m_lastObjectId = 0;
    std::size_t m_liveResources = 0;
    std::atomic<std::size_t> m_debugErrorCount{0};
    bool m_inFrame = false;
    bool m_debugOutput = false;
    /// @brief 离屏帧用设备自己的上下文画，结束时换回开始前的当前上下文
    bool m_offscreenFrame = false;
    QOpenGLContext* m_offscreenPreviousContext = nullptr;
    QSurface* m_offscreenPreviousSurface = nullptr;
    QOpenGLContext* m_frameContext = nullptr;
    QOpenGLContext* m_lastFrameContext = nullptr;
    /// @brief 设备自己的上下文里做过事（建资源、读回）后插的 fence，下一帧在别的上下文里先在 GPU 上等它
    GLsync m_internalFence = nullptr;
};

#endif // GLRHIDEVICE_H
