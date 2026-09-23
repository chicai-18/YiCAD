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


/// @file Datamodel.h
/// @brief 数据模型核心定义，包含全局常量、枚举和命名空间

#ifndef DM_H
#define DM_H

// Windoze XP can't handle the original MAX/MINDOUBLE's
constexpr double DM_MAXDOUBLE = 1.0E+10;
constexpr double DM_MINDOUBLE = -1.0E+10;
// tolerance
constexpr double DM_TOLERANCE = 1.0e-10;
// squared tolerance
constexpr double DM_TOLERANCE15 = 1.5e-15;
constexpr double DM_TOLERANCE2 = 1.0e-20;
constexpr double DM_TOLERANCE_ANGLE = 1.0e-8;
// 圆的打断次数 默认60段（TODO 待删除）
constexpr int DM_CURVE_VERTEXS = 60;

constexpr int CIRCLE_SEGMENT_COUNT = 120;
constexpr int ELLIPSE_SEGMENT_COUNT = 120;

constexpr double DM_PI = 3.14159265;

// 默认表格样式名
constexpr const char* DEFAULT_TABLESTYLE_NAME = "Standard";

// Class namespace for various enums along with some simple
// wrapper methods for converting the enums to the Qt counterparts.

namespace DM
{
    enum Flags
    {
        FlagUndone = 1,         // Flag for Undoables.
        FlagVisible = 2,        // Entity Visibility.
        FlagByLayer = 4,        // Entity attribute (e.g. color) is defined by layer.
        FlagByBlock = 8,        // Entity attribute (e.g. color) defined by block.
        FlagFrozen = 16,        // Layer frozen.
        FlagDefFrozen = 32,     // Layer frozen by default.
        FlagLocked = 64,        // Layer locked.
        FlagInvalid = 128,      // Used for invalid pens.
        FlagSelected = 256,     // Entity in current selection.
        FlagClosed = 512,       // Polyline closed?
        FlagTemp = 1024,        // Flag for temporary entities (e.g. hatch)
        FlagProcessed = 2048,   // Flag for processed entities (optcontour)
        FlagSelected1 = 4096,   // Startpoint selected
        FlagSelected2 = 8192,   // Endpoint selected
        FlagHighlighted = 16384 // Entity is highlighted temporarily (as a user action feedback)
    };

    // Variable types used by VariableDict and Variable.
    enum VariableType
    {
        VariableString,
        VariableInt,
        VariableDouble,
        VariableVector,
        VariableVoid
    };

    /// @brief 实体类型
    enum EntityType
    {
        EntityUnknown,              // Unknown
        EntityAttribute,            // Attribute
        EntityAttributeDefinition,  // AttributeDefinition
        EntityContainer,            // Container
        EntityBlock,                // Block (Group definition)
        EntityCharTemplate,         // Font character
        EntityChar,
        EntityBlockReference,       // BlockReference
        EntityPoint,                // Point
        EntityLine,                 // Line
        EntityLineStrip,            // LineStrip
        EntityPolyline,             // Polyline
        EntityArc,                  // Arc
        EntityCircle,               // Circle
        EntityEllipse,              // Ellipse
        EntityTriangle,             // 填充三角形
        EntitySolid,                // Solid
        EntityConstructionLine,     // Construction line
        EntityMText,                // Multi-line Text
        EntityText,                 // Single-line Text
        EntityDimAligned,           // Aligned Dimension
        EntityDimLinear,            // Linear Dimension
        EntityDimRadial,            // Radial Dimension
        EntityDimDiametric,         // Diametric Dimension
        EntityDimAngular,           // Angular Dimension
        EntityDimLeader,            // Leader Dimension
        EntityHatch,                // Hatch
        EntityRegion,               // 区域
        EntityImage,                // Image
        EntitySpline,               // Spline
        EntityOverlayBox,           // OverlayBox
        EntityPreview,              // Preview Container
        EntityPattern,              // Line Type
        EntityOverlayLine,
        EntityRay,
        EntityXline,
        EntityOverlayCircle,
        EntityOverlayPoint,
    };


    /// @brief 活动类型
    enum ActionType
    {
        ActionNone,     // 无效的操作

        ActionDefault,

