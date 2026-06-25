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
#include <QTextCodec>
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

#include "DmBlock.h"

#define IMPORTTYPE "ycd"
#define EXPORTTYPE "Drawing Exchange YCD 2023 (*.ycd)"

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
    XMLReader reader(strfileName.c_str(), archive.stream());

    if (!reader.isValid())
    {
        throw OneException("Error reading compression file");
    }

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
        , texts, mtexts, attributeDefinitions, attributes, dimLinears, dimAligneds, dimAngulars, dimRadials, dimDiametrics, dimLeaders;
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
        //image

        writer.Stream() << writer.ind() << "</Entities>" << std::endl;
    }
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
}
