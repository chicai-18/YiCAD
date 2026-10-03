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

/// @file GLRhiDevice.cpp
/// @brief RHI 的 OpenGL 4.3 core 实现：设备

#include "GLRhiDevice.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <set>
#include <utility>

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QtGlobal>

#include "GLRhiCommandList.h"
#include "GLRhiResources.h"
#include "GLRhiSurface.h"
#include "YiCadLog.h"

#define RHI_ERROR YICAD_LOG(yicad::log::render(), yicad::LogLevel::Error) << "RHI："

namespace
{
/// @brief 上传环形缓冲里每段的对齐（缓冲复制与像素解包都满足）
constexpr std::size_t kUploadAlignment = 256;
/// @brief 等 fence 时每次的超时（纳秒）
constexpr GLuint64 kFenceTimeout = 1'000'000'000;

std::size_t alignUp(std::size_t value, std::size_t alignment)
{
    return (value + alignment - 1) / alignment * alignment;
}

bool debugRequested()
{
#ifndef NDEBUG
    return true;
#else
    return qEnvironmentVariableIntValue("YICAD_GL_DEBUG") != 0;
#endif
}

void GLAPIENTRY debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei /*length*/,
                              const GLchar* message, const void* userParam)
{
    auto* device = static_cast<GLRhiDevice*>(const_cast<void*>(userParam));
    device->onDebugMessage(source, type, id, severity, message);
}

bool isTexelBufferFormat(RhiFormat format)
{
    switch (format)
    {
    case RhiFormat::R8Unorm:
    case RhiFormat::RGBA8Unorm:
    case RhiFormat::R32Float:
    case RhiFormat::RG32Float:
    case RhiFormat::RGBA32Float:
    case RhiFormat::R32Uint:
    case RhiFormat::RG32Uint:
    case RhiFormat::RGBA32Uint:
        return true;
    default:
        return false;
    }
}

std::string programResourceName(GLuint program, GLenum interface, GLuint index)
{
    char name[256] = {};
    glGetProgramResourceName(program, interface, index, sizeof(name), nullptr, name);
    return name;
}

/// @brief 按程序反射核对绑定点：着色器里的每个常量块、存储块、采样器都要落在管线布局给出的平铺绑定点上
bool verifyProgramBindings(GLuint program, const RhiPipelineDesc& desc,
                           const std::vector<std::vector<GLuint>>& flat)
{
    std::set<GLuint> uniformBuffers;
    std::set<GLuint> storageBuffers;
    std::set<GLuint> texelUnits;
    std::set<GLuint> textureUnits;
    for (std::size_t set = 0; set < desc.bindGroupLayouts.size(); ++set)
    {
        if (!desc.bindGroupLayouts[set])
        {
            continue;
        }
        const std::vector<RhiBindGroupLayoutEntry>& entries = desc.bindGroupLayouts[set]->entries();
        for (std::size_t i = 0; i < entries.size(); ++i)
        {
            switch (entries[i].type)
            {
            case RhiBindingType::UniformBuffer: uniformBuffers.insert(flat[set][i]); break;
            case RhiBindingType::StorageBuffer: storageBuffers.insert(flat[set][i]); break;
            case RhiBindingType::TexelBuffer: texelUnits.insert(flat[set][i]); break;
            case RhiBindingType::CombinedTextureSampler: textureUnits.insert(flat[set][i]); break;
            }
        }
    }

    bool ok = true;
    const auto checkBlocks = [&](GLenum interface, const std::set<GLuint>& expected, const char* what)
    {
        GLint count = 0;
        glGetProgramInterfaceiv(program, interface, GL_ACTIVE_RESOURCES, &count);
        for (GLint i = 0; i < count; ++i)
        {
            const GLenum property = GL_BUFFER_BINDING;
            GLint binding = -1;
            glGetProgramResourceiv(program, interface, static_cast<GLuint>(i), 1, &property, 1, nullptr, &binding);
            if (!expected.count(static_cast<GLuint>(binding)))
            {
                RHI_ERROR << "管线 " << desc.debugName << "：" << what << " "
                          << programResourceName(program, interface, static_cast<GLuint>(i)) << " 的绑定点 " << binding
                          << " 不在管线布局里（着色器与 shaders.json 不一致？）";
                ok = false;
            }
        }
    };
    checkBlocks(GL_UNIFORM_BLOCK, uniformBuffers, "常量块");
    checkBlocks(GL_SHADER_STORAGE_BLOCK, storageBuffers, "存储块");

    GLint uniformCount = 0;
    glGetProgramInterfaceiv(program, GL_UNIFORM, GL_ACTIVE_RESOURCES, &uniformCount);
    for (GLint i = 0; i < uniformCount; ++i)
    {
        const GLenum properties[] = {GL_TYPE, GL_BLOCK_INDEX, GL_LOCATION};
        GLint values[3] = {0, -1, -1};
        glGetProgramResourceiv(program, GL_UNIFORM, static_cast<GLuint>(i), 3, properties, 3, nullptr, values);
        if (values[1] != -1)
        {
            continue;  // 块里的成员
        }
        const std::string name = programResourceName(program, GL_UNIFORM, static_cast<GLuint>(i));
        const std::set<GLuint>* expected = nullptr;
        switch (static_cast<GLenum>(values[0]))
        {
        case GL_SAMPLER_BUFFER:
        case GL_INT_SAMPLER_BUFFER:
        case GL_UNSIGNED_INT_SAMPLER_BUFFER:
            expected = &texelUnits;
            break;
        case GL_SAMPLER_2D:
        case GL_INT_SAMPLER_2D:
        case GL_UNSIGNED_INT_SAMPLER_2D:
            expected = &textureUnits;
            break;
        default:
            RHI_ERROR << "管线 " << desc.debugName << "：uniform " << name
                      << " 不在块里或类型不受支持（RHI 只有常量块、存储块、纹素缓冲与二维纹理）";
            ok = false;
            continue;
        }
        GLint unit = -1;
        glGetUniformiv(program, values[2], &unit);
        if (!expected->count(static_cast<GLuint>(unit)))
        {
            RHI_ERROR << "管线 " << desc.debugName << "：采样器 " << name << " 的纹理单元 " << unit
                      << " 不在管线布局里（着色器与 shaders.json 不一致？）";
            ok = false;
        }
    }
    return ok;
}
}  // namespace

// ---------------------------------------------------------------------------
// 上传环形缓冲
// ---------------------------------------------------------------------------

