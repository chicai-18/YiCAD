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

/// @file SamplePipeEntity.h
/// @brief 测试用的示例自定义实体"管道"（RENDER_PLAN.md 第 7.4 步），只编进测试程序、不随产品发布
///
/// 一串折点加管径。只实现自定义实体必须实现的函数（worldDraw、变换、clone、数据读写）与夹点，
/// 包围框、拾取、捕捉、交叉选、炸开都用宿主的默认实现（DmCustomEntity），测试据此检查默认实现。

#ifndef YICAD_TEST_SAMPLE_PIPE_ENTITY_H
#define YICAD_TEST_SAMPLE_PIPE_ENTITY_H

#include <vector>

#include "DmCustomEntity.h"
#include "DmCustomEntityRegistry.h"

/// @brief 示例实体"管道"，类名 ext.sample.Pipe
/// @details 画法：沿折点的中心线；每段两侧距中心线管径一半的两条边线；起点、终点各一个朝外的半圆端头；
///          第一段中点一个红色实心箭头（两个三角形），指向第二个折点。
///          夹点：每个折点一个，起点处垂直于第一段、距中心线管径一半处一个（拖它改管径）
class SamplePipeEntity final : public DmCustomEntity
{
    TYPESYSTEM_HEADER();

public:
    static constexpr std::uint32_t kVersion = 1;    ///< 数据版本，同 TYPESYSTEM_SOURCE_NAMED 的版本号

    SamplePipeEntity() = default;
    SamplePipeEntity(const std::vector<DmVector>& vertices, double diameter);

    DmEntity* clone() const override;

    const std::vector<DmVector>& vertices() const { return m_vertices; }
    double diameter() const { return m_diameter; }
    void setVertices(const std::vector<DmVector>& vertices);
    void setDiameter(double diameter);

    void worldDraw(IGiWorldDraw& wd) const override;

    void move(const DmVector& offset) override;
    void rotate(const DmVector& center, const DmVector& angleVector) override;
    void scale(const DmVector& center, const DmVector& factor) override;
    void mirror(const DmVector& axisPoint1, const DmVector& axisPoint2) override;

    DmVectorSolutions getRefPoints() const override;
    void moveRef(const DmVector& ref, const DmVector& offset) override;

    void saveData(OutputStream& out) const override;
    bool restoreData(InputStream& in, std::uint32_t version) override;

    /// @brief 改管径的夹点
    DmVector diameterGrip() const;

private:
    std::vector<DmVector> m_vertices;
    double m_diameter = 1.0;
};

/// @brief 用例期间把 ext.sample.Pipe 登记进注册表，析构时注销（不经扩展的模型层用例用）
struct SamplePipeRegistration
{
    explicit SamplePipeRegistration(DmProxyFlags proxyFlags = DmProxyFlags::Erase | DmProxyFlags::Transform)
    {
        registered = DmCustomEntityRegistry::instance().registerClass(
            DmCustomEntityRegistry::describe<SamplePipeEntity>(proxyFlags, QStringLiteral("ext.sample")));
    }
    ~SamplePipeRegistration()
    {
        if (registered)
        {
            DmCustomEntityRegistry::instance().unregisterClass(QStringLiteral("ext.sample.Pipe"));
        }
    }
    SamplePipeRegistration(const SamplePipeRegistration&) = delete;
    SamplePipeRegistration& operator=(const SamplePipeRegistration&) = delete;

    bool registered = false;
};

#endif // YICAD_TEST_SAMPLE_PIPE_ENTITY_H
