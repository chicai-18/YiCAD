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

/// @file RhiTypes.h
/// @brief RHI 的枚举、描述结构与能力（RENDER_PLAN.md 第 4.7 节）
/// @details RHI 按 Vulkan 1.3 的形状设计：管线状态不可变、资源经固定布局的绑定组绑定、命令录制进命令列表、
///          资源释放延迟到用过它的帧结束之后。坐标约定：视口与裁剪矩形以帧缓冲左上角为原点（同 Vulkan），
///          投影矩阵按 GL 习惯写（y 向上、深度 -1..1），乘 RhiDevice::clipSpaceCorrection() 换到本后端

#ifndef RHITYPES_H
#define RHITYPES_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

class RhiBuffer;
class RhiTexture;
class RhiSampler;
class RhiShader;
class RhiPipeline;
class RhiBindGroupLayout;
class RhiBindGroup;
class RhiRenderTarget;
class RhiQuerySet;

/// @brief 资源句柄：引用计数，最后一个引用释放时进入设备的延迟释放队列
using RhiBufferPtr = std::shared_ptr<RhiBuffer>;
using RhiTexturePtr = std::shared_ptr<RhiTexture>;
using RhiSamplerPtr = std::shared_ptr<RhiSampler>;
using RhiShaderPtr = std::shared_ptr<RhiShader>;
using RhiPipelinePtr = std::shared_ptr<RhiPipeline>;
using RhiBindGroupLayoutPtr = std::shared_ptr<RhiBindGroupLayout>;
using RhiBindGroupPtr = std::shared_ptr<RhiBindGroup>;
using RhiRenderTargetPtr = std::shared_ptr<RhiRenderTarget>;
using RhiQuerySetPtr = std::shared_ptr<RhiQuerySet>;

/// @brief 为位标志枚举定义 |、&、|= 与 rhiAny()（是否有任一位）
#define YICAD_RHI_FLAGS(E)                                                                          \
    constexpr E operator|(E a, E b)                                                                 \
    {                                                                                               \
        return static_cast<E>(static_cast<std::underlying_type_t<E>>(a) | static_cast<std::underlying_type_t<E>>(b)); \
    }                                                                                               \
    constexpr E operator&(E a, E b)                                                                 \
    {                                                                                               \
        return static_cast<E>(static_cast<std::underlying_type_t<E>>(a) & static_cast<std::underlying_type_t<E>>(b)); \
    }                                                                                               \
    constexpr E& operator|=(E& a, E b)                                                              \
    {                                                                                               \
        return a = a | b;                                                                           \
    }                                                                                               \
    constexpr bool rhiAny(E e)                                                                         \
    {                                                                                               \
        return static_cast<std::underlying_type_t<E>>(e) != 0;                                      \
    }

/// @brief 纹理、渲染目标与纹素缓冲的格式
enum class RhiFormat : std::uint8_t
{
    Undefined,
    R8Unorm,
    RGBA8Unorm,
    R32Float,
    RG32Float,
    RGBA32Float,
    R32Uint,
    RG32Uint,
    RGBA32Uint,
    Depth24Stencil8,
    Depth32Float
};

/// @brief 格式每个像素（纹素）的字节数
constexpr std::uint32_t rhiFormatSize(RhiFormat format)
{
    switch (format)
    {
    case RhiFormat::R8Unorm: return 1;
    case RhiFormat::RGBA8Unorm: return 4;
    case RhiFormat::R32Float: return 4;
    case RhiFormat::RG32Float: return 8;
    case RhiFormat::RGBA32Float: return 16;
    case RhiFormat::R32Uint: return 4;
    case RhiFormat::RG32Uint: return 8;
    case RhiFormat::RGBA32Uint: return 16;
    case RhiFormat::Depth24Stencil8: return 4;
    case RhiFormat::Depth32Float: return 4;
    case RhiFormat::Undefined: break;
    }
    return 0;
}

