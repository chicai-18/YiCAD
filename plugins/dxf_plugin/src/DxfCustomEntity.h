/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

/// @file DxfCustomEntity.h
/// @brief 自定义实体在 DXF 里的写法（RENDER_PLAN.md 第 8.4 步，照 AutoCAD）
///
/// 一个自定义实体写成：自己的类型名（CLASSES 段登记 DXF 名、类名、应用名、代理权限）、公共属性、
/// 代理图形（160 + 310），之后是子类段：
/// - YiCAD 的类（插件的、进程内扩展的、它们的代理）：`100 YiCadCustomEntity`、`90` 数据版本、`91` 字节数、
///   `310` 数据字节，代理累计了变换时另有 `40`..`45`（a b c d tx ty）。本机 AutoCAD 2026 读成代理、另存 DXF 时
///   这些组码原样写回（2026-10-05 核对）；
/// - 别的程序的类（读进来时类不是 YiCAD 的）：子类段与扩展数据原样保留在代理的数据里（ForeignEntityData），
///   另存时原样写回，同 AutoCAD 保留代理数据的做法。
///
/// AutoCAD 还有一种写法 `ACAD_PROXY_ENTITY`：子类段 `AcDbProxyEntity` 的组码 91 是类在 CLASSES 段里的序号加 500，
/// 组码 70 是原数据的格式（本机 AutoCAD 2026 核对，2026-10-05）：
/// - 70 为 1，原数据是 DXF 组码，跟在代理图形之后。2013 版 DXF（YiCAD 写的就是）里读成代理的实体另存成 2018 版 DXF
///   时这样写，另存成同版本时保留原记录名。读的时候按 91 找类，之后照普通记录处理，写回时用类的记录名（同 AutoCAD
///   同版本另存）；
/// - 70 为 0，原数据是 DWG 的二进制（别的程序的对象经 DWG 转来）。读成代理，子类段原样保留在代理的数据里
///   （代理图形换成占位）；`AcDbProxyEntity` 的写法随 DXF 版本变（2018 版 71、97、160、162、161、94，
///   2013 版 95、160、162、161、94 加扩展数据），AutoCAD 不认版本不对的写法、整张图纸打不开，没有真实样本核对，
///   另存 DXF 时写成它的图形（DxfExporter）。

#ifndef YICAD_DXF_CUSTOM_ENTITY_H
#define YICAD_DXF_CUSTOM_ENTITY_H

#include "YiCadPluginSdk.h"

#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

class DRW_UnknownEntity;

namespace dxf
{

/// @brief YiCAD 的类写在子类段里的标记
constexpr const char* YiCadSubclassMarker = "YiCadCustomEntity";
/// @brief AutoCAD 的代理实体记录名
constexpr const char* AcadProxyRecordName = "ACAD_PROXY_ENTITY";
/// @brief ACAD_PROXY_ENTITY 的组码 91：CLASSES 段第一个类的编号，之后依次加一
constexpr int AcadProxyClassIdBase = 500;

using GroupCodes = std::vector<std::pair<int, std::string>>;

/// @brief 类名对应的 DXF 记录名：大写，字母数字以外的换成下划线（"com.yicad.demo.Pipe" → "COM_YICAD_DEMO_PIPE"）
std::string recordNameFor(const std::string& className);
/// @brief 类名对应的应用名：最后一个点之前的部分（插件或扩展的 ID）
std::string appNameFor(const std::string& className);

/// @brief 别的程序的自定义实体在 YiCAD 代理里保管的数据（编码由 DXF 插件定义）
struct ForeignEntityData
{
    std::string recordName;         ///< 类的 DXF 记录名（CLASSES 组码 1）
    std::string appName;            ///< 应用名（CLASSES 组码 3）
    /// @brief 原数据是 DWG 二进制的 ACAD_PROXY_ENTITY：records 从 `100 AcDbProxyEntity` 起，
    ///        其中的代理图形换成了占位（组码 160、值为空）
    bool acadProxy = false;
    GroupCodes records;             ///< 子类段与扩展数据的原始组码
    std::vector<uint8_t> graphics;  ///< 原始代理图形：没变换过时原样写回
};

/// @brief 编码别的程序的实体数据（带标记头，与 YiCAD 类的数据区分）
std::vector<uint8_t> encodeForeign(const ForeignEntityData& value);
/// @brief 解码；不是 encodeForeign 写的返回 false
bool decodeForeign(std::span<const uint8_t> bytes, ForeignEntityData& value);

/// @brief 从 DXF 读到的一个自定义实体
struct ParsedCustomEntity
{
    // 公共属性（AcDbEntity）
    std::string layer = "0";
    std::string lineType = "BYLAYER";
    int color = 256;
    int color24 = -1;
    int lineWeight = -1;            ///< DXF 组码 370 的值
    double lineTypeScale = 1.0;
    bool visible = true;
    bool paperSpace = false;
    std::vector<uint8_t> graphics;  ///< 代理图形
    bool planar = true;             ///< 拉伸方向（组码 210..230）是 Z 轴
    // YiCAD 的类
    bool yicad = false;
    uint32_t version = 0;
    std::vector<uint8_t> data;
    YiCadMatrix2d transform{1.0, 0.0, 0.0, 1.0, 0.0, 0.0};
    // 别的程序的类：子类段起的全部组码
    GroupCodes subclassRecords;
    // ACAD_PROXY_ENTITY 记录
    bool acadProxy = false;
    int proxyClassId = -1;          ///< 组码 91
    bool proxyDxfData = false;      ///< 组码 70 为 1：原数据是跟在后面的 DXF 组码（subclassRecords 或 YiCAD 的数据）
    GroupCodes proxyRecords;        ///< `100 AcDbProxyEntity` 起的原始组码，代理图形换成了占位
};

/// @brief 解析 libdxfrw 交来的不认识类型的实体
bool parseCustomEntity(const DRW_UnknownEntity& entity, ParsedCustomEntity& out);

/// @brief 代理图形的组码：160 字节数、310 十六进制块（每块至多 127 字节）
GroupCodes graphicsCodes(std::span<const uint8_t> graphics);
/// @brief YiCAD 类的子类段
GroupCodes yicadDataCodes(uint32_t version, std::span<const uint8_t> data, const YiCadMatrix2d& transform);

} // namespace dxf

#endif