        ActionFileNew,
        ActionFileOpen,
        ActionFileSave,
        ActionFileSaveAs,
        ActionFileExport,
        ActionFileExportImage,
        ActionFileClose,
        ActionFilePrint,
        ActionFilePrintPDF,
        ActionFilePrintPreview,

        ActionFileQuit,

        ActionEditKillAllActions,
        ActionEditUndo,
        ActionEditRedo,
        ActionEditCut,
        ActionEditCutNoSelect,
        ActionEditCopy,
        ActionEditCopyNoSelect,
        ActionEditPaste,

        ActionViewStatusBar,
        ActionViewLayerTable,
        ActionViewBlockList,
        ActionViewCommandLine,
        ActionViewLibrary,

        ActionViewPenToolbar,
        ActionViewOptionToolbar,
        ActionViewCadToolbar,
        ActionViewFileToolbar,
        ActionViewEditToolbar,
        ActionViewSnapToolbar,

        ActionViewGrid,
        ActionViewDraft,

        ActionZoomIn,
        ActionZoomOut,
        ActionZoomPan,

        ActionSelect,
        ActionSelectSingle,
        ActionSelectMultiple,

        ActionDrawArc,
        ActionDrawArc3P,
        ActionDrawArcTangential,
        ActionDrawCircle,
        ActionDrawCircle2P,
        ActionDrawCircle3P,
        ActionDrawCircleTan2,
        ActionDrawCircleTan3,

        ActionDrawEllipseArcAxis,
        ActionDrawEllipseAxis,
        ActionDrawEllipseFociPoint,
        ActionDrawEllipse4Points,
        ActionDrawEllipseCenter3Points,
        ActionDrawEllipseInscribe,

        ActionDrawHatch,
        ActionDrawHatchNoSelect,
        ActionDrawImage,
        ActionDrawLine,
        ActionDrawLineBisector,
        ActionDrawLineFree,
        ActionDrawLineOrthTan,
        ActionDrawLinePolygonCenCor,
        ActionDrawLinePolygonCenTan,
        ActionDrawLineRectangle,
        ActionDrawLineTangent1,
        ActionDrawLineTangent2,
        ActionDrawMText,
        ActionDrawPoint,
        ActionDrawRay,
        ActionDrawSpline,
        ActionDrawSplinePoints,
        ActionDrawPolyline,
        ActionDrawText,
        ActionDrawXline,

        ActionPolylineAdd,
        ActionPolylineAppend,
        ActionPolylineDel,
        ActionCloudLineRectangle,
        ActionCloudLinePolygon,
        ActionCloudLineFree,

        ActionDimAligned,
        ActionDimLinear,
        ActionDimRadial,
        ActionDimDiametric,
        ActionDimAngular,
        ActionDimLeader,
        ActionDimBaseline,
        ActionDimStyle,
        ActionTextStyle,

        ActionModifyCopy,
        ActionModifyCopyNoSelect,
        ActionModifyDelete,
        ActionModifyDeleteNoSelect,
        ActionModifyMove,
        ActionModifyMoveNoSelect,
        ActionModifyRotate,
        ActionModifyRotateNoSelect,
        ActionModifyScale,
        ActionModifyScaleNoSelect,
        ActionModifyMirror,
        ActionModifyMirrorNoSelect,
        ActionModifyEntity,
        ActionModifyMText,
        ActionModifyTrim,
        ActionModifyCut,
        ActionModifyCut2P,
        ActionModifyBevel,
        ActionModifyRound,
        ActionModifySingleOffset,
        ActionModifyExtend,
        ActionModifyExplode,
        ActionModifyExplodeNoSelect,
        ActionModifyReverse,
        ActionModifyReverseNoSelect,

        ActionSnapFree,
        ActionSnapGrid,
        ActionSnapEndpoint,
        ActionSnapOnEntity,
        ActionSnapCenter,
        ActionSnapMiddle,
        ActionSnapIntersection,

        ActionRestrictNothing,
        ActionRestrictOrthogonal,
        ActionRestrictHorizontal,
        ActionRestrictVertical,

        ActionInfoDist,
        ActionInfoAngle,
        ActionInfoTotalLength,
        ActionInfoTotalLengthNoSelect,
        ActionInfoArea,
        ActionInfoSelected,