/// @brief 上传用的环形缓冲：按帧分配区段，区段在写它的帧完成后回收
/// @details 持久映射时 CPU 直接写映射的内存（一致映射，写入对之后的 GL 命令可见）；
///          否则每段用 glMapBufferRange 的非同步映射写入，靠 fence 保证不覆盖 GPU 还在读的区段
class GLRhiUploadRing
{
public:
    GLRhiUploadRing(GLRhiDevice& device, bool persistent, std::size_t capacity)
        : m_device(device)
        , m_capacity(capacity)
    {
        glGenBuffers(1, &m_buffer);
        glBindBuffer(GL_COPY_READ_BUFFER, m_buffer);
        if (persistent)
        {
            const GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
            glBufferStorage(GL_COPY_READ_BUFFER, static_cast<GLsizeiptr>(capacity), nullptr, flags);
            m_mapped = static_cast<std::byte*>(
                glMapBufferRange(GL_COPY_READ_BUFFER, 0, static_cast<GLsizeiptr>(capacity), flags));
        }
        else
        {
            glBufferData(GL_COPY_READ_BUFFER, static_cast<GLsizeiptr>(capacity), nullptr, GL_STREAM_DRAW);
        }
        device.setObjectLabel(GL_BUFFER, m_buffer, "RHI upload ring");
    }

    /// @brief 交出 GL 缓冲（扩容时旧缓冲按帧回收）
    GLuint release()
    {
        return std::exchange(m_buffer, 0);
    }

    ~GLRhiUploadRing()
    {
        if (m_buffer != 0)
        {
            glDeleteBuffers(1, &m_buffer);  // 删除时自动解除映射
        }
    }

    std::size_t capacity() const { return m_capacity; }
    GLuint buffer() const { return m_buffer; }

