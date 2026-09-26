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

/// @file ImageData.h
/// @brief 图像数据结构类，定义图像的插入点、方向向量、UV向量、文件路径等属性

#ifndef IMAGEDATA_H
#define IMAGEDATA_H

#include "EntityData.h"
#include "DmVector.h"
#include "DmRect.h"

/// @brief 图像数据结构
class ImageData : public EntityData
{
public:
    /// @brief 默认构造函数
    ImageData();

    /// @brief 带全部参数的构造函数
    /// @param handle 图像句柄
    /// @param insertionPoint 插入点
    /// @param uVector U方向向量
    /// @param vVector V方向向量
    /// @param size 图像尺寸
    /// @param file 文件路径
    /// @param brightness 亮度（0-100）
    /// @param contrast 对比度（0-100）
    /// @param fade 褪色度（0-100）
    /// @param perLine 每行字节数
    /// @param buf 图像数据缓冲区
    ImageData(int handle, const DmVector& insertionPoint, const DmVector& uVector, const DmVector& vVector, const DmVector& size, const std::string& file, int brightness, int contrast, int fade, int perLine = 1, unsigned char* buf = nullptr);

public:
    /// @brief 获取句柄
    /// @return 图像句柄
    int getHandle() const;

    /// @brief 设置句柄
    /// @param handle 图像句柄
    void setHandle(const int& handle);

    /// @brief 获取插入点
    /// @return 插入点坐标
    DmVector getInsertionPoint() const;

    /// @brief 设置插入点
    /// @param pos 插入点坐标
    void setInsertionPoint(const DmVector& pos);

    /// @brief 获取U方向向量
    /// @return U方向向量
    DmVector getUVector() const;

    /// @brief 设置U方向向量
    /// @param vec U方向向量
    void setUVector(const DmVector& vec);

    /// @brief 获取V方向向量
    /// @return V方向向量
    DmVector getVVector() const;

    /// @brief 设置V方向向量
    /// @param vec V方向向量
    void setVVector(const DmVector& vec);

    /// @brief 获取图像尺寸
    /// @return 图像尺寸向量
    DmVector getSize() const;

    /// @brief 设置图像尺寸
    /// @param size 图像尺寸向量
    void setSize(const DmVector& size);

    /// @brief 获取文件路径
    /// @return 文件路径字符串
    std::string getPath() const;

    /// @brief 设置文件路径
    /// @param path 文件路径字符串
    void setPath(const std::string& path);

    /// @brief 获取亮度
    /// @return 亮度值（0-100）
    int getBrightness() const;

    /// @brief 设置亮度
    /// @param bri 亮度值（0-100）
    void setBrightness(const int& bri);

    /// @brief 获取对比度
    /// @return 对比度值（0-100）
    int getContrast() const;

    /// @brief 设置对比度
    /// @param contrast 对比度值（0-100）
    void setContrast(const int& contrast);

    /// @brief 获取褪色度
    /// @return 褪色度值（0-100）
    int getFade() const;

    /// @brief 设置褪色度
    /// @param fade 褪色度值（0-100）
    void setFade(const int& fade);

    /// @brief 获取包围矩形
    /// @return 包围矩形
    DmRect getRoundPoint() const;

    /// @brief 设置包围矩形
    /// @param pt 包围矩形
    void setRoundPoint(const DmRect& pt);

    /// @brief 获取缩放比例
    /// @return 缩放向量
    DmVector getScale() const;

    /// @brief 设置缩放比例
    /// @param scale 缩放向量
    void setScale(const DmVector& scale);

    /// @brief 获取每行字节数
    /// @return 每行字节数
    int getBytesPerLine();

    /// @brief 设置每行字节数
    /// @param len 每行字节数
    void setBytesPerLine(const int& len);

    /// @brief 获取位图数据指针
    /// @return 位图数据指针
    unsigned char* getbits();

    /// @brief 设置位图数据
    /// @param bit 位图数据指针
    void setBits(unsigned char* bit);

    /// @brief 设置绘制插入点
    /// @param pt 插入点坐标
    void setDrawInsertionPoint(const DmVector& pt);

    /// @brief 设置绘制宽度
    /// @param dWidth 绘制宽度
    void setDrawWidth(const double& dWidth);

    /// @brief 设置绘制高度
    /// @param dHeight 绘制高度
    void setDrawHeight(const double& dHeight);

    /// @brief 设置绘制旋转角度
    /// @param dRotation 旋转角度
    void setDrawRotation(const double& dRotation);

    /// @brief 设置绘制ID
    /// @param id 绘制ID
    void setDrawId(const unsigned long int& id);

private:
    int             m_iHandle;              ///< 图像句柄
    DmVector        m_ptInsertionPoint;     ///< 插入点
    DmVector        m_ptUVector;            ///< U方向向量
    DmVector        m_ptVVector;            ///< V方向向量
    DmVector        m_ptSize;               ///< 图像尺寸
    std::string     m_strFilePath;          ///< 文件路径
    int             m_iBrightness;          ///< 亮度
    int             m_iContrast;            ///< 对比度
    int             m_iFade;                ///< 褪色度
    int             m_length;               ///< 每行字节数
    DmRect          m_posVec;               ///< 包围矩形
    DmVector        m_scale;                ///< 缩放
    unsigned char*  m_pFirst;               ///< 位图数据首指针
};

#endif // IMAGEDATA_H
