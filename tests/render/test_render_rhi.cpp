/// @file test_render_rhi.cpp
/// @brief RHI 一致性测试（doc/RENDER_PLAN.md 第 5 节 3.4 步）
///
/// 在 Mesa llvmpipe 上检查 RHI 的 GL 实现：缓冲上传与读回（含上传环形缓冲的回绕与扩容）、每种绑定类型、
/// 多重间接绘制配合 baseInstance 的实例属性、裁剪空间校正与左上角原点的视口与裁剪矩形、离屏目标读回、
/// 多重采样解析、延迟释放，以及画到 QOpenGLWidget 的表面。每个用例在持久映射与非同步映射两条上传路径上各跑一遍，
/// 结束时要求 GL 调试输出没有报错、全部资源都已销毁。
///
/// 着色器在 tests/render/shaders/，构建时经工具链（tools/compile_shaders.py）编译，
/// 布局头文件 RhiTestShaders.h 是同一次生成的。

// GLEW 必须先于 Qt 拉入的 gl.h
#include "GLRhiDevice.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <vector>

#include <QImage>
#include <QOpenGLWidget>

#include "GLRhiSurface.h"
#include "RhiShaderLibrary.h"
#include "RhiTestShaders.h"

#ifndef YICAD_RHI_TEST_SHADER_DIR
#error "test_render 需要 YICAD_RHI_TEST_SHADER_DIR（构建期编译出的测试着色器目录）"
#endif

namespace
{
constexpr std::uint32_t kSize = 64;

/// @brief 着色器里的 Frame 常量块（std140）
struct FrameConstants
{
    glm::mat4 transform{1.0f};
    glm::vec4 color{1.0f};
    glm::vec4 sampleUv{0.0f};
};
static_assert(sizeof(FrameConstants) == 96);

/// @brief records 着色器的纹素缓冲：xy 平移、z 缩放
using Placement = glm::vec4;

template <typename T>
std::span<const std::byte> bytes(const std::vector<T>& values)
{
    return std::as_bytes(std::span<const T>(values));
}

/// @brief 正方形 [-1, 1]²，两个三角形
const std::vector<glm::vec2> kQuad = {{-1, -1}, {1, -1}, {1, 1}, {-1, -1}, {1, 1}, {-1, 1}};

/// @brief 像素在容差内等于期望颜色
::testing::AssertionResult pixelIs(const QImage& image, int x, int y, QColor expected, int tolerance = 2)
{
    const QColor actual = image.pixelColor(x, y);
    if (std::abs(actual.red() - expected.red()) <= tolerance && std::abs(actual.green() - expected.green()) <= tolerance
        && std::abs(actual.blue() - expected.blue()) <= tolerance
        && std::abs(actual.alpha() - expected.alpha()) <= tolerance)
    {
        return ::testing::AssertionSuccess();
    }
    return ::testing::AssertionFailure() << "像素 (" << x << ", " << y << ") 是 (" << actual.red() << ", "
                                         << actual.green() << ", " << actual.blue() << ", " << actual.alpha()
                                         << ")，期望 (" << expected.red() << ", " << expected.green() << ", "
                                         << expected.blue() << ", " << expected.alpha() << ")";
}

/// @brief 一个程序的着色器与绑定组布局
struct Program
{
    RhiProgramShaders shaders;
    std::vector<RhiBindGroupLayoutPtr> layouts;
};

/// @brief 离屏渲染目标；多重采样时另有解析目标
struct Target
{
    RhiTexturePtr color;
    RhiTexturePtr resolve;
    RhiRenderTargetPtr target;
    RhiBufferPtr readback;
};

/// @brief 第 0 组：常量缓冲里依次放几份 FrameConstants，绘制时按动态偏移选一份
struct FrameSlots
{
    RhiBufferPtr buffer;
    RhiBindGroupPtr group;
    std::size_t stride = 0;

    std::uint32_t offset(std::size_t slot) const { return static_cast<std::uint32_t>(slot * stride); }
};

class RhiTest : public ::testing::TestWithParam<bool>
{
protected:
    void SetUp() override
    {
        GLRhiDevice::Options options;
        options.disablePersistentMapping = !GetParam();
        options.uploadRingSize = 64u * 1024u;  // 小一点，用例里才回绕、扩容
        device = GLRhiDevice::create(options);
        ASSERT_TRUE(device) << "建不了 GL 设备（YICAD_LOG=render:debug 看原因）";
        if (GetParam() && !device->caps().persistentMapping)
        {
            GTEST_SKIP() << "驱动没有 ARB_buffer_storage";
        }
        ASSERT_EQ(device->caps().persistentMapping, GetParam());
    }

