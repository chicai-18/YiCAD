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

/// @file GsView.h
/// @brief 图形系统的视图：一个画布怎么画模型（RENDER_PLAN.md 第 4.3.1、4.3.8 节）
///
/// 每个画布一个。持有相机（double）、场景底图、叠加层与每视图的状态（高亮、临时隐藏）。一帧的流程（第 4.3.8 节）：
/// 1. 模型处理累积的变更（GsModel::update）；
/// 2. 场景作废时（模型内容、相机、尺寸、显示设置、大高亮集或临时隐藏变了）裁剪出可见分块，画进离屏的场景底图：
///    网格 → 几何（深度测试表达绘图次序，选中的画在最上面）；
/// 3. 叠加通道每帧都画，画到画布上：贴场景底图 → 高亮（重画被高亮的对象，不测深度）→ 临时模型（预览）→
///    叠加层的动态批次（夹点、原点标记、选择框、光标、捕捉标记）。
/// 只移动光标时场景不作废，这一帧的开销与图纸大小无关（R1）。

#ifndef GSVIEW_H
#define GSVIEW_H

#include <cstdint>
#include <memory>
#include <vector>

#include <QColor>

#include "DmVector.h"
#include "GiTransform.h"
#include "GsModel.h"
#include "GsTypes.h"

class DmBlock;
class DmEntity;
class GsDevice;
class RhiCommandList;
class RhiSurface;

/// @brief 叠加层的动态批次：每帧由画布按像素坐标（左上角为原点）重新生成，三角形列表
class GsOverlay
{
public:
    void clear() { m_vertices.clear(); }

    /// @brief 线段：宽 width 像素的四边形；dashed 时按 5 像素划线、5 像素空白
    void line(double x0, double y0, double x1, double y1, double width, const QColor& color, bool dashed = false);

    /// @brief 实心矩形
    void fillRect(double x0, double y0, double x1, double y1, const QColor& color);

    const std::vector<GsOverlayVertex>& vertices() const { return m_vertices; }

private:
    void triangle(const GsOverlayVertex& a, const GsOverlayVertex& b, const GsOverlayVertex& c);

    std::vector<GsOverlayVertex> m_vertices;
};

/// @brief 视图的显示设置
struct GsViewStyle
{
    QColor background = QColor(30, 30, 30);
    QColor selected = QColor(Qt::blue);
    QColor highlight = QColor(Qt::cyan);
    QColor grid = QColor(50, 55, 72);
    QColor metaGrid = QColor(73, 79, 105);
    bool lineWidths = false;      ///< 显示线宽（旧渲染器的换算：线宽代码 × 0.05 像素）
    bool gridOn = false;
    double gridSpacing = 0.0;     ///< 细网格的间距（世界长度），粗网格为它的 5 倍

    bool operator==(const GsViewStyle&) const = default;
};

/// @brief 图形系统的视图，见文件说明
class GsView
{
public:
    GsView();
    ~GsView();
    GsView(const GsView&) = delete;
    GsView& operator=(const GsView&) = delete;

    /// @brief 场景里画的模型（文档模型或容器模型）；selection 为真时按对象状态画选中
    void setModel(std::shared_ptr<GsModel> model, bool selection);
    /// @brief 叠加通道里画的临时模型（预览），不持有；可为空
    void setTransient(GsModel* transient);
    /// @brief 只画模型里的一个块定义（块缩略图）：块的定义坐标即世界坐标
    void setBlock(std::shared_ptr<GsModel> model, const DmBlock* block);

    /// @brief 相机：画布中心的世界坐标与每个（逻辑）像素的世界长度
    void setCamera(const DmVector& center, double worldPerPixel);
    void setStyle(const GsViewStyle& style);

    /// @brief 高亮的顶层实体（视图的高亮集）；不超过阈值时在叠加通道里重画，否则走状态位图（场景作废）
    void setHighlighted(std::vector<DmEntity*> entities);
    /// @brief 临时隐藏的顶层实体（视图的临时隐藏集），场景作废
    void setHidden(std::vector<DmEntity*> entities);
    /// @brief 选中实体的夹点（世界坐标），叠加层里画成蓝色方块
    void setGrips(std::vector<DmVector> grips);

    /// @brief 叠加层：画布每帧按逻辑像素重新填
    GsOverlay& overlay() { return m_overlay; }

    /// @brief 场景底图作废，下一帧重画
    void invalidateScene() { m_sceneValid = false; }

    /// @brief 画一帧到表面（在 QOpenGLWidget::paintGL 里调用）
    /// @param dpr 设备像素比：叠加层的逻辑像素乘它
    /// @return 设备建不成时返回 false，什么也没画
    bool render(RhiSurface& surface, double dpr);

