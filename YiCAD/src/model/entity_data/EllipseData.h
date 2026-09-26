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

/// @file EllipseData.h
/// @brief 椭圆/椭圆弧数据结构类，定义长轴、短轴比、起始终止参数等几何属性

#ifndef ELLIPSEDATA_H
#define ELLIPSEDATA_H

#include "EntityData.h"
#include "DmVector.h"

/// @brief 椭圆/椭圆弧数据结构
class EllipseData : public EntityData
{
public:
    /// @brief 默认构造函数
    EllipseData();

    /// @brief 带参数的构造函数
    /// @param center 椭圆中心
    /// @param major 长轴端点向量（相对于中心，长度为椭圆方程中的a）
    /// @param normal 法向量
    /// @param ratio 短轴与长轴的比率
    /// @param isClosed 是否闭合（true则为椭圆，false则为椭圆弧）
    /// @param startParam 起始参数（相对长轴方向）
    /// @param endParam 终止参数（相对长轴方向）
    /// @note 关于起始终止参数的定义，可参考：http://zz.xdf.cn/gkbk/202101/128610513.html
    EllipseData(const DmVector& center, const DmVector& major, const DmVector& normal, const double& ratio, const bool& isClosed, const double& startParam, const double& endParam);

public:
    /// @brief 获取起始参数
    /// @return 起始参数
    double getStartParam() const;

    /// @brief 设置起始参数
    /// @param dStartParam 起始参数
    void setStartParam(const double& dStartParam);

    /// @brief 获取终止参数
    /// @return 终止参数
    double getEndParam() const;

    /// @brief 设置终止参数
    /// @param dEndParam 终止参数
    void setEndParam(const double& dEndParam);

    /// @brief 获取椭圆中心
    /// @return 椭圆中心坐标
    DmVector getCenter() const;

    /// @brief 设置椭圆中心
    /// @param pt 椭圆中心坐标
    void setCenter(const DmVector& pt);

    /// @brief 获取长轴端点向量
    /// @return 长轴端点向量（相对于中心）
    DmVector getMajorP() const;

    /// @brief 设置长轴端点向量
    /// @param vec 长轴端点向量
    void setMajorP(const DmVector& vec);

    /// @brief 获取短轴与长轴的比率
    /// @return 短轴与长轴比率
    double getRatio() const;

    /// @brief 设置短轴与长轴的比率
    /// @param dRatio 短轴与长轴比率
    void setRatio(const double& dRatio);

    /// @brief 获取是否闭合
    /// @return 若闭合则返回true
    bool getIsClosed() const;

    /// @brief 设置是否闭合
    /// @param isClosed 是否闭合
    void setIsClosed(const bool& isClosed);

    /// @brief 获取法向量
    /// @return 法向量
    DmVector getNormal() const;

    /// @brief 设置法向量
    /// @param normal 法向量
    void setNormal(const DmVector& normal);

    /// @brief 获取渲染顶点数据引用
    /// @return 顶点数据常量引用
    const std::vector<float>& getVerticesRef() const;

    /// @brief 设置渲染顶点数据
    /// @param vs 顶点数据向量
    void setVertices(const std::vector<float>& vs);

private:
    double              m_dStartParam;  ///< 椭圆弧起始角度（相对长轴方向，以m_vecMajorP为半径画圆所得）
    double              m_dEndParam;    ///< 椭圆弧终止角度（相对长轴方向）
    DmVector            m_ptCenter;     ///< 圆心
    DmVector            m_vecMajorP;    ///< 长轴相对于中心的端点（向量），长度对应椭圆方程中的a
    double              m_dRatio;       ///< 短轴与长轴的比率
    DmVector            m_normal;       ///< 法向量
    bool                m_bIsClosed;    ///< 是否闭合（闭合则为椭圆，不闭合为椭圆弧）

    std::vector<float>  m_vertices;     ///< 用于渲染的特定结构数据（x,y,z, parameter, total_length）
};

#endif // ELLIPSEDATA_H