    void TearDown() override
    {
        if (!device)
        {
            return;
        }
        device->waitIdle();
        EXPECT_EQ(device->debugErrorCount(), 0u) << "GL 调试输出报告了错误（YICAD_LOG=render:warning 看详情）";
        EXPECT_EQ(device->pendingReleaseCount(), 0u);
        EXPECT_EQ(device->liveResourceCount(), 0u) << "用例结束时还有资源没有销毁";
        device.reset();
    }

    RhiBufferPtr buffer(std::size_t size, RhiBufferUsage usage, RhiMemory memory = RhiMemory::Device)
    {
        RhiBufferDesc desc;
        desc.size = size;
        desc.usage = usage;
        desc.memory = memory;
        RhiBufferPtr result = device->createBuffer(desc);
        EXPECT_TRUE(result);
        return result;
    }

    template <typename T>
    RhiBufferPtr bufferWith(const std::vector<T>& values, RhiBufferUsage usage)
    {
        RhiBufferPtr result = buffer(values.size() * sizeof(T), usage);
        device->upload(*result, 0, bytes(values));
        return result;
    }

    Program load(const RhiTestShaders::Program& program)
    {
        Program result;
        result.shaders = rhiLoadProgram(*device, YICAD_RHI_TEST_SHADER_DIR, program.name);
        EXPECT_TRUE(result.shaders.isValid()) << "读不到或编译不了着色器 " << program.name;
        result.layouts = rhiCreateBindGroupLayouts(*device, program.bindGroups);
        return result;
    }

    /// @brief solid 程序：每个顶点一个 vec2
    RhiPipelinePtr solidPipeline(const Program& program, std::uint32_t samples = 1,
                                 RhiFormat depth = RhiFormat::Undefined)
    {
        RhiPipelineDesc desc;
        desc.vertexShader = program.shaders.vertex;
        desc.fragmentShader = program.shaders.fragment;
        desc.bindGroupLayouts = program.layouts;
        desc.vertexBuffers = {{sizeof(glm::vec2), RhiVertexStepMode::Vertex, {{0, RhiVertexFormat::Float2, 0}}}};
        desc.colorTargets = {RhiColorTargetState{}};
        desc.depthStencil.format = depth;
        desc.sampleCount = samples;
        desc.debugName = "solid";
        return device->createPipeline(desc);
    }

    /// @brief records 程序：槽 0 每个顶点一个 vec2，槽 1 每个实例一个记录序号
    RhiPipelinePtr recordsPipeline(const Program& program)
    {
        RhiPipelineDesc desc;
        desc.vertexShader = program.shaders.vertex;
        desc.fragmentShader = program.shaders.fragment;
        desc.bindGroupLayouts = program.layouts;
        desc.vertexBuffers = {
            {sizeof(glm::vec2), RhiVertexStepMode::Vertex, {{0, RhiVertexFormat::Float2, 0}}},
            {sizeof(std::uint32_t), RhiVertexStepMode::Instance, {{1, RhiVertexFormat::Uint, 0}}},
        };
        desc.colorTargets = {RhiColorTargetState{}};
        desc.debugName = "records";
        return device->createPipeline(desc);
    }

    FrameSlots frameSlots(const RhiBindGroupLayoutPtr& layout, const std::vector<FrameConstants>& constants)
    {
        FrameSlots result;
        const std::size_t alignment = device->caps().uniformBufferOffsetAlignment;
        result.stride = (sizeof(FrameConstants) + alignment - 1) / alignment * alignment;
        std::vector<std::byte> data(result.stride * constants.size());
        for (std::size_t i = 0; i < constants.size(); ++i)
        {
            std::memcpy(data.data() + i * result.stride, &constants[i], sizeof(FrameConstants));
        }
        result.buffer = bufferWith(data, RhiBufferUsage::Uniform);
        RhiBindGroupDesc desc;
        desc.layout = layout;
        desc.entries = {{.binding = 0, .buffer = result.buffer, .offset = 0, .size = sizeof(FrameConstants)}};
        result.group = device->createBindGroup(desc);
        EXPECT_TRUE(result.group);
        return result;
    }

    Target makeTarget(std::uint32_t samples = 1)
    {
        Target result;
        RhiTextureDesc color;
        color.width = kSize;
        color.height = kSize;
        color.format = RhiFormat::RGBA8Unorm;
        color.sampleCount = samples;
        color.usage = RhiTextureUsage::ColorTarget | (samples == 1 ? RhiTextureUsage::CopySrc : RhiTextureUsage::None);
        result.color = device->createTexture(color);
        EXPECT_TRUE(result.color);
        if (samples > 1)
        {
            RhiTextureDesc resolve = color;
            resolve.sampleCount = 1;
            resolve.usage = RhiTextureUsage::ColorTarget | RhiTextureUsage::CopySrc;
            result.resolve = device->createTexture(resolve);
            EXPECT_TRUE(result.resolve);
        }
        RhiRenderTargetDesc target;
        target.colorAttachments = {result.color};
        result.target = device->createRenderTarget(target);
        EXPECT_TRUE(result.target);
        result.readback = buffer(kSize * kSize * 4, RhiBufferUsage::CopyDst, RhiMemory::Readback);
        return result;
    }

