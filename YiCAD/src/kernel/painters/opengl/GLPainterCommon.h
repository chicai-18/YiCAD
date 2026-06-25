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

/// @file GLPainterCommon.h
/// @brief 画笔通用功能类，提供设备坐标转换、视图矩阵、着色器管理等共享功能

#ifndef GLPAINTERCOMMON_H
#define GLPAINTERCOMMON_H

#include <glm/glm.hpp>
#include "GLShader.h"
#include "GLVertexArray.h"
#include "GLVertexBuffer.h"
#include "GLFrameBuffer.h"
#include "GLRenderBuffer.h"
#include "GLTexture.h"
#include "GLPenData.h"

namespace opengl
{

/// @brief 画笔的通用数据及方法
class GLPainterCommon
{
public:
    GLPainterCommon();
    ~GLPainterCommon();

    /// @brief 设备宽度（像素）
    /// @return 设备宽度
    double device_width();

    /// @brief 设备高度（像素）
    /// @return 设备高度
    double device_height();

    /// @brief 初始化glew及shader
    void create_resources();

    /// @brief 指定视图尺寸
    /// @param [in] width 宽度（像素）
    /// @param [in] height 高度（像素）
    void new_device_size(unsigned int width, unsigned int height);

    /// @brief 用户坐标转设备坐标
    /// @param [in] x 用户坐标X
    /// @param [in] y 用户坐标Y
    /// @param [out] deviceX 设备坐标X
    /// @param [out] deviceY 设备坐标Y
    void user_to_device(double x, double y, double* deviceX, double* deviceY);

    /// @brief 设备坐标转用户坐标
    /// @param [in] x 设备坐标X
    /// @param [in] y 设备坐标Y
    /// @param [out] userX 用户坐标X
    /// @param [out] userY 用户坐标Y
    void device_to_user(double x, double y, double* userX, double* userY);

    /// @brief 获取当前视图比例
    /// @return 单位设备坐标对应的世界坐标
    double getScale();

    /// @brief 直接指定视图比例，即单位设备坐标对应的世界坐标
    /// @param [in] s 比例值
    void setScale(double s);

    /// @brief 直接指定视图的原点，即屏幕中心对应的世界坐标
    /// @param [in] posx 原点X坐标
    /// @param [in] posy 原点Y坐标
    void setViewPosition(double posx, double posy);

    /// @brief 在指定位置做视图缩放。s<1时，放大视图以查看更小的实体。
    /// @param [in] s 缩放因子
    /// @param [in] x_world 缩放中心世界坐标X
    /// @param [in] y_world 缩放中心世界坐标Y
    void scale(double s, double x_world, double y_world);

    /// @brief 移动视图
    /// @param [in] x 水平偏移
    /// @param [in] y 垂直偏移
    void translateView(double x, double y);

    /// @brief 指定模型矩阵的偏移量
    /// @param [in] offsetX 水平偏移
    /// @param [in] offsetY 垂直偏移
    void setModelOffset(double offsetX, double offsetY);

    /// @brief 设置颜色
    /// @param [in] R 红色分量
    /// @param [in] G 绿色分量
    /// @param [in] B 蓝色分量
    /// @param [in] A 透明度分量
    void selectColor(float R, float G, float B, float A);

    /// @brief 设置选中颜色
    /// @param [in] R 红色分量
    /// @param [in] G 绿色分量
    /// @param [in] B 蓝色分量
    /// @param [in] A 透明度分量
    void selectSelectedColor(float R, float G, float B, float A);

    /// @brief 设置高亮颜色
    /// @param [in] R 红色分量
    /// @param [in] G 绿色分量
    /// @param [in] B 蓝色分量
    /// @param [in] A 透明度分量
    void selectHighlightColor(float R, float G, float B, float A);

    /// @brief 使用指定类型着色器
    /// @param [in] shaderType 着色器类型
    void useShader(ShaderType shaderType);

    /// @brief 发送uniform变量
    /// @param [in] isSelected 是否选中状态
    /// @param [in] isHighlight 是否高亮状态
    /// @param [in] isDisplayLineWidth 是否显示线宽
    /// @param [in] penData 画笔数据
    void sendUniform(bool isSelected, bool isHighlight, bool isDisplayLineWidth, const opengl::GLPenData& penData);

    /// @brief 计算并发送mvp矩阵
    void sendMVP();

    /// @brief 获得当前shader类型
    /// @return 着色器类型
    ShaderType getShaderType();

    /// @brief 获取当前使用的着色器
    /// @return 着色器指针
    GLShader* getCurrentShader() const;

private:
    /// @brief 计算MVP矩阵
    void calculateMVP();

    /// @brief 更新透视矩阵
    void updateProj();

private:
    glm::mat4           m_model;        ///< 模型矩阵
    glm::mat4           m_view;         ///< 视图矩阵，仅含移动变换参数
    glm::mat4           m_proj;         ///< 透视矩阵，它将视体映射为[-1,1]的正方体，视体外的点映射在正方体外。参考：https://blog.csdn.net/qq_37987033/article/details/129110785
    glm::mat4           m_mv;
    glm::mat4           m_mvp;
    bool                m_matChanged = true; ///< 矩阵是否已修改

    GLShaders_book      m_shaderBook;
    GLShader*           m_currentShader = nullptr;    ///< 当前shader
    std::string         m_shader_path;                ///< shader 文件所在目录

    float               m_deviceWidth = 0.0f;   ///< 设备宽度
    float               m_deviceHeight = 0.0f;  ///< 设备高度
    float               m_scale = 1.0f;         ///< 单位设备坐标对应的世界坐标
};

/// @brief 将 GLPainterCommon 的功能委托给子类的宏定义
/// 用于 GLPainter 和 GLCachePainter 共享通用功能代码
#define GL_PAINTER_COMMON() \
private: \
    GLPainterCommon m_painterCommon; \
public: \
    double device_width() { return m_painterCommon.device_width(); } \
    double device_height() { return m_painterCommon.device_height(); } \
    void create_resources() { m_painterCommon.create_resources(); } \
    void new_device_size(unsigned int width, unsigned int height) { m_painterCommon.new_device_size(width, height); } \
    void user_to_device(double x, double y, double* deviceX, double* deviceY) { m_painterCommon.user_to_device(x, y, deviceX, deviceY); } \
    void device_to_user(double x, double y, double* userX, double* userY) { m_painterCommon.device_to_user(x, y, userX, userY); } \
    double getScale() { return m_painterCommon.getScale(); } \
    void setScale(double s) { m_painterCommon.setScale(s); } \
    void setViewPosition(double posx, double posy) { m_painterCommon.setViewPosition(posx, posy); } \
    void scale(double s, double x_world, double y_world) { m_painterCommon.scale(s, x_world, y_world); } \
    void translateView(double x, double y) { m_painterCommon.translateView(x, y); } \
    void setModelOffset(double offsetX, double offsetY) { m_painterCommon.setModelOffset(offsetX, offsetY); }

}

#endif //GLPAINTERCOMMON_H