    /// @brief 释放 GPU 资源（画布析构、上下文销毁前）
    void release();

    /// @brief 上一帧有没有重画场景底图（测试与计数用）
    bool lastFrameRedrewScene() const { return m_lastFrameRedrewScene; }

private:
    struct Pass;
    bool ensureDevice();
    bool ensureTargets(std::uint32_t width, std::uint32_t height, std::uint32_t samples);
    void prepareViewBits();
    RhiBufferPtr ensureBuffer(RhiBufferPtr& buffer, std::size_t bytes, RhiBufferUsage usage, const char* name);
    RhiBindGroupPtr frameGroup(std::uint32_t slot, const RhiBufferPtr& cells);
    void uploadCellOffsets(const std::vector<DmVector>& origins, RhiBufferPtr& buffer, std::vector<float>& uploaded,
                           double eyeX, double eyeY);
    void drawList(RhiCommandList& commands, const GsDrawList& list, const RhiBuffer& indirect, std::size_t indirectBase,
                  const GsModel& model, const RhiBindGroup& frame, const RhiBuffer& instances, std::uint32_t samples,
                  bool scene, bool hairlines);

    std::shared_ptr<GsDevice> m_device;
    std::shared_ptr<GsModel> m_model;
    bool m_selection = false;
    GsModel* m_transient = nullptr;
    const DmBlock* m_block = nullptr;

    DmVector m_center = DmVector(0.0, 0.0);
    double m_worldPerPixel = 1.0;
    GsViewStyle m_style;

    std::vector<DmEntity*> m_highlighted;
    std::vector<DmEntity*> m_hidden;
    std::vector<DmVector> m_grips;
    bool m_viewBitsDirty = true;
    bool m_highlightDirty = true;
    bool m_useBitmapHighlight = false;
    std::vector<std::uint32_t> m_viewBits;
    GsOverlay m_overlay;

    // 场景是否作废
    bool m_sceneValid = false;
    std::uint64_t m_sceneModelVersion = 0;
    DmVector m_sceneCenter;
    double m_sceneWorldPerPixel = 0.0;
    std::uint32_t m_sceneWidth = 0;
    std::uint32_t m_sceneHeight = 0;
    GsViewStyle m_sceneStyle;
    bool m_lastFrameRedrewScene = false;

    GsDrawList m_sceneList;
    GsDrawList m_emphasisList;           ///< 场景按细线画时，选中的实体另按四边形画的命令
    bool m_sceneHairlines = false;       ///< 场景里的线段按细线画
    std::size_t m_emphasisBase = 0;      ///< 加宽命令在场景间接参数里的位置
    std::size_t m_emphasisCount = 0;
    GsDrawList m_highlightList;
    std::uint64_t m_highlightModelVersion = 0;
    GsDrawList m_transientList;
    std::vector<GsInstanceRecord> m_blockInstances;

    // GPU：每视图
    RhiBufferPtr m_frameBuffer;          ///< 每帧常量，每个通道一段（256 字节对齐）
    RhiBufferPtr m_cellOffsets;          ///< 模型各分块原点相对视点的偏移（RG32F）
    RhiBufferPtr m_transientCellOffsets;
    std::vector<float> m_cellOffsetsUploaded;            ///< 上一次上传的分块偏移，没变就不重传
    std::vector<float> m_transientCellOffsetsUploaded;
    RhiBufferPtr m_viewBitsBuffer;       ///< 状态位图（R32UI）
    RhiBufferPtr m_indirect;              ///< 叠加通道的间接参数，每帧上传
    RhiBufferPtr m_sceneIndirect;         ///< 场景的间接参数，重画场景时上传
    std::vector<RhiDrawIndirectArgs> m_sceneIndirectData;  ///< 组装场景间接参数的 CPU 缓冲（复用容量）
    std::vector<RhiDrawIndirectArgs> m_indirectData;
    RhiBufferPtr m_overlayVertices;
    RhiBufferPtr m_blockInstanceBuffer;
    RhiTexturePtr m_sceneColor;          ///< 场景底图（多重采样时为多重采样的附件）
    RhiTexturePtr m_sceneDepth;
    RhiTexturePtr m_sceneResolve;        ///< 解析后的场景底图（单采样，贴图用）
    RhiRenderTargetPtr m_sceneTarget;
    RhiBindGroupPtr m_blitGroup;
    std::uint32_t m_targetWidth = 0;
    std::uint32_t m_targetHeight = 0;
    std::uint32_t m_targetSamples = 0;
};

#endif // GSVIEW_H