    static RhiRenderPassDesc passTo(const Target& target, std::array<float, 4> clear = {0.0f, 0.0f, 0.0f, 1.0f})
    {
        RhiRenderPassDesc desc;
        desc.target = target.target.get();
        RhiColorAttachmentOps ops;
        ops.clearColor = clear;
        ops.resolveTarget = target.resolve.get();
        desc.colorOps = {ops};
        return desc;
    }

    /// @brief 读回目标（多重采样时读解析目标），按 RhiCaps 翻成第 0 行在画面顶部
    QImage read(const Target& target)
    {
        RhiCommandList& commands = device->beginOffscreenFrame();
        commands.copyTextureToBuffer(target.resolve ? *target.resolve : *target.color, *target.readback);
        device->endFrame();
        QImage image(kSize, kSize, QImage::Format_RGBA8888);
        const std::span<std::byte> out(reinterpret_cast<std::byte*>(image.bits()),
                                       static_cast<std::size_t>(image.sizeInBytes()));
        EXPECT_TRUE(device->readBuffer(*target.readback, 0, out));
        return device->caps().framebufferOriginBottomLeft ? image.mirrored(false, true) : image;
    }

    std::unique_ptr<GLRhiDevice> device;
};

std::string uploadPathName(const ::testing::TestParamInfo<bool>& info)
{
    return info.param ? "PersistentMapping" : "MapBufferRange";
}

/// @brief 在 paintGL 里经 RHI 画的 QOpenGLWidget
class RhiTestWidget : public QOpenGLWidget
{
public:
    std::function<void(RhiSurface&)> paint;

protected:
    void paintGL() override
    {
        GLRhiWidgetSurface surface(*this);
        paint(surface);
    }
};
}  // namespace

TEST_P(RhiTest, 缓冲上传后经复制读回)
{
    std::vector<std::uint8_t> data(3000);
    for (std::size_t i = 0; i < data.size(); ++i)
    {
        data[i] = static_cast<std::uint8_t>(i * 7 % 251);
    }
    const RhiBufferPtr source = buffer(data.size(), RhiBufferUsage::CopySrc);
    const RhiBufferPtr readback = buffer(data.size(), RhiBufferUsage::CopyDst, RhiMemory::Readback);
    device->upload(*source, 0, bytes(data));
    // 后上传的覆盖先上传的
    const std::vector<std::uint8_t> patch(50, 0xAB);
    device->upload(*source, 100, bytes(patch));
    std::vector<std::uint8_t> expected = data;
    std::fill(expected.begin() + 100, expected.begin() + 150, std::uint8_t{0xAB});

    RhiCommandList& commands = device->beginOffscreenFrame();
    commands.copyBuffer(*source, 0, *readback, 0, data.size());
    device->endFrame();

    std::vector<std::uint8_t> actual(data.size());
    ASSERT_TRUE(device->readBuffer(*readback, 0, std::as_writable_bytes(std::span(actual))));
    EXPECT_EQ(actual, expected);
}

