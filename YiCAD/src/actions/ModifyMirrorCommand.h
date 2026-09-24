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

/// @file ModifyMirrorCommand.h
/// @brief 镜像命令，取代原 ActionModifyMirror：指定镜像线的两点，复制或替换选择集

#ifndef MODIFYMIRRORCOMMAND_H
#define MODIFYMIRRORCOMMAND_H

#include <memory>

#include <QCoreApplication>

#include "SelectFirstCommand.h"

class CommandPreview;
class DmVector;

/// @brief 镜像命令；交互由 ModifyMirrorTool 驱动
class ModifyMirrorCommand : public SelectFirstCommand
{
    Q_DECLARE_TR_FUNCTIONS(ModifyMirrorCommand)

public:
    ModifyMirrorCommand();
    ~ModifyMirrorCommand() override;

    /// @brief 是否保留原实体（复制）；false 表示删除原实体
    bool copies() const { return m_copy; }
    /// @brief 命令行输入 Y/N 切换复制与删除原实体
    /// @return 输入不是 y/n 时返回 false，设置不变
    bool setCopyMode(const QString& input);

    /// @brief 预览选择集关于镜像线的镜像，连同镜像线
    void previewMirror(const DmVector& axisPoint1, const DmVector& axisPoint2);
    /// @brief 清除预览
    void clearPreview();
    /// @brief 按镜像线镜像选择集，然后结束命令
    void commitMirror(const DmVector& axisPoint1, const DmVector& axisPoint2);

protected:
    /// @brief 从设置读取复制方式，激活镜像工具
    bool onSelectionReady() override;
    /// @brief 把复制方式写回设置
    void onStop() override;

private:
    std::unique_ptr<CommandPreview> m_preview;
    bool m_copy = true;
};

#endif // MODIFYMIRRORCOMMAND_H