    /// @brief 写入一段数据，返回偏移；本帧的数据已占满整个缓冲时返回 npos，调用方扩容
    std::size_t write(std::span<const std::byte> data, std::uint64_t serial)
    {
        std::size_t offset = 0;
        if (!allocate(data.size(), serial, offset))
        {
            return std::numeric_limits<std::size_t>::max();
        }
        if (m_mapped)
        {
            std::memcpy(m_mapped + offset, data.data(), data.size());
        }
        else
        {
            glBindBuffer(GL_COPY_READ_BUFFER, m_buffer);
            void* target = glMapBufferRange(GL_COPY_READ_BUFFER, static_cast<GLintptr>(offset),
                                            static_cast<GLsizeiptr>(data.size()),
                                            GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
            if (target)
            {
                std::memcpy(target, data.data(), data.size());
            }
            glUnmapBuffer(GL_COPY_READ_BUFFER);
        }
        return offset;
    }

private:
    struct Region
    {
        std::size_t begin = 0;
        std::size_t end = 0;
        std::uint64_t serial = 0;
    };

    bool allocate(std::size_t size, std::uint64_t serial, std::size_t& offset)
    {
        if (size > m_capacity)
        {
            return false;
        }
        for (;;)
        {
            while (!m_regions.empty() && m_regions.front().serial <= m_device.completedFrameSerial())
            {
                m_regions.pop_front();
            }
            if (m_regions.empty())
            {
                m_head = 0;
            }

            std::size_t position = alignUp(m_head, kUploadAlignment);
            bool fits = false;
            if (m_regions.empty())
            {
                fits = position + size <= m_capacity;
            }
            else
            {
                const std::size_t tail = m_regions.front().begin;
                if (m_head > tail)
                {
                    // 占用的是 [tail, head)：先往后放，放不下回绕到开头
                    if (position + size <= m_capacity)
                    {
                        fits = true;
                    }
                    else
                    {
                        position = 0;
                        fits = size <= tail;
                    }
                }
                else
                {
                    // 占用已回绕：空闲的只有 [head, tail)
                    fits = position + size <= tail;
                }
            }
            if (fits)
            {
                m_regions.push_back({position, position + size, serial});
                m_head = position + size;
                offset = position;
                return true;
            }

            const std::uint64_t oldest = m_regions.front().serial;
            if (oldest >= serial)
            {
                return false;  // 本帧自己的数据就放不下
            }
            m_device.waitForFrame(oldest);
        }
    }

    GLRhiDevice& m_device;
    GLuint m_buffer = 0;
    std::size_t m_capacity = 0;
    std::byte* m_mapped = nullptr;
    std::size_t m_head = 0;
    std::deque<Region> m_regions;
};

// ---------------------------------------------------------------------------
// 上下文
// ---------------------------------------------------------------------------

GLRhiDevice::ContextGuard::ContextGuard(GLRhiDevice& device)
    : m_device(device)
{
    QOpenGLContext* current = QOpenGLContext::currentContext();
    if (current && device.isInShareGroup(current))
    {
        return;
    }
    m_previousContext = current;
    m_previousSurface = current ? current->surface() : nullptr;
    device.m_context->makeCurrent(device.m_offscreenSurface.get());
    device.currentContextState();
    m_switched = true;
}

GLRhiDevice::ContextGuard::~ContextGuard()
{
    // 帧内建资源会动纹理、缓冲的绑定点
    if (m_device.m_inFrame)
    {
        m_device.m_commandList->invalidateBindings();
    }
    if (!m_switched)
    {
        return;
    }
    // 设备上下文里做的修改，别的上下文要等它完成才看得到：插 fence，下一帧在帧的上下文里先等它
    if (m_device.m_internalFence)
    {
        glDeleteSync(m_device.m_internalFence);
    }
    m_device.m_internalFence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    glFlush();
    if (m_previousContext)
    {
        m_previousContext->makeCurrent(m_previousSurface);
    }
    else
    {
        m_device.m_context->doneCurrent();
    }
}

QSurfaceFormat GLRhiDevice::surfaceFormat()
{
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setVersion(4, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setOption(QSurfaceFormat::DeprecatedFunctions);
    if (debugRequested())
    {
        format.setOption(QSurfaceFormat::DebugContext);
    }
    return format;
}

GLRhiDevice::Support GLRhiDevice::checkSupport()
{
    Support support;
    QOpenGLContext* previousContext = QOpenGLContext::currentContext();
    QSurface* previousSurface = previousContext ? previousContext->surface() : nullptr;

    QOffscreenSurface surface;
    surface.setFormat(surfaceFormat());
    surface.create();
    QOpenGLContext context;
    context.setFormat(surfaceFormat());
    if (context.create() && context.makeCurrent(&surface))
    {
        support.contextCreated = true;
        const QSurfaceFormat actual = context.format();
        support.majorVersion = actual.majorVersion();
        support.minorVersion = actual.minorVersion();
        support.coreProfile = actual.profile() == QSurfaceFormat::CoreProfile;
        // glGetString 是 GL 1.1 的函数，opengl32.dll 直接导出，不用先初始化 GLEW
        const auto text = [](GLenum name)
        {
            const auto* value = reinterpret_cast<const char*>(glGetString(name));
            return value ? std::string(value) : std::string();
        };
        support.version = text(GL_VERSION);
        support.renderer = text(GL_RENDERER);
        support.ok = support.coreProfile
                     && (support.majorVersion > 4 || (support.majorVersion == 4 && support.minorVersion >= 3));
        context.doneCurrent();
    }

    if (previousContext)
    {
        previousContext->makeCurrent(previousSurface);
    }
    return support;
}

std::unique_ptr<GLRhiDevice> GLRhiDevice::create(const Options& options)
{
    std::unique_ptr<GLRhiDevice> device(new GLRhiDevice(options));
    if (!device->initialize())
    {
        return nullptr;
    }
    return device;
}

GLRhiDevice::GLRhiDevice(const Options& options)
    : m_options(options)
    , m_link(std::make_shared<Link>())
{
    m_link->device = this;
}

bool GLRhiDevice::initialize()
{
    QOpenGLContext* share = QOpenGLContext::globalShareContext();
    if (!share)
    {
        RHI_ERROR << "没有全局共享上下文：要在建 QApplication 之前设置 Qt::AA_ShareOpenGLContexts";
        return false;
    }

    m_offscreenSurface = std::make_unique<QOffscreenSurface>();
    m_offscreenSurface->setFormat(surfaceFormat());
    m_offscreenSurface->create();
    m_context = std::make_unique<QOpenGLContext>();
    m_context->setFormat(surfaceFormat());
    m_context->setShareContext(share);
    if (!m_context->create())
    {
        RHI_ERROR << "建不了 OpenGL 上下文";
        return false;
    }

    QOpenGLContext* previousContext = QOpenGLContext::currentContext();
    QSurface* previousSurface = previousContext ? previousContext->surface() : nullptr;
    if (!m_context->makeCurrent(m_offscreenSurface.get()))
    {
        RHI_ERROR << "OpenGL 上下文不能成为当前";
        return false;
    }

    bool ok = true;
    const QSurfaceFormat actual = m_context->format();
    if (actual.profile() != QSurfaceFormat::CoreProfile
        || actual.majorVersion() < 4 || (actual.majorVersion() == 4 && actual.minorVersion() < 3))
    {
        RHI_ERROR << "需要 OpenGL 4.3 core profile，驱动给出的是 " << actual.majorVersion() << "."
                  << actual.minorVersion();
        ok = false;
    }
    if (ok)
    {
        // core profile 下不设它，GLEW 取不到部分函数（第 4.7.3 节）
        glewExperimental = GL_TRUE;
        const GLenum error = glewInit();
        // glewInit 在 core profile 下会留下一个 GL_INVALID_ENUM（它查了 GL_EXTENSIONS），清掉
        while (glGetError() != GL_NO_ERROR)
        {
        }
        if (error != GLEW_OK)
        {
            RHI_ERROR << "GLEW 初始化失败：" << reinterpret_cast<const char*>(glewGetErrorString(error));
            ok = false;
        }
    }
    if (ok)
    {
        m_debugOutput = actual.testOption(QSurfaceFormat::DebugContext);
        queryCaps();
        currentContextState();
        m_uploadRing = std::make_unique<GLRhiUploadRing>(*this, m_caps.persistentMapping, m_options.uploadRingSize);
        m_commandList = std::make_unique<GLRhiCommandList>(*this);
        glFlush();
    }

    if (previousContext)
    {
        previousContext->makeCurrent(previousSurface);
    }
    else
    {
        m_context->doneCurrent();
    }
    return ok;
}

void GLRhiDevice::queryCaps()
{
    const auto integer = [](GLenum name)
    {
        GLint value = 0;
        glGetIntegerv(name, &value);
        return static_cast<std::uint32_t>(std::max(value, 0));
    };
    const auto integer64 = [](GLenum name)
    {
        GLint64 value = 0;
        glGetInteger64v(name, &value);
        return static_cast<std::size_t>(std::max<GLint64>(value, 0));
    };
    const auto text = [](GLenum name)
    {
        const auto* value = reinterpret_cast<const char*>(glGetString(name));
        return value ? std::string(value) : std::string();
    };

    m_caps.backend = "OpenGL";
    m_caps.apiVersion = text(GL_VERSION);
    m_caps.renderer = text(GL_RENDERER);
    m_caps.vendor = text(GL_VENDOR);
    m_caps.shaderLanguage = RhiShaderLanguage::Glsl430;
    m_caps.maxUniformBufferRange = integer64(GL_MAX_UNIFORM_BLOCK_SIZE);
    m_caps.maxStorageBufferRange = integer64(GL_MAX_SHADER_STORAGE_BLOCK_SIZE);
    m_caps.maxTexelBufferElements = integer64(GL_MAX_TEXTURE_BUFFER_SIZE);
    m_caps.uniformBufferOffsetAlignment = std::max<std::size_t>(integer(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT), 1);
    m_caps.storageBufferOffsetAlignment = std::max<std::size_t>(integer(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT), 1);
    m_caps.texelBufferOffsetAlignment = std::max<std::size_t>(integer(GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT), 1);
    m_caps.maxTextureSize = integer(GL_MAX_TEXTURE_SIZE);
    m_caps.maxColorSamples = std::max(integer(GL_MAX_COLOR_TEXTURE_SAMPLES), 1u);
    m_caps.maxDepthSamples = std::max(integer(GL_MAX_DEPTH_TEXTURE_SAMPLES), 1u);
    m_caps.maxVertexAttributes = integer(GL_MAX_VERTEX_ATTRIBS);
    m_caps.maxVertexBuffers = integer(GL_MAX_VERTEX_ATTRIB_BINDINGS);
    m_caps.maxVertexUniformBuffers = integer(GL_MAX_VERTEX_UNIFORM_BLOCKS);
    m_caps.maxFragmentUniformBuffers = integer(GL_MAX_FRAGMENT_UNIFORM_BLOCKS);
    m_caps.maxVertexStorageBuffers = integer(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS);
    m_caps.maxFragmentStorageBuffers = integer(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS);
    m_caps.maxVertexTextures = integer(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS);
    m_caps.maxFragmentTextures = integer(GL_MAX_TEXTURE_IMAGE_UNITS);
    m_caps.persistentMapping = !m_options.disablePersistentMapping && (GLEW_VERSION_4_4 || GLEW_ARB_buffer_storage);
    m_caps.framebufferOriginBottomLeft = true;
}

GLRhiDevice::~GLRhiDevice()
{
    if (m_inFrame)
    {
        RHI_ERROR << "设备销毁时还有帧没有结束";
    }

    QOpenGLContext* previousContext = QOpenGLContext::currentContext();
    QSurface* previousSurface = previousContext ? previousContext->surface() : nullptr;
    if (m_context && m_context->makeCurrent(m_offscreenSurface.get()))
    {
        while (!m_inFlight.empty())
        {
            waitForFrame(m_inFlight.back().serial);
        }
        m_pendingUploads.clear();
        // 帧都完成了，队列里的全部可以销毁；销毁时又进队列的（绑定组放掉的缓冲）一并处理
        m_completedSerial = std::numeric_limits<std::uint64_t>::max();
        releaseCompleted();
        m_uploadRing.reset();
        if (m_internalFence)
        {
            glDeleteSync(m_internalFence);
            m_internalFence = nullptr;
        }
        // 设备自己上下文里的容器对象；别的上下文里的随上下文销毁
        auto it = m_contextStates.find(m_context.get());
        if (it != m_contextStates.end())
        {
            GLRhiContextState& state = *it->second;
            for (const auto& [id, vertexArray] : state.vertexArrays)
            {
                glDeleteVertexArrays(1, &vertexArray);
            }
            for (const auto& [id, framebuffer] : state.framebuffers)
            {
                glDeleteFramebuffers(1, &framebuffer);
            }
            glDeleteFramebuffers(1, &state.scratchReadFramebuffer);
            glDeleteFramebuffers(1, &state.scratchDrawFramebuffer);
        }
        m_context->doneCurrent();
    }
    if (previousContext && previousContext != m_context.get())
    {
        previousContext->makeCurrent(previousSurface);
    }

    // 之后才释放的句柄不再碰 GL
    m_link->device = nullptr;
    if (m_liveResources != 0)
    {
        RHI_ERROR << "设备销毁时还有 " << m_liveResources << " 个资源没有释放";
    }
    for (auto& [context, state] : m_contextStates)
    {
        QObject::disconnect(state->destroyConnection);
    }
    m_contextStates.clear();
    m_commandList.reset();
    m_context.reset();
    m_offscreenSurface.reset();
}

bool GLRhiDevice::isInShareGroup(QOpenGLContext* context) const
{
    return context && m_context && context->shareGroup() == m_context->shareGroup();
}

GLRhiContextState& GLRhiDevice::contextState(QOpenGLContext* context)
{
    auto it = m_contextStates.find(context);
    if (it != m_contextStates.end())
    {
        return *it->second;
    }

    auto state = std::make_unique<GLRhiContextState>();
    state->context = context;
    // 上下文销毁时它的 VAO、FBO 随之消失，只丢掉记录
    state->destroyConnection = QObject::connect(context, &QOpenGLContext::aboutToBeDestroyed, [this, context]()
    {
        m_contextStates.erase(context);
        if (m_lastFrameContext == context)
        {
            m_lastFrameContext = nullptr;
        }
    });
    if (m_debugOutput)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(&debugCallback, this);
        // 驱动的提示类消息（缓冲放在哪种显存之类）太多，不要
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
    }
    return *m_contextStates.emplace(context, std::move(state)).first->second;
}

GLRhiContextState& GLRhiDevice::currentContextState()
{
    QOpenGLContext* current = QOpenGLContext::currentContext();
    Q_ASSERT(isInShareGroup(current));
    GLRhiContextState& state = contextState(current);
    flushPendingContainerDeletes(state);
    return state;
}

void GLRhiDevice::flushPendingContainerDeletes(GLRhiContextState& state)
{
    if (!state.pendingVertexArrayDeletes.empty())
    {
        glDeleteVertexArrays(static_cast<GLsizei>(state.pendingVertexArrayDeletes.size()),
                             state.pendingVertexArrayDeletes.data());
        state.pendingVertexArrayDeletes.clear();
    }
    if (!state.pendingFramebufferDeletes.empty())
    {
        glDeleteFramebuffers(static_cast<GLsizei>(state.pendingFramebufferDeletes.size()),
                             state.pendingFramebufferDeletes.data());
        state.pendingFramebufferDeletes.clear();
    }
}

void GLRhiDevice::forgetPipeline(std::uint64_t id)
{
    QOpenGLContext* current = QOpenGLContext::currentContext();
    for (auto& [context, state] : m_contextStates)
    {
        auto it = state->vertexArrays.find(id);
        if (it == state->vertexArrays.end())
        {
            continue;
        }
        if (context == current)
        {
            glDeleteVertexArrays(1, &it->second);
        }
        else
        {
            state->pendingVertexArrayDeletes.push_back(it->second);
        }
        state->vertexArrays.erase(it);
    }
}

void GLRhiDevice::forgetRenderTarget(std::uint64_t id)
{
    QOpenGLContext* current = QOpenGLContext::currentContext();
    for (auto& [context, state] : m_contextStates)
    {
        auto it = state->framebuffers.find(id);
        if (it == state->framebuffers.end())
        {
            continue;
        }
        if (context == current)
        {
            glDeleteFramebuffers(1, &it->second);
        }
        else
        {
            state->pendingFramebufferDeletes.push_back(it->second);
        }
        state->framebuffers.erase(it);
    }
}

void GLRhiDevice::onDebugMessage(GLenum /*source*/, GLenum type, GLuint id, GLenum severity, const char* message)
{
    yicad::LogLevel level = yicad::LogLevel::Debug;
    if (type == GL_DEBUG_TYPE_ERROR)
    {
        ++m_debugErrorCount;
        level = yicad::LogLevel::Warning;
    }
    else if (severity == GL_DEBUG_SEVERITY_HIGH || severity == GL_DEBUG_SEVERITY_MEDIUM)
    {
        level = yicad::LogLevel::Info;
    }
    YICAD_LOG(yicad::log::render(), level) << "GL 调试输出 [" << id << "] " << message;
}

void GLRhiDevice::setObjectLabel(GLenum identifier, GLuint name, const std::string& label) const
{
    if (!label.empty() && glObjectLabel)
    {
        glObjectLabel(identifier, name, static_cast<GLsizei>(label.size()), label.data());
    }
}

// ---------------------------------------------------------------------------
// 资源
// ---------------------------------------------------------------------------

template <typename T>
std::shared_ptr<T> GLRhiDevice::adopt(T* resource)
{
    ++m_liveResources;
    std::shared_ptr<Link> link = m_link;
    return std::shared_ptr<T>(resource, [link](T* object)
    {
        if (link->device)
        {
            link->device->deferRelease(object);
        }
        else
        {
            object->abandon();
            delete object;
        }
    });
}

void GLRhiDevice::deferRelease(RhiResource* resource)
{
    m_releaseQueue.push_back({m_frameSerial, std::unique_ptr<RhiResource>(resource), 0});
}

void GLRhiDevice::retireBuffer(GLuint buffer)
{
    m_releaseQueue.push_back({m_frameSerial, nullptr, buffer});
}

void GLRhiDevice::dropPendingUploads(const void* target)
{
    std::erase_if(m_pendingUploads, [target](const PendingUpload& upload)
    {
        return upload.buffer == target || upload.texture == target;
    });
}

std::size_t GLRhiDevice::uploadRingCapacity() const
{
    return m_uploadRing ? m_uploadRing->capacity() : 0;
}

RhiBufferPtr GLRhiDevice::createBuffer(const RhiBufferDesc& desc)
{
    if (desc.size == 0 || desc.usage == RhiBufferUsage::None)
    {
        RHI_ERROR << "缓冲 " << desc.debugName << " 的大小或用途为空";
        return nullptr;
    }
    ContextGuard guard(*this);
    GLuint name = 0;
    glGenBuffers(1, &name);
    glBindBuffer(GL_COPY_WRITE_BUFFER, name);
    const bool readback = desc.memory == RhiMemory::Readback;
    if (m_caps.persistentMapping)
    {
        // 不可变存储：内容只经复制命令写入（上传走环形缓冲），不需要 GL_DYNAMIC_STORAGE_BIT
        const GLbitfield flags = readback ? (GL_MAP_READ_BIT | GL_CLIENT_STORAGE_BIT) : 0;
        glBufferStorage(GL_COPY_WRITE_BUFFER, static_cast<GLsizeiptr>(desc.size), nullptr, flags);
    }
    else
    {
        glBufferData(GL_COPY_WRITE_BUFFER, static_cast<GLsizeiptr>(desc.size), nullptr,
                     readback ? GL_STREAM_READ : GL_DYNAMIC_DRAW);
    }
    setObjectLabel(GL_BUFFER, name, desc.debugName);
    return adopt(new GLRhiBuffer(*this, desc, name));
}

RhiTexturePtr GLRhiDevice::createTexture(const RhiTextureDesc& desc)
{
    const GLRhiFormatInfo info = glRhiFormat(desc.format);
    if (desc.width == 0 || desc.height == 0 || desc.width > m_caps.maxTextureSize
        || desc.height > m_caps.maxTextureSize || info.internalFormat == GL_NONE)
    {
        RHI_ERROR << "纹理 " << desc.debugName << " 的尺寸或格式不对";
        return nullptr;
    }
    const std::uint32_t maxSamples = rhiIsDepthFormat(desc.format) ? m_caps.maxDepthSamples : m_caps.maxColorSamples;
    if (desc.sampleCount == 0 || desc.sampleCount > maxSamples
        || (desc.sampleCount > 1
            && (desc.mipLevels != 1 || rhiAny(desc.usage & (RhiTextureUsage::Sampled | RhiTextureUsage::CopySrc)))))
    {
        RHI_ERROR << "纹理 " << desc.debugName << " 的采样数不对（多重采样纹理只能作附件，经解析后使用）";
        return nullptr;
    }

    ContextGuard guard(*this);
    GLuint name = 0;
    glGenTextures(1, &name);
    if (desc.sampleCount > 1)
    {
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, name);
        glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, static_cast<GLsizei>(desc.sampleCount),
                                  info.internalFormat, static_cast<GLsizei>(desc.width),
                                  static_cast<GLsizei>(desc.height), GL_TRUE);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    }
    else
    {
        const GLsizei levels = static_cast<GLsizei>(std::max(desc.mipLevels, 1u));
        glBindTexture(GL_TEXTURE_2D, name);
        glTexStorage2D(GL_TEXTURE_2D, levels, info.internalFormat, static_cast<GLsizei>(desc.width),
                       static_cast<GLsizei>(desc.height));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, levels - 1);
        // 没配采样器时（texelFetch）纹理也完整；整数纹理只能是 NEAREST
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    setObjectLabel(GL_TEXTURE, name, desc.debugName);
    return adopt(new GLRhiTexture(*this, desc, name));
}

