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

/// @file Quadratic.h
/// @brief 一元或二次曲线方程类，支持平移及旋转

#ifndef QUADRATIC_H
#define QUADRATIC_H

#include "DmVector.h"
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/io.hpp>

class DmVectorSolutions;
class DmAtomicEntity;
class DmEntity;

/// @brief 代表一次或二次方程，支持平移及旋转。
/// @details 不能表示一段有界的曲线，例如：（同心同半径）圆弧和圆的表示是一样的。
/// @details 包含移动，旋转变换
class Quadratic
{
public:
    explicit Quadratic();
    Quadratic(const Quadratic& lc0);
    Quadratic& operator=(const Quadratic& lc0);

    /// @brief 构造椭圆或双曲线，作为经过该点的切圆中心的轨迹
    /// @param [in] circle 原子实体（圆/弧/线）
    /// @param [in] point 经过的点
    Quadratic(const DmAtomicEntity* circle, const DmVector& point);

    /// @brief 构造椭圆或双曲线，作为这两个给定实体的公切圆中心的轨迹
    /// mirror选项允许指定关于直线的镜像二次曲线
    /// @param [in] circle0 第一个实体
    /// @param [in] circle1 第二个实体
    /// @param [in] mirror 是否镜像
    Quadratic(const DmEntity* circle0, const DmEntity* circle1, bool mirror = false);

    /// @brief 构造垂直平分线，即经过point0和point1的圆的圆心轨迹
    /// @param [in] point0 第一个点
    /// @param [in] point1 第二个点
    Quadratic(const DmVector& point0, const DmVector& point1);

    /// @brief 通过方程系数构造曲线方程
    /// @details 若系数个数为3，则代表D,E,F；若系数个数为6，则代表A,B,C,D,E,F
    /// @param [in] ce 系数向量
    Quadratic(std::vector<double> ce);

    /// @brief 获得方程系数（A,B,C,D,E,F）
    /// @return 系数向量
    std::vector<double> getCoefficients() const;

    /// @brief 平移
    /// @param [in] v 平移向量
    /// @return 平移后的 Quadratic 引用
    Quadratic move(const DmVector& v);

    /// @brief 旋转（绕原点）
    /// @param [in] a 旋转角度（弧度）
    /// @return 旋转后的 Quadratic 引用
    Quadratic rotate(const double& a);

    /// @brief 旋转（绕指定中心）
    /// @param [in] center 旋转中心
    /// @param [in] a 旋转角度（弧度）
    /// @return 旋转后的 Quadratic 引用
    Quadratic rotate(const DmVector& center, const double& a);

    /// @brief 是否为二次曲线
    /// @return 是二次曲线返回true
    bool isQuadratic() const;

    /// @brief 显式转换到bool
    explicit operator bool() const;

    /// @brief 是否有效
    /// @return 有效返回true
    bool isValid() const;

    /// @brief 与bool比较有效性
    /// @param [in] valid 布尔参数
    /// @return 有效性相同返回true
    bool operator==(bool valid) const;

    /// @brief 与bool比较有效性
    /// @param [in] valid 布尔参数
    /// @return 有效性不同返回true
    bool operator!=(bool valid) const;

    /// @brief 获取线性部分向量（可写）
    /// @return 线性部分向量的引用
    boost::numeric::ublas::vector<double>& getLinear();

    /// @brief 获取线性部分向量（只读）
    /// @return 线性部分向量的const引用
    const boost::numeric::ublas::vector<double>& getLinear() const;

    /// @brief 获取二次部分矩阵（可写）
    /// @return 二次部分矩阵的引用
    boost::numeric::ublas::matrix<double>& getQuad();

    /// @brief 获取二次部分矩阵（只读）
    /// @return 二次部分矩阵的const引用
    const boost::numeric::ublas::matrix<double>& getQuad() const;

    /// @brief 获取常数项（只读）
    /// @return 常数项的const引用
    double const& constTerm() const;

    /// @brief 获取常数项（可写）
    /// @return 常数项的引用
    double& constTerm();

    /// @brief 交换x,y坐标
    /// @return 交换后的新Quadratic
    Quadratic flipXY(void) const;

    /// @brief 获得指定角度的旋转矩阵的转置
    /// @param [in] angle 旋转角度（弧度）
    /// @return 旋转矩阵的转置
    static boost::numeric::ublas::matrix<double> rotationMatrix(const double& angle);

    /// @brief 求两个Quadratic的交点
    /// @param [in] l1 第一个曲线方程
    /// @param [in] l2 第二个曲线方程
    /// @return 交点的向量集
    static DmVectorSolutions getIntersection(const Quadratic& l1, const Quadratic& l2);

    friend std::ostream& operator<<(std::ostream& os, const Quadratic& l);

private:
    /// @brief 判断实体是否为Arc/Circle/Line类型
    /// @param [in] e 待判断实体
    /// @return 是则返回true
    bool isEntityArcCircleLine(const DmEntity* e) const;

private:
    // 说明：
    // 一般二次曲线方程：A*x^2 + B*xy + C*y^2 + D*x + E*y + F = 0，可分解为三部分：
    // （1）令m_mQuad = [ A   B/2 ]
    //                 [ B/2 C   ]
    //  [x y] × m_mQuad × [x y]T = A*x^2 + B*xy + C*y^2，（T表示转置）
    // （2）令m_vLinear = [ D, E ]
    //  m_vLinear * [x y] = D*x + E*y
    // （3）令m_dConst = F
    boost::numeric::ublas::matrix<double> m_mQuad;        ///< 二次部分矩阵
    boost::numeric::ublas::vector<double> m_vLinear;      ///< 线性部分向量
    double m_dConst = 0.0;                                ///< 常数项
    bool m_bIsQuadratic = false;                          ///< 标识是否为真正的二次曲线
    bool m_bValid = false;                                ///< 此二次型是否有效
};

#endif
