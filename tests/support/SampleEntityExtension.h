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

/// @file SampleEntityExtension.h
/// @brief 测试用的示例扩展 ext.sample：登记示例自定义实体"管道"与它的命令（RENDER_PLAN.md 第 7.4 步）
///
/// 只编进 test_interaction，不随产品发布。登记：
///   - 实体类 ext.sample.Pipe（SamplePipeEntity），代理权限见 kProxyFlags；
///   - 交互命令 ext.sample.pipe：指定起点、终点画一段管道（管径 kDiameter），移动鼠标时预览；
///   - 即时命令 ext.sample.properties，按类名登记为 ext.sample.Pipe 的属性编辑命令：记下要编辑的实体。

#ifndef YICAD_TEST_SAMPLE_ENTITY_EXTENSION_H
#define YICAD_TEST_SAMPLE_ENTITY_EXTENSION_H

#include <string_view>

#include "DmCustomEntity.h"
#include "IExtension.h"

class DmEntity;

/// @brief 示例扩展，见文件说明
class SampleEntityExtension final : public IExtension
{
public:
    /// @brief 管道类登记的代理权限：删除、变换、改图层、改颜色
    static constexpr DmProxyFlags kProxyFlags =
        DmProxyFlags::Erase | DmProxyFlags::Transform | DmProxyFlags::LayerChange | DmProxyFlags::ColorChange;
    /// @brief ext.sample.pipe 画的管道的管径
    static constexpr double kDiameter = 2.0;

    void OnRegister(IExtensionContext& ctx) override;
    std::string_view Id() const override { return "ext.sample"; }

    /// @brief ext.sample.properties 运行的次数与最后一次要编辑的实体
    static int propertyRuns;
    static DmEntity* lastEditedEntity;
};

#endif // YICAD_TEST_SAMPLE_ENTITY_EXTENSION_H
