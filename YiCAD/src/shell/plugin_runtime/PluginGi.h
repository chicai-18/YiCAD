/// @file PluginGi.h
/// @brief 宿主的 GI 表（YiCadGiApiV4）：插件实体的 worldDraw 经它把图元交给 IGiWorldDraw（RENDER_PLAN.md 第 4.8.3 节）

#ifndef PLUGIN_GI_H
#define PLUGIN_GI_H

#include "YiCadPluginAbi.h"

#include "Datamodel.h"
#include "DmColor.h"

#include <cstddef>

class DmDocument;
class DmEntity;
class DmLineType;
class IGiWorldDraw;

/// @brief 一次插件 worldDraw 的 GI 上下文，作为 YiCadGiContextHandle 交给插件
/// @details 记下插件设的颜色、线宽、线型（drawBlock 时块里的随块属性取它们，初值为实体自己的），
///          数着输出的点与图元，超过上限后丢弃之后的图元并记一次日志；析构时补上插件没弹出的变换
class PluginGiContext
{
public:
    /// @brief 单次 worldDraw 输出的点与图元的上限（RENDER_PLAN.md 第 4.8.3 节"健壮性"）
    static constexpr std::size_t kMaxElements = 1000000;

    /// @param wd 接收方
    /// @param entity 被画的实体：属性的初值与找资源的文档取它的；可为空（全随块，没有文档）
    /// @param className 插件实体的类名，日志用
    PluginGiContext(IGiWorldDraw& wd, const DmEntity* entity, const char* className);
    ~PluginGiContext();

    PluginGiContext(const PluginGiContext&) = delete;
    PluginGiContext& operator=(const PluginGiContext&) = delete;

    IGiWorldDraw& wd() { return m_wd; }
    DmDocument* document() const { return m_document; }

    /// @brief 记下 count 个点或图元；超过上限时返回 false（之后的都丢弃）
    bool consume(std::size_t count);

    DmColor color;                              ///< 当前颜色
    DM::LineWidth lineWeight = DM::WidthByBlock;///< 当前线宽
    const DmLineType* lineType = nullptr;       ///< 当前线型，为空即随块
    int transformDepth = 0;                     ///< 插件压入、还没弹出的变换个数
    bool screenSpace = false;                   ///< 是否以像素为单位

private:
    IGiWorldDraw& m_wd;
    DmDocument* m_document = nullptr;
    const char* m_className;
    std::size_t m_used = 0;
    bool m_truncated = false;
};

/// @brief 宿主的 GI 表，生命周期为整个进程
const YiCadGiApiV4& pluginGiApi() noexcept;

#endif // PLUGIN_GI_H
