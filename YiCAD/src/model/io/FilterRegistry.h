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

/// @file FilterRegistry.h
/// @brief 文件格式注册表：按文件找导入过滤器、按格式名找导出过滤器、列出文件对话框的过滤串
///
/// 原生格式（FilterOcdIO）在 DmSystem::init 时登记；插件格式由 shell/plugin_runtime 在插件
/// 加载后登记、shutdown 前注销（doc/LAYER_RESTRUCTURE_PLAN.md 8.3 节）。查找按登记顺序，
/// 先登记的优先，因此同一后缀原生格式优先于插件。
///
/// 是否接得住某个文件或格式由过滤器自己判断（FilterInterface::canImport、canExport）：
/// 查找时按登记顺序逐个创建过滤器去问，与原先 FileIO 的做法相同。

#ifndef FILTER_REGISTRY_H
#define FILTER_REGISTRY_H

#include <functional>
#include <memory>
#include <vector>

#include <QString>
#include <QStringList>

class FilterInterface;

/// @brief 文件格式注册表，见文件说明
/// @note 只在界面主线程使用，不做线程同步
class FilterRegistry
{
public:
    /// @brief 创建过滤器；每次查找都新建一个，调用方独占它
    using Factory = std::function<std::unique_ptr<FilterInterface>()>;

    /// @brief 进程唯一的注册表
    static FilterRegistry& instance();

    FilterRegistry(const FilterRegistry&) = delete;
    FilterRegistry& operator=(const FilterRegistry&) = delete;

    /// @brief 登记一种导入格式
    /// @param nameFilter 打开文件对话框里的过滤串，如 "Drawing Exchange YCD (*.ycd)"
    /// @param factory 创建过滤器
    /// @return 登记号，注销时用；从 1 开始
    int addImport(const QString& nameFilter, Factory factory);

    /// @brief 登记一种导出格式
    /// @param formatType 格式名，DmDocument 的保存格式与 FilterInterface::fileExport 的参数
    /// @param nameFilter 保存文件对话框里的过滤串；原生格式与格式名相同
    /// @param factory 创建过滤器
    /// @return 登记号，注销时用；从 1 开始
    int addExport(const QString& formatType, const QString& nameFilter, Factory factory);

    /// @brief 注销；登记号不存在时什么也不做
    /// @param id addImport 或 addExport 的返回值
    void remove(int id);

    /// @brief 按登记顺序找第一个接得住该文件的导入过滤器
    /// @param file 文件路径
    /// @return 过滤器，没有时返回空
    std::unique_ptr<FilterInterface> importFilter(const QString& file) const;

    /// @brief 按登记顺序找第一个接得住该格式的导出过滤器
    /// @param formatType 格式名
    /// @return 过滤器，没有时返回空
    std::unique_ptr<FilterInterface> exportFilter(const QString& formatType) const;

    /// @brief 打开文件对话框的过滤串，按登记顺序
    QStringList importNameFilters() const;

    /// @brief 保存文件对话框的过滤串，按登记顺序
    QStringList exportNameFilters() const;

    /// @brief 保存文件对话框选中的过滤串对应的格式名
    /// @param nameFilter 对话框选中的过滤串
    /// @return 格式名；没有登记过这个过滤串时原样返回
    QString exportFormatType(const QString& nameFilter) const;

private:
    FilterRegistry() = default;

    /// @brief 一条登记
    struct Entry
    {
        int id = 0;             ///< 登记号
        bool isImport = false;  ///< 导入还是导出
        QString formatType;     ///< 导出的格式名；导入为空
        QString nameFilter;     ///< 文件对话框的过滤串
        Factory factory;        ///< 创建过滤器
    };

    std::vector<Entry> m_entries;  ///< 按登记顺序
    int m_nextId = 1;              ///< 下一个登记号
};

#endif
