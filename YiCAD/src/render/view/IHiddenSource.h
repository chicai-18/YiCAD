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

/// @file IHiddenSource.h
/// @brief 绘制时取临时隐藏的实体的只读接口（RENDER_PLAN.md 第 4.3.9 节）
///
/// 命令为了显示预览要暂时不画文档里的某些实体（修剪时光标下的实体换成修剪后的预览）。原先命令直接改实体的
/// 可见性再要求整图重建（P18），把持久属性当临时显示状态用；现在临时隐藏归 Application 层的临时隐藏集
/// （HiddenSet，每个视图一个，命令结束时清空），与高亮集同样的设计，Render 只经本接口读取。

#ifndef IHIDDENSOURCE_H
#define IHIDDENSOURCE_H

#include <vector>

class DmEntity;

/// @brief 绘制时取临时隐藏的实体，见文件说明
class IHiddenSource
{
public:
    virtual ~IHiddenSource() = default;

    /// @brief 临时不画的顶层实体
    virtual std::vector<DmEntity*> hiddenEntities() const = 0;

    /// @brief 实体是否临时不画
    virtual bool isHidden(const DmEntity& entity) const = 0;
};

#endif  // IHIDDENSOURCE_H