RhiSamplerPtr GLRhiDevice::createSampler(const RhiSamplerDesc& desc)
{
    const auto filter = [](RhiFilter value) { return value == RhiFilter::Linear ? GL_LINEAR : GL_NEAREST; };
    const auto address = [](RhiAddressMode mode)
    {
        switch (mode)
        {
        case RhiAddressMode::Repeat: return GL_REPEAT;
        case RhiAddressMode::MirroredRepeat: return GL_MIRRORED_REPEAT;
        case RhiAddressMode::ClampToEdge: break;
        }
        return GL_CLAMP_TO_EDGE;
    };
    GLint minFilter = filter(desc.minFilter);
    if (desc.mipmapMode == RhiMipmapMode::Nearest)
    {
        minFilter = desc.minFilter == RhiFilter::Linear ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_NEAREST;
    }
    else if (desc.mipmapMode == RhiMipmapMode::Linear)
    {
        minFilter = desc.minFilter == RhiFilter::Linear ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR;
    }

    ContextGuard guard(*this);
    GLuint name = 0;
    glGenSamplers(1, &name);
    glSamplerParameteri(name, GL_TEXTURE_MIN_FILTER, minFilter);
    glSamplerParameteri(name, GL_TEXTURE_MAG_FILTER, filter(desc.magFilter));
    glSamplerParameteri(name, GL_TEXTURE_WRAP_S, address(desc.addressU));
    glSamplerParameteri(name, GL_TEXTURE_WRAP_T, address(desc.addressV));
    return adopt(new GLRhiSampler(*this, desc, name));
}