TEST_P(RhiTest, 上传环形缓冲回绕与扩容)
{
    // 40 帧各上传 5000 字节，共约 200 KB，经过 64 KB 的环形缓冲：要回绕，并等前面的帧完成
    constexpr std::size_t kChunk = 5000;
    constexpr std::size_t kFrames = 40;
    const RhiBufferPtr source = buffer(kChunk, RhiBufferUsage::CopySrc);
    const RhiBufferPtr readback = buffer(kChunk * kFrames, RhiBufferUsage::CopyDst, RhiMemory::Readback);
    for (std::size_t frame = 0; frame < kFrames; ++frame)
    {
        const std::vector<std::uint8_t> data(kChunk, static_cast<std::uint8_t>(frame + 1));
        device->upload(*source, 0, bytes(data));
        RhiCommandList& commands = device->beginOffscreenFrame();
        commands.copyBuffer(*source, 0, *readback, frame * kChunk, kChunk);
        device->endFrame();
    }
    EXPECT_EQ(device->uploadRingCapacity(), 64u * 1024u) << "每帧的上传都放得下，不该扩容";

    std::vector<std::uint8_t> actual(kChunk * kFrames);
    ASSERT_TRUE(device->readBuffer(*readback, 0, std::as_writable_bytes(std::span(actual))));
    for (std::size_t frame = 0; frame < kFrames; ++frame)
    {
        const auto first = actual.begin() + static_cast<std::ptrdiff_t>(frame * kChunk);
        EXPECT_EQ(std::count(first, first + kChunk, static_cast<std::uint8_t>(frame + 1)),
                  static_cast<std::ptrdiff_t>(kChunk))
            << "第 " << frame << " 帧的数据不对";
    }

    // 一次上传比环形缓冲大：扩容
    std::vector<std::uint8_t> big(200u * 1024u);
    for (std::size_t i = 0; i < big.size(); ++i)
    {
        big[i] = static_cast<std::uint8_t>(i % 253);
    }
    const RhiBufferPtr bigSource = buffer(big.size(), RhiBufferUsage::CopySrc);
    const RhiBufferPtr bigReadback = buffer(big.size(), RhiBufferUsage::CopyDst, RhiMemory::Readback);
    device->upload(*bigSource, 0, bytes(big));
    RhiCommandList& commands = device->beginOffscreenFrame();
    commands.copyBuffer(*bigSource, 0, *bigReadback, 0, big.size());
    device->endFrame();
    EXPECT_GT(device->uploadRingCapacity(), 64u * 1024u);
    std::vector<std::uint8_t> bigActual(big.size());
    ASSERT_TRUE(device->readBuffer(*bigReadback, 0, std::as_writable_bytes(std::span(bigActual))));
    EXPECT_EQ(bigActual, big);
}

TEST_P(RhiTest, 每种绑定类型)
{
    // 常量缓冲（带动态偏移）、纹素缓冲（顶点阶段）、存储缓冲（片段阶段）、纹理与采样器合一
    const Target target = makeTarget();
    const Program program = load(RhiTestShaders::records);
    const RhiPipelinePtr pipeline = recordsPipeline(program);
    ASSERT_TRUE(pipeline);

    // 两份常量：同样的纹理采样位置，色调不同（第二份蓝色减半），靠动态偏移切换
    const FrameSlots frame = frameSlots(program.layouts[0], {
        {glm::mat4(1.0f), {1.0f, 1.0f, 1.0f, 1.0f}, {0.25f, 0.75f, 0.0f, 0.0f}},
        {glm::mat4(1.0f), {1.0f, 1.0f, 0.5f, 1.0f}, {0.25f, 0.75f, 0.0f, 0.0f}},
    });

    const RhiBufferPtr placements = bufferWith(std::vector<Placement>{{-0.5f, 0.0f, 0.4f, 0.0f}, {0.5f, 0.0f, 0.4f, 0.0f}},
                                               RhiBufferUsage::Texel);
    const RhiBufferPtr colors = bufferWith(std::vector<glm::vec4>{{1, 1, 0, 1}, {0, 1, 1, 1}}, RhiBufferUsage::Storage);
    RhiBindGroupDesc tablesDesc;
    tablesDesc.layout = program.layouts[1];
    tablesDesc.entries = {{.binding = 0, .buffer = placements, .texelFormat = RhiFormat::RGBA32Float},
                          {.binding = 1, .buffer = colors}};
    const RhiBindGroupPtr tables = device->createBindGroup(tablesDesc);
    ASSERT_TRUE(tables);

    // 2×2 纹理，只有第 1 行第 0 列不是黑的；采样位置 (0.25, 0.75) 落在它上面。
    // 上传的数据第 0 行是纹理的第 0 行（v = 0 处），行序弄反了就采到黑色
    RhiTextureDesc textureDesc;
    textureDesc.width = 2;
    textureDesc.height = 2;
    textureDesc.usage = RhiTextureUsage::Sampled;
    const RhiTexturePtr texture = device->createTexture(textureDesc);
    ASSERT_TRUE(texture);
    const std::vector<std::uint8_t> texels = {0, 0, 0, 255, 0, 0, 0, 255, 255, 128, 255, 255, 0, 0, 0, 255};
    device->upload(*texture, {0, 0, 0, 2, 2}, bytes(texels));
    RhiSamplerDesc samplerDesc;
    samplerDesc.minFilter = RhiFilter::Nearest;
    samplerDesc.magFilter = RhiFilter::Nearest;
    const RhiSamplerPtr sampler = device->createSampler(samplerDesc);
    RhiBindGroupDesc imageDesc;
    imageDesc.layout = program.layouts[2];
    imageDesc.entries = {{.binding = 0, .texture = texture, .sampler = sampler}};
    const RhiBindGroupPtr image = device->createBindGroup(imageDesc);
    ASSERT_TRUE(image);

    const RhiBufferPtr quad = bufferWith(kQuad, RhiBufferUsage::Vertex);
    const RhiBufferPtr records = bufferWith(std::vector<std::uint32_t>{0, 1}, RhiBufferUsage::Vertex);

    RhiCommandList& commands = device->beginOffscreenFrame();
    commands.beginRenderPass(passTo(target));
    commands.setPipeline(*pipeline);
    commands.setBindGroup(1, *tables);
    commands.setBindGroup(2, *image);
    const RhiVertexBufferBinding vertexBuffers[] = {{quad.get(), 0}, {records.get(), 0}};
    commands.setVertexBuffers(0, vertexBuffers);
    const std::uint32_t first[] = {frame.offset(0)};
    commands.setBindGroup(0, *frame.group, first);
    commands.draw(6, 1, 0, 0);
    const std::uint32_t second[] = {frame.offset(1)};
    commands.setBindGroup(0, *frame.group, second);
    commands.draw(6, 1, 0, 1);  // firstInstance 1：实例属性读到记录 1
    commands.endRenderPass();
    device->endFrame();

    const QImage result = read(target);
    // 记录 0：(1, 1, 0) × (1, 0.502, 1) × (1, 1, 1)；记录 1：(0, 1, 1) × (1, 0.502, 1) × (1, 1, 0.5)
    EXPECT_TRUE(pixelIs(result, 16, 32, QColor(255, 128, 0, 255)));
    EXPECT_TRUE(pixelIs(result, 48, 32, QColor(0, 128, 128, 255)));
    EXPECT_TRUE(pixelIs(result, 32, 4, QColor(0, 0, 0, 255)));
}

