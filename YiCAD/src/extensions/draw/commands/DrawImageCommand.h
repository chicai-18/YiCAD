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

/// @file DrawImageCommand.h
/// @brief 插入图片命令 ext.draw.image，取代原 ActionDrawImage：选图片文件，指定插入点
///
/// 启动时弹出选择图片的对话框，取消则命令启动失败（原先 Action 被标记为结束）。
/// 放下一张图片后命令结束。

#ifndef DRAWIMAGECOMMAND_H
#define DRAWIMAGECOMMAND_H

#include <memory>

#include <QCoreApplication>

#include "PlaceCommand.h"

class DmVector;
class ImageData;

/// @brief 插入图片命令；交互由 DrawImageTool 驱动，图片数据在本类
class DrawImageCommand : public PlaceCommand
{
    Q_DECLARE_TR_FUNCTIONS(DrawImageCommand)

public:
    DrawImageCommand();
    ~DrawImageCommand() override;

    // ---- 选项条（UIImageOptions）----

    /// @brief 旋转角（弧度）
    double getAngle() const;
    void setAngle(double a);
    /// @brief 缩放因子
    double getFactor() const;
    void setFactor(double f);
    /// @brief 按文档单位把 DPI 换算为缩放因子
    double dpiToScale(double dpi) const;
    /// @brief 按文档单位把缩放因子换算为 DPI
    double scaleToDpi(double scale) const;

    // ---- 预览与提交（工具调用）----

    /// @brief 在插入点预览图片的外框
    void previewFrame(const DmVector& insertionPoint);
    /// @brief 在插入点放下图片，然后结束命令
    void commitImage(const DmVector& insertionPoint);

protected:
    /// @brief 先弹出选择图片的对话框：取消时返回空，命令启动失败
    std::unique_ptr<BasePlaceTool> createTool() override;
    void showOptions() override;
    void hideOptions() override;

private:
    struct Image;
    std::unique_ptr<Image> m_image; ///< 图片数据与像素
};

#endif // DRAWIMAGECOMMAND_H