RhiShaderPtr GLRhiDevice::createShader(const RhiShaderDesc& desc)
{
    GLenum type = GL_NONE;
    if (desc.stage == RhiShaderStage::Vertex)
    {
        type = GL_VERTEX_SHADER;
    }
    else if (desc.stage == RhiShaderStage::Fragment)
    {
        type = GL_FRAGMENT_SHADER;
    }
    // SPIR-V 的魔数 0x07230203（小端序）
    const bool isSpirV = desc.code.size() >= 4 && desc.code[0] == std::byte{0x03} && desc.code[1] == std::byte{0x02}
                         && desc.code[2] == std::byte{0x23} && desc.code[3] == std::byte{0x07};
    if (type == GL_NONE || desc.code.empty() || isSpirV)
    {
        RHI_ERROR << "着色器 " << desc.debugName << "：GL 后端要单个阶段的 GLSL 430 源码";
        return nullptr;
    }

    ContextGuard guard(*this);
    const GLuint name = glCreateShader(type);
    const auto* source = reinterpret_cast<const GLchar*>(desc.code.data());
    const auto length = static_cast<GLint>(desc.code.size());
    glShaderSource(name, 1, &source, &length);
    glCompileShader(name);
    GLint status = GL_FALSE;
    glGetShaderiv(name, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE)
    {
        GLint logLength = 0;
        glGetShaderiv(name, GL_INFO_LOG_LENGTH, &logLength);
        std::string log(static_cast<std::size_t>(std::max(logLength, 1)), '\0');
        glGetShaderInfoLog(name, logLength, nullptr, log.data());
        RHI_ERROR << "着色器 " << desc.debugName << " 编译失败：" << log;
        glDeleteShader(name);
        return nullptr;
    }
    setObjectLabel(GL_SHADER, name, desc.debugName);
    return adopt(new GLRhiShader(*this, desc.stage, name));
}

