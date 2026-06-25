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

/// @file DocumentInterface.h
/// @brief 文档接口类，提供插件操作CAD文档的抽象方法

#ifndef DOCUMENT_INTERFACE_H
#define DOCUMENT_INTERFACE_H

#include <QHash>
#include <QPointF>
#include <QVariant>
#include <vector>
#include <string>

#include <nlohmann/json.hpp>

class QString;

/// @brief 文档接口，定义插件对CAD文档实体和属性进行操作的标准抽象方法
class Document_Interface
{
public:
    Document_Interface() = default;
    virtual ~Document_Interface() = default;

    /// @brief 保存文件
    /// @param formatType 输出参数，返回保存时使用的格式类型
    /// @return 保存结果字符串
    virtual QString save(QString& formatType) = 0;

    /// @brief 打开文件
    /// @return 打开结果字符串
    virtual QString open() = 0;

    /// @brief 添加线型
    /// @param json 包含线型定义的JSON对象
    virtual void addLineType(const nlohmann::json& json) = 0;

    /// @brief 添加图层
    /// @param json 包含图层定义的JSON对象
    virtual void addLayer(const nlohmann::json& json) = 0;

    /// @brief 添加块表记录
    /// @param json 包含块表记录定义的JSON对象
    virtual void addBlockTableRecord(const nlohmann::json& json) = 0;

    /// @brief 添加文字样式
    /// @param json 包含文字样式定义的JSON对象
    virtual void addTextStyle(const nlohmann::json& json) = 0;

    /// @brief 添加标注样式
    /// @param json 包含标注样式定义的JSON对象
    virtual void addDimStyle(const nlohmann::json& json) = 0;

    /// @brief 添加点
    /// @param json 包含点定义的JSON对象
    virtual void addPoint(const nlohmann::json& json) = 0;

    /// @brief 添加线
    /// @param json 包含线定义的JSON对象
    virtual void addLine(const nlohmann::json& json) = 0;

    /// @brief 添加圆
    /// @param json 包含圆定义的JSON对象
    virtual void addCircle(const nlohmann::json& json) = 0;

    /// @brief 添加填充
    /// @param json 包含填充定义的JSON对象
    virtual void addHatch(const nlohmann::json& json) = 0;

    /// @brief 添加椭圆
    /// @param json 包含椭圆定义的JSON对象
    virtual void addEllipse(const nlohmann::json& json) = 0;

    /// @brief 添加圆弧
    /// @param json 包含圆弧定义的JSON对象
    virtual void addArc(const nlohmann::json& json) = 0;

    /// @brief 添加射线
    /// @param json 包含射线定义的JSON对象
    virtual void addRay(const nlohmann::json& json) = 0;

    /// @brief 添加构造线
    /// @param json 包含构造线定义的JSON对象
    virtual void addXline(const nlohmann::json& json) = 0;

    /// @brief 添加多段线
    /// @param json 包含多段线定义的JSON对象
    virtual void addPolyline(const nlohmann::json& json) = 0;

    /// @brief 添加样条曲线
    /// @param json 包含样条曲线定义的JSON对象
    virtual void addSpline(const nlohmann::json& json) = 0;

    /// @brief 添加实体
    /// @param json 包含实体定义的JSON对象
    virtual void addSolid(const nlohmann::json& json) = 0;

    /// @brief 添加单行文字
    /// @param json 包含单行文字定义的JSON对象
    virtual void addText(const nlohmann::json& json) = 0;

    /// @brief 添加多行文字
    /// @param json 包含多行文字定义的JSON对象
    virtual void addMText(const nlohmann::json& json) = 0;

    /// @brief 添加属性定义
    /// @param json 包含属性定义数据的JSON对象
    virtual void addAttributeDefinition(const nlohmann::json& json) = 0;

    /// @brief 添加线性标注
    /// @param json 包含线性标注定义的JSON对象
    virtual void addDimLinear(const nlohmann::json& json) = 0;