TEST_P(RhiTest, 多重间接绘制按baseInstance读实例属性)
{
    const Target target = makeTarget();
    const Program program = load(RhiTestShaders::records);
    const RhiPipelinePtr pipeline = recordsPipeline(program);
    ASSERT_TRUE(pipeline);
    const FrameSlots frame = frameSlots(program.layouts[0], {{glm::mat4(1.0f), glm::vec4(1.0f), glm::vec4(0.5f)}});

    // 上一行三块走 drawIndirect，下一行三块走 drawIndexedIndirect；每条间接参数的 firstInstance 选一条记录
    const std::vector<Placement> placements = {
        {-0.6f, 0.5f, 0.2f, 0.0f}, {0.0f, 0.5f, 0.2f, 0.0f}, {0.6f, 0.5f, 0.2f, 0.0f},
        {-0.6f, -0.5f, 0.2f, 0.0f}, {0.0f, -0.5f, 0.2f, 0.0f}, {0.6f, -0.5f, 0.2f, 0.0f},
    };
    const std::vector<glm::vec4> colors = {
        {1, 0, 0, 1}, {0, 1, 0, 1}, {0, 0, 1, 1}, {1, 1, 0, 1}, {0, 1, 1, 1}, {1, 0, 1, 1},
    };
    RhiBindGroupDesc tablesDesc;
    tablesDesc.layout = program.layouts[1];
    tablesDesc.entries = {{.binding = 0, .buffer = bufferWith(placements, RhiBufferUsage::Texel),
                           .texelFormat = RhiFormat::RGBA32Float},
                          {.binding = 1, .buffer = bufferWith(colors, RhiBufferUsage::Storage)}};
    const RhiBindGroupPtr tables = device->createBindGroup(tablesDesc);

    RhiTextureDesc textureDesc;
    textureDesc.width = 1;
    textureDesc.height = 1;
    const RhiTexturePtr white = device->createTexture(textureDesc);
    device->upload(*white, {0, 0, 0, 1, 1}, bytes(std::vector<std::uint8_t>{255, 255, 255, 255}));
    RhiBindGroupDesc imageDesc;
    imageDesc.layout = program.layouts[2];
    imageDesc.entries = {{.binding = 0, .texture = white, .sampler = device->createSampler({})}};
    const RhiBindGroupPtr image = device->createBindGroup(imageDesc);

    const RhiBufferPtr quad = bufferWith(kQuad, RhiBufferUsage::Vertex);
    const RhiBufferPtr corners = bufferWith(std::vector<glm::vec2>{{-1, -1}, {1, -1}, {1, 1}, {-1, 1}},
                                            RhiBufferUsage::Vertex);
    const RhiBufferPtr indices = bufferWith(std::vector<std::uint16_t>{0, 1, 2, 0, 2, 3}, RhiBufferUsage::Index);
    const RhiBufferPtr records = bufferWith(std::vector<std::uint32_t>{0, 1, 2, 3, 4, 5}, RhiBufferUsage::Vertex);
    const RhiBufferPtr arrayArgs = bufferWith(
        std::vector<RhiDrawIndirectArgs>{{6, 1, 0, 0}, {6, 1, 0, 1}, {6, 1, 0, 2}}, RhiBufferUsage::Indirect);
    const RhiBufferPtr indexedArgs = bufferWith(
        std::vector<RhiDrawIndexedIndirectArgs>{{6, 1, 0, 0, 3}, {6, 1, 0, 0, 4}, {6, 1, 0, 0, 5}},
        RhiBufferUsage::Indirect);

    RhiCommandList& commands = device->beginOffscreenFrame();
    commands.beginRenderPass(passTo(target));
    commands.setPipeline(*pipeline);
    const std::uint32_t offset[] = {0};
    commands.setBindGroup(0, *frame.group, offset);
    commands.setBindGroup(1, *tables);
    commands.setBindGroup(2, *image);
    const RhiVertexBufferBinding arrayBuffers[] = {{quad.get(), 0}, {records.get(), 0}};
    commands.setVertexBuffers(0, arrayBuffers);
    commands.drawIndirect(*arrayArgs, 0, 3);
    const RhiVertexBufferBinding indexedBuffers[] = {{corners.get(), 0}};
    commands.setVertexBuffers(0, indexedBuffers);
    commands.setIndexBuffer(*indices, 0, RhiIndexFormat::Uint16);
    commands.drawIndexedIndirect(*indexedArgs, 0, 3);
    commands.endRenderPass();
    device->endFrame();

    const QImage result = read(target);
    // NDC x = -0.6、0、0.6 是像素 13、32、51；y = 0.5、-0.5 是第 16、48 行（第 0 行在上）
    const int xs[] = {13, 32, 51};
    for (int i = 0; i < 6; ++i)
    {
        const glm::vec4& c = colors[static_cast<std::size_t>(i)];
        EXPECT_TRUE(pixelIs(result, xs[i % 3], i < 3 ? 16 : 48,
                            QColor::fromRgbF(c.r, c.g, c.b, c.a)))
            << "记录 " << i;
    }
}

