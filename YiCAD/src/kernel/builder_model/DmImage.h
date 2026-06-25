/**
 * Copyright (c) 2011-2018 by Andrew Mustun. All rights reserved.
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is part of the YiCAD project.
 *
 * YiCAD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YiCAD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */


/// @file DmImage.h
/// @brief 光栅图像实体，支持图片载入、变换和查询

#ifndef DMIMAGE_H
#define DMIMAGE_H

#include <memory>

#include "DmAtomicEntity.h"
#include "ImageData.h"

class QImage;

class DmImage : public DmAtomicEntity
{
public:
    /// @brief 构造图像实体
    /// @param [in] parent 父实体
    /// @param [in] d 图像数据
    DmImage(DmEntity* parent, const ImageData& d);

    DmImage(const DmImage& _image);
    DmImage(DmImage&& _image);
    DmImage& operator=(const DmImage& _image);
    DmImage& operator=(DmImage&& _image);

    DmEntity* clone() const override;

    /// @return DM::EntityImage
    DM::EntityType getEntityType() const override;

    void update() override;

    /// @return Copy of data that defines the image.
    ImageData getData() const;

    /// @return Insertion point of the entity
    DmVector getInsertionPoint() const;

    /// Sets the insertion point for the image.
    void setInsertionPoint(DmVector ip);

    /// Update image data ONLY for plugins.
    /// @param [in] size 图像尺寸
    /// @param [in] Uv U方向向量
    /// @param [in] Vv V方向向量
    void updateData(DmVector size, DmVector Uv, DmVector Vv);

    /// @return File name of the image.
    QString getFile() const;

    /// Sets the file name of the image.
    void setFile(const QString& file);

    /// @return u Vector. Points along bottom, 1 pixel long.
    DmVector getUVector() const;

    /// @return v Vector. Points along left, 1 pixel long.
    DmVector getVVector() const;

    /// @return Width of image in pixels.
    int getWidth() const;

    /// @return Height of image in pixels.
    int getHeight() const;

    /// @return Brightness.
    int getBrightness() const;

    /// @return Contrast.
    int getContrast() const;

    /// @return Fade.
    int getFade() const;

    /// @return Image definition handle.
    int getHandle() const;

    /// Sets the image definition handle.
    void setHandle(int h);

    /// @return The four corners.
    DmVectorSolutions getCorners() const;

    /// @return image width in document units.
    double getImageWidth();

    /// @return image height in document units.
    double getImageHeight();

    /// @return 每行字节数
    int getBytesPerLine();

    /// @brief 设置每行字节数
    void setBytesPerLine(const int& len);

    /// @return 图像位数据指针
    unsigned char* getbits();

    /// @brief 设置图像位数据
    void setBits(unsigned char* bit);

    DmVector getNearestEndpoint(const DmVector& coord,
        double* dist = nullptr) const override;
    DmVector getNearestPointOnEntity(const DmVector& coord,
        bool onEntity = true, double* dist = nullptr,
        DmEntity** entity = nullptr) const override;
    DmVector getNearestCenter(const DmVector& coord,
        double* dist = nullptr) const override;
    DmVector getNearestMiddle(const DmVector& coord,
        double* dist = nullptr, int middlePoints = 1) const override;
    double getDistanceToPoint(const DmVector& coord,
        DmEntity** entity = nullptr,
        DM::ResolveLevel level = DM::ResolveNone) const override;

    void move(const DmVector& offset) override;
    void rotate(const DmVector& center, const DmVector& angleVector) override;
    void scale(const DmVector& center, const DmVector& factor) override;
    void mirror(const DmVector& axisPoint1, const DmVector& axisPoint2) override;

    void calculateBorders() override;

    void saveStream(OutputStream& wrt) const override;
    void restoreStream(InputStream& reader, const std::vector<PAIR>& revs) override;
    void restoreStreamWithRev(InputStream& rdr, int rev) override;
    void restoreStream(InputStream& rdr) override;

    std::list<DmEntity*> getSubEntities() const override;

protected:
    /// whether the point is within image
    bool containsPoint(const DmVector& coord) const;

protected:
    ImageData data;                       ///< 图像数据
    std::unique_ptr<QImage> img;          ///< Qt图像对象
    bool isInit = false;                  ///< 初始化标志

private:
    bool isModify = false;                ///< 修改标志
};

#endif // DMIMAGE_H