    /// @brief 添加对齐标注
    /// @param json 包含对齐标注定义的JSON对象
    virtual void addDimAligned(const nlohmann::json& json) = 0;

    /// @brief 添加角度标注
    /// @param json 包含角度标注定义的JSON对象
    virtual void addDimAngular(const nlohmann::json& json) = 0;

    /// @brief 添加半径标注
    /// @param json 包含半径标注定义的JSON对象
    virtual void addDimRadial(const nlohmann::json& json) = 0;

    /// @brief 添加直径标注
    /// @param json 包含直径标注定义的JSON对象
    virtual void addDimDiametric(const nlohmann::json& json) = 0;

    /// @brief 添加引线
    /// @param json 包含引线定义的JSON对象
    virtual void addLeader(const nlohmann::json& json) = 0;

    /// @brief 添加块参照
    /// @param json 包含块参照定义的JSON对象
    virtual void addBlockReference(const nlohmann::json& json) = 0;

    // TODO: 以下阵列功能待实现，当前暂时注释
    // /// @brief 添加矩形阵列
    // virtual void addArrayRect(const nlohmann::json& json) = 0;
    // /// @brief 添加环形阵列
    // virtual void addArrayPolar(const nlohmann::json& json) = 0;

    /// @brief 设置当前单位
    /// @param m 单位代码
    virtual void setMeasurement(int m) = 0;

    /// @brief 获取线型集合
    /// @return 线型JSON对象列表
    virtual QList<nlohmann::json> getLineTypes() = 0;

    /// @brief 获取图层集合
    /// @return 图层JSON对象列表
    virtual QList<nlohmann::json> getLayers() = 0;

    /// @brief 获取块表集合
    /// @return 块表记录JSON对象列表
    virtual QList<nlohmann::json> getBlockTableRecords() = 0;

    /// @brief 获得文字样式集合
    /// @return 文字样式JSON对象列表
    virtual QList<nlohmann::json> getTextstyles() = 0;

    /// @brief 获得标注样式集合
    /// @return 标注样式JSON对象列表
    virtual QList<nlohmann::json> getDimstyles() = 0;

    /// @brief 获取实体集合
    /// @return 实体JSON对象列表
    virtual QList<nlohmann::json> getAllEntities() = 0;

    /// @brief 设置当前图层
    /// @param layerName 图层名称
    virtual void setActiveLayer(const std::wstring& layerName) = 0;

    /// @brief 设置当前文字样式
    /// @param styleName 文字样式名称
    virtual void setActiveTextStyle(const std::wstring& styleName) = 0;

    /// @brief 设置当前标注样式
    /// @param styleName 标注样式名称
    virtual void setActiveDimStyle(const std::wstring& styleName) = 0;

    /// @brief 获得当前图层名称
    /// @return 当前图层名称
    virtual std::wstring getActiveLayer() = 0;

    /// @brief 获得当前文字样式名称
    /// @return 当前文字样式名称
    virtual std::wstring getActiveTextStyle() = 0;

    /// @brief 获得当前标注样式名称
    /// @return 当前标注样式名称
    virtual std::wstring getActiveDimStyle() = 0;

    /// @brief 获得当前单位
    /// @return 当前单位代码
    virtual int getMeasurement() = 0;

    /// @brief 放缩画布铺满视口
    virtual void zoomAutoView() = 0;

    /// @brief 更新主窗体UI
    virtual void updateAppWindow() = 0;

    /// @brief 设置导入时文件的格式
    /// @param formatType 格式类型字符串
    virtual void setFormatType(const std::string& formatType) = 0;

    /// @brief 获取导入时文件的格式
    /// @return 当前格式类型字符串
    virtual std::string getFormatType() = 0;

    /// @brief 设置导入时文件路径
    /// @param filename 文件路径
    virtual void setFileName(const std::wstring& filename) = 0;

    /// @brief 获取导入时文件路径
    /// @return 当前文件路径
    virtual std::wstring getFileName() = 0;

};

#endif
