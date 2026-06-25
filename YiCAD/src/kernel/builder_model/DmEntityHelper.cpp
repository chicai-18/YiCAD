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


/// @file DmEntityHelper.cpp
/// @brief 实体工厂辅助类实现，根据类型名创建实体及获取实体类型名称

#include "DmEntityHelper.h"
#include "DmLine.h"
#include "DmCircle.h"
#include "DmPoint.h"
#include "DmPolyline.h"
#include "DmArc.h"
#include "DmEllipse.h"
#include "DmSolid.h"
#include "DmSpline.h"
#include "DmRay.h"
#include "DmXline.h"
#include "DmBlockReference.h"
#include "DmText.h"
#include "DmMText.h"
#include "DmAttributeDefinition.h"
#include "DmAttribute.h"
#include "DmDimLinear.h"
#include "DmDimAligned.h"
#include "DmDimAngular.h"
#include "DmDimRadial.h"
#include "DmDimDiametric.h"
#include "DmLeader.h"
#include "DmImage.h"
#include "DmHatch.h"
#include "DmRegion.h"

DmEntity* DmEntityHelper::createEntityByName(const std::string& strType)
{
    DmEntity* pEntity = nullptr;

    if (strType == "Line")
    {
        pEntity = new DmLine();
    }
    else if (strType == "Circle")
    {
        pEntity = new DmCircle();
    }
    else if (strType == "Point")
    {
        pEntity = new DmPoint();
    }
    else if (strType == "Polyline")
    {
        pEntity = new DmPolyline();
    }
    else if (strType == "Arc")
    {
        pEntity = new DmArc();
    }
    else if (strType == "Ellipse")
    {
        pEntity = new DmEllipse();
    }
    else if (strType == "Solid")
    {
        pEntity = new DmSolid();
    }
    else if (strType == "Spline")
    {
        pEntity = new DmSpline();
    }
    else if (strType == "Ray")
    {
        pEntity = new DmRay();
    }
    else if (strType == "Xline")
    {
        pEntity = new DmXline();
    }
    else if (strType == "Insert")
    {
        pEntity = new DmBlockReference();
    }
    else if (strType == "Text")
    {
        pEntity = new DmText();
    }
    else if (strType == "MText")
    {
        pEntity = new DmMText();
    }
    else if (strType == "AttributeDefinition")
    {
        pEntity = new DmAttributeDefinition();
    }
    else if (strType == "Attribute")
    {
        pEntity = new DmAttribute();
    }
    else if (strType == "DimLinear")
    {
        pEntity = new DmDimLinear();
    }
    else if (strType == "DimAligned")
    {
        pEntity = new DmDimAligned();
    }
    else if (strType == "DimAngular")
    {
        pEntity = new DmDimAngular();
    }
    else if (strType == "DimRadial")
    {
        pEntity = new DmDimRadial();
    }
    else if (strType == "DimDiametric")
    {
        pEntity = new DmDimDiametric();
    }
    else if (strType == "DimLeader")
    {
        pEntity = new DmLeader();
    }
    else if (strType == "Hatch")
    {
        pEntity = new DmHatch();
    }
    else if (strType == "Region")
    {
        pEntity = new DmRegion();
    }
    else
    {
        // 未匹配的实体类型，返回 nullptr
        pEntity = nullptr;
    }

    return pEntity;
}

std::string DmEntityHelper::getEntityNameByType(DM::EntityType entityType)
{
    switch (entityType)
    {
        case DM::EntityUnknown:
        {
            return "Unknown";
        }
        case DM::EntityAttribute:
        {
            return "Attribute";
        }
        case DM::EntityAttributeDefinition:
        {
            return "AttributeDefinition";
        }
        case DM::EntityContainer:
        {
            return "Container";
        }
        case DM::EntityBlock:
        {
            return "Block";
        }
        case DM::EntityCharTemplate:
        {
            return "CharTemplate";
        }
        case DM::EntityBlockReference:
        {
            return "Insert";
        }
        case DM::EntityPoint:
        {
            return "Point";
        }
        case DM::EntityLine:
        {
            return "Line";
        }
        case DM::EntityPolyline:
        {
            return "Polyline";
        }
        case DM::EntityArc:
        {
            return "Arc";
        }
        case DM::EntityCircle:
        {
            return "Circle";
        }
        case DM::EntityEllipse:
        {
            return "Ellipse";
        }
        case DM::EntitySolid:
        {
            return "Solid";
        }
        case DM::EntityConstructionLine:
        {
            return "ConstructionLine";
        }
        case DM::EntityMText:
        {
            return "MText";
        }
        case DM::EntityText:
        {
            return "Text";
        }
        case DM::EntityDimAligned:
        {
            return "DimAligned";
        }
        case DM::EntityDimLinear:
        {
            return "DimLinear";
        }
        case DM::EntityDimRadial:
        {
            return "DimRadial";
        }
        case DM::EntityDimDiametric:
        {
            return "DimDiametric";
        }
        case DM::EntityDimAngular:
        {
            return "DimAngular";
        }
        case DM::EntityDimLeader:
        {
            return "DimLeader";
        }
        case DM::EntityHatch:
        {
            return "Hatch";
        }
        case DM::EntityRegion:
        {
            return "Region";
        }
        case DM::EntityImage:
        {
            return "Image";
        }
        case DM::EntitySpline:
        {
            return "Spline";
        }
        case DM::EntityOverlayBox:
        {
            return "OverlayBox";
        }
        case DM::EntityPreview:
        {
            return "Preview";
        }
        case DM::EntityPattern:
        {
            return "Pattern";
        }
        case DM::EntityOverlayLine:
        {
            return "OverlayLine";
        }
        case DM::EntityRay:
        {
            return "Ray";
        }
        case DM::EntityXline:
        {
            return "Xline";
        }
        default:
        {
            return std::string();
        }
    }
}