/// @brief 是否深度（模板）格式
constexpr bool rhiIsDepthFormat(RhiFormat format)
{
    return format == RhiFormat::Depth24Stencil8 || format == RhiFormat::Depth32Float;
}

/// @brief 顶点属性的格式
enum class RhiVertexFormat : std::uint8_t
{
    Float,
    Float2,
    Float3,
    Float4,
    Uint,
    Uint2,
    Uint4,
    Sint,
    UByte4Norm  ///< 4 个 0..255 的字节，着色器里读成 0..1 的 vec4
};

/// @brief 缓冲的用途
enum class RhiBufferUsage : std::uint32_t
{
    None = 0,
    Vertex = 1u << 0,
    Index = 1u << 1,
    Uniform = 1u << 2,
    Storage = 1u << 3,
    Texel = 1u << 4,      ///< 纹素缓冲（GL 的纹理缓冲对象，Vulkan 的 uniform texel buffer）
    Indirect = 1u << 5,   ///< 间接绘制的参数
    CopySrc = 1u << 6,    ///< RhiCommandList::copyBuffer 的源
    CopyDst = 1u << 7     ///< copyBuffer、copyTextureToBuffer 的目标
};
YICAD_RHI_FLAGS(RhiBufferUsage)

/// @brief 缓冲放在哪里
enum class RhiMemory : std::uint8_t
{
    Device,   ///< GPU 读写；CPU 只能经 RhiDevice::upload() 写入
    Readback  ///< CPU 可读：复制命令的目标，帧完成后经 RhiDevice::readBuffer() 读出
};

/// @brief 纹理的用途
enum class RhiTextureUsage : std::uint32_t
{
    None = 0,
    Sampled = 1u << 0,       ///< 着色器采样
    ColorTarget = 1u << 1,   ///< 渲染目标的颜色附件，或多重采样解析的目标
    DepthStencil = 1u << 2,  ///< 渲染目标的深度模板附件
    CopySrc = 1u << 3        ///< copyTextureToBuffer 的源
};
YICAD_RHI_FLAGS(RhiTextureUsage)

/// @brief 着色器阶段
enum class RhiShaderStage : std::uint32_t
{
    None = 0,
    Vertex = 1u << 0,
    Fragment = 1u << 1
};
YICAD_RHI_FLAGS(RhiShaderStage)

/// @brief 绑定的资源类型
/// @details GL 4.3 的顶点阶段只保证有常量缓冲与纹素缓冲，存储缓冲只在片段阶段用（第 4.7.3 节），
///          由构建期的着色器检查保证
enum class RhiBindingType : std::uint8_t
{
    UniformBuffer,
    StorageBuffer,          ///< 只读
    TexelBuffer,            ///< 着色器里是 samplerBuffer/usamplerBuffer，按格式取纹素
    CombinedTextureSampler  ///< 着色器里是 sampler2D 等，纹理与采样器合一
};

/// @brief 绑定组布局的一项
struct RhiBindGroupLayoutEntry
{
    std::uint32_t binding = 0;
    RhiBindingType type = RhiBindingType::UniformBuffer;
    RhiShaderStage stages = RhiShaderStage::None;
    bool hasDynamicOffset = false;  ///< 常量缓冲、存储缓冲：setBindGroup 时另给偏移
};

/// @brief 图元拓扑
enum class RhiPrimitiveTopology : std::uint8_t
{
    PointList,
    LineList,
    LineStrip,
    TriangleList,
    TriangleStrip
};

enum class RhiIndexFormat : std::uint8_t
{
    Uint16,
    Uint32
};

enum class RhiCompareOp : std::uint8_t
{
    Never,
    Less,
    Equal,
    LessOrEqual,
    Greater,
    NotEqual,
    GreaterOrEqual,
    Always
};

enum class RhiBlendFactor : std::uint8_t
{
    Zero,
    One,
    SrcColor,
    OneMinusSrcColor,
    DstColor,
    OneMinusDstColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha
};

