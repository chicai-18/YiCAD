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

/// @file ImageData.cpp
/// @brief 图像数据结构类实现

#include "ImageData.h"

/// @brief 默认构造函数
ImageData::ImageData()
    : EntityData()
    , m_iHandle(0)
    , m_ptInsertionPoint(0, 0, 0)
    , m_ptUVector(1, 0, 0)
    , m_ptVVector(0, 1, 0)
    , m_ptSize(1, 1, 0)
    , m_strFilePath("")
    , m_iBrightness(50)
    , m_iContrast(50)
    , m_iFade(0)
{
    setEntityType(EEntityType::eImage);
}

/// @brief 带全部参数的构造函数
/// @param handle 图像句柄
/// @param insertionPoint 插入点
/// @param uVector U方向向量
/// @param vVector V方向向量
/// @param size 图像尺寸
/// @param file 文件路径
/// @param brightness 亮度
/// @param contrast 对比度
/// @param fade 褪色度
/// @param perLine 每行字节数
/// @param buf 图像数据缓冲区
ImageData::ImageData(int handle, const DmVector& insertionPoint, const DmVector& uVector, const DmVector& vVector, const DmVector& size, const std::string& file, int brightness, int contrast, int fade, int perLine, unsigned char* buf)
    : EntityData()
    , m_iHandle(handle)
    , m_ptInsertionPoint(insertionPoint)
    , m_ptUVector(uVector)
    , m_ptVVector(vVector)
    , m_ptSize(size)
    , m_strFilePath(file)
    , m_iBrightness(brightness)
    , m_iContrast(contrast)
    , m_iFade(fade)
    , m_length(perLine)
    , m_pFirst(buf)
{
    setEntityType(EEntityType::eImage);
}

int ImageData::getHandle() const
{
    return m_iHandle;
}

void ImageData::setHandle(const int& handle)
{
    m_iHandle = handle;
}

DmVector ImageData::getInsertionPoint() const
{
    return m_ptInsertionPoint;
}

void ImageData::setInsertionPoint(const DmVector& pos)
{
    m_ptInsertionPoint = pos;
}

DmVector ImageData::getUVector() const
{
    return m_ptUVector;
}

void ImageData::setUVector(const DmVector& vec)
{
    m_ptUVector = vec;
}

DmVector ImageData::getVVector() const
{
    return m_ptVVector;
}

void ImageData::setVVector(const DmVector& vec)
{
    m_ptVVector = vec;
}

DmVector ImageData::getSize() const
{
    return m_ptSize;
}

void ImageData::setSize(const DmVector& size)
{
    m_ptSize = size;
}

std::string ImageData::getPath() const
{
    return m_strFilePath;
}

void ImageData::setPath(const std::string& path)
{
    m_strFilePath = path;
}

int ImageData::getBrightness() const
{
    return m_iBrightness;
}

void ImageData::setBrightness(const int& bri)
{
    m_iBrightness = bri;
}

int ImageData::getContrast() const
{
    return m_iContrast;
}

void ImageData::setContrast(const int& contrast)
{
    m_iContrast = contrast;
}

int ImageData::getFade() const
{
    return m_iFade;
}

void ImageData::setFade(const int& fade)
{
    m_iFade = fade;
}

DmRect ImageData::getRoundPoint() const
{
    return m_posVec;
}

void ImageData::setRoundPoint(const DmRect& pt)
{
    m_posVec = pt;
}

DmVector ImageData::getScale() const
{
    return m_scale;
}

void ImageData::setScale(const DmVector& scale)
{
    m_scale = scale;
}

int ImageData::getBytesPerLine()
{
    return m_length;
}

void ImageData::setBytesPerLine(const int& len)
{
    m_length = len;
}

unsigned char* ImageData::getbits()
{
    return m_pFirst;
}

void ImageData::setBits(unsigned char* bit)
{
    m_pFirst = bit;
}
