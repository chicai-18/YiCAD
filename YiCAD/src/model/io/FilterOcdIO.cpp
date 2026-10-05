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

/// @file FilterOcdIO.cpp
/// @brief OCD文件读写类实现

#include "FilterOcdIO.h"

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <zlib.h>
#include <QStringList>
#include <regex>

//persistent
#include "backuppolicy.h"
#include "FileInfo.h"
#include "MigratorBase.h"
#include "Reader.h"
#include "Stream.h"
#include "Tools.h"
#include "Uuid.h"
#include "Writer.h"
#include "MinizipNgArchive.h"

#include "MetaLineTypes.h"
#include "MetaLayers.h"
#include "MetaTextStyles.h"
#include "MetaDimensionStyles.h"
#include "MetaLines.h"
#include "MetaCircles.h"
#include "MetaArcs.h"
#include "MetaPoints.h"
#include "MetaEllipses.h"
#include "MetaRays.h"
#include "MetaXlines.h"
#include "MetaSolids.h"
#include "MetaTriangles.h"
#include "MetaPolylines.h"
#include "MetaSplines.h"
#include "MetaBlockReferences.h"
#include "MetaBlockTableRecords.h"
#include "MetaTexts.h"
#include "MetaMTexts.h"
#include "MetaAttributeDefinitions.h"
#include "MetaAttributes.h"
#include "MetaDimLinears.h"
#include "MetaDimAligneds.h"
#include "MetaDimAngulars.h"
#include "MetaDimRadials.h"
#include "MetaDimDiametrics.h"
#include "MetaDimLeaders.h"
#include "MetaHatchs.h"
#include "MetaCustomEntities.h"

#include "DmBlock.h"
#include "DmBlockTable.h"
#include "DmDimensionStyleTable.h"
#include "DmDocument.h"
#include "DmLayerTable.h"
#include "DmTextStyleTable.h"
#include "EntityTable.h"

#define IMPORTTYPE "ycd"
#define EXPORTTYPE "Drawing Exchange YCD 2023 (*.ycd)"

namespace
{
/// @brief 读入前清空文档：实体、块、标注样式、文字样式与图层都以文件为准
///
/// 新建的文档里这些表只有自带的默认条目（"0" 图层、"Standard"、"ISO-25"、标注箭头块），文件里也有，
/// 不清空就各有两份，按名字查找取到默认那份（分层重组方案 4.5 节 R4，做法见 12 节 D9）。读失败后
/// DocumentFileService 会把备份读进同一份文档，这时文档里还有读了一半的内容，一并清掉。
/// 线型表不动：固定线型读入时已存在则跳过（MetaLineTypes）。按引用关系从上往下删，与 ~DmDocument 同序。
void clearForImport(DmDocument& document)
{
    document.getEntityTable()->clear_direct();
    document.getDimStyleTable()->clear_direct();
    document.getBlockTable()->clear_direct();
    document.getTextStyleTable()->clear_direct();
    document.getLayerTable()->clear_direct();
}

/// @brief 读完（或读失败）后补上文件里缺少的默认条目，并保证各表有当前项
///
/// 标注样式表补 "ISO-25" 时要取 "Standard" 文字样式，所以放在文字样式表之后
void addMissingDefaults(DmDocument& document)
{
    document.getLayerTable()->addMissingDefaults();
    document.getTextStyleTable()->addMissingDefaults();
    document.getDimStyleTable()->addMissingDefaults();
}
}  // namespace