enum class RhiBlendOp : std::uint8_t
{
    Add,
    Subtract,
    ReverseSubtract,
    Min,
    Max
};

enum class RhiColorWriteMask : std::uint8_t
{
    None = 0,
    Red = 1u << 0,
    Green = 1u << 1,
    Blue = 1u << 2,
    Alpha = 1u << 3,
    All = 0xF
};
YICAD_RHI_FLAGS(RhiColorWriteMask)

enum class RhiFilter : std::uint8_t
{
    Nearest,
    Linear
};

enum class RhiMipmapMode : std::uint8_t
{
    None,     ///< 只用第 0 级
    Nearest,
    Linear
};

enum class RhiAddressMode : std::uint8_t
{
    ClampToEdge,
    Repeat,
    MirroredRepeat
};

enum class RhiLoadOp : std::uint8_t
{
    Load,
    Clear,
    DontCare
};

enum class RhiStoreOp : std::uint8_t
{
    Store,
    DontCare
};

/// @brief 着色器代码的形式，由后端决定（RhiCaps::shaderLanguage）
enum class RhiShaderLanguage : std::uint8_t
{
    Glsl430,  ///< spirv-cross 生成的 GLSL 430 源码（文件 <程序>.<阶段>.glsl）
    SpirV     ///< SPIR-V（文件 <程序>.<阶段>.spv）
};

// ---------------------------------------------------------------------------
// 描述结构
// ---------------------------------------------------------------------------

struct RhiBufferDesc
{
    std::size_t size = 0;
    RhiBufferUsage usage = RhiBufferUsage::None;
    RhiMemory memory = RhiMemory::Device;
    std::string debugName;
};

struct RhiTextureDesc
{
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    RhiFormat format = RhiFormat::RGBA8Unorm;
    std::uint32_t mipLevels = 1;
    std::uint32_t sampleCount = 1;  ///< 大于 1 时只能作渲染目标的附件，经渲染通道解析后使用
    RhiTextureUsage usage = RhiTextureUsage::Sampled;
    std::string debugName;
};