RhiBindGroupLayoutPtr GLRhiDevice::createBindGroupLayout(const RhiBindGroupLayoutDesc& desc)
{
    std::set<std::uint32_t> bindings;
    for (const RhiBindGroupLayoutEntry& entry : desc.entries)
    {
        if (!bindings.insert(entry.binding).second)
        {
            RHI_ERROR << "绑定组布局 " << desc.debugName << " 的绑定号 " << entry.binding << " 重复";
            return nullptr;
        }
        if (entry.hasDynamicOffset && entry.type != RhiBindingType::UniformBuffer
            && entry.type != RhiBindingType::StorageBuffer)
        {
            RHI_ERROR << "绑定组布局 " << desc.debugName << "：只有常量缓冲与存储缓冲可以带动态偏移";
            return nullptr;
        }
    }
    return adopt(new GLRhiBindGroupLayout(*this, desc.entries));
}

RhiBindGroupPtr GLRhiDevice::createBindGroup(const RhiBindGroupDesc& desc)
{
    if (!desc.layout)
    {
        RHI_ERROR << "绑定组 " << desc.debugName << " 没有布局";
        return nullptr;
    }
    const std::vector<RhiBindGroupLayoutEntry>& layoutEntries = desc.layout->entries();
    if (desc.entries.size() != layoutEntries.size())
    {
        RHI_ERROR << "绑定组 " << desc.debugName << " 的项数与布局不同";
        return nullptr;
    }

    ContextGuard guard(*this);
    std::vector<GLRhiBindGroup::Binding> bindings;
    const auto fail = [&](const std::string& message) -> RhiBindGroupPtr
    {
        for (const GLRhiBindGroup::Binding& binding : bindings)
        {
            if (binding.type == RhiBindingType::TexelBuffer && binding.texture != 0)
            {
                glDeleteTextures(1, &binding.texture);
            }
        }
        RHI_ERROR << "绑定组 " << desc.debugName << "：" << message;
        return nullptr;
    };

    for (const RhiBindGroupLayoutEntry& layoutEntry : layoutEntries)
    {
        const auto it = std::find_if(desc.entries.begin(), desc.entries.end(),
            [&](const RhiBindGroupEntry& entry) { return entry.binding == layoutEntry.binding; });
        const std::string where = "绑定 " + std::to_string(layoutEntry.binding);
        if (it == desc.entries.end())
        {
            return fail(where + " 没有给出");
        }
        const RhiBindGroupEntry& entry = *it;
        GLRhiBindGroup::Binding binding;
        binding.type = layoutEntry.type;
        binding.hasDynamicOffset = layoutEntry.hasDynamicOffset;

        if (layoutEntry.type == RhiBindingType::CombinedTextureSampler)
        {
            if (!entry.texture || !entry.sampler)
            {
                return fail(where + " 要纹理与采样器");
            }
            const auto& texture = static_cast<const GLRhiTexture&>(*entry.texture);
            if (!rhiAny(texture.desc().usage & RhiTextureUsage::Sampled) || texture.sampleCount() != 1)
            {
                return fail(where + " 的纹理要有 Sampled 用途且是单采样");
            }
            binding.texture = texture.name();
            binding.textureTarget = GL_TEXTURE_2D;
            binding.sampler = static_cast<const GLRhiSampler&>(*entry.sampler).name();
            bindings.push_back(binding);
            continue;
        }

        if (!entry.buffer)
        {
            return fail(where + " 要缓冲");
        }
        const auto& buffer = static_cast<const GLRhiBuffer&>(*entry.buffer);
        const std::size_t size = entry.size != 0 ? entry.size
                                 : (entry.offset < buffer.size() ? buffer.size() - entry.offset : 0);
        if (size == 0 || entry.offset + size > buffer.size())
        {
            return fail(where + " 的范围超出缓冲");
        }
        if (layoutEntry.hasDynamicOffset && entry.size == 0)
        {
            return fail(where + " 带动态偏移，要给出大小");
        }
        std::size_t alignment = 1;
        RhiBufferUsage usage = RhiBufferUsage::None;
        std::size_t maxRange = 0;
        switch (layoutEntry.type)
        {
        case RhiBindingType::UniformBuffer:
            alignment = m_caps.uniformBufferOffsetAlignment;
            usage = RhiBufferUsage::Uniform;
            maxRange = m_caps.maxUniformBufferRange;
            break;
        case RhiBindingType::StorageBuffer:
            alignment = m_caps.storageBufferOffsetAlignment;
            usage = RhiBufferUsage::Storage;
            maxRange = m_caps.maxStorageBufferRange;
            break;
        case RhiBindingType::TexelBuffer:
            alignment = m_caps.texelBufferOffsetAlignment;
            usage = RhiBufferUsage::Texel;
            maxRange = std::numeric_limits<std::size_t>::max();
            break;
        case RhiBindingType::CombinedTextureSampler:
            break;
        }
        if (!rhiAny(buffer.desc().usage & usage))
        {
            return fail(where + " 的缓冲没有对应的用途");
        }
        if (entry.offset % alignment != 0)
        {
            return fail(where + " 的偏移没有按 " + std::to_string(alignment) + " 对齐");
        }
        if (size > maxRange)
        {
            return fail(where + " 的大小超过设备上限");
        }
        binding.buffer = buffer.name();
        binding.offset = static_cast<GLintptr>(entry.offset);
        binding.size = static_cast<GLsizeiptr>(size);

        if (layoutEntry.type == RhiBindingType::TexelBuffer)
        {
            const std::uint32_t texelSize = rhiFormatSize(entry.texelFormat);
            if (!isTexelBufferFormat(entry.texelFormat) || size % texelSize != 0
                || size / texelSize > m_caps.maxTexelBufferElements)
            {
                return fail(where + " 的纹素格式或大小不对");
            }
            glGenTextures(1, &binding.texture);
            glBindTexture(GL_TEXTURE_BUFFER, binding.texture);
            glTexBufferRange(GL_TEXTURE_BUFFER, glRhiFormat(entry.texelFormat).internalFormat, buffer.name(),
                             binding.offset, binding.size);
            glBindTexture(GL_TEXTURE_BUFFER, 0);
            binding.textureTarget = GL_TEXTURE_BUFFER;
        }
        bindings.push_back(binding);
    }
    return adopt(new GLRhiBindGroup(*this, desc, std::move(bindings)));
}

