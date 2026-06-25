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


/// @file DmCharTemplate.h
/// @brief 文字模板类，代表一个文字模板

#ifndef DMCHARTEMPLATE_H
#define DMCHARTEMPLATE_H

#include <vector>

#include "DmEntity.h"

class DmChar;
class DmLine;
class DmFont;
class DmCharTemplateList;
class DmSpline;
class DmTriangle;

/// @brief 代表一个文字的模板
class DmCharTemplate : public DmEntity
{
public:
    /// @brief 构造文字模板
    /// @param parent 一般为空
    /// @param name 文字模板代表的文字
    /// @param owner 文字模板所在列表，默认为空
    DmCharTemplate(DmEntity* parent, const QString& name, DmCharTemplateList* owner = nullptr);

    /// @brief 克隆文字模板
    /// @return 不支持克隆，返回nullptr
    DmEntity* clone() const override;

    /// @brief 析构函数
    ~DmCharTemplate();

    /// @brief 获得实体类型
    /// @return DM::EntityCharTemplate
    virtual DM::EntityType getEntityType() const;

    /// @brief 根据宽度系数、倾斜角度生成文字实例
    /// @param widthFactor 宽度系数
    /// @param slashAngle 倾斜角度
    /// @return 生成的文字实例，父节点为空
    DmChar* generateChar(const double& widthFactor, const double& slashAngle);

    /// @brief 获得宽高比
    /// @return 文字宽高比
    double getWHFactor() const;

    /// @brief 设置宽高比
    /// @param factor 宽高比
    void setWHFactor(double factor);

    /// @brief 获得文字的真实高度
    /// @return 文字高度
    double getHeight() const;

    /// @brief 设置文字高度
    /// @param height 文字高度
    void setHeight(const double height);

    /// @brief 获得基线以上高度比例
    /// @return 基线以上高度与m_dHeight的比值
    double getAscenderFactor() const;

    /// @brief 设置基线以上高度比例
    /// @param ascenderFactor 基线以上高度因子
    void setAscenderFactor(const double& ascenderFactor);

    /// @brief 获得文字模板所在的列表
    /// @return 模板列表指针
    DmCharTemplateList* getOwner() const;

    /// @brief 设置文字模板所在的列表
    /// @param owner 模板列表指针
    void setOwner(DmCharTemplateList* owner);

    /// @brief 获得文字模板名称
    /// @return 文字模板名称
    QString getName() const;

    /// @brief 添加实体
    /// @param e 要添加的实体
    void addEntity(DmEntity* e);

    /// @brief 判断是否为空
    /// @return 如果无实体返回true
    bool isEmpty() const;

public:
    /// @brief 是否为容器
    /// @return 返回false
    bool isContainer() const override;

    /// @brief 计算包围框
    void calculateBorders() override;

    /// @brief 获得最近端点
    /// @param coord 坐标
    /// @param dist 距离输出参数
    /// @return 返回无效向量
    DmVector getNearestEndpoint(const DmVector& coord, double* dist = nullptr) const override;

    /// @brief 获得实体上最近点
    /// @return 返回无效向量
    DmVector getNearestPointOnEntity(const DmVector& /*coord*/, bool onEntity = true, double* dist = nullptr, DmEntity** entity = nullptr) const override;

    /// @brief 获得最近中心点
    /// @return 返回无效向量
    DmVector getNearestCenter(const DmVector& coord, double* dist = nullptr) const override;

    /// @brief 获得最近中点
    /// @return 返回无效向量
    DmVector getNearestMiddle(const DmVector& coord, double* dist = nullptr, int middlePoints = 1) const override;

    /// @brief 旋转
    void rotate(const DmVector& center, const DmVector& angleVector) override;

    /// @brief 镜像
    void mirror(const DmVector& axisPoint1, const DmVector& axisPoint2) override;

    /// @brief 缩放
    void scale(const DmVector& center, const DmVector& factor) override;

    /// @brief 移动
    void move(const DmVector& offset) override;

    /// @brief 获得子实体列表
    /// @return 子实体列表
    std::list<DmEntity*> getSubEntities() const override;

public:
    /// @brief 为圆弧生成切变实体
    /// @param arc 原始圆弧
    /// @param c 目标文字
    /// @param shearOrigin 切变原点
    /// @param shearDirection 切变方向
    /// @param angle 倾斜角
    /// @param widthScale 宽度缩放
    /// @return 生成的折线
    static DmPolyline* getShearedEntityForArc(const DmArc& arc, DmChar* c, const DmVector& shearOrigin, DM::ShearDirection shearDirection, double angle, double widthScale);

    /// @brief 为圆生成切变实体
    static DmPolyline* getShearedEntityForCircle(const DmCircle& circle, DmChar* c, const DmVector& shearOrigin, DM::ShearDirection shearDirection, double angle, double widthScale);

    /// @brief 为线段生成切变实体
    static DmLine* getShearedEntityForLine(const DmLine& line, const DmVector& shearOrigin, DM::ShearDirection shearDirection, double angle, double widthScale);

    /// @brief 为多段线生成切变实体
    static std::vector<DmEntity*> getShearedEntityForPolyline(const DmPolyline& poly, DmChar* c, const DmVector& shearOrigin, DM::ShearDirection shearDirection, double angle, double widthScale);

    /// @brief 为三角形生成切变实体
    static DmTriangle* getShearedEntityForTriangle(const DmTriangle& triangle, DmChar* c, const DmVector& shearOrigin, DM::ShearDirection shearDirection, double angle, double widthScale);

private:
    double m_dWidthHeightFactor;      ///< 文字宽高比。文字宽度与真实高度的比值，可能等于0
    double m_dHeight;                 ///< 文字的真实高度（去掉留白）。以字母"A"高度等于1作为基准
    double m_dAscenderFactor;         ///< 基线以上（非留白）高度与m_dHeight的比值
    DmCharTemplateList* m_charTemplateList; ///< 文字模板所在的列表
    QString m_name;                   ///< 文字字符

    std::list<DmEntity*> entities;
};

#endif
