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

/// @file DocPluginInterface.h
/// @brief 插件文档接口类，为插件提供文档级实体操作与查询能力

#ifndef DOC_PLUGIN_INTERFACE_H
#define DOC_PLUGIN_INTERFACE_H

#include <QObject>

#include "DocumentInterface.h"
#include "FilterJsonIO.h"
#include <nlohmann/json.hpp>

class Doc_plugin_interface;

/// @brief 插件文档接口，提供实体增删、图层/样式管理、文件I/O等功能
class Doc_plugin_interface : public Document_Interface
{
public:
    Doc_plugin_interface(DmDocument* d, GuiDocumentView* gv, QWidget* parent);

    /// @brief 保存文件
    /// @param [in,out] formatType 导出格式类型
    /// @return 保存的文件路径
    virtual QString save(QString& formatType);

    /// @brief 打开文件
    /// @return 打开的文件路径
    virtual QString open();

    virtual void addLineType(const nlohmann::json& json);
    virtual void addLayer(const nlohmann::json& json);
    virtual void addBlockTableRecord(const nlohmann::json& json);
    virtual void addTextStyle(const nlohmann::json& json);
    virtual void addDimStyle(const nlohmann::json& json);

    virtual void addPoint(const nlohmann::json& json);
    virtual void addLine(const nlohmann::json& json);
    virtual void addCircle(const nlohmann::json& json);
    virtual void addHatch(const nlohmann::json& json);
    virtual void addEllipse(const nlohmann::json& json);
    virtual void addArc(const nlohmann::json& json);
    virtual void addRay(const nlohmann::json& json);
    virtual void addXline(const nlohmann::json& json);
    virtual void addPolyline(const nlohmann::json& json);
    virtual void addSpline(const nlohmann::json& json);
    virtual void addSolid(const nlohmann::json& json);
    virtual void addText(const nlohmann::json& json);
    virtual void addMText(const nlohmann::json& json);
    virtual void addAttributeDefinition(const nlohmann::json& json);
    virtual void addDimLinear(const nlohmann::json& json);
    virtual void addDimAligned(const nlohmann::json& json);
    virtual void addDimAngular(const nlohmann::json& json);
    virtual void addDimRadial(const nlohmann::json& json);
    virtual void addDimDiametric(const nlohmann::json& json);
    virtual void addLeader(const nlohmann::json& json);
    virtual void addBlockReference(const nlohmann::json& json);

    /// @brief 设置单位制
    /// @param [in] m 单位制值（0=英制，1=公制）
    virtual void setMeasurement(int m);

    /// @brief 获取线型集合
    /// @return 线型的JSON列表
    virtual QList<nlohmann::json> getLineTypes();

    /// @brief 获取图层集合
    /// @return 图层的JSON列表
    virtual QList<nlohmann::json> getLayers();

    /// @brief 获取块表集合
    /// @return 块表记录的JSON列表
    virtual QList<nlohmann::json> getBlockTableRecords();

    /// @brief 获得文字样式
    /// @return 文字样式的JSON列表
    virtual QList<nlohmann::json> getTextstyles();

    /// @brief 获得标注样式
    /// @return 标注样式的JSON列表
    virtual QList<nlohmann::json> getDimstyles();

    /// @brief 获取实体集合
    /// @return 所有实体的JSON列表
    virtual QList<nlohmann::json> getAllEntities();

    /// @brief 设置当前图层
    /// @param [in] layerName 图层名称
    virtual void setActiveLayer(const std::wstring& layerName);

    /// @brief 设置当前文字样式
    /// @param [in] styleName 样式名称
    virtual void setActiveTextStyle(const std::wstring& styleName);

    /// @brief 设置当前标注样式
    /// @param [in] styleName 样式名称
    virtual void setActiveDimStyle(const std::wstring& styleName);

    /// @brief 获得当前图层
    /// @return 当前图层名称
    virtual std::wstring getActiveLayer();

    /// @brief 获得当前文字样式
    /// @return 当前文字样式名称
    virtual std::wstring getActiveTextStyle();

    /// @brief 获得当前标注样式
    /// @return 当前标注样式名称
    virtual std::wstring getActiveDimStyle();

    /// @brief 获得当前单位
    /// @return 单位制值（0=英制，1=公制）
    virtual int getMeasurement();

    /// @brief 放缩画布铺满视口
    virtual void zoomAutoView();

    /// @brief 更新主窗体UI
    virtual void updateAppWindow();

    /// @brief 设置导入时文件的格式
    /// @param [in] formatType 格式类型字符串
    virtual void setFormatType(const std::string& formatType);

    /// @brief 获取导入时文件的格式
    /// @return 格式类型字符串
    virtual std::string getFormatType();

    /// @brief 设置导入时文件路径
    /// @param [in] filename 文件路径
    virtual void setFileName(const std::wstring& filename);

    /// @brief 获取导入时文件路径
    /// @return 文件路径
    virtual std::wstring getFileName();

private:
    /// @brief 将实体添加到文档
    /// @param [in] entity 待添加的实体
    void addEntity(DmEntity* entity);

private:
    DmDocument*         m_pDocument = nullptr;      ///< 关联的文档对象
    GuiDocumentView*    m_pView = nullptr;          ///< 关联的文档视图
    FilterJsonIO*       m_jsonConverter = nullptr;  ///< JSON格式转换器
};

#endif  // DOC_PLUGIN_INTERFACE_H