        ActionLayersFreeze,
        ActionLayersLock,
        ActionLayersPrint,
        ActionLayersColor,
        ActionLayersDelete,
        ActionLayersDefreezeAll,
        ActionLayersFreezeAll,
        ActionLayersUnlockAll,
        ActionLayersLockAll,
        ActionLayersRename,
        ActionLayersAdd,
        ActionLayersActivate,

        ActionBlocksSave,
        ActionBlocksSaveAs,
        ActionBlockInsertPrepare,
        ActionBlocksInsert,
        ActionBlocksCreate,
        ActionBlocksCreateNoSelect,
        ActionBlocksDelete,
        ActionBlocksEdit,
        ActionBlocksEditNoSelect,
        ActionBlocksImport,
        ActionDefineAttributes,

        ActionOptionsGeneral,
        ActionOptionsDrawing,

        ActionScriptOpenIDE,
        ActionScriptRun,

        ActionNoSelectCopyToLayer,
        ActionCopyToLayer,
        ActionSelectedChanged,

        ActionLast,    // Needed to loop through all actions

    };

    // Entity ending. Used for returning which end of an entity is meant.
    enum Ending
    {
        EndingStart,    // Start point.
        EndingEnd,      // End point.
        EndingNone      // Neither.
    };

    // Update mode for non-atomic entities that need to be updated when they change. e.g. texts, inserts, ...
    enum UpdateMode
    {
        NoUpdate,       // No automatic updates.
        Update,         // Always update automatically when modified.
        PreviewUpdate   // Update automatically but only for previews (quick update)
    };

    /// @brief 绘制实体
    enum DrawingMode
    {
        ModeFull,       // Draw everything always detailed (default)
        ModeAuto,       // Draw details when reasonable
        ModePreview,    // Draw only in black/white without styles
        ModeBW,         // Black/white. Can be used for printing.
        ModeWB,         // White/black, used for export
    };

    enum UndoableType
    {
        UndoableUnknown,    // Unknown undoable
        UndoableEntity,     // Entity
        UndoableLayer       // Layer
    };

    /// @brief 单位
    enum Unit
    {
        None = 0,               // No unit (unit from parent)
        Inch = 1,               // Inch
        Foot = 2,               // Foot: 12 Inches
        Mile = 3,               // Mile: 1760 Yards = 1609 m
        Millimeter = 4,         // Millimeter: 0.001m
        Centimeter = 5,         // Centimeter: 0.01m
        Meter = 6,              // Meter
        Kilometer = 7,          // Kilometer: 1000m
        Microinch = 8,          // Microinch: 0.000001
        Mil = 9,                // Mil = 0.001 Inch
        Yard = 10,              // Yard: 3 Feet
        Angstrom = 11,          // Angstrom: 10^-10m
        Nanometer = 12,         // Nanometer: 10^-9m
        Micron = 13,            // Micron: 10^-6m
        Decimeter = 14,         // Decimeter: 0.1m
        Decameter = 15,         // Decameter: 10m
        Hectometer = 16,        // Hectometer: 100m
        Gigameter = 17,         // Gigameter: 1000000m
        Astro = 18,             // Astro: 149.6 x 10^9m
        Lightyear = 19,         // Lightyear: 9460731798 x 10^6m
        Parsec = 20,            // Parsec: 30857 x 10^12
        LastUnit = 21           // Used to iterate through units
    };

    // Format for length values.
    enum LinearFormat
    {
        Scientific,         // Scientific (e.g. 2.5E+05)
        Decimal,            // Decimal (e.g. 9.5)
        Engineering,        // Engineering (e.g. 7' 11.5")
        Architectural,      // Architectural (e.g. 7'-9 1/8")
        Fractional,         // Fractional (e.g. 7 9 1/8)
        ArchitecturalMetric // Metric Architectural using DIN 406 (e.g. 1.12⁵)
    };

    // Angle Units.
    enum AngleUnit
    {
        Deg,               // Degrees
        Rad,               // Radians
        Gra                // Gradians
    };

    // Display formats for angles.
    enum AngleFormat
    {
        DegreesDecimal,         // Degrees with decimal point (e.g. 24.5)
        DegreesMinutesSeconds,  // Degrees, Minutes and Seconds (e.g. 24�30'5'')
        Gradians,               // Gradians with decimal point (e.g. 390.5)
        Radians,                // Radians with decimal point (e.g. 1.57)
        Surveyors               // Surveyor's units
    };

