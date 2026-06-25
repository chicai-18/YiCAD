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

/// @file QuarticEquation.h
/// @brief 一元四次方程求解类，参考：https://baike.baidu.com/item/%E4%B8%80%E5%85%83%E5%9B%9B%E6%AC%A1%E6%96%B9%E7%A8%8B/5945955?fr=ge_ala

#ifndef QUARTICEQUATION_H
#define QUARTICEQUATION_H

#include <vector>
#include <complex>

/// @brief 一元四次方程 a*x^4 + b*x^3 + c*x^2 + d*x + e = 0 求解
class QuarticEquation
{
public:
    /// @brief 构造函数
    /// @param [in] a 四次项系数
    /// @param [in] b 三次项系数
    /// @param [in] c 二次项系数
    /// @param [in] d 一次项系数
    /// @param [in] e 常数项
    QuarticEquation(double a, double b, double c, double d, double e);

    /// @brief 获取所有实数解
    /// @return 实数解向量
    std::vector<double> getResults();

private:
    /// @brief 对一个复数开n次方
    /// @param [in] x 复数
    /// @param [in] n 方根次数
    /// @return 开n次方后的复数
    std::complex<double> sqrtn(const std::complex<double>& x, double n);

    /// @brief 使用费拉里法求解一元四次方程
    /// @param [out] x 四个复数解的数组
    /// @param [in] a 四次项系数
    /// @param [in] b 三次项系数
    /// @param [in] c 二次项系数
    /// @param [in] d 一次项系数
    /// @param [in] e 常数项
    void Ferrari(std::complex<double> x[4]
        , std::complex<double> a
        , std::complex<double> b
        , std::complex<double> c
        , std::complex<double> d
        , std::complex<double> e);

private:
    double m_a;   ///< 四次项系数
    double m_b;   ///< 三次项系数
    double m_c;   ///< 二次项系数
    double m_d;   ///< 一次项系数
    double m_e;   ///< 常数项
};

#endif //QUARTICEQUATION_H