TEST_P(RhiTest, 裁剪空间校正与视口裁剪矩形以左上角为原点)
{
    const Target target = makeTarget();
    const Program program = load(RhiTestShaders::solid);
    const RhiPipelinePtr pipeline = solidPipeline(program);
    ASSERT_TRUE(pipeline);
    // 投影按 GL 写（这里是恒等），乘本后端的校正
    const glm::mat4 transform = device->clipSpaceCorrection();
    const FrameSlots frame = frameSlots(program.layouts[0], {
        {transform, {1, 0, 0, 1}, glm::vec4(0.0f)},
        {transform, {0, 1, 0, 1}, glm::vec4(0.0f)},
    });
    // GL 的 NDC 里 y 向上：这个三角形在画面左上角
    const RhiBufferPtr triangle = bufferWith(std::vector<glm::vec2>{{-1, 1}, {0, 1}, {-1, 0}}, RhiBufferUsage::Vertex);
    const RhiBufferPtr quad = bufferWith(kQuad, RhiBufferUsage::Vertex);

    const auto drawPass = [&](const RhiBuffer& vertices, std::uint32_t count, std::size_t slot,
                              const std::function<void(RhiCommandList&)>& setup)
    {
        RhiCommandList& commands = device->beginOffscreenFrame();
        commands.beginRenderPass(passTo(target));
        setup(commands);
        commands.setPipeline(*pipeline);
        const std::uint32_t offset[] = {frame.offset(slot)};
        commands.setBindGroup(0, *frame.group, offset);
        const RhiVertexBufferBinding binding[] = {{&vertices, 0}};
        commands.setVertexBuffers(0, binding);
        commands.draw(count, 1, 0, 0);
        commands.endRenderPass();
        device->endFrame();
        return read(target);
    };

    const QImage clip = drawPass(*triangle, 3, 0, [](RhiCommandList&) {});
    EXPECT_TRUE(pixelIs(clip, 2, 2, QColor(255, 0, 0, 255))) << "三角形应在左上角";
    EXPECT_TRUE(pixelIs(clip, 61, 61, QColor(0, 0, 0, 255)));
    EXPECT_TRUE(pixelIs(clip, 2, 61, QColor(0, 0, 0, 255)));
    EXPECT_TRUE(pixelIs(clip, 61, 2, QColor(0, 0, 0, 255)));

    // 裁剪矩形 (0, 0, 32, 32) 是左上的四分之一
    const QImage scissor = drawPass(*quad, 6, 1, [](RhiCommandList& commands) { commands.setScissor({0, 0, 32, 32}); });
    EXPECT_TRUE(pixelIs(scissor, 8, 8, QColor(0, 255, 0, 255)));
    EXPECT_TRUE(pixelIs(scissor, 40, 8, QColor(0, 0, 0, 255)));
    EXPECT_TRUE(pixelIs(scissor, 8, 40, QColor(0, 0, 0, 255)));

    // 视口 (32, 0, 32, 32) 是右上的四分之一
    const QImage viewport = drawPass(*quad, 6, 1, [](RhiCommandList& commands)
    {
        RhiViewport vp;
        vp.x = 32.0f;
        vp.y = 0.0f;
        vp.width = 32.0f;
        vp.height = 32.0f;
        commands.setViewport(vp);
    });
    EXPECT_TRUE(pixelIs(viewport, 40, 8, QColor(0, 255, 0, 255)));
    EXPECT_TRUE(pixelIs(viewport, 8, 8, QColor(0, 0, 0, 255)));
    EXPECT_TRUE(pixelIs(viewport, 40, 40, QColor(0, 0, 0, 255)));
}

