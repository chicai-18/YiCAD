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

/// @file Math2d.h
/// @brief 二维数学工具类

#ifndef MATH2D_H
#define MATH2D_H

#include <vector>
#include <cmath>

class DmVector;
class DmVectorSolutions;
class QString;

/// @brief 二维数学工具类
class Math2d
{
private:
    Math2d() = delete;

public:
    /// @brief 四舍五入到最近的整数
    /// @param [in] v 待舍入值
    /// @return 舍入后的整数
    static int round(double v);

    /// @brief 根据精度四舍五入
    /// @param [in] v 待舍入值
    /// @param [in] precision 精度
    /// @return 舍入后的值
    static double round(double v, double precision);

    /// @brief 安全的幂函数
    /// @param [in] x 底数
    /// @param [in] y 指数
    /// @return 幂结果
    static double pow(double x, double y);

    /// @brief 对向量各分量求幂
    /// @param [in] x 向量
    /// @param [in] y 指数
    /// @return 幂结果向量
    static DmVector pow(DmVector x, double y);

    /// @brief 判断两个double是否相等（容差内）
    /// @param [in] d1 第一个值
    /// @param [in] d2 第二个值
    /// @return 相等返回true
    static bool equal(const double d1, const double d2);

    /// @brief 弧度转角度
    /// @param [in] a 弧度值
    /// @return 角度值
    static double rad2deg(double a);

    /// @brief 角度转弧度
    /// @param [in] a 角度值
    /// @return 弧度值
    static double deg2rad(double a);

    /// @brief 弧度转百分度
    /// @param [in] a 弧度值
    /// @return 百分度值
    static double rad2gra(double a);

    /// @brief 百分度转弧度
    /// @param [in] a 百分度值
    /// @return 弧度值
    static double gra2rad(double a);

    /// @brief 求最大公约数（欧几里得算法）
    /// @param [in] a 第一个数
    /// @param [in] b 第二个数
    /// @return 最大公约数
    static unsigned findGCD(unsigned a, unsigned b);

    /// @brief 判断角度a是否在a1和a2之间
    /// @param [in] a 测试角度
    /// @param [in] a1 范围起始
    /// @param [in] a2 范围结束
    /// @return 在范围内返回true
    static bool isAngleBetween(double a, double a1, double a2);

    /// @brief 将角度校正到 [0, +PI*2.0) 范围
    /// @param [in] a 待校正角度
    /// @return 校正后的角度
    static double correctAngle(double a);

    /// @brief 将角度校正到 [-PI, +PI) 范围
    /// @param [in] a 待校正角度
    /// @return 校正后的角度
    static double correctAngle2(double a);

    /// @brief 将角度校正为无符号 [0, +PI) 范围
    /// @param [in] a 待校正角度
    /// @return 校正后的角度
    static double correctAngleU(double a);

    /// @brief 计算角度差
    /// @param [in] a1 第一个角度
    /// @param [in] a2 第二个角度
    /// @param [in] reversed 是否反转
    /// @return 角度差
    static double getAngleDifference(double a1, double a2, bool reversed = false);

    /// @brief 计算最小无符号角度差绝对值
    /// @param [in] a1 第一个角度
    /// @param [in] a2 第二个角度
    /// @return 最小角度差
    static double getAngleDifferenceU(double a1, double a2);

    /// @brief 使角度可读（用于标注文字和文字镜像）
    /// @param [in] angle 原始角度
    /// @param [in] readable true: 使角度可读, false: 不可读
    /// @param [out] corrected 如果校正过则设为true
    /// @return 根据可读性校正后的角度
    static double makeAngleReadable(double angle, bool readable = true, bool* corrected = nullptr);

    /// @brief 判断角度是否可读
    /// @param [in] angle 测试角度
    /// @return 可读返回true
    static bool isAngleReadable(double angle);

    /// @brief 判断两个角是否指向同一方向
    /// @param [in] dir1 方向1
    /// @param [in] dir2 方向2
    /// @param [in] tol 容差（弧度）
    /// @return 同向返回true
    static bool isSameDirection(double dir1, double dir2, double tol);