FilterOcdIO::FilterOcdIO()
    : m_pDocument(nullptr)
    , m_spPersistLineTypes(nullptr)
    , m_spPersistLayers(nullptr)
    , m_spPersistViewports(nullptr)
    , m_spPersistTextStyles(nullptr)
    , m_spPersistDimensionStyles(nullptr)
    , m_spPersistTableStyles(nullptr)
    , m_spPersistLines(nullptr)
    , m_spPersistCircles(nullptr)
    , m_spPersistArcs(nullptr)
    , m_spPersistPoints(nullptr)
    , m_spPersistEllipses(nullptr)
    , m_spPersistRays(nullptr)
    , m_spPersistXlines(nullptr)
    , m_spPersistSolids(nullptr)
    , m_spPersistTriangles(nullptr)
    , m_spPersistPolylines(nullptr)
    , m_spPersistSplines(nullptr)
    , m_spPersistBlockTableRecords(nullptr)
    , m_spPersistBlcokReferences(nullptr)
    , m_spPersistTexts(nullptr)
    , m_spPersistMTexts(nullptr)
    , m_spPersistAttributeDefinitions(nullptr)
    , m_spPersistAttributes(nullptr)
    , m_spPersistDimLinears(nullptr)
    , m_spPersistDimAligneds(nullptr)
    , m_spPersistDimAngulars(nullptr)
    , m_spPersistDimRadials(nullptr)
    , m_spPersistDimDiametrics(nullptr)
    , m_spPersistDimLeaders(nullptr)
    , m_spPersistHatchs(nullptr)
    , m_spPersistCustomEntities(nullptr)
{
}

FilterOcdIO::~FilterOcdIO()
{
}