TEST_P(RhiTest, 多重采样解析)
{
    if (device->caps().maxColorSamples < 4)
    {
        GTEST_SKIP() << "设备不支持 4 重采样";
    }
    const Program program = load(RhiTestShaders::solid);
    const FrameSlots frame = frameSlots(program.layouts[0], {{glm::mat4(1.0f), glm::vec4(1.0f), glm::vec4(0.0f)}});
    // 斜边穿过整个画面的三角形：多重采样时斜边上有半覆盖的像素
    const RhiBufferPtr triangle = bufferWith(std::vector<glm::vec2>{{-1, -1}, {1, -1}, {-1, 1}}, RhiBufferUsage::Vertex);

    const auto render = [&](std::uint32_t samples)
    {
        const Target target = makeTarget(samples);
        const RhiPipelinePtr pipeline = solidPipeline(program, samples);
        EXPECT_TRUE(pipeline);
        RhiCommandList& commands = device->beginOffscreenFrame();
        commands.beginRenderPass(passTo(target));
        commands.setPipeline(*pipeline);
        const std::uint32_t offset[] = {0};
        commands.setBindGroup(0, *frame.group, offset);
        const RhiVertexBufferBinding binding[] = {{triangle.get(), 0}};
        commands.setVertexBuffers(0, binding);
        commands.draw(3, 1, 0, 0);
        commands.endRenderPass();
        device->endFrame();
        return read(target);
    };
    const auto partiallyCovered = [](const QImage& image)
    {
        int count = 0;
        for (int y = 0; y < image.height(); ++y)
        {
            for (int x = 0; x < image.width(); ++x)
            {
                const int red = image.pixelColor(x, y).red();
                count += red > 8 && red < 247 ? 1 : 0;
            }
        }
        return count;
    };

    const QImage resolved = render(4);
    EXPECT_TRUE(pixelIs(resolved, 4, 59, QColor(255, 255, 255, 255))) << "三角形在左下";
    EXPECT_TRUE(pixelIs(resolved, 59, 4, QColor(0, 0, 0, 255)));
    EXPECT_GT(partiallyCovered(resolved), 16) << "解析后斜边上应有半覆盖的像素";
    EXPECT_EQ(partiallyCovered(render(1)), 0) << "单采样时没有半覆盖的像素";
}

TEST_P(RhiTest, 延迟释放)
{
    const std::vector<std::uint8_t> data(256, 0x5A);
    RhiBufferPtr source = buffer(data.size(), RhiBufferUsage::CopySrc);
    const RhiBufferPtr readback = buffer(data.size(), RhiBufferUsage::CopyDst, RhiMemory::Readback);
    device->upload(*source, 0, bytes(data));
    const std::size_t live = device->liveResourceCount();

    // 帧内放掉句柄：本帧还要用它，进延迟释放队列，等本帧在 GPU 上完成才销毁
    RhiCommandList& commands = device->beginOffscreenFrame();
    const std::uint64_t serial = device->frameSerial();
    commands.copyBuffer(*source, 0, *readback, 0, data.size());
    source.reset();
    EXPECT_EQ(device->pendingReleaseCount(), 1u);
    EXPECT_EQ(device->liveResourceCount(), live);
    EXPECT_LT(device->completedFrameSerial(), serial);
    device->endFrame();
    device->waitIdle();
    EXPECT_GE(device->completedFrameSerial(), serial);
    EXPECT_EQ(device->pendingReleaseCount(), 0u);
    EXPECT_EQ(device->liveResourceCount(), live - 1);
    std::vector<std::uint8_t> actual(data.size());
    ASSERT_TRUE(device->readBuffer(*readback, 0, std::as_writable_bytes(std::span(actual))));
    EXPECT_EQ(actual, data) << "句柄在帧内放掉，复制照样完成";

    // 绑定组持有它的缓冲：缓冲的句柄放掉后，绑定组还在就不销毁
    const RhiBindGroupLayoutPtr layout = rhiCreateBindGroupLayouts(*device, RhiTestShaders::solid.bindGroups)[0];
    RhiBufferPtr uniforms = buffer(sizeof(FrameConstants), RhiBufferUsage::Uniform);
    RhiBindGroupDesc desc;
    desc.layout = layout;
    desc.entries = {{.binding = 0, .buffer = uniforms, .size = sizeof(FrameConstants)}};
    RhiBindGroupPtr group = device->createBindGroup(desc);
    desc.entries.clear();
    const std::size_t withGroup = device->liveResourceCount();
    uniforms.reset();
    device->waitIdle();
    EXPECT_EQ(device->liveResourceCount(), withGroup);
    group.reset();
    device->waitIdle();
    EXPECT_EQ(device->liveResourceCount(), withGroup - 2);
}