    /// @brief 求数学表达式的值
    /// @param [in] expr 表达式字符串
    /// @param [in] def 出错时的默认返回值
    /// @return 表达式值
    static double eval(const QString& expr, double def = 0.0);

    /// @brief 求数学表达式的值
    /// @param [in] expr 表达式字符串
    /// @param [out] ok 是否计算成功
    /// @return 表达式值
    static double eval(const QString& expr, bool* ok);

    /// @brief 求解一元二次方程
    /// @param [in] ce 系数向量，大小为2
    /// @return 实根向量
    static std::vector<double> quadraticSolver(const std::vector<double>& ce);

    /// @brief 求解一元三次方程
    /// @param [in] ce 系数向量，大小为3
    /// @return 实根向量
    static std::vector<double> cubicSolver(const std::vector<double>& ce);

    /// @brief 求解一元四次方程
    /// x^4 + ce[0] x^3 + ce[1] x^2 + ce[2] x + ce[3] = 0
    /// @param [in] ce 系数向量，大小为4，按顺序排列
    /// @return 实根向量
    static std::vector<double> quarticSolver(const std::vector<double>& ce);

    /// @brief 求解线性方程组
    /// @param [in] m 增广矩阵
    /// @param [out] sn 解向量
    /// @return 有唯一解返回true
    static bool linearSolver(const std::vector<std::vector<double>>& m, std::vector<double>& sn);

    /// @brief 求解以下类型的二元二次方程组
    /// ma000 x^2 + ma011 y^2 - 1 = 0
    /// ma100 x^2 + 2 ma101 xy + ma111 y^2 + mb10 x + mb11 y + mc1 = 0
    /// @param [in] m 系数向量，大小为8，严格顺序：ma000 ma011 ma100 ma101 ma111 mb10 mb11 mc1
    /// @return 实数解(x,y)
    static DmVectorSolutions simultaneousQuadraticSolver(const std::vector<double>& m);

    /// @brief 求解以下类型的二元二次方程组（完整形式）
    /// ma000 x^2 + ma001 xy + ma011 y^2 + mb00 x + mb01 y + mc0 = 0
    /// ma100 x^2 + ma101 xy + ma111 y^2 + mb10 x + mb11 y + mc1 = 0
    /// @param [in] m 系数向量，大小为2，每个包含6个系数，严格顺序：
    ///           ma000 ma001 ma011 mb00 mb01 mc0
    ///           ma100 ma101 ma111 mb10 mb11 mc1
    /// @return 实数解(x,y)
    static DmVectorSolutions simultaneousQuadraticSolverFull(const std::vector<std::vector<double>>& m);

    /// @brief 求解混合形式（含线性）的二元二次方程组
    /// @param [in] m 系数矩阵
    /// @return 实数解
    static DmVectorSolutions simultaneousQuadraticSolverMixed(const std::vector<std::vector<double>>& m);

    /// @brief 验证二元二次方程组的解
    /// @param [in] m 系数矩阵
    /// @param [in] v 待验证解
    /// @return 有效解返回true
    static bool simultaneousQuadraticVerify(const std::vector<std::vector<double>>& m, DmVector& v);

    /// @brief 第二类椭圆积分的封装，Legendre形式
    /// @param [in] k 椭圆模数或离心率
    /// @param [in] phi 椭圆角度，必须在[0, M_PI]范围内
    /// @return 椭圆积分值
    static double ellipticIntegral_2(const double& k, const double& phi);

    /// @brief 将double转为尽可能短的字符串
    /// @param [in] value 数值
    /// @param [in] prec 精度
    /// @return 字符串表示
    static QString doubleToString(double value, double prec);

    /// @brief 将double转为字符串
    /// @param [in] value 数值
    /// @param [in] prec 小数位数
    /// @return 字符串表示
    static QString doubleToString(double value, int prec);
};

#endif