bool FilterOcdIO::canImport(const QString& filename) const
{
    QFileInfo fileInfo = QFileInfo(filename);
    auto duffix = fileInfo.suffix();
    if (duffix == IMPORTTYPE)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool FilterOcdIO::canExport(const QString& type) const
{
    if (type == EXPORTTYPE)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool FilterOcdIO::fileImport(DmDocument& g, const QString& filename)
{
    auto strfileName = filename.toStdString();

    // 初始化
    m_pDocument = &g;
    FileInfo fi(strfileName.c_str());
    Oifstream file(fi, std::ios::in | std::ios::binary);
    std::streambuf* buf = file.rdbuf();
    std::streamoff size = buf->pubseekoff(0, std::ios::end, std::ios::in);
    buf->pubseekoff(0, std::ios::beg, std::ios::in);
    if (size < 22) // an empty zip archive has 22 bytes
        throw OneException("Invalid file");

    MinizipNgArchiveReader archive(file);
    // ArchiveReader 要求读第一个条目之前先调一次 nextEntry()；第一个条目是 Document.xml，
    // 其余条目随后由 XMLReader::readFiles 接着往下读
    if (!archive.nextEntry())
    {
        throw OneException("Error reading compression file");
    }
    XMLReader reader(strfileName.c_str(), archive.stream());

    if (!reader.isValid())
    {
        throw OneException("Error reading compression file");
    }

    clearForImport(*m_pDocument);
    try
    {
        //restore xml files
        initPersist(*m_pDocument);
        restoreXML(reader);

        //restore separate files
        reader.readFiles(archive);

        //Post Restore
        auto bMigration = DmMigrateContext::GetInstance()->postRestore();
        if (!bMigration)
        {
            auto msgs = DmMigrateContext::GetInstance()->errMsgs();
            if (!msgs.empty())
            {
                std::string errMsgs = std::accumulate(msgs.begin(), msgs.end(), std::string(""));
                throw OneException(errMsgs.c_str());
            }
        }
    }
    catch (...)
    {
        // 读了一半的文档也要有当前图层与样式，调用方可能还要显示它或再读备份
        addMissingDefaults(*m_pDocument);
        throw;
    }
    addMissingDefaults(*m_pDocument);
    return true;
}

bool FilterOcdIO::fileExport(DmDocument& g, const QString& strfile, const QString& type)
{
    this->m_pDocument = &g;

    QString path = QFileInfo(strfile).absolutePath();
    if (QFileInfo(path).isWritable()==false)
    {
        return false;
    }

    bool bsuccess = true;
    int compression = 3;
    compression = clamp<int>(compression, Z_NO_COMPRESSION, Z_BEST_COMPRESSION);
    bool bpolicy = true;
    //todo save file

    // make a tmp. file where to save the project data first and then rename to
    // the actual file name. This may be useful if overwriting an existing file
    // fails so that the data of the work up to now isn't lost.
    std::string uuid = Uuid::createUuid();
    std::string fn = strfile.toStdString();
    if (bpolicy)
    {
        fn += ".";
        fn += uuid;
    }

    FileInfo tmp(fn);
    // In case some folders in the path do not exist
    QString utf8Name = strfile;
    auto parentPath = std::filesystem::absolute(std::filesystem::path(utf8Name.toStdString())).parent_path();
    std::filesystem::create_directories(parentPath);

    // open extra scope to close ZipWriter properly
    {
        Oofstream file(tmp, std::ios::out | std::ios::binary);
        ZipWriter writer(file);
        if (!file.is_open())
        {
            throw OneException("Failed to save file");
        }

        writer.setComment("YiCAD Document");
        writer.setLevel(compression);

         writer.putNextEntry("Document.xml");
         initPersist(*m_pDocument);
         saveXML(writer);

        // write additional files
        writer.writeFiles();
        writer.close();

        if (writer.hasErrors())
        {
            throw OneException("Failed to write all data to file");
        }
    }

    {
        BackupPolicy policy;
        policy.setPolicy(BackupPolicy::Standard);
        policy.setNumberOfFiles(1/*count_bak*/);
        policy.apply(fn, strfile.toStdString());

    }

    if (!bsuccess)
    {
        return false;
    }
    return bsuccess;
}

void FilterOcdIO::saveXML(Writer& writer)
{
    writer.Stream() << "<?xml version='1.0' encoding='utf-8'?>" << std::endl
        << "<!--" << std::endl
        << " YiCAD Document, see https://www.yicad.com for more information..." << std::endl
        << "-->" << std::endl;

    writer.Stream() << "<EntityContainer SchemaVersion=\"0\"" << ">" << std::endl;

    writer.incInd();

    // variables
    saveVariables(writer);
    // lineTypes
    saveLineTypes(writer);
    //layers
    saveLayers(writer);
    //textStyles
    saveTextStyles(writer);
    //dimStyles
    saveDimStyles(writer);
    //blockTableRecords
    saveBlockTableRecords(writer);

    //entities
    saveEntities(writer);

    writer.decInd();
    writer.Stream() << "</EntityContainer>" << std::endl;
}

void FilterOcdIO::restoreXML(XMLReader& reader)
{
    reader.readElement("EntityContainer");
    //read Container level 

    long scheme = reader.getAttributeAsInteger("SchemaVersion");
    reader.DocumentSchema = scheme;
    if (reader.hasAttribute("ProgramVersion"))
    {
        reader.ProgramVersion = reader.getAttribute("ProgramVersion");
    }
    else
    {
        reader.ProgramVersion = "pre-1.0";
    }
    if (reader.hasAttribute("FileVersion"))
    {
        reader.FileVersion = reader.getAttributeAsUnsigned("FileVersion");
    }
    else
    {
        reader.FileVersion = 0;
    }

    //variables
    restoreVariables(reader);
    //lineTypes
    restoreLineTypes(reader);
    //layers
    restoreLayers(reader);
    //textStyles
    restoreTextStyles(reader);
    //dimStyles
    restoreDimStyles(reader);
    //blockTableRecords
    restoreBlockTableRecords(reader);

    //entities
    restoreEntities(reader);

    reader.readEndElement("EntityContainer");
}

void FilterOcdIO::saveVariables(Writer& writer)
{
    // 按名字排序：变量字典是散列表，不排序时同一份文档每次写出的顺序都不同
    const QHash<QString, DmVariable>& variables = m_pDocument->getVariableDict();
    QStringList names = variables.keys();
    names.sort();
    writer.Stream() << writer.ind() << "<Variables Count=\"" << names.size() << "\">" << std::endl;
    writer.incInd();
    for (const QString& name : names)
    {
        const DmVariable& variable = variables[name];
        const char* type = nullptr;
        QString value;
        switch (variable.getType())
        {
        case DM::VariableInt:
            type = "int";
            value = QString::number(variable.getInt());
            break;
        case DM::VariableDouble:
            type = "double";
            value = QString::number(variable.getDouble(), 'g', 17);
            break;
        case DM::VariableString:
            type = "string";
            value = variable.getString();
            break;
        case DM::VariableVector:
            type = "vector";
            value = QStringLiteral("%1 %2 %3")
                        .arg(variable.getVector().x, 0, 'g', 17)
                        .arg(variable.getVector().y, 0, 'g', 17)
                        .arg(variable.getVector().z, 0, 'g', 17);
            break;
        case DM::VariableVoid:
            break;
        }
        if (!type)
        {
            continue;
        }
        writer.Stream() << writer.ind() << "<Variable name=\"" << Persistence::encodeAttribute(name.toStdString())
                        << "\" code=\"" << variable.getCode() << "\" type=\"" << type << "\" value=\""
                        << Persistence::encodeAttribute(value.toStdString()) << "\"/>" << std::endl;
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</Variables>" << std::endl;
}

void FilterOcdIO::saveLineTypes(Writer& writer)
{
    auto lineTypes = m_pDocument->getLineTypeTable();
    writer.Stream() << writer.ind() << "<LineTypes Count=\"" << lineTypes->count() << "\">" << std::endl;
    m_spPersistLineTypes->saveXML(writer);
    writer.Stream() << writer.ind() << "</LineTypes>" << std::endl;
}

void FilterOcdIO::saveLayers(Writer& writer)
{
    auto layers = m_pDocument->getLayerTable();
    writer.Stream() << writer.ind() << "<Layers Count=\"" << layers->count() << "\">" << std::endl;
    m_spPersistLayers->saveXML(writer);
    writer.Stream() << writer.ind() << "</Layers>" << std::endl;
}

void FilterOcdIO::saveTextStyles(Writer& writer)
{
    std::vector<DmTextStyle*> vec;
    auto table = m_pDocument->getTextStyleTable();
    for(auto it=table->begin();it!=table->end();++it){
        vec.emplace_back(*it);
    }
    m_spPersistTextStyles->setTextStyles(vec);
    m_spPersistTextStyles->saveXML(writer);
}

void FilterOcdIO::saveDimStyles(Writer& writer)
{
    std::vector<DmDimensionStyle*> dimStyles;
    for(auto s:*m_pDocument->getDimStyleTable())
    {
        dimStyles.emplace_back(s);
    }
    m_spPersistDimensionStyles->setDimStyles(dimStyles);
    m_spPersistDimensionStyles->saveXML(writer);
}

void FilterOcdIO::saveBlockTableRecords(Writer& writer)
{
    std::vector<DmBlock*> blocks;
    for (auto block : *m_pDocument->getBlockTable())
    {
        blocks.push_back(block);
    }
    m_spPersistBlockTableRecords->setBlockList(blocks);
    m_spPersistBlockTableRecords->saveXML(writer);
}

void FilterOcdIO::saveEntities(Writer& writer)
{
    std::list<DmEntity*> lines, arcs, points, circles, ellipses, solids, triangles, rays, xlines, polylines, splines, inserts, hatchs
        , texts, mtexts, attributeDefinitions, attributes, dimLinears, dimAligneds, dimAngulars, dimRadials, dimDiametrics, dimLeaders
        , customs;
    // 给实体分组保存
    auto entTable = m_pDocument->getEntityTable();
    for (auto& e : *entTable)
    {
        auto entType = e->getEntityType();
        switch (entType)
        {
        case DM::EntityAttribute:
            attributes.emplace_back(e);
            break;
        case DM::EntityAttributeDefinition:
            attributeDefinitions.emplace_back(e);
            break;
        case DM::EntityBlockReference:
            inserts.emplace_back(e);
            break;
        case DM::EntityPoint:
            points.emplace_back(e);
            break;
        case DM::EntityLine:
            lines.emplace_back(e);
            break;
        case DM::EntityPolyline:
            polylines.emplace_back(e);
            break;
        case DM::EntityArc:
            arcs.emplace_back(e);
            break;
        case DM::EntityCircle:
            circles.emplace_back(e);
            break;
        case DM::EntityEllipse:
            ellipses.emplace_back(e);
            break;
        case DM::EntitySolid:
            solids.emplace_back(e);
            break;
        case DM::EntityTriangle:
            triangles.emplace_back(e);
            break;
        case DM::EntityConstructionLine:
            break;
        case DM::EntityMText:
            mtexts.emplace_back(e);
            break;
        case DM::EntityText:
            texts.emplace_back(e);
            break;
        case DM::EntityDimAligned:
            dimAligneds.emplace_back(e);
            break;
        case DM::EntityDimLinear:
            dimLinears.emplace_back(e);
            break;
        case DM::EntityDimRadial:
            dimRadials.emplace_back(e);
            break;
        case DM::EntityDimDiametric:
            dimDiametrics.emplace_back(e);
            break;
        case DM::EntityDimAngular:
            dimAngulars.emplace_back(e);
            break;
        case DM::EntityDimLeader:
            dimLeaders.emplace_back(e);
            break;
        case DM::EntityHatch:
            hatchs.emplace_back(e);
            break;
        case DM::EntityImage:
            break;
        case DM::EntitySpline:
            splines.emplace_back(e);
            break;
        case DM::EntityRay:
            rays.emplace_back(e);
            break;
        case DM::EntityXline:
            xlines.emplace_back(e);
            break;
        case DM::EntityCustom:
            customs.emplace_back(e);
            break;
        default:
            break;
        }
    }

    // save
    {
        writer.Stream() << writer.ind() << "<Entities Count=\"" << m_pDocument->getEntityTable()->count() << "\">" << std::endl;

        //lines
        m_spPersistLines->setEntities(lines);
        m_spPersistLines->saveXML(writer);
        //circles
        m_spPersistCircles->setEntities(circles);
        m_spPersistCircles->saveXML(writer);
        //arcs
        m_spPersistArcs->setEntities(arcs);
        m_spPersistArcs->saveXML(writer);
        //points
        m_spPersistPoints->setEntities(points);
        m_spPersistPoints->saveXML(writer);
        //ellipses
        m_spPersistEllipses->setEntities(ellipses);
        m_spPersistEllipses->saveXML(writer);
        //solids
        m_spPersistSolids->setEntities(solids);
        m_spPersistSolids->saveXML(writer);
        //triangles
        m_spPersistTriangles->setEntities(triangles);
        m_spPersistTriangles->saveXML(writer);
        //rays
        m_spPersistRays->setEntities(rays);
        m_spPersistRays->saveXML(writer);
        //xlines
        m_spPersistXlines->setEntities(xlines);
        m_spPersistXlines->saveXML(writer);
        //polylines
        m_spPersistPolylines->setEntities(polylines);
        m_spPersistPolylines->saveXML(writer);
        //splines
        m_spPersistSplines->setEntities(splines);
        m_spPersistSplines->saveXML(writer);
        //blcokReferences
        m_spPersistBlcokReferences->setEntities(inserts);
        m_spPersistBlcokReferences->saveXML(writer);
        //texts
        m_spPersistTexts->setEntities(texts);
        m_spPersistTexts->saveXML(writer);
        //mText
        m_spPersistMTexts->setEntities(mtexts);
        m_spPersistMTexts->saveXML(writer);
        //attributeDefinition
        m_spPersistAttributeDefinitions->setEntities(attributeDefinitions);
        m_spPersistAttributeDefinitions->saveXML(writer);
        //attribute
        m_spPersistAttributes->setEntities(attributes);
        m_spPersistAttributes->saveXML(writer);
        //dimLinear
        m_spPersistDimLinears->setEntities(dimLinears);
        m_spPersistDimLinears->saveXML(writer);
        //dimAligned
        m_spPersistDimAligneds->setEntities(dimAligneds);
        m_spPersistDimAligneds->saveXML(writer);
        //dimAngular
        m_spPersistDimAngulars->setEntities(dimAngulars);
        m_spPersistDimAngulars->saveXML(writer);
        //dimRadial
        m_spPersistDimRadials->setEntities(dimRadials);
        m_spPersistDimRadials->saveXML(writer);
        //dimDiametric
        m_spPersistDimDiametrics->setEntities(dimDiametrics);
        m_spPersistDimDiametrics->saveXML(writer);
        //dimLeader
        m_spPersistDimLeaders->setEntities(dimLeaders);
        m_spPersistDimLeaders->saveXML(writer);
        //hatch
        m_spPersistHatchs->setEntities(hatchs);
        m_spPersistHatchs->saveXML(writer);
        //custom entities
        m_spPersistCustomEntities->setEntities(customs);
        m_spPersistCustomEntities->saveXML(writer);
        //image

        writer.Stream() << writer.ind() << "</Entities>" << std::endl;
    }
}


void FilterOcdIO::restoreVariables(XMLReader& reader)
{
    reader.readElement("Variables");
    const long count = reader.getAttributeAsInteger("Count");
    for (long i = 0; i < count; ++i)
    {
        reader.readElement("Variable");
        const QString name = QString::fromUtf8(reader.getAttribute("name"));
        const int code = static_cast<int>(reader.getAttributeAsInteger("code"));
        const std::string type = reader.getAttribute("type");
        const QString value = QString::fromUtf8(reader.getAttribute("value"));
        if (type == "int")
        {
            m_pDocument->addVariable(name, value.toInt(), code);
        }
        else if (type == "double")
        {
            m_pDocument->addVariable(name, value.toDouble(), code);
        }
        else if (type == "string")
        {
            m_pDocument->addVariable(name, value, code);
        }
        else if (type == "vector")
        {
            const QStringList xyz = value.split(QLatin1Char(' '), Qt::SkipEmptyParts);
            if (xyz.size() == 3)
            {
                m_pDocument->addVariable(name, DmVector(xyz[0].toDouble(), xyz[1].toDouble(), xyz[2].toDouble()), code);
            }
        }
    }
    reader.readEndElement("Variables");
}

void FilterOcdIO::restoreLineTypes(XMLReader& reader)
{
    reader.readElement("LineTypes");
    //read lineTypes
    m_spPersistLineTypes->restoreXML(reader);
    reader.readEndElement("LineTypes");
}

void FilterOcdIO::restoreLayers(XMLReader& reader)
{
    reader.readElement("Layers");
    //read layers
    m_spPersistLayers->restoreXML(reader);
    reader.readEndElement("Layers");
}

void FilterOcdIO::restoreTextStyles(XMLReader& reader)
{
    m_spPersistTextStyles->restoreXML(reader);
}

void FilterOcdIO::restoreDimStyles(XMLReader& reader)
{
    m_spPersistDimensionStyles->restoreXML(reader);
}

void FilterOcdIO::restoreBlockTableRecords(XMLReader& reader)
{
    m_spPersistBlockTableRecords->restoreXML(reader);
}

void FilterOcdIO::restoreEntities(XMLReader& reader)
{
    reader.readElement("Entities");
    auto icunt = 0;
    icunt = reader.getAttributeAsInteger("Count");

    // todo: 这里读取的顺序要跟saveXML()方法里存储的顺序一致 否则会程序报错!!!!!
    //read lines
    m_spPersistLines->restoreXML(reader);
    //read circles
    m_spPersistCircles->restoreXML(reader);
    //read arcs
    m_spPersistArcs->restoreXML(reader);
    //read points
    m_spPersistPoints->restoreXML(reader);
    //read ellipses
    m_spPersistEllipses->restoreXML(reader);
    //read solids
    m_spPersistSolids->restoreXML(reader);
    //read triangles
    m_spPersistTriangles->restoreXML(reader);
    //read rays
    m_spPersistRays->restoreXML(reader);
    //read xlines
    m_spPersistXlines->restoreXML(reader);
    //read polylines
    m_spPersistPolylines->restoreXML(reader);
    //read splines
    m_spPersistSplines->restoreXML(reader);
    //read BlcokReferences
    m_spPersistBlcokReferences->restoreXML(reader);
    //read Texts
    m_spPersistTexts->restoreXML(reader);
    //read MTexts
    m_spPersistMTexts->restoreXML(reader);
    //read AttributeDefinitions
    m_spPersistAttributeDefinitions->restoreXML(reader);
    //read Attributes
    m_spPersistAttributes->restoreXML(reader);
    //read Dimlinears
    m_spPersistDimLinears->restoreXML(reader);
    //read DimAligneds
    m_spPersistDimAligneds->restoreXML(reader);
    //read dimAngular
    m_spPersistDimAngulars->restoreXML(reader);
    //read dimRadials
    m_spPersistDimRadials->restoreXML(reader);
    //read dimDiametrics
    m_spPersistDimDiametrics->restoreXML(reader);
    //read dimLeader
    m_spPersistDimLeaders->restoreXML(reader);
    // read hatchs
    m_spPersistHatchs->restoreXML(reader);
    // read custom entities
    m_spPersistCustomEntities->restoreXML(reader);

    reader.readEndElement("Entities");
}

FilterInterface *FilterOcdIO::createFilter()
{
    return new FilterOcdIO();
}

void FilterOcdIO::initPersist(const DmDocument& document)
{
    m_spPersistLineTypes.reset(new MetaLineTypesContainer(m_pDocument));
    m_spPersistLayers.reset(new MetaLayersContainer(m_pDocument));
    m_spPersistTextStyles.reset(new MetaTextStylesContainer(m_pDocument));
    m_spPersistDimensionStyles.reset(new MetaDimensionStylesContainer(m_pDocument));
    m_spPersistLines.reset(new MetaLinesContainer(m_pDocument));
    m_spPersistCircles.reset(new MetaCirclesContainer(m_pDocument));
    m_spPersistArcs.reset(new MetaArcsContainer(m_pDocument));
    m_spPersistPoints.reset(new MetaPointsContainer(m_pDocument));
    m_spPersistEllipses.reset(new MetaEllipsesContainer(m_pDocument));
    m_spPersistRays.reset(new MetaRaysContainer(m_pDocument));
    m_spPersistXlines.reset(new MetaXlinesContainer(m_pDocument));
    m_spPersistSolids.reset(new MetaSolidsContainer(m_pDocument));
    m_spPersistTriangles.reset(new MetaTrianglesContainer(m_pDocument));
    m_spPersistPolylines.reset(new MetaPolylinesContainer(m_pDocument));
    m_spPersistSplines.reset(new MetaSplinesContainer(m_pDocument));
    m_spPersistBlockTableRecords.reset(new MetaBlockTableRecordsContainer(m_pDocument));
    m_spPersistBlcokReferences.reset(new MetaBlcokReferencesContainer(m_pDocument));
    m_spPersistTexts.reset(new MetaTextsContainer(m_pDocument));
    m_spPersistMTexts.reset(new MetaMTextsContainer(m_pDocument));
    m_spPersistAttributeDefinitions.reset(new MetaAttributeDefinitionsContainer(m_pDocument));
    m_spPersistAttributes.reset(new MetaAttributesContainer(m_pDocument));
    m_spPersistDimLinears.reset(new MetaDimLinearsContainer(m_pDocument));
    m_spPersistDimAligneds.reset(new MetaDimAlignedsContainer(m_pDocument));
    m_spPersistDimAngulars.reset(new MetaDimAngularsContainer(m_pDocument));
    m_spPersistDimRadials.reset(new MetaDimRadialsContainer(m_pDocument));
    m_spPersistDimDiametrics.reset(new MetaDimDiametricsContainer(m_pDocument));
    m_spPersistDimLeaders.reset(new MetaDimLeadersContainer(m_pDocument));
    m_spPersistHatchs.reset(new MetaHatchsContainer(m_pDocument));
    m_spPersistCustomEntities.reset(new MetaCustomEntitiesContainer(m_pDocument));
}