    // Enum of levels of resolving when iterating through an entity tree.
    enum ResolveLevel
    {
        ResolveNone,            // Groups are not resolved
        ResolveAllButInserts,   // Resolve all but not Inserts.
        ResolveAllButTexts,     // Resolve all but not Text or MText.
        ResolveAllButTextImage, // Resolve no text or images
        ResolveAll              // all Entity Containers are resolved (including Texts, Polylines, ...)
    };

    // Direction used for scrolling actions.
    enum Direction
    {
        Up,
        Left,
        Right,
        Down
    };

    enum SubWindowMode
    {
        CurrentMode = -1,
        Maximized,
        Cascade,
        Tile,
        TileVertical,
        TileHorizontal
    };

    enum TabShape
    {
        AnyShape = -1,
        Rounded,
        Triangular
    };

    enum TabPosition
    {
        AnyPosition = -1,
        North,
        South,
        West,
        East
    };

    // Leader path type.
    enum LeaderPathType
    {
        Straight,      // Straight line segments
        Spline         // Splines
    };

    // Direction for zooming actions.
    enum ZoomDirection
    {
        In,
        Out
    };

    // Axis specification for zooming actions.
    enum Axis
    {
        OnlyX,
        OnlyY,
        Both
    };

    // Snapping modes
    enum SnapMode
    {
        SnapFree,               // Free positioning
        SnapGrid,               // Snap to grid points
        SnapEndpoint,           // Snap to endpoints
        SnapMiddle,             // Snap to middle points
        SnapCenter,             // Snap to centers
        SnapOnEntity,           // Snap to the next point on an entity
        SnapIntersection,       // Snap to intersection
        SnapIntersectionManual  // Snap to intersection manually
    };

    /// 捕捉的正交模式
    enum SnapRestriction
    {
        RestrictNothing,        // No restriction to snap mode
        RestrictHorizontal,     // Restrict to 0,180 degrees
        RestrictVertical,       // Restrict to 90,270 degrees
        RestrictOrthogonal      // Restrict to 90,180,270,0 degrees
    };

    // 线宽
    enum LineWidth
    {
        Width00 = 0,       // 0.00mm
        Width01 = 5,       // 0.05mm
        Width02 = 9,       // 0.09mm
        Width03 = 13,      // 0.13mm
        Width04 = 15,      // 0.15mm
        Width05 = 18,      // 0.18mm
        Width06 = 20,      // 0.20mm
        Width07 = 25,      // 0.25mm
        Width08 = 30,      // 0.30mm
        Width09 = 35,      // 0.35mm
        Width10 = 40,      // 0.40mm
        Width11 = 50,      // 0.50mm
        Width12 = 53,      // 0.53mm
        Width13 = 60,      // 0.60mm
        Width14 = 70,      // 0.70mm
        Width15 = 80,      // 0.80mm
        Width16 = 90,      // 0.90mm
        Width17 = 100,     // 1.00mm
        Width18 = 106,     // 1.06mm
        Width19 = 120,     // 1.20mm
        Width20 = 140,     // 1.40mm
        Width21 = 158,     // 1.58mm
        Width22 = 200,     // 2.00mm
        Width23 = 211,     // 2.11mm
        WidthByLayer = -1, // ByLayer
        WidthByBlock = -2, // ByBlock
        WidthDefault = -3  // Default
    };

    // Wrapper for Qt
    LineWidth intToLineWidth(int w);

    // 鼠标的光标样式
    enum CursorType
    {
        ArrowCursor,          // ArrowCursor - standard arrow cursor.
        UpArrowCursor,        // UpArrowCursor - upwards arrow.
        CrossCursor,          // CrossCursor - crosshair.
        WaitCursor,           // WaitCursor - hourglass/watch.
        IbeamCursor,          // IbeamCursor - ibeam/text entry.
        SizeVerCursor,        // SizeVerCursor - vertical resize.
        SizeHorCursor,        // SizeHorCursor - horizontal resize.
        SizeBDiagCursor,      // SizeBDiagCursor - diagonal resize (/).
        SizeFDiagCursor,      // SizeFDiagCursor - diagonal resize (\).
        SizeAllCursor,        // SizeAllCursor - all directions resize.
        BlankCursor,          // BlankCursor - blank/invisible cursor.
        SplitVCursor,         // SplitVCursor - vertical splitting.
        SplitHCursor,         // SplitHCursor - horizontal splitting.
        PointingHandCursor,   // PointingHandCursor - a pointing hand.
        ForbiddenCursor,      // ForbiddenCursor - a slashed circle.
        WhatsThisCursor,      // WhatsThisCursor - an arrow with a ?.
        OpenHandCursor,       // Qt OpenHandCursor
        ClosedHandCursor,     // Qt ClosedHandCursor
        CadCursor,            // CadCursor - a bigger cross.
        DelCursor,            // DelCursor - cursor for choosing entities
        SelectCursor,         // SelectCursor - for selecting single entities
        MagnifierCursor,      // MagnifierCursor - a magnifying glass.
        MovingHandCursor      // Moving hand - a little flat hand.
    };