RhiPipelinePtr GLRhiDevice::createPipeline(const RhiPipelineDesc& desc)
{
    if (!desc.vertexShader || desc.vertexShader->stage() != RhiShaderStage::Vertex || !desc.fragmentShader
        || desc.fragmentShader->stage() != RhiShaderStage::Fragment)
    {
        RHI_ERROR << "管线 " << desc.debugName << " 要顶点与片段着色器";
        return nullptr;
    }
    if (desc.bindGroupLayouts.size() > 4 || desc.vertexBuffers.size() > 16
        || desc.vertexBuffers.size() > m_caps.maxVertexBuffers || desc.sampleCount == 0)
    {
        RHI_ERROR << "管线 " << desc.debugName << " 的绑定组、顶点缓冲或采样数超出范围";
        return nullptr;
    }

    ContextGuard guard(*this);
    const GLuint program = glCreateProgram();
    const GLuint vertex = static_cast<const GLRhiShader&>(*desc.vertexShader).name();
    const GLuint fragment = static_cast<const GLRhiShader&>(*desc.fragmentShader).name();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDetachShader(program, vertex);
    glDetachShader(program, fragment);
    GLint status = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE)
    {
        GLint logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        std::string log(static_cast<std::size_t>(std::max(logLength, 1)), '\0');
        glGetProgramInfoLog(program, logLength, nullptr, log.data());
        RHI_ERROR << "管线 " << desc.debugName << " 链接失败：" << log;
        glDeleteProgram(program);
        return nullptr;
    }

    std::vector<std::vector<GLuint>> flat = GLRhiPipeline::computeFlatBindings(desc.bindGroupLayouts);
    if (!verifyProgramBindings(program, desc, flat))
    {
        glDeleteProgram(program);
        return nullptr;
    }
    setObjectLabel(GL_PROGRAM, program, desc.debugName);
    return adopt(new GLRhiPipeline(*this, desc, program, std::move(flat)));
}

RhiRenderTargetPtr GLRhiDevice::createRenderTarget(const RhiRenderTargetDesc& desc)
{
    const RhiTexture* first = !desc.colorAttachments.empty() ? desc.colorAttachments.front().get()
                                                             : desc.depthStencil.get();
    if (!first)
    {
        RHI_ERROR << "渲染目标 " << desc.debugName << " 没有附件";
        return nullptr;
    }
    const auto sameShape = [first](const RhiTexture& texture)
    {
        return texture.width() == first->width() && texture.height() == first->height()
               && texture.sampleCount() == first->sampleCount();
    };
    for (const RhiTexturePtr& color : desc.colorAttachments)
    {
        if (!color || !sameShape(*color) || rhiIsDepthFormat(color->format())
            || !rhiAny(color->desc().usage & RhiTextureUsage::ColorTarget))
        {
            RHI_ERROR << "渲染目标 " << desc.debugName << " 的颜色附件不对（尺寸、采样数、格式或用途）";
            return nullptr;
        }
    }
    if (desc.depthStencil
        && (!sameShape(*desc.depthStencil) || !rhiIsDepthFormat(desc.depthStencil->format())
            || !rhiAny(desc.depthStencil->desc().usage & RhiTextureUsage::DepthStencil)))
    {
        RHI_ERROR << "渲染目标 " << desc.debugName << " 的深度附件不对（尺寸、采样数、格式或用途）";
        return nullptr;
    }
    return adopt(new GLRhiRenderTarget(*this, desc));
}

// ---------------------------------------------------------------------------
// 上传
// ---------------------------------------------------------------------------

void GLRhiDevice::upload(RhiBuffer& dst, std::size_t offset, std::span<const std::byte> data)
{
    if (data.empty())
    {
        return;
    }
    if (dst.desc().memory != RhiMemory::Device || offset + data.size() > dst.size())
    {
        RHI_ERROR << "上传到缓冲 " << dst.desc().debugName << " 的范围不对，或缓冲不是 Device 内存";
        return;
    }
    PendingUpload upload;
    upload.buffer = &static_cast<GLRhiBuffer&>(dst);
    upload.offset = offset;
    upload.data.assign(data.begin(), data.end());
    m_pendingUploads.push_back(std::move(upload));
}

void GLRhiDevice::upload(RhiTexture& dst, const RhiTextureRegion& region, std::span<const std::byte> data)
{
    const std::uint32_t levelWidth = std::max(dst.width() >> region.mipLevel, 1u);
    const std::uint32_t levelHeight = std::max(dst.height() >> region.mipLevel, 1u);
    const std::size_t expected = static_cast<std::size_t>(region.width) * region.height * rhiFormatSize(dst.format());
    if (dst.sampleCount() != 1 || rhiIsDepthFormat(dst.format()) || region.mipLevel >= dst.desc().mipLevels
        || region.x + region.width > levelWidth || region.y + region.height > levelHeight
        || data.size() != expected || expected == 0)
    {
        RHI_ERROR << "上传到纹理 " << dst.desc().debugName << " 的区域或数据大小不对";
        return;
    }
    PendingUpload upload;
    upload.texture = &static_cast<GLRhiTexture&>(dst);
    upload.region = region;
    upload.data.assign(data.begin(), data.end());
    m_pendingUploads.push_back(std::move(upload));
}

void GLRhiDevice::flushUploads()
{
    for (const PendingUpload& upload : m_pendingUploads)
    {
        std::size_t offset = m_uploadRing->write(upload.data, m_frameSerial);
        if (offset == std::numeric_limits<std::size_t>::max())
        {
            // 本帧的上传放不下：换一个更大的，旧的等本帧完成后删除
            const std::size_t capacity = std::max(m_uploadRing->capacity() * 2,
                                                  alignUp(upload.data.size() * 2, kUploadAlignment));
            retireBuffer(m_uploadRing->release());
            m_uploadRing = std::make_unique<GLRhiUploadRing>(*this, m_caps.persistentMapping, capacity);
            offset = m_uploadRing->write(upload.data, m_frameSerial);
        }

        if (upload.buffer)
        {
            glBindBuffer(GL_COPY_READ_BUFFER, m_uploadRing->buffer());
            glBindBuffer(GL_COPY_WRITE_BUFFER, upload.buffer->name());
            glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, static_cast<GLintptr>(offset),
                                static_cast<GLintptr>(upload.offset), static_cast<GLsizeiptr>(upload.data.size()));
        }
        else
        {
            const GLRhiFormatInfo info = glRhiFormat(upload.texture->format());
            const RhiTextureRegion& r = upload.region;
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_uploadRing->buffer());
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, upload.texture->name());
            glTexSubImage2D(GL_TEXTURE_2D, static_cast<GLint>(r.mipLevel), static_cast<GLint>(r.x),
                            static_cast<GLint>(r.y), static_cast<GLsizei>(r.width), static_cast<GLsizei>(r.height),
                            info.format, info.type, reinterpret_cast<const void*>(offset));
            glBindTexture(GL_TEXTURE_2D, 0);
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        }
    }
    m_pendingUploads.clear();
}

