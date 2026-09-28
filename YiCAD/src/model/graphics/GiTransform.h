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

/// @file GiTransform.h
/// @brief GI 的二维仿射变换（2×3，double）

#ifndef GITRANSFORM_H
#define GITRANSFORM_H

#include "DmVector.h"

/// @brief 二维仿射变换，x' = a·x + c·y + tx，y' = b·x + d·y + ty
/// @details 列 (a, b)、(c, d) 是 X、Y 轴的像，(tx, ty) 是原点的像。块参照的插入、字符的排版都用它表示，
///          可以是非等比缩放、镜像与错切（RENDER_PLAN.md 第 4.3.3 节）
class GiTransform
{
public:
    /// @brief 恒等变换
    GiTransform() = default;

    /// @brief 由 6 个系数构造，含义见类说明
    GiTransform(double a, double b, double c, double d, double tx, double ty);

    /// @brief 平移
    static GiTransform translation(const DmVector& offset);

    /// @brief 绕 center 逆时针旋转 angle 弧度
    static GiTransform rotation(double angle, const DmVector& center = DmVector(0.0, 0.0));

    /// @brief 以 center 为中心按 factor.x、factor.y 缩放
    static GiTransform scaling(const DmVector& factor, const DmVector& center = DmVector(0.0, 0.0));

    /// @brief 以过 axisPoint1、axisPoint2 的直线为轴镜像；两点重合时为恒等变换
    static GiTransform mirroring(const DmVector& axisPoint1, const DmVector& axisPoint2);

    double a() const { return m_a; }
    double b() const { return m_b; }
    double c() const { return m_c; }
    double d() const { return m_d; }
    double tx() const { return m_tx; }
    double ty() const { return m_ty; }

    /// @brief 变换一个点
    DmVector apply(const DmVector& point) const;

    /// @brief 变换一个向量（不含平移）
    DmVector applyVector(const DmVector& vector) const;

    /// @brief 合成：先做 rhs，再做本变换
    GiTransform operator*(const GiTransform& rhs) const;

    /// @brief 逆变换；行列式为 0 时返回恒等变换
    GiTransform inverse() const;

    /// @brief 线性部分的行列式；小于 0 表示含镜像
    double determinant() const;

    /// @brief 是否为恒等变换
    bool isIdentity(double tolerance = 1.0e-12) const;

    /// @brief 是否为相似变换（旋转、等比缩放、镜像与平移的组合），即圆仍变成圆
    /// @param scale 非空时输出缩放比例（正数）
    bool isSimilarity(double* scale = nullptr, double tolerance = 1.0e-9) const;

    bool operator==(const GiTransform& other) const;
    bool operator!=(const GiTransform& other) const { return !(*this == other); }

private:
    double m_a = 1.0;   ///< X 轴的像的 x 分量
    double m_b = 0.0;   ///< X 轴的像的 y 分量
    double m_c = 0.0;   ///< Y 轴的像的 x 分量
    double m_d = 1.0;   ///< Y 轴的像的 y 分量
    double m_tx = 0.0;  ///< 平移 x
    double m_ty = 0.0;  ///< 平移 y
};

#endif // GITRANSFORM_H
