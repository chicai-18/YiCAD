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

/// @file CopyToLayerCommand.h
/// @brief 复制到图层命令，取代原 ActionCopyToLayer：拾取目标图层上的实体，指定基点与终点，
///        把选择集复制到该图层

#ifndef COPYTOLAYERCOMMAND_H
#define COPYTOLAYERCOMMAND_H

#include <memory>

#include <QCoreApplication>

#include "DmVector.h"
#include "SelectFirstCommand.h"

class CommandPreview;
class DmLayer;

/// @brief 复制到图层命令；交互由 CopyToLayerTool 驱动
class CopyToLayerCommand : public SelectFirstCommand
{
    Q_DECLARE_TR_FUNCTIONS(CopyToLayerCommand)

public:
    CopyToLayerCommand();
    ~CopyToLayerCommand() override;

    /// @brief 设置目标图层（拾取到的实体所在图层）
    void setTargetLayer(DmLayer* layer) { m_targetLayer = layer; }

    /// @brief 预览复制结果跟随鼠标：第一次按基点放置选择集的克隆，之后按鼠标位移平移
    /// @param basePoint 基点
    /// @param mouse 鼠标位置
    void previewAt(const DmVector& basePoint, const DmVector& mouse);
    /// @brief 清除预览
    void clearPreview();
    /// @brief 把选择集按基点到终点的位移复制到目标图层，然后结束命令
    void commitCopy(const DmVector& basePoint, const DmVector& endPoint);

protected:
    /// @brief 激活复制到图层工具
    bool onSelectionReady() override;

private:
    std::unique_ptr<CommandPreview> m_preview;
    DmLayer* m_targetLayer = nullptr; ///< 目标图层
    DmVector m_previewPos;            ///< 预览当前所在的位置
};

#endif // COPYTOLAYERCOMMAND_H