// ---------------------------------------------------------------------------
// 帧
// ---------------------------------------------------------------------------

RhiCommandList& GLRhiDevice::beginFrame(RhiSurface& surface)
{
    auto& glSurface = static_cast<GLRhiSurface&>(surface);
    glSurface.makeCurrent();
    QOpenGLContext* context = glSurface.context();
    if (!isInShareGroup(context))
    {
        RHI_ERROR << "表面的上下文不在设备的共享组里：要在建 QApplication 之前设置 Qt::AA_ShareOpenGLContexts";
    }
    m_offscreenFrame = false;
    return startFrame(context, &glSurface);
}

RhiCommandList& GLRhiDevice::beginOffscreenFrame()
{
    m_offscreenPreviousContext = QOpenGLContext::currentContext();
    m_offscreenPreviousSurface = m_offscreenPreviousContext ? m_offscreenPreviousContext->surface() : nullptr;
    m_context->makeCurrent(m_offscreenSurface.get());
    m_offscreenFrame = true;
    return startFrame(m_context.get(), nullptr);
}

RhiCommandList& GLRhiDevice::startFrame(QOpenGLContext* context, GLRhiSurface* surface)
{
    if (m_inFrame)
    {
        RHI_ERROR << "上一帧没有调用 endFrame";
    }
    GLRhiContextState& state = currentContextState();

    // 共享对象在别的上下文里改过：在 GPU 上等那边完成（GL 4.3 规范附录 D）
    if (m_internalFence)
    {
        if (context != m_context.get())
        {
            glWaitSync(m_internalFence, 0, GL_TIMEOUT_IGNORED);
        }
        glDeleteSync(m_internalFence);
        m_internalFence = nullptr;
    }
    if (m_lastFrameContext && m_lastFrameContext != context && !m_inFlight.empty())
    {
        glWaitSync(m_inFlight.back().fence, 0, GL_TIMEOUT_IGNORED);
    }

    ++m_frameSerial;
    m_inFrame = true;
    m_frameContext = context;
    pollCompletedFrames();
    releaseCompleted();
    resetGlState();
    flushUploads();
    m_commandList->begin(state, surface);
    return *m_commandList;
}

void GLRhiDevice::resetGlState()
{
    // 同一个上下文里别的代码（QPainter、旧渲染器）可能留下各种状态
    glDisable(GL_CULL_FACE);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_PRIMITIVE_RESTART);
    glDisable(GL_RASTERIZER_DISCARD);
    glDisable(GL_FRAMEBUFFER_SRGB);
    glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glLineWidth(1.0f);
}

void GLRhiDevice::endFrame()
{
    if (!m_inFrame)
    {
        RHI_ERROR << "endFrame 之前没有开始帧";
        return;
    }
    m_commandList->end();
    const GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    // 别的上下文会等这个 fence，先把本上下文的命令送出去，免得互等
    glFlush();
    m_inFlight.push_back({m_frameSerial, fence});
    m_lastFrameContext = m_frameContext;
    m_inFrame = false;
    m_frameContext = nullptr;
    pollCompletedFrames();
    releaseCompleted();

    if (m_offscreenFrame)
    {
        if (m_offscreenPreviousContext && m_offscreenPreviousContext != m_context.get())
        {
            m_offscreenPreviousContext->makeCurrent(m_offscreenPreviousSurface);
        }
        else
        {
            m_context->doneCurrent();
        }
        m_offscreenPreviousContext = nullptr;
        m_offscreenPreviousSurface = nullptr;
        m_offscreenFrame = false;
    }
}

void GLRhiDevice::pollCompletedFrames()
{
    while (!m_inFlight.empty())
    {
        const GLenum result = glClientWaitSync(m_inFlight.front().fence, 0, 0);
        if (result != GL_ALREADY_SIGNALED && result != GL_CONDITION_SATISFIED)
        {
            break;
        }
        m_completedSerial = m_inFlight.front().serial;
        glDeleteSync(m_inFlight.front().fence);
        m_inFlight.pop_front();
    }
}

void GLRhiDevice::waitForFrame(std::uint64_t serial)
{
    while (!m_inFlight.empty() && m_inFlight.front().serial <= serial)
    {
        const GLsync fence = m_inFlight.front().fence;
        GLenum result = GL_TIMEOUT_EXPIRED;
        while (result == GL_TIMEOUT_EXPIRED)
        {
            result = glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, kFenceTimeout);
        }
        if (result == GL_WAIT_FAILED)
        {
            RHI_ERROR << "等待第 " << m_inFlight.front().serial << " 帧的 fence 失败";
        }
        m_completedSerial = m_inFlight.front().serial;
        glDeleteSync(fence);
        m_inFlight.pop_front();
    }
}

void GLRhiDevice::releaseCompleted()
{
    while (!m_releaseQueue.empty() && m_releaseQueue.front().serial <= m_completedSerial)
    {
        // 先出队再销毁：销毁绑定组时它放掉的缓冲会再进队列
        PendingRelease item = std::move(m_releaseQueue.front());
        m_releaseQueue.pop_front();
        if (item.retiredBuffer != 0)
        {
            glDeleteBuffers(1, &item.retiredBuffer);
        }
        item.resource.reset();
    }
}

void GLRhiDevice::waitIdle()
{
    if (m_inFrame)
    {
        RHI_ERROR << "帧内不能 waitIdle";
        return;
    }
    ContextGuard guard(*this);
    waitForFrame(m_frameSerial);
    releaseCompleted();
}

bool GLRhiDevice::readBuffer(const RhiBuffer& buffer, std::size_t offset, std::span<std::byte> out)
{
    if (m_inFrame)
    {
        RHI_ERROR << "帧内不能读回";
        return false;
    }
    if (buffer.desc().memory != RhiMemory::Readback || offset + out.size() > buffer.size())
    {
        RHI_ERROR << "读回缓冲 " << buffer.desc().debugName << " 的范围不对，或缓冲不是 Readback 内存";
        return false;
    }
    ContextGuard guard(*this);
    waitForFrame(m_frameSerial);
    glBindBuffer(GL_COPY_READ_BUFFER, static_cast<const GLRhiBuffer&>(buffer).name());
    glGetBufferSubData(GL_COPY_READ_BUFFER, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(out.size()),
                       out.data());
    return true;
}

const glm::mat4& GLRhiDevice::clipSpaceCorrection() const
{
    // 投影本来就按 GL 写，不用换
    static const glm::mat4 identity(1.0f);
    return identity;
}
