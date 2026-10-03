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

/// @file GuiPreviewWidget.h
/// @brief 预览控件：块面板、块对话框、样式对话框里的小画布，以及多行文字编辑器的基类
///
/// 用图形系统的视图 GsView 画（RENDER_PLAN.md 第 4.8 步），两种内容：
/// - 块定义（setBlock）：直接画文档图形模型里这个块的共享几何，与画布共用，不克隆实体、不重新编译；
/// - 实体容器（setContainer）：控件自己的容器模型，内容改了调用 specifyModified() 整体重建。

#ifndef GUIPREVIEWWIDGET_H
#define GUIPREVIEWWIDGET_H

// GLEW 要求先于 gl.h（QOpenGLWidget 会带进来）被包含；本控件的实现里用 GL 后端的表面
#include <GL/glew.h>

#include <memory>

#include <QOpenGLWidget>

#include "DmColor.h"
#include "DmVector.h"

class DmBlock;
class DmEntityContainer;
class GLRhiWidgetSurface;
class GsModel;
class GsView;

/// @brief 预览控件，见文件说明
class GuiPreviewWidget : public QOpenGLWidget
{
public:
    /// @brief 构造预览控件
    /// @param parent 父控件
    /// @param fl 窗口标志
    GuiPreviewWidget(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~GuiPreviewWidget() override;

    /// @brief 预览一个实体容器；容器归调用方，内容改了调用 specifyModified()
    void setContainer(DmEntityContainer* container);

    /// @brief 预览一个块定义（定义坐标）
    /// @param model 文档的图形模型，块的几何从它取；为空时（没有画布的场合）控件为这个块建自己的模型
    /// @param block 块定义；为空时什么也不画
    void setBlock(std::shared_ptr<GsModel> model, const DmBlock* block);

    /// @brief 适屏显示：内容居中、铺满控件；之后控件尺寸或内容变了也保持适屏
    void zoomAuto();
    /// @brief 重绘控件
    void redraw();
    /// @brief 容器的内容改了：下一帧整体重建
    void specifyModified();

protected:
    /// @brief 相机：控件中心的世界坐标与每个（逻辑）像素的世界长度；设了就不再自动适屏
    void setCamera(const DmVector& center, double worldPerPixel);

    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

private:
    /// @brief 容器模式下容器换了（派生类直接改 container）时换模型
    void syncContainerModel();
    /// @brief 内容的包围框
    bool contentBounds(DmVector& minCorner, DmVector& maxCorner) const;
    /// @brief 按当前尺寸与内容算适屏的相机
    void fitCamera();

protected:
    DmEntityContainer*  container = nullptr;                ///< 预览的实体容器（容器模式），归调用方
    DmColor             background = DmColor(30, 30, 30, 255);///< 背景色

private:
    std::unique_ptr<GsView>             m_view;
    std::unique_ptr<GLRhiWidgetSurface> m_surface;
    std::shared_ptr<GsModel>            m_model;            ///< 文档的图形模型（块模式）或容器模型
    const DmEntityContainer*            m_modelContainer = nullptr;  ///< m_model 画的容器（容器模式）
    const DmBlock*                      m_block = nullptr;  ///< 块模式下预览的块
    DmVector                            m_blockMin;         ///< 块的包围框（setBlock 时算好）
    DmVector                            m_blockMax;
    bool                                m_hasBlockBounds = false;
    DmVector                            m_center = DmVector(0.0, 0.0);
    double                              m_worldPerPixel = 1.0;
    bool                                m_fit = true;       ///< 自动适屏
};

#endif
