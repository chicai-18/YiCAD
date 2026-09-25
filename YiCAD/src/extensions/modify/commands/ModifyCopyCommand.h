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

/// @file ModifyCopyCommand.h
/// @brief 复制命令，取代原 ActionModifyCopy：指定参考点与目标点，按复制数量阵列复制选择集

#ifndef MODIFYCOPYCOMMAND_H
#define MODIFYCOPYCOMMAND_H

#include <memory>
#include <vector>

#include <QCoreApplication>

#include "SelectFirstCommand.h"

class CommandPreview;
class DmEntity;
class DmVector;

/// @brief 复制命令；交互由 ModifyCopyTool 驱动
class ModifyCopyCommand : public SelectFirstCommand
{
    Q_DECLARE_TR_FUNCTIONS(ModifyCopyCommand)

public:
    ModifyCopyCommand();
    ~ModifyCopyCommand() override;

    /// @brief 复制数量（每份相对上一份再偏移一次）
    int copyCount() const { return m_copyCount; }
    /// @brief 命令行输入复制数量
    /// @return 输入不是大于 0 的整数时返回 false，数量不变
    bool setCopyCount(const QString& input);

    /// @brief 预览偏移后的各份复制，按住 Shift 时另画一条引导线
    void previewCopy(const DmVector& reference, const DmVector& target, bool showGuide);
    /// @brief 清除预览
    void clearPreview();
    /// @brief 按参考点到目标点的偏移复制选择集，然后结束命令
    void commitCopy(const DmVector& reference, const DmVector& target);

protected:
    /// @brief 从设置读取复制数量，激活复制工具
    bool onSelectionReady() override;
    /// @brief 把复制数量写回设置
    void onStop() override;

private:
    /// @brief 按偏移与复制数量克隆选中实体
    std::vector<DmEntity*> cloneSelection(const DmVector& offset) const;

    std::unique_ptr<CommandPreview> m_preview;
    int m_copyCount = 1;
};

#endif // MODIFYCOPYCOMMAND_H