/// @brief 纹理里的一块区域；上传的数据按行紧密排列，第 0 行是纹理的第 0 行（采样坐标 v = 0 处）
struct RhiTextureRegion
{
    std::uint32_t mipLevel = 0;
    std::uint32_t x = 0;
    std::uint32_t y = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

struct RhiSamplerDesc
{
    RhiFilter minFilter = RhiFilter::Linear;
    RhiFilter magFilter = RhiFilter::Linear;
    RhiMipmapMode mipmapMode = RhiMipmapMode::None;
    RhiAddressMode addressU = RhiAddressMode::ClampToEdge;
    RhiAddressMode addressV = RhiAddressMode::ClampToEdge;
};

/// @brief 着色器：代码由构建期工具链生成，形式见 RhiCaps::shaderLanguage
struct RhiShaderDesc
{
    RhiShaderStage stage = RhiShaderStage::Vertex;
    std::span<const std::byte> code;
    std::string debugName;
};

struct RhiBindGroupLayoutDesc
{
    std::span<const RhiBindGroupLayoutEntry> entries;
    std::string debugName;
};

/// @brief 绑定组的一项，按 binding 对应布局里的同号项
struct RhiBindGroupEntry
{
    std::uint32_t binding = 0;
    RhiBufferPtr buffer;           ///< 常量缓冲、存储缓冲、纹素缓冲
    std::size_t offset = 0;        ///< 按 RhiCaps 里对应的对齐要求对齐
    std::size_t size = 0;          ///< 0 表示到缓冲末尾
    RhiFormat texelFormat = RhiFormat::Undefined;  ///< 纹素缓冲的格式
    RhiTexturePtr texture;         ///< 纹理与采样器合一的绑定
    RhiSamplerPtr sampler;
};

struct RhiBindGroupDesc
{
    RhiBindGroupLayoutPtr layout;
    std::vector<RhiBindGroupEntry> entries;
    std::string debugName;
};

/// @brief 顶点属性：location 对应着色器里的 layout(location)
struct RhiVertexAttribute
{
    std::uint32_t location = 0;
    RhiVertexFormat format = RhiVertexFormat::Float;
    std::uint32_t offset = 0;
};

enum class RhiVertexStepMode : std::uint8_t
{
    Vertex,   ///< 每个顶点前进一步
    Instance  ///< 每个实例前进一步；读取遵守 firstInstance（GL 的 baseInstance），用来传记录序号（第 4.7.3 节）
};

/// @brief 一个顶点缓冲绑定槽的布局；槽号即在 RhiPipelineDesc::vertexBuffers 里的下标
struct RhiVertexBufferLayout
{
    std::uint32_t stride = 0;
    RhiVertexStepMode stepMode = RhiVertexStepMode::Vertex;
    std::vector<RhiVertexAttribute> attributes;
};

struct RhiBlendComponent
{
    RhiBlendFactor srcFactor = RhiBlendFactor::One;
    RhiBlendFactor dstFactor = RhiBlendFactor::Zero;
    RhiBlendOp op = RhiBlendOp::Add;
};

struct RhiColorTargetState
{
    RhiFormat format = RhiFormat::RGBA8Unorm;
    bool blendEnabled = false;
    RhiBlendComponent color;
    RhiBlendComponent alpha;
    RhiColorWriteMask writeMask = RhiColorWriteMask::All;
};

struct RhiDepthStencilState
{
    RhiFormat format = RhiFormat::Undefined;  ///< Undefined 表示没有深度附件
    bool depthTestEnabled = false;
    bool depthWriteEnabled = false;
    RhiCompareOp depthCompare = RhiCompareOp::Less;
};

/// @brief 管线：创建后不可变
/// @details 绑定组布局按组号排列，空指针表示该组不用。颜色目标的格式、深度格式与采样数必须与绘制时的渲染通道一致
///          （Vulkan 的动态渲染要求在创建管线时给出）
struct RhiPipelineDesc
{
    RhiShaderPtr vertexShader;
    RhiShaderPtr fragmentShader;
    std::vector<RhiBindGroupLayoutPtr> bindGroupLayouts;
    std::vector<RhiVertexBufferLayout> vertexBuffers;
    RhiPrimitiveTopology topology = RhiPrimitiveTopology::TriangleList;
    std::vector<RhiColorTargetState> colorTargets;
    RhiDepthStencilState depthStencil;
    std::uint32_t sampleCount = 1;
    std::string debugName;
};

/// @brief 离屏渲染目标：颜色附件与可选的深度模板附件，尺寸与采样数必须相同
struct RhiRenderTargetDesc
{
    std::vector<RhiTexturePtr> colorAttachments;
    RhiTexturePtr depthStencil;
    std::string debugName;
};

/// @brief 渲染通道里一个颜色附件的加载、存储与解析
struct RhiColorAttachmentOps
{
    RhiLoadOp load = RhiLoadOp::Clear;
    RhiStoreOp store = RhiStoreOp::Store;
    std::array<float, 4> clearColor = {0.0f, 0.0f, 0.0f, 0.0f};
    /// @brief 非空：通道结束时把多重采样附件解析到这张单采样纹理（尺寸、格式相同）
    const RhiTexture* resolveTarget = nullptr;
};

/// @brief 渲染通道
struct RhiRenderPassDesc
{
    /// @brief 空指针表示画到本帧的表面（交换链图像，GL 下是 QOpenGLWidget 的帧缓冲）
    const RhiRenderTarget* target = nullptr;
    /// @brief 每个颜色附件一项；少于附件数时其余按默认值（清除为 0、存储）
    std::vector<RhiColorAttachmentOps> colorOps;
    RhiLoadOp depthLoad = RhiLoadOp::Clear;
    RhiStoreOp depthStore = RhiStoreOp::DontCare;
    float clearDepth = 1.0f;
    std::uint32_t clearStencil = 0;
};

/// @brief 视口：以帧缓冲左上角为原点，像素
struct RhiViewport
{
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float minDepth = 0.0f;
    float maxDepth = 1.0f;
};

/// @brief 矩形：以帧缓冲左上角为原点，像素
struct RhiRect
{
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

/// @brief setVertexBuffers 的一项
struct RhiVertexBufferBinding
{
    const RhiBuffer* buffer = nullptr;
    std::size_t offset = 0;
};

/// @brief drawIndirect 的一条参数，GL 与 Vulkan 的布局相同
struct RhiDrawIndirectArgs
{
    std::uint32_t vertexCount = 0;
    std::uint32_t instanceCount = 0;
    std::uint32_t firstVertex = 0;
    std::uint32_t firstInstance = 0;
};

/// @brief drawIndexedIndirect 的一条参数，GL 与 Vulkan 的布局相同
struct RhiDrawIndexedIndirectArgs
{
    std::uint32_t indexCount = 0;
    std::uint32_t instanceCount = 0;
    std::uint32_t firstIndex = 0;
    std::int32_t vertexOffset = 0;
    std::uint32_t firstInstance = 0;
};

/// @brief 一组 GPU 时间戳查询（Vulkan 的 timestamp 查询池）：命令列表往里写，设备不等待地读出（RhiCaps::timestampQueries）
struct RhiQuerySetDesc
{
    std::uint32_t count = 0;
    std::string debugName;
};

/// @brief 设备能力。GS 按能力选路径，不靠猜（第 4.7.1 节）
struct RhiCaps
{
    std::string backend;       ///< "OpenGL"
    std::string apiVersion;    ///< 驱动给出的版本串
    std::string renderer;      ///< 显卡（或软件实现）的名字
    std::string vendor;