TEST_P(RhiTest, 画到QOpenGLWidget)
{
    // 资源在设备自己的上下文里建，在窗口部件的上下文里用（共享组）；VAO 在部件的上下文里惰性创建
    const Program program = load(RhiTestShaders::solid);
    const RhiPipelinePtr pipeline = solidPipeline(program, 1, RhiFormat::Depth24Stencil8);
    ASSERT_TRUE(pipeline);
    const FrameSlots frame = frameSlots(program.layouts[0], {{device->clipSpaceCorrection(), {1, 0, 0, 1}, glm::vec4(0.0f)}});
    const RhiBufferPtr triangle = bufferWith(std::vector<glm::vec2>{{-1, 1}, {0, 1}, {-1, 0}}, RhiBufferUsage::Vertex);

    RhiTestWidget widget;
    widget.setFormat(GLRhiDevice::surfaceFormat());
    widget.paint = [&](RhiSurface& surface)
    {
        EXPECT_EQ(surface.width(), kSize);
        RhiCommandList& commands = device->beginFrame(surface);
        RhiRenderPassDesc pass;
        RhiColorAttachmentOps ops;
        ops.clearColor = {0.0f, 0.0f, 1.0f, 1.0f};
        pass.colorOps = {ops};
        commands.beginRenderPass(pass);
        commands.setPipeline(*pipeline);
        const std::uint32_t offset[] = {0};
        commands.setBindGroup(0, *frame.group, offset);
        const RhiVertexBufferBinding binding[] = {{triangle.get(), 0}};
        commands.setVertexBuffers(0, binding);
        commands.draw(3, 1, 0, 0);
        commands.endRenderPass();
        device->endFrame();
    };
    widget.setAttribute(Qt::WA_DontShowOnScreen);
    widget.resize(static_cast<int>(kSize), static_cast<int>(kSize));
    widget.show();
    const QImage image = widget.grabFramebuffer();
    ASSERT_EQ(image.width(), static_cast<int>(kSize));
    EXPECT_TRUE(pixelIs(image, 2, 2, QColor(255, 0, 0, 255))) << "三角形应在左上角";
    EXPECT_TRUE(pixelIs(image, 61, 61, QColor(0, 0, 255, 255)));
}

TEST_P(RhiTest, 绑定与着色器不一致时建管线失败)
{
    Program program = load(RhiTestShaders::records);
    // 第 1、2 组对调：平铺后纹素缓冲与纹理的纹理单元对不上，按程序反射核对时发现
    std::swap(program.layouts[1], program.layouts[2]);
    EXPECT_FALSE(recordsPipeline(program));

    // 布局里多出着色器没用到的绑定不要紧（Vulkan 同样允许）
    const Program solid = load(RhiTestShaders::solid);
    Program wider = load(RhiTestShaders::records);
    wider.shaders = solid.shaders;
    EXPECT_TRUE(solidPipeline(wider));
}

INSTANTIATE_TEST_SUITE_P(GL, RhiTest, ::testing::Bool(), uploadPathName);

TEST(RhiDebugOutputTest, GL错误由调试输出计数)
{
    // 每个用例结束时要求 debugErrorCount() 为 0；先确认它真会响（测试进程是调试上下文，见 MesaLoader.cpp）
    const std::unique_ptr<GLRhiDevice> device = GLRhiDevice::create();
    ASSERT_TRUE(device);
    if (!GLRhiDevice::surfaceFormat().testOption(QSurfaceFormat::DebugContext))
    {
        GTEST_SKIP() << "不是调试上下文（YICAD_GL_DEBUG 没有设置）";
    }
    device->beginOffscreenFrame();
    glBindBuffer(GL_ARRAY_BUFFER, 0x7FFFFFFF);  // core profile 里绑定没生成过的名字是 GL_INVALID_OPERATION
    device->endFrame();
    EXPECT_EQ(device->debugErrorCount(), 1u);
}