    // Paper formats.
    enum PaperFormat
    {
        FirstPaperFormat,
        Custom = FirstPaperFormat,

        // ISO "A" Series
        A0,   // 841 x 1189 mm   33.1 x 46.8 in
        A1,   // 594 x 841 mm    23.4 x 33.1 in
        A2,   // 420 x 594 mm    16.5 x 23.4 in
        A3,   // 297 x 420 mm    11.7 x 16.5 in
        A4,   // 210 x 297 mm    8.3 x 11.7 in

        // Removed ISO "B" and "C" series, C5E, Comm10E, DLE, (envelope sizes)

        // US "Office"
        Letter,   // 216 x 279 mm   8.5 x 11.0 in
        Legal,    // 216 x 356 mm   8.5 x 14.0 in
        Tabloid,  // 279 x 432 mm   11.0 x 17.0 in

        // Tabloid = Ledger = ANSI B.  Although, technically, both ANSI B and
        // Ledger are defined in the qt library as 431.8 mm x 279.4 mm / 17
        // x 11", while Tabloid is 279 x 432 mm / 11.0 x 17.0 in .  Using either
        // "Ledger" or "AnsiB" will result in the wrong page orientation when
        // printing or exporting to PDF.)

        // ANSI
        Ansi_A,   // 216 x 279 mm   8.5 x 11.0 in
        Ansi_B,   // 279 x 432 mm   11.0 x 17.0 in
        Ansi_C,   // 432 x 559 mm   17.0 x 22.0 in
        Ansi_D,   // 559 x 864 mm   22.0 x 34.0 in
        Ansi_E,   // 864 x 1118 mm  34.0 x 44.0 in

        // Architectural
        Arch_A,    // 229 x 305 mm   9.0 x 12.0 in
        Arch_B,    // 305 x 457 mm   12.0 x 18.0 in
        Arch_C,    // 457 x 610 mm   18.0 x 24.0 in
        Arch_D,    // 610 x 914 mm   24.0 x 36.0 in
        Arch_E,    // 914 x 1219 mm  36.0 x 48.0 in

        NPageFormat
    };

    // Items that can be put on a overlay, the items are rendered in this order. Best is to leave snapper as last so it always shows up
    enum OverlayDocument
    {
        ActionPreviewEntity,  // Action Entities
        Snapper               // Snapper
    };

    // Text drawing direction.
    enum TextLocaleDirection
    {
        locLeftToRight,     // Left to right
        locRightToLeft      // Right to Left
    };

    /// @brief 切变方向
    enum ShearDirection
    {
        X,  //沿X方向切变，Y值不变
        Y   //沿Y方向切变，X值不变
    };

    /// @brief 箭头类型
    enum class ArrowType
    {
        ClosedFilled = 0,       //实心闭合
        ClosedBlank,            //空心闭合
        Closed,                 //闭合
        Dot,                    //点
        ArchitecturalTick,      //建筑标记
        Oblique,                //倾斜
        Open,                   //打开
        OriginIndicator,        //指示原点
        OriginIndicator2,       //指示原点2
        RightAngle,             //直角
        Open30,                 //30度角
        DotSmall,               //小点
        DotBlank,               //空心点
        DotSmallBlank,          //空心小点
        Box,                    //方框
        BoxFilled,              //实心方框
        DatumTriangle,          //基准三角形
        DatumTriangleFilled,    //实心基准三角形
        Integral,               //积分
        None,                   //无
        UserArrow,              //用户箭头...
    };

    enum ObserverType
    {
        ArrayRectObserver,
        ArrayPolarObserver
    };
};

#endif