    RhiShaderLanguage shaderLanguage = RhiShaderLanguage::Glsl430;

    std::size_t maxUniformBufferRange = 0;          ///< 一次绑定的常量缓冲最大字节数
    std::size_t maxStorageBufferRange = 0;          ///< 一次绑定的存储缓冲最大字节数
    std::size_t maxTexelBufferElements = 0;         ///< 纹素缓冲最多的纹素数
    std::size_t uniformBufferOffsetAlignment = 256; ///< 常量缓冲绑定偏移（含动态偏移）的对齐
    std::size_t storageBufferOffsetAlignment = 256;
    std::size_t texelBufferOffsetAlignment = 256;

    std::uint32_t maxTextureSize = 0;
    std::uint32_t maxColorSamples = 1;
    std::uint32_t maxDepthSamples = 1;
    std::uint32_t maxVertexAttributes = 0;
    std::uint32_t maxVertexBuffers = 0;
    std::uint32_t maxVertexUniformBuffers = 0;
    std::uint32_t maxFragmentUniformBuffers = 0;
    std::uint32_t maxVertexStorageBuffers = 0;      ///< GL 4.3 的最低要求是 0
    std::uint32_t maxFragmentStorageBuffers = 0;
    std::uint32_t maxVertexTextures = 0;            ///< 顶点阶段的纹理单元（纹素缓冲也占）
    std::uint32_t maxFragmentTextures = 0;

    bool persistentMapping = false;  ///< 上传环形缓冲用持久映射（GL 的 ARB_buffer_storage）
    bool timestampQueries = false;   ///< 命令列表能写 GPU 时间戳（RhiCommandList::writeTimestamp）
    /// @brief 渲染结果的第 0 行是画面底部（GL）；读回渲染结果、采样渲染过的纹理时据此翻转（第 4.7.3 节）
    bool framebufferOriginBottomLeft = false;
};

#endif // RHITYPES_H
