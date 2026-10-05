#ifndef YICAD_PLUGIN_ABI_H
#define YICAD_PLUGIN_ABI_H

#include <stddef.h>
#include <stdint.h>

#if !defined(__cplusplus) && \
    (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 201112L)
#error "YiCadPluginAbi.h requires C11 or later"
#endif

/** @brief 历史版本号，仅用于说明既有字段来源。 */
#define YICAD_PLUGIN_ABI_V1 UINT32_C(1)
/** @brief 历史版本号，仅用于说明既有字段来源。 */
#define YICAD_PLUGIN_ABI_V2 UINT32_C(2)
/** @brief 历史版本号，仅用于说明既有字段来源（名字带 V3 的结构是这一版引入的，布局在 v4 不变）。 */
#define YICAD_PLUGIN_ABI_V3 UINT32_C(3)
/** @brief 当前且唯一受支持的插件 ABI 版本号：v3 加自定义实体（各函数表尾部追加）。 */
#define YICAD_PLUGIN_ABI_V4 UINT32_C(4)
/** @brief 当前 SDK 支持的最低 C ABI 版本。 */
#define YICAD_PLUGIN_ABI_MIN_VERSION YICAD_PLUGIN_ABI_V4
/** @brief 当前 SDK 支持的最高 C ABI 版本。 */
#define YICAD_PLUGIN_ABI_MAX_VERSION YICAD_PLUGIN_ABI_V4
/** @brief 当前 C ABI 版本。 */
#define YICAD_PLUGIN_ABI_VERSION YICAD_PLUGIN_ABI_V4

#if defined(_WIN32)
#define YICAD_PLUGIN_CALL __cdecl
#define YICAD_PLUGIN_EXPORT_ATTRIBUTE __declspec(dllexport)
#elif defined(__GNUC__) || defined(__clang__)
#define YICAD_PLUGIN_CALL
#define YICAD_PLUGIN_EXPORT_ATTRIBUTE __attribute__((visibility("default")))
#else
#define YICAD_PLUGIN_CALL
#define YICAD_PLUGIN_EXPORT_ATTRIBUTE
#endif

#if defined(__cplusplus)
#define YICAD_PLUGIN_EXTERN_C extern "C"
#else
#define YICAD_PLUGIN_EXTERN_C extern
#endif

#if defined(YICAD_PLUGIN_BUILD)
#define YICAD_PLUGIN_API \
    YICAD_PLUGIN_EXTERN_C YICAD_PLUGIN_EXPORT_ATTRIBUTE
#else
#define YICAD_PLUGIN_API YICAD_PLUGIN_EXTERN_C
#endif

#define YICAD_PLUGIN_EXPORT YICAD_PLUGIN_API

typedef int32_t YiCadResult;

#define YICAD_FAILURE ((YiCadResult)0)
#define YICAD_SUCCESS ((YiCadResult)1)

/** @brief 非拥有型文档句柄，仅在对应文档保持打开期间有效。 */
typedef void* YiCadDocumentHandle;
/**
 * @brief 宿主持有的事务句柄。
 * @note 插件必须且只能调用一次 commit 或 rollback；两者都会释放句柄。
 */
typedef void* YiCadTransactionHandle;
/**
 * @brief 宿主持有的只读实体快照迭代器句柄。
 * @note 插件必须调用 entityIteratorDestroy；销毁前快照保持有效。
 */
typedef void* YiCadEntityIteratorHandle;

/** @brief 宿主持有的导入会话句柄，提交或回滚后立即失效。 */
typedef void* YiCadImportSessionHandle;
/** @brief 所属导入会话内有效的容器句柄。 */
typedef void* YiCadImportContainerHandle;
/** @brief 所属导入会话内有效的资源、块定义或块引用句柄。 */
typedef void* YiCadImportResourceHandle;
/** @brief 文档只读期间有效的资源或块句柄。 */
typedef const void* YiCadReadResourceHandle;

/** @brief 导入子接口使用的固定宽度结果码。 */
typedef int32_t YiCadImportResult;

#define YICAD_IMPORT_SUCCESS ((YiCadImportResult)0)
#define YICAD_IMPORT_ERROR_INVALID_ARGUMENT ((YiCadImportResult)-1)
#define YICAD_IMPORT_ERROR_INVALID_HANDLE ((YiCadImportResult)-2)
#define YICAD_IMPORT_ERROR_NAME_CONFLICT ((YiCadImportResult)-3)
#define YICAD_IMPORT_ERROR_RESOURCE_NOT_FOUND ((YiCadImportResult)-4)
#define YICAD_IMPORT_ERROR_UNSUPPORTED ((YiCadImportResult)-5)
#define YICAD_IMPORT_ERROR_OUT_OF_RANGE ((YiCadImportResult)-6)
#define YICAD_IMPORT_ERROR_OUT_OF_MEMORY ((YiCadImportResult)-7)
#define YICAD_IMPORT_ERROR_TRANSACTION_FAILED ((YiCadImportResult)-8)

/**
 * @brief ABI v3 输入结构的共同约定。
 * @note 调用方应先将完整结构清零，再把 structSize 设为 sizeof(结构) 并填写字段。
 * 所有浮点数必须有限且绝对值不超过宿主可表示范围；角度使用弧度。布尔型
 * uint32_t 字段只接受 0 或 1。未明确声明可为空的字符串、数组和句柄不得为空。
 * @note 所有结构和数组只在函数调用期间借用，宿主会在返回前复制需要保留的
 * UTF-8 字符串和数组。插件不得把宿主句柄跨导入回调缓存或自行释放。
 * @note 所有导入函数只允许在 YiCAD UI 主线程调用。失败返回 YiCadImportResult，
 * 且不得留下本次函数调用的部分修改；详细 UTF-8 文本由 getLastError 读取。
 */

/** @brief UTF-8 字符串视图；宿主在调用返回前复制内容。 */
typedef struct YiCadStringView
{
    const char* data;
    uint32_t size;
} YiCadStringView;

/** @brief 只读 double 数组视图；宿主在调用返回前复制内容。 */
typedef struct YiCadDoubleArrayView
{
    const double* data;
    uint32_t count;
} YiCadDoubleArrayView;

/** @brief 二维点；坐标必须是有限值，具体坐标系由包含它的结构说明。 */
typedef struct YiCadPoint2d
{
    double x;
    double y;
} YiCadPoint2d;

/** @brief 三维点；坐标必须是有限值，具体坐标系由包含它的结构说明。 */
typedef struct YiCadPoint3d
{
    double x;
    double y;
    double z;
} YiCadPoint3d;

/** @brief 二维向量；非零和归一化要求由包含它的结构说明。 */
typedef YiCadPoint2d YiCadVector2d;
/** @brief 三维向量；非零和归一化要求由包含它的结构说明。 */
typedef YiCadPoint3d YiCadVector3d;

/** @brief 只读二维点数组视图；宿主在调用返回前复制内容。 */
typedef struct YiCadPoint2dArrayView
{
    const YiCadPoint2d* data;
    uint32_t count;
} YiCadPoint2dArrayView;

typedef int32_t YiCadColorMethod;
#define YICAD_COLOR_BY_LAYER ((YiCadColorMethod)0)
#define YICAD_COLOR_BY_BLOCK ((YiCadColorMethod)1)
#define YICAD_COLOR_ACI ((YiCadColorMethod)2)
#define YICAD_COLOR_RGB ((YiCadColorMethod)3)

/** @brief 固定布局颜色值；支持随层、随块和 RGB，ACI 输入由宿主转换为 RGB。 */
typedef struct YiCadColorData
{
    YiCadColorMethod method;
    uint32_t aci;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t reserved;
} YiCadColorData;

typedef int32_t YiCadResourceConflictPolicy;
#define YICAD_RESOURCE_CONFLICT_FAIL ((YiCadResourceConflictPolicy)0)
#define YICAD_RESOURCE_CONFLICT_REPLACE ((YiCadResourceConflictPolicy)1)
#define YICAD_RESOURCE_CONFLICT_RENAME ((YiCadResourceConflictPolicy)2)

/**
 * @brief 文档设置；代码页仅作为源文件元数据保存。
 * @note insertionUnits 范围为 0..20；measurement 为 0 或 1；
 * globalLineTypeScale（LTSCALE）必须大于 0；sourceCodePage 可为空。
 * @note v4 追加 currentEntityLineTypeScale（CELTSCALE，新建实体的线型比例），
 * 必须大于 0；structSize 不覆盖它时按 1 处理。
 */
typedef struct YiCadDocumentSettings
{
    uint32_t structSize;
    int32_t insertionUnits;
    int32_t measurement;
    double globalLineTypeScale;
    YiCadStringView sourceCodePage;
    double currentEntityLineTypeScale;
} YiCadDocumentSettings;

/**
 * @brief 简单线型定义；名称非空，空 elements 表示连续线，其他元素必须有限。
 * @note complex 必须为 0；非零时返回 YICAD_IMPORT_ERROR_UNSUPPORTED，禁止降级。
 */
typedef struct YiCadLineTypeDataV3
{
    uint32_t structSize;
    YiCadStringView name;
    YiCadStringView description;
    YiCadDoubleArrayView elements;
    uint32_t complex;
} YiCadLineTypeDataV3;

/**
 * @brief 图层定义。
 * @note frozen、locked、plottable 为 0 或 1；lineType 为空时使用 Continuous；
 * lineWidth 只接受 YiCAD 已定义的标准线宽以及 -3、-2、-1。
 */
typedef struct YiCadLayerDataV3
{
    uint32_t structSize;
    YiCadStringView name;
    uint32_t frozen;
    uint32_t locked;
    uint32_t plottable;
    YiCadColorData color;
    YiCadImportResourceHandle lineType;
    int32_t lineWidth;
} YiCadLayerDataV3;

#define YICAD_TEXT_GENERATION_BACKWARD UINT32_C(1)
#define YICAD_TEXT_GENERATION_UPSIDE_DOWN UINT32_C(2)
#define YICAD_TEXT_GENERATION_VERTICAL UINT32_C(4)

/**
 * @brief 文字样式定义；字体文件原名在字体缺失时仍会保存。
 * @note fixedHeight 不得小于 0，widthFactor 必须大于 0，obliqueAngle 使用弧度；
 * generationFlags 只能组合 YICAD_TEXT_GENERATION_* 标志。
 */
typedef struct YiCadTextStyleDataV3
{
    uint32_t structSize;
    YiCadStringView name;
    YiCadStringView fontFile;
    YiCadStringView bigFontFile;
    double fixedHeight;
    double widthFactor;
    double obliqueAngle;
    uint32_t generationFlags;
} YiCadTextStyleDataV3;

/**
 * @brief YiCAD 当前可表达的标注样式字段。
 * @note 所有长度和比例字段必须有限，正值字段不得为零；unsupportedFieldMask
 * 非零且 allowUnsupportedFields 为 0 时返回 YICAD_IMPORT_ERROR_UNSUPPORTED。
 */
typedef struct YiCadDimensionStyleDataV3
{
    uint32_t structSize;
    YiCadStringView name;
    YiCadImportResourceHandle textStyle;
    YiCadImportResourceHandle dimLineType;
    YiCadImportResourceHandle extensionLineType;
    YiCadColorData dimLineColor;
    YiCadColorData extensionLineColor;
    YiCadColorData textColor;
    YiCadColorData textFillColor;
    int32_t dimLineWidth;
    int32_t extensionLineWidth;
    uint32_t hideDimLine1;
    uint32_t hideDimLine2;
    uint32_t hideExtensionLine1;
    uint32_t hideExtensionLine2;
    double extensionBeyondDimLine;
    double extensionOriginOffset;
    uint32_t fixedExtensionLineLengthEnabled;
    double fixedExtensionLineLength;
    int32_t firstArrow;
    int32_t secondArrow;
    int32_t leaderArrow;
    double arrowSize;
    double textHeight;
    double fractionHeightScale;
    uint32_t drawTextBoundary;
    int32_t textVerticalPosition;
    int32_t textHorizontalPosition;
    int32_t textDirection;
    double textOffset;
    int32_t linearUnitFormat;
    int32_t linearPrecision;
    int32_t fractionFormat;
    int32_t decimalSeparator;
    double roundOff;
    YiCadStringView prefix;
    YiCadStringView suffix;
    double measurementScale;
    uint32_t suppressLeadingZeros;
    uint32_t suppressTrailingZeros;
    int32_t angularUnitFormat;
    int32_t angularPrecision;
    uint32_t suppressAngularLeadingZeros;
    uint32_t suppressAngularTrailingZeros;
    uint64_t unsupportedFieldMask;
    uint32_t allowUnsupportedFields;
} YiCadDimensionStyleDataV3;

/**
 * @brief 实体公共属性。
 * @note 该结构只在调用期间借用。layer 为空时使用活动图层，lineType 为空时使用
 * ByLayer；visible 为 0 或 1；lineTypeScale 是实体线型比例（DXF 组码 48），
 * 必须大于 0；当前二维模型要求 normal 为正 Z 轴。宿主导出时给出实体自身的
 * 线型比例。
 * @note 所有实体输入中的 attributes 为空时统一使用活动图层、ByLayer 线型和颜色、
 * 标准线宽 -1、线型比例 1、可见以及法向量 (0,0,1)。
 */
typedef struct YiCadEntityAttributes
{
    uint32_t structSize;
    YiCadImportResourceHandle layer;
    YiCadImportResourceHandle lineType;
    YiCadColorData color;
    int32_t lineWidth;
    double lineTypeScale;
    uint32_t visible;
    YiCadVector3d normal;
} YiCadEntityAttributes;

/** @brief 点实体；position 使用 WCS。 */
typedef struct YiCadPointDataV3
{
    uint32_t structSize;
    const YiCadEntityAttributes* attributes;
    YiCadPoint2d position;
} YiCadPointDataV3;

/** @brief 线段实体；startPoint 和 endPoint 使用 WCS。 */
typedef struct YiCadLineDataV3
{
    uint32_t structSize;
    const YiCadEntityAttributes* attributes;
    YiCadPoint2d startPoint;
    YiCadPoint2d endPoint;
} YiCadLineDataV3;

/** @brief 射线实体；basePoint 使用 WCS，direction 为非零 WCS 向量。 */
typedef struct YiCadRayDataV3
{
    uint32_t structSize;
    const YiCadEntityAttributes* attributes;
    YiCadPoint2d basePoint;
    YiCadVector2d direction;
} YiCadRayDataV3;

/** @brief 无限长线实体；basePoint 使用 WCS，direction 为非零 WCS 向量。 */
typedef struct YiCadXLineDataV3
{
    uint32_t structSize;
    const YiCadEntityAttributes* attributes;
    YiCadPoint2d basePoint;
    YiCadVector2d direction;
} YiCadXLineDataV3;

/** @brief 圆弧实体；center 使用 WCS，角度为弧度。 */
typedef struct YiCadArcDataV3
{
    uint32_t structSize;
    const YiCadEntityAttributes* attributes;
    YiCadPoint2d center;
    double radius;
    double startAngle;
    double endAngle;
} YiCadArcDataV3;

/** @brief 圆实体；center 使用 WCS。 */
typedef struct YiCadCircleDataV3
{
    uint32_t structSize;
    const YiCadEntityAttributes* attributes;
    YiCadPoint2d center;
    double radius;
} YiCadCircleDataV3;

/** @brief 椭圆或椭圆弧；center 和 majorAxis 使用 WCS，参数为弧度。 */
typedef struct YiCadEllipseDataV3
{
    uint32_t structSize;
    const YiCadEntityAttributes* attributes;
    YiCadPoint2d center;
    YiCadVector2d majorAxis;
    double minorToMajorRatio;
    double startParameter;
    double endParameter;
    uint32_t closed;
} YiCadEllipseDataV3;

/** @brief 二维多段线顶点；位置使用 WCS，宽度必须非负。 */
typedef struct YiCadVertex2d
{
    YiCadPoint2d position;
    double startWidth;
    double endWidth;
    double bulge;
} YiCadVertex2d;

/** @brief 只读二维多段线顶点数组视图。 */
typedef struct YiCadVertex2dArrayView
{
    const YiCadVertex2d* data;
    uint32_t count;
} YiCadVertex2dArrayView;

/** @brief 二维多段线；顶点位置使用 WCS。 */
typedef struct YiCadPolylineDataV3
{
    uint32_t structSize;
    const YiCadEntityAttributes* attributes;
    YiCadVertex2dArrayView vertices;
    uint32_t closed;
} YiCadPolylineDataV3;

typedef int32_t YiCadSplineDefinition;
#define YICAD_SPLINE_CONTROL_POINTS ((YiCadSplineDefinition)0)
#define YICAD_SPLINE_FIT_POINTS ((YiCadSplineDefinition)1)

/**
 * @brief 非有理、非周期 B 样条；点使用 WCS。
 * @note 控制点定义要求节点数等于控制点数加 degree 加一。
 */
typedef struct YiCadSplineDataV3
{
    uint32_t structSize;
    const YiCadEntityAttributes* attributes;
    YiCadSplineDefinition definition;
    uint32_t degree;
    uint32_t closed;
    uint32_t rational;
    uint32_t periodic;
    YiCadPoint2dArrayView controlPoints;
    YiCadDoubleArrayView knots;
    YiCadDoubleArrayView weights;
    YiCadPoint2dArrayView fitPoints;
} YiCadSplineDataV3;

/** @brief 三点或四点二维实体填充；corners 按边界顺序排列。 */
typedef struct YiCadSolidDataV3
{
    uint32_t structSize;
    const YiCadEntityAttributes* attributes;
    YiCadPoint2d corners[4];
    uint32_t cornerCount;
} YiCadSolidDataV3;

typedef int32_t YiCadTextHorizontalAlignment;
#define YICAD_TEXT_ALIGN_LEFT ((YiCadTextHorizontalAlignment)0)
#define YICAD_TEXT_ALIGN_CENTER ((YiCadTextHorizontalAlignment)1)
#define YICAD_TEXT_ALIGN_RIGHT ((YiCadTextHorizontalAlignment)2)
#define YICAD_TEXT_ALIGN_ALIGNED ((YiCadTextHorizontalAlignment)3)
#define YICAD_TEXT_ALIGN_MIDDLE ((YiCadTextHorizontalAlignment)4)
#define YICAD_TEXT_ALIGN_FIT ((YiCadTextHorizontalAlignment)5)

typedef int32_t YiCadTextVerticalAlignment;
#define YICAD_TEXT_ALIGN_BASELINE ((YiCadTextVerticalAlignment)0)
#define YICAD_TEXT_ALIGN_BOTTOM ((YiCadTextVerticalAlignment)1)
#define YICAD_TEXT_ALIGN_VERTICAL_MIDDLE ((YiCadTextVerticalAlignment)2)
#define YICAD_TEXT_ALIGN_TOP ((YiCadTextVerticalAlignment)3)

/** @brief 单行文字；坐标使用当前容器的二维坐标系，角度为弧度。 */
typedef struct YiCadTextDataV3
{
    uint32_t structSize;
    const YiCadEntityAttributes* attributes;
    YiCadStringView text;
    YiCadPoint2d insertionPoint;
    YiCadPoint2d alignmentPoint;
    double height;
    double rotation;
    double widthFactor;
    double obliqueAngle;
    YiCadTextHorizontalAlignment horizontalAlignment;
    YiCadTextVerticalAlignment verticalAlignment;
    YiCadImportResourceHandle textStyle;
} YiCadTextDataV3;

typedef int32_t YiCadMTextAttachment;
#define YICAD_MTEXT_TOP_LEFT ((YiCadMTextAttachment)1)
#define YICAD_MTEXT_TOP_CENTER ((YiCadMTextAttachment)2)
#define YICAD_MTEXT_TOP_RIGHT ((YiCadMTextAttachment)3)
#define YICAD_MTEXT_MIDDLE_LEFT ((YiCadMTextAttachment)4)
#define YICAD_MTEXT_MIDDLE_CENTER ((YiCadMTextAttachment)5)
#define YICAD_MTEXT_MIDDLE_RIGHT ((YiCadMTextAttachment)6)
#define YICAD_MTEXT_BOTTOM_LEFT ((YiCadMTextAttachment)7)
#define YICAD_MTEXT_BOTTOM_CENTER ((YiCadMTextAttachment)8)
#define YICAD_MTEXT_BOTTOM_RIGHT ((YiCadMTextAttachment)9)

/** @brief 多行文字背景填充；非空指针表示启用，比例是文字边界外扩系数。 */
typedef struct YiCadMTextBackgroundData
{
    uint32_t structSize;
    uint32_t useDrawingBackgroundColor;
    YiCadColorData color;
    double borderScaleFactor;
} YiCadMTextBackgroundData;

/** @brief 多行文字；原始 UTF-8 格式串由宿主完整复制并保留。 */
typedef struct YiCadMTextDataV3
{
    uint32_t structSize;
    const YiCadEntityAttributes* attributes;
    YiCadStringView contents;
    YiCadPoint2d insertionPoint;
    YiCadVector2d direction;
    double characterHeight;
    double rectangleWidth;
    double lineSpacingFactor;
    YiCadMTextAttachment attachment;
    YiCadImportResourceHandle textStyle;
    const YiCadMTextBackgroundData* background;
} YiCadMTextDataV3;

#define YICAD_BLOCK_ANONYMOUS UINT32_C(1)
#define YICAD_BLOCK_HAS_ATTRIBUTES UINT32_C(2)
#define YICAD_BLOCK_EXTERNAL_REFERENCE UINT32_C(4)
#define YICAD_BLOCK_EXTERNAL_OVERLAY UINT32_C(8)
#define YICAD_BLOCK_EXTERNALLY_DEPENDENT UINT32_C(16)
#define YICAD_BLOCK_RESOLVED_EXTERNAL_REFERENCE UINT32_C(32)
#define YICAD_BLOCK_REFERENCED_EXTERNAL_REFERENCE UINT32_C(64)

/** @brief 块定义；创建成功后必须调用 endBlock 结束块容器。 */
typedef struct YiCadBlockDataV3
{
    uint32_t structSize;
    YiCadStringView name;
    YiCadPoint2d basePoint;
    uint32_t flags;
    YiCadStringView description;
    YiCadStringView externalReferencePath;
} YiCadBlockDataV3;

/**
 * @brief 块引用；只允许引用本会话中已经完成定义的块。
 * @note Z 轴比例必须为 1；宿主不自动炸开块引用。
 */
typedef struct YiCadInsertDataV3
{
    uint32_t structSize;
    const YiCadEntityAttributes* attributes;
    YiCadImportResourceHandle block;
    YiCadPoint2d insertionPoint;
    YiCadVector3d scale;
    double rotation;
    uint32_t columnCount;
    uint32_t rowCount;
    double columnSpacing;
    double rowSpacing;
} YiCadInsertDataV3;

#define YICAD_ATTRIBUTE_INVISIBLE UINT32_C(1)
#define YICAD_ATTRIBUTE_CONSTANT UINT32_C(2)
#define YICAD_ATTRIBUTE_VERIFY UINT32_C(4)
#define YICAD_ATTRIBUTE_PRESET UINT32_C(8)
#define YICAD_ATTRIBUTE_LOCK_POSITION UINT32_C(16)
#define YICAD_ATTRIBUTE_MULTILINE UINT32_C(32)

/** @brief 属性定义；只能添加到活动块定义容器，text 必须非空。 */
typedef struct YiCadAttributeDefinitionDataV3
{
    uint32_t structSize;
    const YiCadTextDataV3* text;
    YiCadStringView tag;
    YiCadStringView prompt;
    YiCadStringView defaultValue;
    uint32_t flags;
} YiCadAttributeDefinitionDataV3;

/**
 * @brief 块引用属性值；tag 与块内属性定义关联，不依赖数组顺序。
 * @note text 必须非空；insert 必须是 createInsert 返回的句柄，且与 container
 * 属于同一容器。
 */
typedef struct YiCadAttributeDataV3
{
    uint32_t structSize;
    const YiCadTextDataV3* text;
    YiCadImportResourceHandle insert;
    YiCadStringView tag;
    YiCadStringView value;
    uint32_t flags;
} YiCadAttributeDataV3;

typedef int32_t YiCadDimensionKind;
#define YICAD_DIMENSION_LINEAR ((YiCadDimensionKind)0)
#define YICAD_DIMENSION_ALIGNED ((YiCadDimensionKind)1)
#define YICAD_DIMENSION_ANGULAR ((YiCadDimensionKind)2)
#define YICAD_DIMENSION_RADIAL ((YiCadDimensionKind)3)
#define YICAD_DIMENSION_DIAMETRIC ((YiCadDimensionKind)4)
#define YICAD_DIMENSION_ORDINATE ((YiCadDimensionKind)5)

/**
 * @brief 二维语义标注；所有点使用当前容器坐标，角度为弧度。
 * @note 线性/对齐标注使用 extensionPoint1、extensionPoint2 和
 * definitionPoint；角度标注使用两组 line 点和 arcPoint；半径标注使用
 * definitionPoint 作为圆心、featurePoint 作为箭头点；直径标注使用这两点
 * 作为两个箭头点。坐标标注当前明确返回不支持。
 */
typedef struct YiCadDimensionDataV3
{
    uint32_t structSize;
    const YiCadEntityAttributes* attributes;
    YiCadDimensionKind kind;
    YiCadImportResourceHandle dimensionStyle;
    YiCadStringView textOverride;
    YiCadPoint2d definitionPoint;
    YiCadPoint2d textPosition;
    double textRotation;
    double lineSpacingFactor;
    YiCadPoint2d extensionPoint1;
    YiCadPoint2d extensionPoint2;
    YiCadPoint2d line1Start;
    YiCadPoint2d line1End;
    YiCadPoint2d line2Start;
    YiCadPoint2d line2End;
    YiCadPoint2d arcPoint;
    YiCadPoint2d featurePoint;
    double leaderLength;
} YiCadDimensionDataV3;

/**
 * @brief 引线；顶点使用当前容器坐标。
 * @note text 非空时，宿主在同一导入事务中创建原生文字实体；空指针表示无文字。
 * 插件负责提供文字的完整属性。YiCAD 当前以两个关联的原生实体表达引线及其文字。
 */
typedef struct YiCadLeaderDataV3
{
    uint32_t structSize;
    const YiCadEntityAttributes* attributes;
    YiCadPoint2dArrayView vertices;
    uint32_t hasArrow;
    YiCadImportResourceHandle dimensionStyle;
    const YiCadTextDataV3* text;
} YiCadLeaderDataV3;

typedef int32_t YiCadHatchEdgeType;
#define YICAD_HATCH_EDGE_LINE ((YiCadHatchEdgeType)0)
#define YICAD_HATCH_EDGE_CIRCULAR_ARC ((YiCadHatchEdgeType)1)
#define YICAD_HATCH_EDGE_ELLIPTIC_ARC ((YiCadHatchEdgeType)2)
#define YICAD_HATCH_EDGE_SPLINE ((YiCadHatchEdgeType)3)

/**
 * @brief 填充边界边；按 type 读取对应字段，未使用字段必须置零。
 * @note 圆弧和椭圆弧角度为弧度；样条仅支持非有理、非周期控制点定义。
 */
typedef struct YiCadHatchEdgeDataV3
{
    uint32_t structSize;
    YiCadHatchEdgeType type;
    YiCadPoint2d startPoint;
    YiCadPoint2d endPoint;
    YiCadPoint2d center;
    YiCadVector2d majorAxis;
    double radius;
    double minorToMajorRatio;
    double startParameter;
    double endParameter;
    uint32_t counterClockwise;
    uint32_t degree;
    uint32_t rational;
    uint32_t periodic;
    YiCadPoint2dArrayView controlPoints;
    YiCadDoubleArrayView knots;
    YiCadDoubleArrayView weights;
} YiCadHatchEdgeDataV3;

/**
 * @brief 只读填充边界边数组；宿主在调用返回前复制数据。
 * @note 非空数组的 byteStride 必须至少覆盖当前版本的必需前缀，
 * data 和每个元素均必须按 YiCadHatchEdgeDataV3 对齐。
 */
typedef struct YiCadHatchEdgeArrayView
{
    const YiCadHatchEdgeDataV3* data;
    uint32_t count;
    uint32_t byteStride;
} YiCadHatchEdgeArrayView;

typedef int32_t YiCadHatchLoopKind;
#define YICAD_HATCH_LOOP_POLYLINE ((YiCadHatchLoopKind)0)
#define YICAD_HATCH_LOOP_EDGES ((YiCadHatchLoopKind)1)

typedef int32_t YiCadHatchLoopRole;
#define YICAD_HATCH_LOOP_OUTER ((YiCadHatchLoopRole)0)
#define YICAD_HATCH_LOOP_HOLE ((YiCadHatchLoopRole)1)

/**
 * @brief 闭合填充环。
 * @note 孔环的 outerLoopIndex 必须引用同一数组中更早的外环；外环的该字段
 * 必须为 UINT32_MAX。折线环必须至少三个顶点并且由 closed 隐式闭合。
 */
typedef struct YiCadHatchLoopDataV3
{
    uint32_t structSize;
    YiCadHatchLoopKind kind;
    YiCadHatchLoopRole role;
    uint32_t outerLoopIndex;
    YiCadVertex2dArrayView polylineVertices;
    YiCadHatchEdgeArrayView edges;
} YiCadHatchLoopDataV3;

/**
 * @brief 只读填充环数组；宿主在调用返回前复制数据。
 * @note 非空数组的 byteStride 必须至少覆盖当前版本的必需前缀，
 * data 和每个元素均必须按 YiCadHatchLoopDataV3 对齐。
 */
typedef struct YiCadHatchLoopArrayView
{
    const YiCadHatchLoopDataV3* data;
    uint32_t count;
    uint32_t byteStride;
} YiCadHatchLoopArrayView;

/** @brief 实体填充或图案填充；支持多外环及各自孔环。 */
typedef struct YiCadHatchDataV3
{
    uint32_t structSize;
    const YiCadEntityAttributes* attributes;
    uint32_t solid;
    YiCadStringView patternName;
    double patternScale;
    double patternAngle;
    YiCadHatchLoopArrayView loops;
} YiCadHatchDataV3;

/**
 * @brief 外部光栅图像引用；U/V 是每像素对应的当前容器坐标向量。
 * @note size 是像素宽高。clipBoundary 非空时明确返回不支持；文件缺失仍保留
 * 原始 UTF-8 路径，并通过 getLastError 提供诊断。
 */
typedef struct YiCadImageDataV3
{
    uint32_t structSize;
    const YiCadEntityAttributes* attributes;
    YiCadStringView path;
    YiCadPoint2d insertionPoint;
    YiCadVector2d uVector;
    YiCadVector2d vVector;
    YiCadVector2d size;
    int32_t brightness;
    int32_t contrast;
    int32_t fade;
    YiCadPoint2dArrayView clipBoundary;
} YiCadImageDataV3;

/**
 * @brief ABI v3 可扩展输入的冻结最小必需前缀。
 * @note 最小值只覆盖当前版本最后一个必需字段，不等同于接收方的 sizeof(T)。
 * 调用方可以提供更大的结构；宿主必须忽略未知尾字段。未来新增可选尾字段时，
 * 必须另行记录字段缺失时的默认值，并保持下列已有前缀不变。
 */
#define YICAD_ABI_STRUCT_FIELD_END(type, field) \
    (offsetof(type, field) + sizeof(((type*)0)->field))

#define YICAD_DOCUMENT_SETTINGS_V3_MIN_SIZE ((uint32_t) \
    YICAD_ABI_STRUCT_FIELD_END(YiCadDocumentSettings, sourceCodePage))
#define YICAD_LINE_TYPE_DATA_V3_MIN_SIZE ((uint32_t) \
    YICAD_ABI_STRUCT_FIELD_END(YiCadLineTypeDataV3, complex))
#define YICAD_LAYER_DATA_V3_MIN_SIZE ((uint32_t) \
    YICAD_ABI_STRUCT_FIELD_END(YiCadLayerDataV3, lineWidth))
#define YICAD_TEXT_STYLE_DATA_V3_MIN_SIZE ((uint32_t) \
    YICAD_ABI_STRUCT_FIELD_END(YiCadTextStyleDataV3, generationFlags))
#define YICAD_DIMENSION_STYLE_DATA_V3_MIN_SIZE ((uint32_t) \
    YICAD_ABI_STRUCT_FIELD_END( \
        YiCadDimensionStyleDataV3, allowUnsupportedFields))
#define YICAD_ENTITY_ATTRIBUTES_V3_MIN_SIZE ((uint32_t) \
    YICAD_ABI_STRUCT_FIELD_END(YiCadEntityAttributes, normal))
#define YICAD_POINT_DATA_V3_MIN_SIZE ((uint32_t) \
    YICAD_ABI_STRUCT_FIELD_END(YiCadPointDataV3, position))
#define YICAD_LINE_DATA_V3_MIN_SIZE ((uint32_t) \
    YICAD_ABI_STRUCT_FIELD_END(YiCadLineDataV3, endPoint))
#define YICAD_RAY_DATA_V3_MIN_SIZE ((uint32_t) \
    YICAD_ABI_STRUCT_FIELD_END(YiCadRayDataV3, direction))
#define YICAD_XLINE_DATA_V3_MIN_SIZE ((uint32_t) \
    YICAD_ABI_STRUCT_FIELD_END(YiCadXLineDataV3, direction))
#define YICAD_ARC_DATA_V3_MIN_SIZE ((uint32_t) \
    YICAD_ABI_STRUCT_FIELD_END(YiCadArcDataV3, endAngle))
#define YICAD_CIRCLE_DATA_V3_MIN_SIZE ((uint32_t) \
    YICAD_ABI_STRUCT_FIELD_END(YiCadCircleDataV3, radius))
#define YICAD_ELLIPSE_DATA_V3_MIN_SIZE ((uint32_t) \
    YICAD_ABI_STRUCT_FIELD_END(YiCadEllipseDataV3, closed))
#define YICAD_POLYLINE_DATA_V3_MIN_SIZE ((uint32_t) \
    YICAD_ABI_STRUCT_FIELD_END(YiCadPolylineDataV3, closed))
#define YICAD_SPLINE_DATA_V3_MIN_SIZE ((uint32_t) \
    YICAD_ABI_STRUCT_FIELD_END(YiCadSplineDataV3, fitPoints))
#define YICAD_SOLID_DATA_V3_MIN_SIZE ((uint32_t) \
    YICAD_ABI_STRUCT_FIELD_END(YiCadSolidDataV3, cornerCount))
#define YICAD_TEXT_DATA_V3_MIN_SIZE ((uint32_t) \
    YICAD_ABI_STRUCT_FIELD_END(YiCadTextDataV3, textStyle))
#define YICAD_MTEXT_BACKGROUND_DATA_V3_MIN_SIZE ((uint32_t) \
    YICAD_ABI_STRUCT_FIELD_END( \
        YiCadMTextBackgroundData, borderScaleFactor))
#define YICAD_MTEXT_DATA_V3_MIN_SIZE ((uint32_t) \
    YICAD_ABI_STRUCT_FIELD_END(YiCadMTextDataV3, background))
#define YICAD_BLOCK_DATA_V3_MIN_SIZE ((uint32_t) \
    YICAD_ABI_STRUCT_FIELD_END( \
        YiCadBlockDataV3, externalReferencePath))
#define YICAD_INSERT_DATA_V3_MIN_SIZE ((uint32_t) \
    YICAD_ABI_STRUCT_FIELD_END(YiCadInsertDataV3, rowSpacing))
#define YICAD_ATTRIBUTE_DEFINITION_DATA_V3_MIN_SIZE ((uint32_t) \
    YICAD_ABI_STRUCT_FIELD_END(YiCadAttributeDefinitionDataV3, flags))
#define YICAD_ATTRIBUTE_DATA_V3_MIN_SIZE ((uint32_t) \
    YICAD_ABI_STRUCT_FIELD_END(YiCadAttributeDataV3, flags))
#define YICAD_DIMENSION_DATA_V3_MIN_SIZE ((uint32_t) \
    YICAD_ABI_STRUCT_FIELD_END(YiCadDimensionDataV3, leaderLength))
#define YICAD_LEADER_DATA_V3_MIN_SIZE ((uint32_t) \
    YICAD_ABI_STRUCT_FIELD_END(YiCadLeaderDataV3, text))
#define YICAD_HATCH_EDGE_DATA_V3_MIN_SIZE ((uint32_t) \
    YICAD_ABI_STRUCT_FIELD_END(YiCadHatchEdgeDataV3, weights))
#define YICAD_HATCH_LOOP_DATA_V3_MIN_SIZE ((uint32_t) \
    YICAD_ABI_STRUCT_FIELD_END(YiCadHatchLoopDataV3, edges))
#define YICAD_HATCH_DATA_V3_MIN_SIZE ((uint32_t) \
    YICAD_ABI_STRUCT_FIELD_END(YiCadHatchDataV3, loops))
#define YICAD_IMAGE_DATA_V3_MIN_SIZE ((uint32_t) \
    YICAD_ABI_STRUCT_FIELD_END(YiCadImageDataV3, clipBoundary))

typedef int32_t YiCadEntityType;

#define YICAD_ENTITY_UNKNOWN ((YiCadEntityType)0)
#define YICAD_ENTITY_POINT ((YiCadEntityType)1)
#define YICAD_ENTITY_LINE ((YiCadEntityType)2)
#define YICAD_ENTITY_RAY ((YiCadEntityType)3)
#define YICAD_ENTITY_XLINE ((YiCadEntityType)4)
#define YICAD_ENTITY_ARC ((YiCadEntityType)5)
#define YICAD_ENTITY_CIRCLE ((YiCadEntityType)6)
#define YICAD_ENTITY_ELLIPSE ((YiCadEntityType)7)
#define YICAD_ENTITY_POLYLINE ((YiCadEntityType)8)
#define YICAD_ENTITY_SPLINE ((YiCadEntityType)9)
#define YICAD_ENTITY_SOLID ((YiCadEntityType)10)
#define YICAD_ENTITY_TEXT ((YiCadEntityType)11)
#define YICAD_ENTITY_MTEXT ((YiCadEntityType)12)
#define YICAD_ENTITY_DIMENSION ((YiCadEntityType)13)
#define YICAD_ENTITY_LEADER ((YiCadEntityType)14)
#define YICAD_ENTITY_HATCH ((YiCadEntityType)15)
#define YICAD_ENTITY_INSERT ((YiCadEntityType)16)
#define YICAD_ENTITY_ATTRIBUTE_DEFINITION ((YiCadEntityType)17)
#define YICAD_ENTITY_ATTRIBUTE ((YiCadEntityType)18)
#define YICAD_ENTITY_IMAGE ((YiCadEntityType)19)
/** @brief v4：自定义实体（插件的、进程内扩展的与代理），数据为 YiCadCustomEntityDataV4。 */
#define YICAD_ENTITY_CUSTOM ((YiCadEntityType)20)

typedef int32_t YiCadReadResourceKind;
#define YICAD_READ_LINE_TYPE ((YiCadReadResourceKind)1)
#define YICAD_READ_LAYER ((YiCadReadResourceKind)2)
#define YICAD_READ_TEXT_STYLE ((YiCadReadResourceKind)3)
#define YICAD_READ_DIMENSION_STYLE ((YiCadReadResourceKind)4)
/** @brief v4：块定义；只用于 GI 的 findResource（只读枚举的块另有 blockCount、blockAt）。 */
#define YICAD_READ_BLOCK ((YiCadReadResourceKind)5)

/* ==========================================================================
 * ABI v4：插件自定义实体（RENDER_PLAN.md 第 4.8.3 节）
 *
 * 实体的数据是宿主保管的一段字节，编码由插件自己定义；插件提供的是一组纯函数，输入都是这段字节
 * （YiCadEntityClassV4）。撤销、存盘、读盘、插件缺失时的代理显示都由宿主处理，插件不参与。
 * 插件画实体时经宿主的 GI 表（YiCadGiApiV4）输出图元。
 * ========================================================================== */

/** @brief 宿主持有的文档实体引用；所属文档打开且实体仍在文档里时有效，插件不得释放。 */
typedef void* YiCadEntityHandle;
/** @brief 宿主持有的 GI 上下文，只在一次 worldDraw 调用期间有效。 */
typedef void* YiCadGiContextHandle;

/** @brief 只读字节视图；在调用期间借用。 */
typedef struct YiCadByteView
{
    const uint8_t* data;
    uint32_t size;
} YiCadByteView;

/**
 * @brief 宿主提供的字节输出：插件经 write 追加字节，可以调用多次，结果是各次的拼接。
 * @note 只在交出它的那次调用期间有效。
 */
typedef struct YiCadByteSink
{
    uint32_t structSize;
    void* context;
    YiCadResult (YICAD_PLUGIN_CALL* write)(
        void* context, const uint8_t* data, uint32_t size);
} YiCadByteSink;

/**
 * @brief 宿主提供的点输出：插件经 add 追加点，可以调用多次。
 * @note 只在交出它的那次调用期间有效。
 */
typedef struct YiCadPointSink
{
    uint32_t structSize;
    void* context;
    YiCadResult (YICAD_PLUGIN_CALL* add)(
        void* context, const YiCadPoint2d* points, uint32_t count);
} YiCadPointSink;

/** @brief 二维仿射变换：x' = a·x + c·y + tx，y' = b·x + d·y + ty。 */
typedef struct YiCadMatrix2d
{
    double a;
    double b;
    double c;
    double d;
    double tx;
    double ty;
} YiCadMatrix2d;

/** @brief 包围框。 */
typedef struct YiCadExtents2d
{
    YiCadPoint2d minPoint;
    YiCadPoint2d maxPoint;
} YiCadExtents2d;

/** @brief GI 多段线的标志：闭合。 */
#define YICAD_GI_POLYLINE_CLOSED UINT32_C(1)
/** @brief GI 多段线的标志：线型生成（整条连续计算线型，顶点处不重新对齐）。 */
#define YICAD_GI_POLYLINE_CONTINUOUS_LINETYPE UINT32_C(2)
/** @brief GI 填充规则：奇偶。 */
#define YICAD_GI_FILL_EVEN_ODD UINT32_C(0)
/** @brief GI 填充规则：非零环绕数。 */
#define YICAD_GI_FILL_NON_ZERO UINT32_C(1)

/** @brief GI 填充的一个闭合环：顶点与每段的凸度，最后一点连回第一点；bulges 为空或与 points 等长。 */
typedef struct YiCadGiLoopV4
{
    YiCadPoint2dArrayView points;
    YiCadDoubleArrayView bulges;
} YiCadGiLoopV4;

/**
 * @brief 填充图案的一族平行线（同 AutoCAD HATCH 的组码 53/43/44/45/46/49）。
 * @note basePoint 是一条线经过的点；direction 是线的方向，非零；offset 是到下一条线的位移，
 * 不平行于 direction；dashes 正数划线、负数空白、0 是点，为空是实线。都在当前坐标里。
 */
typedef struct YiCadHatchPatternLineV4
{
    YiCadPoint2d basePoint;
    YiCadVector2d direction;
    YiCadVector2d offset;
    YiCadDoubleArrayView dashes;
} YiCadHatchPatternLineV4;

/**
 * @brief GI 文字的位置与外观；含义同 YiCadTextDataV3 的对应字段，角度为弧度。
 * @note widthFactor 为 0 时取文字样式的宽度系数，不得小于 0。
 */
typedef struct YiCadTextPlacementV4
{
    uint32_t structSize;
    YiCadPoint2d insertionPoint;
    YiCadPoint2d alignmentPoint;
    double height;
    double rotation;
    double widthFactor;
    double obliqueAngle;
    YiCadTextHorizontalAlignment horizontalAlignment;
    YiCadTextVerticalAlignment verticalAlignment;
} YiCadTextPlacementV4;

/**
 * @brief 宿主提供的 GI 表：worldDraw 经它输出图元，与 IGiGeometry、IGiSubEntityTraits 一一对应。
 * @note 坐标一律 double，在当前模型变换下；数组只在调用期间借用。属性设置后对之后的图元生效，
 * 初值为实体自己的属性。参数无效的调用返回 YICAD_FAILURE 并被忽略；单次 worldDraw 输出的点与图元
 * 超过宿主的上限（100 万）时，之后的图元被丢弃并记日志。
 * @note 资源句柄（线型、图层、文字样式、块）由 findResource 按名字在实体所属文档里找，
 * 只在本次 worldDraw 期间使用；线型为空表示随块，"ByLayer"、"ByBlock" 是两条保留记录。
 */
typedef struct YiCadGiApiV4
{
    uint32_t structSize;
    uint32_t abiVersion;
    YiCadResult (YICAD_PLUGIN_CALL* setColor)(
        YiCadGiContextHandle ctx, const YiCadColorData* color);
    YiCadResult (YICAD_PLUGIN_CALL* setLayer)(
        YiCadGiContextHandle ctx, YiCadReadResourceHandle layer);
    YiCadResult (YICAD_PLUGIN_CALL* setLineType)(
        YiCadGiContextHandle ctx, YiCadReadResourceHandle lineType);
    YiCadResult (YICAD_PLUGIN_CALL* setLineTypeScale)(
        YiCadGiContextHandle ctx, double scale);
    YiCadResult (YICAD_PLUGIN_CALL* setLineWeight)(
        YiCadGiContextHandle ctx, int32_t weight);
    /** @brief 透明度 0..255，255 为不透明。 */
    YiCadResult (YICAD_PLUGIN_CALL* setTransparency)(
        YiCadGiContextHandle ctx, uint32_t alpha);
    YiCadResult (YICAD_PLUGIN_CALL* setSelectionMarker)(
        YiCadGiContextHandle ctx, int32_t marker);
    /** @brief 之后的 fill 按这些图案线填；count 为 0 时恢复实心。 */
    YiCadResult (YICAD_PLUGIN_CALL* setFillPattern)(
        YiCadGiContextHandle ctx, const YiCadHatchPatternLineV4* lines,
        uint32_t count);
    /**
     * @brief 多段线；bulges 可为空，否则每段一个（闭合时与点数相等，否则少一个）；
     * widths 可为空，否则每段两个（起止宽度）。
     */
    YiCadResult (YICAD_PLUGIN_CALL* polyline)(
        YiCadGiContextHandle ctx, const YiCadPoint2dArrayView* points,
        const YiCadDoubleArrayView* bulges, const YiCadDoubleArrayView* widths,
        uint32_t flags);
    YiCadResult (YICAD_PLUGIN_CALL* circle)(
        YiCadGiContextHandle ctx, YiCadPoint2d center, double radius);
    /** @brief 圆弧，从 startAngle 起转过 sweepAngle（正为逆时针）。 */
    YiCadResult (YICAD_PLUGIN_CALL* arc)(
        YiCadGiContextHandle ctx, YiCadPoint2d center, double radius,
        double startAngle, double sweepAngle);
    YiCadResult (YICAD_PLUGIN_CALL* ellipseArc)(
        YiCadGiContextHandle ctx, YiCadPoint2d center, YiCadVector2d majorAxis,
        double ratio, double startParameter, double endParameter);
    /** @brief 非有理 B 样条：节点数等于控制点数加 degree 加一。 */
    YiCadResult (YICAD_PLUGIN_CALL* nurbs)(
        YiCadGiContextHandle ctx, uint32_t degree,
        const YiCadPoint2dArrayView* controlPoints,
        const YiCadDoubleArrayView* knots, uint32_t closed);
    YiCadResult (YICAD_PLUGIN_CALL* fill)(
        YiCadGiContextHandle ctx, const YiCadGiLoopV4* loops, uint32_t loopCount,
        uint32_t fillRule);
    /** @brief 三角形；indices 每 3 个一组。 */
    YiCadResult (YICAD_PLUGIN_CALL* triangles)(
        YiCadGiContextHandle ctx, const YiCadPoint2dArrayView* vertices,
        const uint32_t* indices, uint32_t indexCount);
    /** @brief 单行文字，由宿主按文字样式排版；textStyle 为空时用 Standard。 */
    YiCadResult (YICAD_PLUGIN_CALL* text)(
        YiCadGiContextHandle ctx, YiCadStringView text,
        YiCadReadResourceHandle textStyle, const YiCadTextPlacementV4* placement);
    /** @brief 来自文件的光栅图像：origin 是左下角，u、v 是整条宽度边与高度边。 */
    YiCadResult (YICAD_PLUGIN_CALL* image)(
        YiCadGiContextHandle ctx, YiCadStringView path, YiCadPoint2d origin,
        YiCadVector2d u, YiCadVector2d v, uint32_t widthPixels,
        uint32_t heightPixels);
    YiCadResult (YICAD_PLUGIN_CALL* point)(
        YiCadGiContextHandle ctx, YiCadPoint2d position);
    YiCadResult (YICAD_PLUGIN_CALL* ray)(
        YiCadGiContextHandle ctx, YiCadPoint2d base, YiCadVector2d direction);
    YiCadResult (YICAD_PLUGIN_CALL* xline)(
        YiCadGiContextHandle ctx, YiCadPoint2d base, YiCadVector2d direction);
    /** @brief 画一个块定义（共享几何）；块里的随块属性取当前属性。 */
    YiCadResult (YICAD_PLUGIN_CALL* drawBlock)(
        YiCadGiContextHandle ctx, YiCadReadResourceHandle block,
        const YiCadMatrix2d* transform);
    YiCadResult (YICAD_PLUGIN_CALL* pushTransform)(
        YiCadGiContextHandle ctx, const YiCadMatrix2d* transform);
    YiCadResult (YICAD_PLUGIN_CALL* popTransform)(YiCadGiContextHandle ctx);
    /** @brief 非空：之后的图元以像素为单位、锚定在该点；为空：恢复世界单位。 */
    YiCadResult (YICAD_PLUGIN_CALL* setScreenSpace)(
        YiCadGiContextHandle ctx, const YiCadPoint2d* anchor);
    /**
     * @brief 按名字在实体所属文档里找资源；kind 为线型、图层、文字样式或 YICAD_READ_BLOCK。
     * @return 找不到时返回空。
     */
    YiCadReadResourceHandle (YICAD_PLUGIN_CALL* findResource)(
        YiCadGiContextHandle ctx, YiCadReadResourceKind kind,
        YiCadStringView name);
} YiCadGiApiV4;

/* 代理权限位：插件不在、实体读成代理时允许的操作；与 DXF CLASSES 段组码 90、
 * ODA 的 OdDbProxyEntity::ProxyFlags 相同。 */
#define YICAD_PROXY_ERASE UINT32_C(0x001)
#define YICAD_PROXY_TRANSFORM UINT32_C(0x002)
#define YICAD_PROXY_COLOR_CHANGE UINT32_C(0x004)
#define YICAD_PROXY_LAYER_CHANGE UINT32_C(0x008)
#define YICAD_PROXY_LINETYPE_CHANGE UINT32_C(0x010)
#define YICAD_PROXY_LINETYPE_SCALE_CHANGE UINT32_C(0x020)
#define YICAD_PROXY_VISIBILITY_CHANGE UINT32_C(0x040)
#define YICAD_PROXY_CLONING UINT32_C(0x080)
#define YICAD_PROXY_LINEWEIGHT_CHANGE UINT32_C(0x100)
#define YICAD_PROXY_ALL UINT32_C(0x1FF)

/** @brief 实体类标志：worldDraw 线程安全，可以在图形系统的工作线程上与别的调用并行。 */
#define YICAD_ENTITY_CLASS_THREAD_SAFE_DRAW UINT32_C(1)

/* getSnapPoints 的捕捉种类。 */
#define YICAD_SNAP_ENDPOINT UINT32_C(1)
#define YICAD_SNAP_MIDPOINT UINT32_C(2)
#define YICAD_SNAP_CENTER UINT32_C(3)
#define YICAD_SNAP_NEAREST UINT32_C(4)

/**
 * @brief 插件提供的一个实体类。
 * @note className 为 "pluginId.类名"；classVersion 是数据编码的版本，读回的数据版本低于它时调用
 * upgrade，没提供 upgrade 或数据比它新时实体读成代理；proxyFlags 只能组合 YICAD_PROXY_*；
 * flags 只能组合 YICAD_ENTITY_CLASS_*。
 * @note worldDraw、getExtents、transform 必须提供，其余可为空：没有夹点只能整体移动，捕捉与炸开
 * 由宿主按 worldDraw 的图元推导；createCache 与 destroyCache 同时提供或同时为空。
 * @note 函数都是纯函数：输入是一段数据字节与实例缓存（createCache 为这段字节建的，没有时为空），
 * 改动的结果写进宿主给的输出，不得保存输入的指针。返回 YICAD_FAILURE 表示失败，宿主不采用输出。
 * 除声明 YICAD_ENTITY_CLASS_THREAD_SAFE_DRAW 的类的 worldDraw 外，都只在 UI 线程调用；
 * 宿主在插件 shutdown 前收回全部实例缓存。
 * @note explode 在宿主开的临时导入会话里用导入函数建基本实体（容器为 container）；这个会话不能
 * 建资源与块，实体属性为空时取被炸开的实体的图层与画笔，图层为空时取它的图层。
 */
typedef struct YiCadEntityClassV4
{
    uint32_t structSize;
    uint32_t abiVersion;
    YiCadStringView className;
    uint32_t classVersion;
    uint32_t proxyFlags;
    uint32_t flags;
    void* userData;
    YiCadResult (YICAD_PLUGIN_CALL* worldDraw)(
        void* userData, YiCadByteView data, void* cache,
        const YiCadGiApiV4* gi, YiCadGiContextHandle ctx);
    YiCadResult (YICAD_PLUGIN_CALL* getExtents)(
        void* userData, YiCadByteView data, void* cache,
        YiCadExtents2d* extents);
    YiCadResult (YICAD_PLUGIN_CALL* transform)(
        void* userData, YiCadByteView data, void* cache,
        const YiCadMatrix2d* matrix, const YiCadByteSink* out);
    YiCadResult (YICAD_PLUGIN_CALL* getGrips)(
        void* userData, YiCadByteView data, void* cache,
        const YiCadPointSink* out);
    YiCadResult (YICAD_PLUGIN_CALL* moveGrips)(
        void* userData, YiCadByteView data, void* cache,
        const uint32_t* indices, uint32_t count, YiCadVector2d offset,
        const YiCadByteSink* out);
    YiCadResult (YICAD_PLUGIN_CALL* getSnapPoints)(
        void* userData, YiCadByteView data, void* cache, uint32_t snapMode,
        YiCadPoint2d pick, const YiCadPointSink* out);
    YiCadResult (YICAD_PLUGIN_CALL* explode)(
        void* userData, YiCadByteView data, void* cache,
        YiCadImportSessionHandle session, YiCadImportContainerHandle container);
    YiCadResult (YICAD_PLUGIN_CALL* upgrade)(
        void* userData, uint32_t fromVersion, YiCadByteView data,
        const YiCadByteSink* out);
    YiCadResult (YICAD_PLUGIN_CALL* createCache)(
        void* userData, YiCadByteView data, void** cache);
    void (YICAD_PLUGIN_CALL* destroyCache)(void* userData, void* cache);
} YiCadEntityClassV4;

/**
 * @brief 自定义实体：导入（importApi->createCustomEntity）、事务内新建（entityApi）与只读枚举共用。
 * @note 输入：attributes 同其他实体；className 为类名；classVersion 是 data 的编码版本；类登记了且
 * 读得了 data 时建原实体，否则建代理实体，保留类名、版本与字节，按 proxyFlags 放行操作、按
 * proxyGraphics（beginProxyGraphics 的收集容器，可为空）里的实体显示。transform 是代理累计的变换
 * （原实体读回后按它改动），一般为恒等。isProxy、entity 只用于输出，输入时置零。
 * @note 输出（只读枚举）：全部字段由宿主填写，字节与字符串保持到同一子表的下一次调用；
 * entity 是可交给 setCustomEntityData 的实体句柄。
 */
typedef struct YiCadCustomEntityDataV4
{
    uint32_t structSize;
    const YiCadEntityAttributes* attributes;
    YiCadStringView className;
    uint32_t classVersion;
    uint32_t proxyFlags;
    YiCadByteView data;
    YiCadMatrix2d transform;
    YiCadImportContainerHandle proxyGraphics;
    uint32_t isProxy;
    YiCadEntityHandle entity;
} YiCadCustomEntityDataV4;

/** @brief v4 输入结构的最小必需前缀，约定同 v3（见 YICAD_ABI_STRUCT_FIELD_END）。 */
#define YICAD_TEXT_PLACEMENT_V4_MIN_SIZE ((uint32_t) \
    YICAD_ABI_STRUCT_FIELD_END(YiCadTextPlacementV4, verticalAlignment))
#define YICAD_ENTITY_CLASS_V4_MIN_SIZE ((uint32_t) \
    YICAD_ABI_STRUCT_FIELD_END(YiCadEntityClassV4, destroyCache))
#define YICAD_CUSTOM_ENTITY_DATA_V4_MIN_SIZE ((uint32_t) \
    YICAD_ABI_STRUCT_FIELD_END(YiCadCustomEntityDataV4, proxyGraphics))
#define YICAD_DOCUMENT_SETTINGS_V4_MIN_SIZE ((uint32_t) \
    YICAD_ABI_STRUCT_FIELD_END(YiCadDocumentSettings, currentEntityLineTypeScale))

typedef struct YiCadLineData
{
    double x1;
    double y1;
    double x2;
    double y2;
} YiCadLineData;

typedef struct YiCadCircleData
{
    double centerX;
    double centerY;
    double radius;
} YiCadCircleData;

/* 查询函数将 POD 数据复制到插件提供的结构，返回后数据归插件所有。 */

/* All strings crossing the ABI are UTF-8. Document handles are non-owning. */

/** @brief UI 线程调用的命令回调；userData 由插件持有且必须保持到 shutdown。 */
typedef void (YICAD_PLUGIN_CALL *YiCadCommandCallback)(void* userData);
/** @brief UI 线程调用的导入回调；文档和路径只在本次调用期间借用。 */
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadImportCallback)(
    YiCadDocumentHandle document,
    const char* filePath,
    void* userData);
/** @brief UI 线程调用的导出回调；文档和路径只在本次调用期间借用。 */
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadExportCallback)(
    YiCadDocumentHandle document,
    const char* filePath,
    void* userData);

/** @brief 在 UI 线程显示 UTF-8 消息；宿主在返回前复制文本。 */
typedef void (YICAD_PLUGIN_CALL *YiCadMessageFn)(const char* text);
/** @brief 在 init 注册命令；字符串在返回前复制，失败时不保留注册。 */
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadRegisterCommandFn)(
    const char* pluginId,
    const char* commandId,
    const char* displayName,
    YiCadCommandCallback callback,
    void* userData);
/** @brief 在 init 注册 Ribbon 按钮；字符串在返回前复制。 */
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadRegisterRibbonButtonFn)(
    const char* pluginId,
    const char* tab,
    const char* group,
    const char* commandId,
    const char* iconPath);
/** @brief 返回当前文档的非拥有句柄；没有活动文档时返回 nullptr。 */
typedef YiCadDocumentHandle (YICAD_PLUGIN_CALL *YiCadCurrentDocumentFn)(void);
/** @brief 向打开文档添加直线；失败时不保留部分实体。 */
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadDocumentAddLineFn)(
    YiCadDocumentHandle document,
    double x1,
    double y1,
    double x2,
    double y2);
/** @brief 向打开文档添加圆；失败时不保留部分实体。 */
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadDocumentAddCircleFn)(
    YiCadDocumentHandle document,
    double centerX,
    double centerY,
    double radius);
/** @brief 在 UI 线程重生成打开文档。 */
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadDocumentRegenFn)(
    YiCadDocumentHandle document);
/** @brief 在 UI 线程对打开文档执行自动缩放。 */
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadDocumentZoomAutoFn)(
    YiCadDocumentHandle document);
/** @brief 在 init 注册导入过滤器；回调和 userData 由插件保持到 shutdown。 */
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadRegisterImportFilterFn)(
    const char* pluginId,
    const char* formatId,
    const char* displayName,
    const char* extension,
    YiCadImportCallback callback,
    void* userData);
/** @brief 在 init 注册导出过滤器；回调和 userData 由插件保持到 shutdown。 */
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadRegisterExportFilterFn)(
    const char* pluginId,
    const char* formatId,
    const char* displayName,
    const char* extension,
    YiCadExportCallback callback,
    void* userData);
/** @brief 在 UI 线程开始非嵌套事务；返回句柄必须提交或回滚一次。 */
typedef YiCadTransactionHandle (YICAD_PLUGIN_CALL *YiCadDocumentBeginTransactionFn)(
    YiCadDocumentHandle document,
    const char* name);
/** @brief 提交并消费事务句柄；失败时句柄也不再可用。 */
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadDocumentCommitTransactionFn)(
    YiCadTransactionHandle transaction);
/** @brief 回滚并消费事务句柄。 */
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadDocumentRollbackTransactionFn)(
    YiCadTransactionHandle transaction);
/** @brief 创建只读实体快照迭代器；插件必须显式销毁返回句柄。 */
typedef YiCadEntityIteratorHandle (YICAD_PLUGIN_CALL *YiCadDocumentCreateEntityIteratorFn)(
    YiCadDocumentHandle document);
/** @brief 移动到下一实体；失败表示结束或参数无效，不再保留当前实体。 */
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadEntityIteratorNextFn)(
    YiCadEntityIteratorHandle iterator,
    YiCadEntityType* entityType);
/** @brief 复制当前直线快照；类型或句柄不匹配时失败且不转移所有权。 */
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadEntityIteratorGetLineFn)(
    YiCadEntityIteratorHandle iterator,
    YiCadLineData* line);
/** @brief 复制当前圆快照；类型或句柄不匹配时失败且不转移所有权。 */
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadEntityIteratorGetCircleFn)(
    YiCadEntityIteratorHandle iterator,
    YiCadCircleData* circle);
/** @brief 销毁宿主持有的迭代器句柄；空句柄和重复销毁安全。 */
typedef void (YICAD_PLUGIN_CALL *YiCadEntityIteratorDestroyFn)(
    YiCadEntityIteratorHandle iterator);

typedef struct YiCadImportApi YiCadImportApi;

/**
 * @brief 为打开的文档开始一个非嵌套导入会话。
 * @param document 非拥有型文档句柄。
 * @param[out] session 成功时接收宿主持有的会话句柄。
 * @return 导入结果码；失败时不创建会话。
 * @note 仅允许在 YiCAD UI 主线程调用。
 */
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportBeginFn)(
    YiCadDocumentHandle document,
    YiCadImportSessionHandle* session);
/**
 * @brief 原子提交导入会话并消费句柄。
 * @note 提交失败时宿主自动回滚；空会话不创建撤销项。
 */
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportCommitFn)(
    YiCadImportSessionHandle session);
/** @brief 回滚导入会话并消费句柄。 */
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportRollbackFn)(
    YiCadImportSessionHandle session);
/**
 * @brief 读取当前线程最后一条导入错误的 UTF-8 文本。
 * @param buffer 调用方提供的缓冲区；可为 nullptr 以查询长度。
 * @param bufferSize 缓冲区字节数。
 * @return 完整文本所需的字节数，包含末尾 NUL。
 * @note 有效缓冲区始终会被 NUL 终止；宿主不保存缓冲区。
 */
typedef uint32_t (YICAD_PLUGIN_CALL *YiCadImportGetLastErrorFn)(
    char* buffer,
    uint32_t bufferSize);
/**
 * @brief 设置会话所属文档的导入元数据。
 * @param session 活动会话句柄。
 * @param settings 调用期间借用的完整设置结构。
 * @return 成功或确定的导入错误码；失败时不修改文档设置。
 */
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportSetDocumentSettingsFn)(
    YiCadImportSessionHandle session,
    const YiCadDocumentSettings* settings);
/** @brief 创建线型资源；输出句柄仅在当前会话内有效。 */
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportCreateLineTypeFn)(
    YiCadImportSessionHandle session,
    const YiCadLineTypeDataV3* data,
    YiCadResourceConflictPolicy conflictPolicy,
    YiCadImportResourceHandle* resource);
/** @brief 创建图层资源；引用的线型必须属于同一活动会话。 */
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportCreateLayerFn)(
    YiCadImportSessionHandle session,
    const YiCadLayerDataV3* data,
    YiCadResourceConflictPolicy conflictPolicy,
    YiCadImportResourceHandle* resource);
/** @brief 创建文字样式资源；缺失字体名由宿主保留并产生诊断。 */
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportCreateTextStyleFn)(
    YiCadImportSessionHandle session,
    const YiCadTextStyleDataV3* data,
    YiCadResourceConflictPolicy conflictPolicy,
    YiCadImportResourceHandle* resource);
/** @brief 创建标注样式资源；未支持字段只有显式允许时才能降级。 */
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportCreateDimensionStyleFn)(
    YiCadImportSessionHandle session,
    const YiCadDimensionStyleDataV3* data,
    YiCadResourceConflictPolicy conflictPolicy,
    YiCadImportResourceHandle* resource);
/**
 * @brief 获取文档唯一的模型空间容器。
 * @note 输出容器由宿主持有，只在当前会话内有效，插件不得释放。
 */
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportGetModelSpaceFn)(
    YiCadImportSessionHandle session,
    YiCadImportContainerHandle* container);
/** @brief 在模型空间或活动块容器中创建点；输入在返回前复制。 */
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportCreatePointFn)(
    YiCadImportSessionHandle session,
    YiCadImportContainerHandle container,
    const YiCadPointDataV3* data);
/** @brief 在模型空间或活动块容器中创建线段；输入在返回前复制。 */
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportCreateLineFn)(
    YiCadImportSessionHandle session,
    YiCadImportContainerHandle container,
    const YiCadLineDataV3* data);
/** @brief 在模型空间或活动块容器中创建射线；输入在返回前复制。 */
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportCreateRayFn)(
    YiCadImportSessionHandle session,
    YiCadImportContainerHandle container,
    const YiCadRayDataV3* data);
/** @brief 在模型空间或活动块容器中创建无限长线；输入在返回前复制。 */
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportCreateXLineFn)(
    YiCadImportSessionHandle session,
    YiCadImportContainerHandle container,
    const YiCadXLineDataV3* data);
/** @brief 在模型空间或活动块容器中创建圆弧；输入在返回前复制。 */
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportCreateArcFn)(
    YiCadImportSessionHandle session,
    YiCadImportContainerHandle container,
    const YiCadArcDataV3* data);
/** @brief 在模型空间或活动块容器中创建圆；半径必须大于零。 */
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportCreateCircleFn)(
    YiCadImportSessionHandle session,
    YiCadImportContainerHandle container,
    const YiCadCircleDataV3* data);
/** @brief 在模型空间或活动块容器中创建椭圆或椭圆弧。 */
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportCreateEllipseFn)(
    YiCadImportSessionHandle session,
    YiCadImportContainerHandle container,
    const YiCadEllipseDataV3* data);
/** @brief 创建二维多段线；宿主在返回前复制全部顶点。 */
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportCreatePolylineFn)(
    YiCadImportSessionHandle session,
    YiCadImportContainerHandle container,
    const YiCadPolylineDataV3* data);
/** @brief 创建样条曲线；宿主在返回前复制节点、控制点和拟合点数组。 */
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportCreateSplineFn)(
    YiCadImportSessionHandle session,
    YiCadImportContainerHandle container,
    const YiCadSplineDataV3* data);
/** @brief 创建三点或四点二维实体填充。 */
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportCreateSolidFn)(
    YiCadImportSessionHandle session,
    YiCadImportContainerHandle container,
    const YiCadSolidDataV3* data);
/** @brief 创建语义单行文字；宿主在返回前复制 UTF-8 内容。 */
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportCreateTextFn)(
    YiCadImportSessionHandle session,
    YiCadImportContainerHandle container,
    const YiCadTextDataV3* data);
/** @brief 创建语义多行文字；宿主在返回前复制原始 UTF-8 格式串。 */
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportCreateMTextFn)(
    YiCadImportSessionHandle session,
    YiCadImportContainerHandle container,
    const YiCadMTextDataV3* data);
/**
 * @brief 开始块定义并返回块资源和活动块容器。
 * @note 成功后必须调用 endBlock；两个输出句柄均由宿主持有。
 */
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportBeginBlockFn)(
    YiCadImportSessionHandle session,
    const YiCadBlockDataV3* data,
    YiCadImportResourceHandle* block,
    YiCadImportContainerHandle* container);
/** @brief 结束块定义并消费块容器句柄；模型空间容器不接受此操作。 */
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportEndBlockFn)(
    YiCadImportSessionHandle session,
    YiCadImportContainerHandle container);
/** @brief 创建共享块引用；输出句柄供同一容器内的属性值引用。 */
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportCreateInsertFn)(
    YiCadImportSessionHandle session,
    YiCadImportContainerHandle container,
    const YiCadInsertDataV3* data,
    YiCadImportResourceHandle* insert);
/** @brief 在活动块定义中创建属性定义，tag 在块内必须唯一。 */
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportCreateAttributeDefinitionFn)(
    YiCadImportSessionHandle session,
    YiCadImportContainerHandle container,
    const YiCadAttributeDefinitionDataV3* data);
/** @brief 为同一容器中的块引用创建按 tag 关联的属性值。 */
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportCreateAttributeFn)(
    YiCadImportSessionHandle session,
    YiCadImportContainerHandle container,
    const YiCadAttributeDataV3* data);
/** @brief 创建语义标注；坐标标注当前明确返回不支持。 */
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportCreateDimensionFn)(
    YiCadImportSessionHandle session,
    YiCadImportContainerHandle container,
    const YiCadDimensionDataV3* data);
/** @brief 创建引线及可选关联文字；顶点数组在返回前复制。 */
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportCreateLeaderFn)(
    YiCadImportSessionHandle session,
    YiCadImportContainerHandle container,
    const YiCadLeaderDataV3* data);
/** @brief 创建填充；所有环和边会先完整验证，再修改文档。 */
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportCreateHatchFn)(
    YiCadImportSessionHandle session,
    YiCadImportContainerHandle container,
    const YiCadHatchDataV3* data);
/** @brief 创建图像引用；缺失文件保留路径并产生可查询诊断。 */
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportCreateImageFn)(
    YiCadImportSessionHandle session,
    YiCadImportContainerHandle container,
    const YiCadImageDataV3* data);
/**
 * @brief v4：创建自定义实体；类登记了且读得了数据时建原实体，否则建代理实体。
 * @note data->proxyGraphics 非空时必须是本会话 beginProxyGraphics 返回的容器，用后失效。
 */
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportCreateCustomEntityFn)(
    YiCadImportSessionHandle session,
    YiCadImportContainerHandle container,
    const YiCadCustomEntityDataV4* data);
/**
 * @brief v4：开始收集一个代理的图形：之后在返回的容器里建的实体不进文档，
 * 交给 createCustomEntity 的 proxyGraphics 后成为代理显示的图形。
 * @note 容器里只能建基本实体（不能建块、块引用、属性）；实体属性为空、图层为空、线型为空、
 * 颜色与线宽随块，表示沿用自定义实体自己的属性。没交给 createCustomEntity 的容器随会话丢弃。
 */
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportBeginProxyGraphicsFn)(
    YiCadImportSessionHandle session,
    YiCadImportContainerHandle* graphics);

/**
 * @brief 导入子函数表（v3 引入，v4 在尾部追加自定义实体）。
 * @note 宿主持有本表，其生命周期与 YiCadHostApi 相同。插件读取任何函数指针前
 * 必须同时检查 abiVersion、structSize 和指针；缺失尾字段表示该能力不支持。
 * 字段顺序是冻结顺序；后续 ABI 只能在尾部追加。
 */
struct YiCadImportApi
{
    uint32_t structSize;
    uint32_t abiVersion;
    YiCadImportBeginFn beginImport;
    YiCadImportCommitFn commitImport;
    YiCadImportRollbackFn rollbackImport;
    YiCadImportGetLastErrorFn getLastError;
    YiCadImportSetDocumentSettingsFn setDocumentSettings;
    YiCadImportCreateLineTypeFn createLineType;
    YiCadImportCreateLayerFn createLayer;
    YiCadImportCreateTextStyleFn createTextStyle;
    YiCadImportCreateDimensionStyleFn createDimensionStyle;
    YiCadImportGetModelSpaceFn getModelSpace;
    YiCadImportCreatePointFn createPoint;
    YiCadImportCreateLineFn createLine;
    YiCadImportCreateRayFn createRay;
    YiCadImportCreateXLineFn createXLine;
    YiCadImportCreateArcFn createArc;
    YiCadImportCreateCircleFn createCircle;
    YiCadImportCreateEllipseFn createEllipse;
    YiCadImportCreatePolylineFn createPolyline;
    YiCadImportCreateSplineFn createSpline;
    YiCadImportCreateSolidFn createSolid;
    YiCadImportCreateTextFn createText;
    YiCadImportCreateMTextFn createMText;
    YiCadImportBeginBlockFn beginBlock;
    YiCadImportEndBlockFn endBlock;
    YiCadImportCreateInsertFn createInsert;
    YiCadImportCreateAttributeDefinitionFn createAttributeDefinition;
    YiCadImportCreateAttributeFn createAttribute;
    YiCadImportCreateDimensionFn createDimension;
    YiCadImportCreateLeaderFn createLeader;
    YiCadImportCreateHatchFn createHatch;
    YiCadImportCreateImageFn createImage;
    /* v4 */
    YiCadImportCreateCustomEntityFn createCustomEntity;
    YiCadImportBeginProxyGraphicsFn beginProxyGraphics;
};

typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadReadDocumentSettingsFn)(
    YiCadDocumentHandle document, YiCadDocumentSettings* output);
typedef uint32_t (YICAD_PLUGIN_CALL *YiCadReadResourceCountFn)(
    YiCadDocumentHandle document, YiCadReadResourceKind kind);
typedef YiCadReadResourceHandle (YICAD_PLUGIN_CALL *YiCadReadResourceAtFn)(
    YiCadDocumentHandle document, YiCadReadResourceKind kind, uint32_t index);
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadReadResourceDataFn)(
    YiCadReadResourceHandle resource, YiCadReadResourceKind kind, void* output);
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadReadResourceNameFn)(
    YiCadReadResourceHandle resource, YiCadStringView* output);
typedef uint32_t (YICAD_PLUGIN_CALL *YiCadReadBlockCountFn)(
    YiCadDocumentHandle document);
typedef YiCadReadResourceHandle (YICAD_PLUGIN_CALL *YiCadReadBlockAtFn)(
    YiCadDocumentHandle document, uint32_t index);
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadReadBlockDataFn)(
    YiCadReadResourceHandle block, YiCadBlockDataV3* output);
typedef YiCadEntityIteratorHandle (YICAD_PLUGIN_CALL *YiCadReadEntitiesFn)(
    YiCadDocumentHandle document, YiCadReadResourceHandle block);
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadReadEntityNextFn)(
    YiCadEntityIteratorHandle iterator, YiCadEntityType* type);
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadReadEntityDataFn)(
    YiCadEntityIteratorHandle iterator, void* output);
typedef void (YICAD_PLUGIN_CALL *YiCadReadEntityDestroyFn)(
    YiCadEntityIteratorHandle iterator);
/**
 * @brief v4：当前自定义实体的图形，做成基本实体逐个枚举（写进别的格式的代理图形用）。
 * @return 新的迭代器，插件用 entityDestroy 销毁；当前项不是自定义实体时返回空。
 * @note 块展开成内容，实心填充为填充，图案填充为切好的线与点，文字为笔画，样条离散成多段线；
 * 各实体的属性是解析后的（与自定义实体相同的属性原样给出）。
 */
typedef YiCadEntityIteratorHandle (YICAD_PLUGIN_CALL *YiCadReadEntityGraphicsFn)(
    YiCadEntityIteratorHandle iterator);

/**
 * @brief 文档只读枚举子表（v3 引入，v4 在尾部追加 entityGraphics）；返回的视图保持到同一子表的
 * 下一次调用。v4 起自定义实体以 YICAD_ENTITY_CUSTOM 交出，不再炸开。
 */
typedef struct YiCadReadApi
{
    uint32_t structSize;
    uint32_t abiVersion;
    YiCadReadDocumentSettingsFn documentSettings;
    YiCadReadResourceCountFn resourceCount;
    YiCadReadResourceAtFn resourceAt;
    YiCadReadResourceDataFn resourceData;
    YiCadReadResourceNameFn resourceName;
    YiCadReadBlockCountFn blockCount;
    YiCadReadBlockAtFn blockAt;
    YiCadReadBlockDataFn blockData;
    YiCadReadEntitiesFn entities;
    YiCadReadEntityNextFn entityNext;
    YiCadReadEntityDataFn entityData;
    YiCadReadEntityDestroyFn entityDestroy;
    /* v4 */
    YiCadReadEntityGraphicsFn entityGraphics;
} YiCadReadApi;

/**
 * @brief v4：在 init 里登记一个实体类。
 * @note 宿主在返回前复制结构与类名；函数指针与 userData 由插件保持到 shutdown。类名必须以
 * pluginId 加点开头；登记与插件的其他注册项一起原子提交。
 */
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadRegisterEntityClassFn)(
    const char* pluginId,
    const YiCadEntityClassV4* entityClass);
/**
 * @brief v4：在文档事务里于模型空间新建一个自定义实体（类必须已登记）。
 * @param[out] entity 成功时接收实体句柄，可为空。
 * @note data->classVersion 必须等于类的当前版本；proxyGraphics 必须为空；attributes 的图层、
 * 线型句柄必须为空（取当前图层、随层）。错误文本经 importApi->getLastError 读取。
 */
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadEntityCreateCustomFn)(
    YiCadTransactionHandle transaction,
    const YiCadCustomEntityDataV4* data,
    YiCadEntityHandle* entity);
/**
 * @brief v4：在文档事务里换一个自定义实体的数据（当前数据版本），可撤销。
 * @note 实体必须是登记了的类的原实体（不是代理），属于事务所在的文档。
 */
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadEntitySetCustomDataFn)(
    YiCadTransactionHandle transaction,
    YiCadEntityHandle entity,
    YiCadByteView data);

/** @brief v4 自定义实体子表；其生命周期与宿主表相同。 */
typedef struct YiCadEntityApiV4
{
    uint32_t structSize;
    uint32_t abiVersion;
    YiCadRegisterEntityClassFn registerEntityClass;
    YiCadEntityCreateCustomFn createCustomEntity;
    YiCadEntitySetCustomDataFn setCustomEntityData;
} YiCadEntityApiV4;

typedef struct YiCadHostApi
{
    uint32_t structSize;
    uint32_t abiVersion;
    YiCadMessageFn message;
    YiCadRegisterCommandFn registerCommand;
    YiCadRegisterRibbonButtonFn registerRibbonButton;
    YiCadCurrentDocumentFn currentDocument;
    YiCadDocumentAddLineFn documentAddLine;
    YiCadDocumentAddCircleFn documentAddCircle;
    YiCadDocumentRegenFn documentRegen;
    YiCadDocumentZoomAutoFn documentZoomAuto;
    YiCadRegisterImportFilterFn registerImportFilter;
    YiCadRegisterExportFilterFn registerExportFilter;
    YiCadDocumentBeginTransactionFn documentBeginTransaction;
    YiCadDocumentCommitTransactionFn documentCommitTransaction;
    YiCadDocumentRollbackTransactionFn documentRollbackTransaction;
    YiCadDocumentCreateEntityIteratorFn documentCreateEntityIterator;
    YiCadEntityIteratorNextFn entityIteratorNext;
    YiCadEntityIteratorGetLineFn entityIteratorGetLine;
    YiCadEntityIteratorGetCircleFn entityIteratorGetCircle;
    YiCadEntityIteratorDestroyFn entityIteratorDestroy;
    /** @brief ABI v3 导入子表；其生命周期与宿主表相同。 */
    const YiCadImportApi* importApi;
    /** @brief ABI v3 文档只读枚举子表；其生命周期与宿主表相同。 */
    const YiCadReadApi* readApi;
    /** @brief ABI v4 自定义实体子表；其生命周期与宿主表相同。 */
    const YiCadEntityApiV4* entityApi;
} YiCadHostApi;

typedef struct YiCadPluginApi
{
    uint32_t structSize;
    uint32_t abiVersion;
    const char* pluginId;
    const char* pluginName;
    const char* pluginVersion;
} YiCadPluginApi;

/** @brief ABI v1 宿主函数表前缀的字节数。 */
#define YICAD_HOST_API_V1_SIZE                                            \
    ((uint32_t)(offsetof(YiCadHostApi, registerExportFilter) +           \
                sizeof(((YiCadHostApi*)0)->registerExportFilter)))
/** @brief ABI v1 插件输出表前缀的字节数。 */
#define YICAD_PLUGIN_API_V1_SIZE                                          \
    ((uint32_t)(offsetof(YiCadPluginApi, pluginVersion) +                 \
                sizeof(((YiCadPluginApi*)0)->pluginVersion)))
/** @brief ABI v2 宿主函数表前缀的字节数。 */
#define YICAD_HOST_API_V2_SIZE                                            \
    ((uint32_t)(offsetof(YiCadHostApi, entityIteratorDestroy) +           \
                sizeof(((YiCadHostApi*)0)->entityIteratorDestroy)))
/** @brief ABI v3 宿主表的冻结字节数。 */
#define YICAD_HOST_API_V3_SIZE                                            \
    ((uint32_t)(offsetof(YiCadHostApi, readApi) +                          \
                sizeof(((YiCadHostApi*)0)->readApi)))
/** @brief ABI v3 导入子表的冻结字节数。 */
#define YICAD_IMPORT_API_V3_SIZE                                          \
    ((uint32_t)(offsetof(YiCadImportApi, createImage) +                    \
                sizeof(((YiCadImportApi*)0)->createImage)))
/** @brief ABI v4 宿主表的冻结字节数。 */
#define YICAD_HOST_API_V4_SIZE                                            \
    ((uint32_t)(offsetof(YiCadHostApi, entityApi) +                        \
                sizeof(((YiCadHostApi*)0)->entityApi)))
/** @brief ABI v4 导入子表的冻结字节数。 */
#define YICAD_IMPORT_API_V4_SIZE                                          \
    ((uint32_t)(offsetof(YiCadImportApi, beginProxyGraphics) +             \
                sizeof(((YiCadImportApi*)0)->beginProxyGraphics)))
/** @brief ABI v4 只读枚举子表的冻结字节数。 */
#define YICAD_READ_API_V4_SIZE                                            \
    ((uint32_t)(offsetof(YiCadReadApi, entityGraphics) +                   \
                sizeof(((YiCadReadApi*)0)->entityGraphics)))

/** @brief 插件入口类型；返回插件实现的最高 ABI 版本且不得抛出异常。 */
typedef uint32_t (YICAD_PLUGIN_CALL *YiCadPluginGetAbiVersionFn)(void);
/** @brief 插件初始化入口类型；宿主表借用到 shutdown，输出表容量由宿主给出。 */
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadPluginInitFn)(
    const YiCadHostApi* host,
    YiCadPluginApi* plugin);
/** @brief 插件关闭入口类型；必须释放回调引用且不得抛出异常。 */
typedef void (YICAD_PLUGIN_CALL *YiCadPluginShutdownFn)(void);

/** @brief 返回插件实现的最高 C ABI 版本。 */
YICAD_PLUGIN_API uint32_t YICAD_PLUGIN_CALL
yicad_plugin_get_abi_version(void);
/** @brief 使用 ABI v4 初始化插件并填写输出表。 */
YICAD_PLUGIN_API YiCadResult YICAD_PLUGIN_CALL
yicad_plugin_init(const YiCadHostApi* host, YiCadPluginApi* plugin);
/** @brief 关闭插件；宿主保证成功调用 init 后至多调用一次。 */
YICAD_PLUGIN_API void YICAD_PLUGIN_CALL
yicad_plugin_shutdown(void);

#if defined(__cplusplus)
#define YICAD_ABI_STATIC_ASSERT(condition, message) static_assert(condition, message)
#define YICAD_ABI_ALIGNOF(type) alignof(type)
#else
#define YICAD_ABI_STATIC_ASSERT(condition, message) _Static_assert(condition, message)
#define YICAD_ABI_ALIGNOF(type) _Alignof(type)
#endif

#define YICAD_ABI_FIELD_FOLLOWS(type, field, previousField)                  \
    YICAD_ABI_STATIC_ASSERT(                                                 \
        offsetof(type, field) >=                                            \
            offsetof(type, previousField) +                                 \
                sizeof(((type*)0)->previousField),                          \
        #type "." #field " must follow " #previousField)

#define YICAD_ABI_FIELD_AT(type, field, expectedOffset)                      \
    YICAD_ABI_STATIC_ASSERT(                                                 \
        offsetof(type, field) == (expectedOffset),                          \
        #type "." #field " offset snapshot changed")

#define YICAD_ABI_MIN_SIZE_IS(name, expectedSize)                            \
    YICAD_ABI_STATIC_ASSERT(                                                 \
        (name) == (expectedSize),                                           \
        #name " snapshot changed")

YICAD_ABI_STATIC_ASSERT(sizeof(uint32_t) == 4, "uint32_t must be 32-bit");
YICAD_ABI_STATIC_ASSERT(sizeof(YiCadResult) == 4, "YiCadResult must be 32-bit");
YICAD_ABI_STATIC_ASSERT(
    sizeof(YiCadEntityType) == 4,
    "YiCadEntityType must be 32-bit");
YICAD_ABI_STATIC_ASSERT(
    sizeof(YiCadLineData) == 32,
    "YiCadLineData layout changed");
YICAD_ABI_STATIC_ASSERT(
    sizeof(YiCadCircleData) == 24,
    "YiCadCircleData layout changed");
YICAD_ABI_STATIC_ASSERT(YICAD_FAILURE == 0, "failure must be zero");
YICAD_ABI_STATIC_ASSERT(YICAD_SUCCESS == 1, "success must be one");
YICAD_ABI_STATIC_ASSERT(
    YICAD_PLUGIN_ABI_V1 == UINT32_C(1),
    "ABI v1 version snapshot changed");
YICAD_ABI_STATIC_ASSERT(
    YICAD_PLUGIN_ABI_V2 == UINT32_C(2),
    "ABI v2 version snapshot changed");
YICAD_ABI_STATIC_ASSERT(
    YICAD_PLUGIN_ABI_V3 == UINT32_C(3),
    "ABI v3 version snapshot changed");
YICAD_ABI_STATIC_ASSERT(
    sizeof(YiCadImportResult) == 4,
    "YiCadImportResult must be 32-bit");
YICAD_ABI_STATIC_ASSERT(
    sizeof(YiCadColorMethod) == 4,
    "YiCadColorMethod must be 32-bit");

#if defined(_WIN32) && UINTPTR_MAX == UINT64_MAX
YICAD_ABI_STATIC_ASSERT(sizeof(YiCadStringView) == 16,
    "unexpected Win64 YiCadStringView size");
YICAD_ABI_STATIC_ASSERT(YICAD_ABI_ALIGNOF(YiCadStringView) == 8,
    "unexpected Win64 YiCadStringView alignment");
YICAD_ABI_FIELD_AT(YiCadStringView, data, 0);
YICAD_ABI_FIELD_AT(YiCadStringView, size, 8);
YICAD_ABI_STATIC_ASSERT(sizeof(YiCadDoubleArrayView) == 16,
    "unexpected Win64 YiCadDoubleArrayView size");
YICAD_ABI_STATIC_ASSERT(YICAD_ABI_ALIGNOF(YiCadDoubleArrayView) == 8,
    "unexpected Win64 YiCadDoubleArrayView alignment");
YICAD_ABI_FIELD_AT(YiCadDoubleArrayView, data, 0);
YICAD_ABI_FIELD_AT(YiCadDoubleArrayView, count, 8);
YICAD_ABI_STATIC_ASSERT(sizeof(YiCadPoint2dArrayView) == 16,
    "unexpected Win64 YiCadPoint2dArrayView size");
YICAD_ABI_STATIC_ASSERT(YICAD_ABI_ALIGNOF(YiCadPoint2dArrayView) == 8,
    "unexpected Win64 YiCadPoint2dArrayView alignment");
YICAD_ABI_FIELD_AT(YiCadPoint2dArrayView, data, 0);
YICAD_ABI_FIELD_AT(YiCadPoint2dArrayView, count, 8);
YICAD_ABI_STATIC_ASSERT(sizeof(YiCadVertex2dArrayView) == 16,
    "unexpected Win64 YiCadVertex2dArrayView size");
YICAD_ABI_STATIC_ASSERT(YICAD_ABI_ALIGNOF(YiCadVertex2dArrayView) == 8,
    "unexpected Win64 YiCadVertex2dArrayView alignment");
YICAD_ABI_FIELD_AT(YiCadVertex2dArrayView, data, 0);
YICAD_ABI_FIELD_AT(YiCadVertex2dArrayView, count, 8);
YICAD_ABI_STATIC_ASSERT(sizeof(YiCadHatchEdgeArrayView) == 16,
    "unexpected Win64 YiCadHatchEdgeArrayView size");
YICAD_ABI_STATIC_ASSERT(YICAD_ABI_ALIGNOF(YiCadHatchEdgeArrayView) == 8,
    "unexpected Win64 YiCadHatchEdgeArrayView alignment");
YICAD_ABI_FIELD_AT(YiCadHatchEdgeArrayView, data, 0);
YICAD_ABI_FIELD_AT(YiCadHatchEdgeArrayView, count, 8);
YICAD_ABI_FIELD_AT(YiCadHatchEdgeArrayView, byteStride, 12);
YICAD_ABI_STATIC_ASSERT(sizeof(YiCadHatchLoopArrayView) == 16,
    "unexpected Win64 YiCadHatchLoopArrayView size");
YICAD_ABI_STATIC_ASSERT(YICAD_ABI_ALIGNOF(YiCadHatchLoopArrayView) == 8,
    "unexpected Win64 YiCadHatchLoopArrayView alignment");
YICAD_ABI_FIELD_AT(YiCadHatchLoopArrayView, data, 0);
YICAD_ABI_FIELD_AT(YiCadHatchLoopArrayView, count, 8);
YICAD_ABI_FIELD_AT(YiCadHatchLoopArrayView, byteStride, 12);
YICAD_ABI_MIN_SIZE_IS(YICAD_DOCUMENT_SETTINGS_V3_MIN_SIZE, 40);
YICAD_ABI_MIN_SIZE_IS(YICAD_LINE_TYPE_DATA_V3_MIN_SIZE, 60);
YICAD_ABI_MIN_SIZE_IS(YICAD_LAYER_DATA_V3_MIN_SIZE, 60);
YICAD_ABI_MIN_SIZE_IS(YICAD_TEXT_STYLE_DATA_V3_MIN_SIZE, 84);
YICAD_ABI_MIN_SIZE_IS(YICAD_DIMENSION_STYLE_DATA_V3_MIN_SIZE, 316);
YICAD_ABI_MIN_SIZE_IS(YICAD_ENTITY_ATTRIBUTES_V3_MIN_SIZE, 80);
YICAD_ABI_MIN_SIZE_IS(YICAD_POINT_DATA_V3_MIN_SIZE, 32);
YICAD_ABI_MIN_SIZE_IS(YICAD_LINE_DATA_V3_MIN_SIZE, 48);
YICAD_ABI_MIN_SIZE_IS(YICAD_RAY_DATA_V3_MIN_SIZE, 48);
YICAD_ABI_MIN_SIZE_IS(YICAD_XLINE_DATA_V3_MIN_SIZE, 48);
YICAD_ABI_MIN_SIZE_IS(YICAD_ARC_DATA_V3_MIN_SIZE, 56);
YICAD_ABI_MIN_SIZE_IS(YICAD_CIRCLE_DATA_V3_MIN_SIZE, 40);
YICAD_ABI_MIN_SIZE_IS(YICAD_ELLIPSE_DATA_V3_MIN_SIZE, 76);
YICAD_ABI_MIN_SIZE_IS(YICAD_POLYLINE_DATA_V3_MIN_SIZE, 36);
YICAD_ABI_MIN_SIZE_IS(YICAD_SPLINE_DATA_V3_MIN_SIZE, 104);
YICAD_ABI_MIN_SIZE_IS(YICAD_SOLID_DATA_V3_MIN_SIZE, 84);
YICAD_ABI_MIN_SIZE_IS(YICAD_TEXT_DATA_V3_MIN_SIZE, 112);
YICAD_ABI_MIN_SIZE_IS(YICAD_MTEXT_BACKGROUND_DATA_V3_MIN_SIZE, 32);
YICAD_ABI_MIN_SIZE_IS(YICAD_MTEXT_DATA_V3_MIN_SIZE, 112);
YICAD_ABI_MIN_SIZE_IS(YICAD_BLOCK_DATA_V3_MIN_SIZE, 80);
YICAD_ABI_MIN_SIZE_IS(YICAD_INSERT_DATA_V3_MIN_SIZE, 96);
YICAD_ABI_MIN_SIZE_IS(
    YICAD_ATTRIBUTE_DEFINITION_DATA_V3_MIN_SIZE, 68);
YICAD_ABI_MIN_SIZE_IS(YICAD_ATTRIBUTE_DATA_V3_MIN_SIZE, 60);
YICAD_ABI_MIN_SIZE_IS(YICAD_DIMENSION_DATA_V3_MIN_SIZE, 232);
YICAD_ABI_MIN_SIZE_IS(YICAD_LEADER_DATA_V3_MIN_SIZE, 56);
YICAD_ABI_MIN_SIZE_IS(YICAD_HATCH_EDGE_DATA_V3_MIN_SIZE, 168);
YICAD_ABI_MIN_SIZE_IS(YICAD_HATCH_LOOP_DATA_V3_MIN_SIZE, 48);
YICAD_ABI_MIN_SIZE_IS(YICAD_HATCH_DATA_V3_MIN_SIZE, 72);
YICAD_ABI_MIN_SIZE_IS(YICAD_IMAGE_DATA_V3_MIN_SIZE, 128);
#elif defined(_WIN32) && UINTPTR_MAX == UINT32_MAX
YICAD_ABI_STATIC_ASSERT(sizeof(YiCadStringView) == 8,
    "unexpected Win32 YiCadStringView size");
YICAD_ABI_STATIC_ASSERT(YICAD_ABI_ALIGNOF(YiCadStringView) == 4,
    "unexpected Win32 YiCadStringView alignment");
YICAD_ABI_FIELD_AT(YiCadStringView, data, 0);
YICAD_ABI_FIELD_AT(YiCadStringView, size, 4);
YICAD_ABI_STATIC_ASSERT(sizeof(YiCadDoubleArrayView) == 8,
    "unexpected Win32 YiCadDoubleArrayView size");
YICAD_ABI_STATIC_ASSERT(YICAD_ABI_ALIGNOF(YiCadDoubleArrayView) == 4,
    "unexpected Win32 YiCadDoubleArrayView alignment");
YICAD_ABI_FIELD_AT(YiCadDoubleArrayView, data, 0);
YICAD_ABI_FIELD_AT(YiCadDoubleArrayView, count, 4);
YICAD_ABI_STATIC_ASSERT(sizeof(YiCadPoint2dArrayView) == 8,
    "unexpected Win32 YiCadPoint2dArrayView size");
YICAD_ABI_STATIC_ASSERT(YICAD_ABI_ALIGNOF(YiCadPoint2dArrayView) == 4,
    "unexpected Win32 YiCadPoint2dArrayView alignment");
YICAD_ABI_FIELD_AT(YiCadPoint2dArrayView, data, 0);
YICAD_ABI_FIELD_AT(YiCadPoint2dArrayView, count, 4);
YICAD_ABI_STATIC_ASSERT(sizeof(YiCadVertex2dArrayView) == 8,
    "unexpected Win32 YiCadVertex2dArrayView size");
YICAD_ABI_STATIC_ASSERT(YICAD_ABI_ALIGNOF(YiCadVertex2dArrayView) == 4,
    "unexpected Win32 YiCadVertex2dArrayView alignment");
YICAD_ABI_FIELD_AT(YiCadVertex2dArrayView, data, 0);
YICAD_ABI_FIELD_AT(YiCadVertex2dArrayView, count, 4);
YICAD_ABI_STATIC_ASSERT(sizeof(YiCadHatchEdgeArrayView) == 12,
    "unexpected Win32 YiCadHatchEdgeArrayView size");
YICAD_ABI_STATIC_ASSERT(YICAD_ABI_ALIGNOF(YiCadHatchEdgeArrayView) == 4,
    "unexpected Win32 YiCadHatchEdgeArrayView alignment");
YICAD_ABI_FIELD_AT(YiCadHatchEdgeArrayView, data, 0);
YICAD_ABI_FIELD_AT(YiCadHatchEdgeArrayView, count, 4);
YICAD_ABI_FIELD_AT(YiCadHatchEdgeArrayView, byteStride, 8);
YICAD_ABI_STATIC_ASSERT(sizeof(YiCadHatchLoopArrayView) == 12,
    "unexpected Win32 YiCadHatchLoopArrayView size");
YICAD_ABI_STATIC_ASSERT(YICAD_ABI_ALIGNOF(YiCadHatchLoopArrayView) == 4,
    "unexpected Win32 YiCadHatchLoopArrayView alignment");
YICAD_ABI_FIELD_AT(YiCadHatchLoopArrayView, data, 0);
YICAD_ABI_FIELD_AT(YiCadHatchLoopArrayView, count, 4);
YICAD_ABI_FIELD_AT(YiCadHatchLoopArrayView, byteStride, 8);
YICAD_ABI_MIN_SIZE_IS(YICAD_DOCUMENT_SETTINGS_V3_MIN_SIZE, 32);
YICAD_ABI_MIN_SIZE_IS(YICAD_LINE_TYPE_DATA_V3_MIN_SIZE, 32);
YICAD_ABI_MIN_SIZE_IS(YICAD_LAYER_DATA_V3_MIN_SIZE, 44);
YICAD_ABI_MIN_SIZE_IS(YICAD_TEXT_STYLE_DATA_V3_MIN_SIZE, 60);
YICAD_ABI_MIN_SIZE_IS(YICAD_DIMENSION_STYLE_DATA_V3_MIN_SIZE, 276);
YICAD_ABI_MIN_SIZE_IS(YICAD_ENTITY_ATTRIBUTES_V3_MIN_SIZE, 72);
YICAD_ABI_MIN_SIZE_IS(YICAD_POINT_DATA_V3_MIN_SIZE, 24);
YICAD_ABI_MIN_SIZE_IS(YICAD_LINE_DATA_V3_MIN_SIZE, 40);
YICAD_ABI_MIN_SIZE_IS(YICAD_RAY_DATA_V3_MIN_SIZE, 40);
YICAD_ABI_MIN_SIZE_IS(YICAD_XLINE_DATA_V3_MIN_SIZE, 40);
YICAD_ABI_MIN_SIZE_IS(YICAD_ARC_DATA_V3_MIN_SIZE, 48);
YICAD_ABI_MIN_SIZE_IS(YICAD_CIRCLE_DATA_V3_MIN_SIZE, 32);
YICAD_ABI_MIN_SIZE_IS(YICAD_ELLIPSE_DATA_V3_MIN_SIZE, 68);
YICAD_ABI_MIN_SIZE_IS(YICAD_POLYLINE_DATA_V3_MIN_SIZE, 20);
YICAD_ABI_MIN_SIZE_IS(YICAD_SPLINE_DATA_V3_MIN_SIZE, 60);
YICAD_ABI_MIN_SIZE_IS(YICAD_SOLID_DATA_V3_MIN_SIZE, 76);
YICAD_ABI_MIN_SIZE_IS(YICAD_TEXT_DATA_V3_MIN_SIZE, 92);
YICAD_ABI_MIN_SIZE_IS(YICAD_MTEXT_BACKGROUND_DATA_V3_MIN_SIZE, 32);
YICAD_ABI_MIN_SIZE_IS(YICAD_MTEXT_DATA_V3_MIN_SIZE, 84);
YICAD_ABI_MIN_SIZE_IS(YICAD_BLOCK_DATA_V3_MIN_SIZE, 52);
YICAD_ABI_MIN_SIZE_IS(YICAD_INSERT_DATA_V3_MIN_SIZE, 88);
YICAD_ABI_MIN_SIZE_IS(
    YICAD_ATTRIBUTE_DEFINITION_DATA_V3_MIN_SIZE, 36);
YICAD_ABI_MIN_SIZE_IS(YICAD_ATTRIBUTE_DATA_V3_MIN_SIZE, 32);
YICAD_ABI_MIN_SIZE_IS(YICAD_DIMENSION_DATA_V3_MIN_SIZE, 208);
YICAD_ABI_MIN_SIZE_IS(YICAD_LEADER_DATA_V3_MIN_SIZE, 28);
YICAD_ABI_MIN_SIZE_IS(YICAD_HATCH_EDGE_DATA_V3_MIN_SIZE, 144);
YICAD_ABI_MIN_SIZE_IS(YICAD_HATCH_LOOP_DATA_V3_MIN_SIZE, 36);
YICAD_ABI_MIN_SIZE_IS(YICAD_HATCH_DATA_V3_MIN_SIZE, 52);
YICAD_ABI_MIN_SIZE_IS(YICAD_IMAGE_DATA_V3_MIN_SIZE, 100);
#endif

YICAD_ABI_STATIC_ASSERT(sizeof(YiCadPoint2d) == 16,
    "unexpected YiCadPoint2d size");
YICAD_ABI_STATIC_ASSERT(YICAD_ABI_ALIGNOF(YiCadPoint2d) == 8,
    "unexpected YiCadPoint2d alignment");
YICAD_ABI_FIELD_AT(YiCadPoint2d, x, 0);
YICAD_ABI_FIELD_AT(YiCadPoint2d, y, 8);
YICAD_ABI_STATIC_ASSERT(sizeof(YiCadPoint3d) == 24,
    "unexpected YiCadPoint3d size");
YICAD_ABI_STATIC_ASSERT(YICAD_ABI_ALIGNOF(YiCadPoint3d) == 8,
    "unexpected YiCadPoint3d alignment");
YICAD_ABI_FIELD_AT(YiCadPoint3d, x, 0);
YICAD_ABI_FIELD_AT(YiCadPoint3d, y, 8);
YICAD_ABI_FIELD_AT(YiCadPoint3d, z, 16);
YICAD_ABI_STATIC_ASSERT(sizeof(YiCadVector2d) == 16,
    "unexpected YiCadVector2d size");
YICAD_ABI_STATIC_ASSERT(YICAD_ABI_ALIGNOF(YiCadVector2d) == 8,
    "unexpected YiCadVector2d alignment");
YICAD_ABI_FIELD_AT(YiCadVector2d, x, 0);
YICAD_ABI_FIELD_AT(YiCadVector2d, y, 8);
YICAD_ABI_STATIC_ASSERT(sizeof(YiCadVector3d) == 24,
    "unexpected YiCadVector3d size");
YICAD_ABI_STATIC_ASSERT(YICAD_ABI_ALIGNOF(YiCadVector3d) == 8,
    "unexpected YiCadVector3d alignment");
YICAD_ABI_FIELD_AT(YiCadVector3d, x, 0);
YICAD_ABI_FIELD_AT(YiCadVector3d, y, 8);
YICAD_ABI_FIELD_AT(YiCadVector3d, z, 16);
YICAD_ABI_STATIC_ASSERT(sizeof(YiCadColorData) == 12,
    "unexpected YiCadColorData size");
YICAD_ABI_STATIC_ASSERT(YICAD_ABI_ALIGNOF(YiCadColorData) == 4,
    "unexpected YiCadColorData alignment");
YICAD_ABI_FIELD_AT(YiCadColorData, method, 0);
YICAD_ABI_FIELD_AT(YiCadColorData, aci, 4);
YICAD_ABI_FIELD_AT(YiCadColorData, red, 8);
YICAD_ABI_FIELD_AT(YiCadColorData, green, 9);
YICAD_ABI_FIELD_AT(YiCadColorData, blue, 10);
YICAD_ABI_FIELD_AT(YiCadColorData, reserved, 11);
YICAD_ABI_STATIC_ASSERT(sizeof(YiCadVertex2d) == 40,
    "unexpected YiCadVertex2d size");
YICAD_ABI_STATIC_ASSERT(YICAD_ABI_ALIGNOF(YiCadVertex2d) == 8,
    "unexpected YiCadVertex2d alignment");
YICAD_ABI_FIELD_AT(YiCadVertex2d, position, 0);
YICAD_ABI_FIELD_AT(YiCadVertex2d, startWidth, 16);
YICAD_ABI_FIELD_AT(YiCadVertex2d, endWidth, 24);
YICAD_ABI_FIELD_AT(YiCadVertex2d, bulge, 32);

YICAD_ABI_STATIC_ASSERT(
    offsetof(YiCadImportApi, structSize) == 0,
    "YiCadImportApi.structSize must be first");
YICAD_ABI_STATIC_ASSERT(
    offsetof(YiCadImportApi, abiVersion) == sizeof(uint32_t),
    "YiCadImportApi.abiVersion must be second");
YICAD_ABI_FIELD_FOLLOWS(YiCadImportApi, beginImport, abiVersion);
YICAD_ABI_FIELD_FOLLOWS(YiCadImportApi, commitImport, beginImport);
YICAD_ABI_FIELD_FOLLOWS(YiCadImportApi, rollbackImport, commitImport);
YICAD_ABI_FIELD_FOLLOWS(YiCadImportApi, getLastError, rollbackImport);
YICAD_ABI_FIELD_FOLLOWS(YiCadImportApi, setDocumentSettings, getLastError);
YICAD_ABI_FIELD_FOLLOWS(YiCadImportApi, createLineType, setDocumentSettings);
YICAD_ABI_FIELD_FOLLOWS(YiCadImportApi, createLayer, createLineType);
YICAD_ABI_FIELD_FOLLOWS(YiCadImportApi, createTextStyle, createLayer);
YICAD_ABI_FIELD_FOLLOWS(
    YiCadImportApi,
    createDimensionStyle,
    createTextStyle);
YICAD_ABI_FIELD_FOLLOWS(YiCadImportApi, getModelSpace, createDimensionStyle);
YICAD_ABI_FIELD_FOLLOWS(YiCadImportApi, createPoint, getModelSpace);
YICAD_ABI_FIELD_FOLLOWS(YiCadImportApi, createLine, createPoint);
YICAD_ABI_FIELD_FOLLOWS(YiCadImportApi, createRay, createLine);
YICAD_ABI_FIELD_FOLLOWS(YiCadImportApi, createXLine, createRay);
YICAD_ABI_FIELD_FOLLOWS(YiCadImportApi, createArc, createXLine);
YICAD_ABI_FIELD_FOLLOWS(YiCadImportApi, createCircle, createArc);
YICAD_ABI_FIELD_FOLLOWS(YiCadImportApi, createEllipse, createCircle);
YICAD_ABI_FIELD_FOLLOWS(YiCadImportApi, createPolyline, createEllipse);
YICAD_ABI_FIELD_FOLLOWS(YiCadImportApi, createSpline, createPolyline);
YICAD_ABI_FIELD_FOLLOWS(YiCadImportApi, createSolid, createSpline);
YICAD_ABI_FIELD_FOLLOWS(YiCadImportApi, createText, createSolid);
YICAD_ABI_FIELD_FOLLOWS(YiCadImportApi, createMText, createText);
YICAD_ABI_FIELD_FOLLOWS(YiCadImportApi, beginBlock, createMText);
YICAD_ABI_FIELD_FOLLOWS(YiCadImportApi, endBlock, beginBlock);
YICAD_ABI_FIELD_FOLLOWS(YiCadImportApi, createInsert, endBlock);
YICAD_ABI_FIELD_FOLLOWS(
    YiCadImportApi,
    createAttributeDefinition,
    createInsert);
YICAD_ABI_FIELD_FOLLOWS(
    YiCadImportApi,
    createAttribute,
    createAttributeDefinition);
YICAD_ABI_FIELD_FOLLOWS(YiCadImportApi, createDimension, createAttribute);
YICAD_ABI_FIELD_FOLLOWS(YiCadImportApi, createLeader, createDimension);
YICAD_ABI_FIELD_FOLLOWS(YiCadImportApi, createHatch, createLeader);
YICAD_ABI_FIELD_FOLLOWS(YiCadImportApi, createImage, createHatch);
YICAD_ABI_FIELD_FOLLOWS(YiCadImportApi, createCustomEntity, createImage);
YICAD_ABI_FIELD_FOLLOWS(
    YiCadImportApi,
    beginProxyGraphics,
    createCustomEntity);
YICAD_ABI_STATIC_ASSERT(
    offsetof(YiCadImportApi, createCustomEntity) == YICAD_IMPORT_API_V3_SIZE,
    "ABI v4 must append to the frozen v3 import table");
YICAD_ABI_STATIC_ASSERT(
    YICAD_IMPORT_API_V4_SIZE ==
    YICAD_IMPORT_API_V3_SIZE + 2 * sizeof(void*),
    "unexpected ABI v4 import-table growth");
YICAD_ABI_STATIC_ASSERT(
    offsetof(YiCadReadApi, structSize) == 0,
    "YiCadReadApi.structSize must be first");
YICAD_ABI_FIELD_FOLLOWS(YiCadReadApi, entityGraphics, entityDestroy);
YICAD_ABI_STATIC_ASSERT(
    sizeof(YiCadReadApi) == YICAD_READ_API_V4_SIZE,
    "ABI v4 read table has unexpected tail padding");
YICAD_ABI_STATIC_ASSERT(
    offsetof(YiCadEntityApiV4, structSize) == 0,
    "YiCadEntityApiV4.structSize must be first");
YICAD_ABI_FIELD_FOLLOWS(YiCadEntityApiV4, registerEntityClass, abiVersion);
YICAD_ABI_FIELD_FOLLOWS(YiCadEntityApiV4, createCustomEntity, registerEntityClass);
YICAD_ABI_FIELD_FOLLOWS(YiCadEntityApiV4, setCustomEntityData, createCustomEntity);
YICAD_ABI_STATIC_ASSERT(
    offsetof(YiCadGiApiV4, structSize) == 0,
    "YiCadGiApiV4.structSize must be first");
YICAD_ABI_FIELD_FOLLOWS(YiCadGiApiV4, findResource, setScreenSpace);
YICAD_ABI_STATIC_ASSERT(
    offsetof(YiCadEntityClassV4, structSize) == 0,
    "YiCadEntityClassV4.structSize must be first");
YICAD_ABI_FIELD_FOLLOWS(YiCadEntityClassV4, destroyCache, createCache);
YICAD_ABI_STATIC_ASSERT(sizeof(YiCadMatrix2d) == 48,
    "unexpected YiCadMatrix2d size");
YICAD_ABI_STATIC_ASSERT(sizeof(YiCadExtents2d) == 32,
    "unexpected YiCadExtents2d size");
YICAD_ABI_STATIC_ASSERT(
    YICAD_PLUGIN_ABI_MIN_VERSION <= YICAD_PLUGIN_ABI_MAX_VERSION,
    "invalid supported ABI version range");

YICAD_ABI_STATIC_ASSERT(
    offsetof(YiCadHostApi, structSize) == 0,
    "YiCadHostApi.structSize must be first");
YICAD_ABI_STATIC_ASSERT(
    offsetof(YiCadHostApi, abiVersion) == sizeof(uint32_t),
    "YiCadHostApi.abiVersion must be second");
YICAD_ABI_FIELD_FOLLOWS(YiCadHostApi, message, abiVersion);
YICAD_ABI_FIELD_FOLLOWS(YiCadHostApi, registerCommand, message);
YICAD_ABI_FIELD_FOLLOWS(YiCadHostApi, registerRibbonButton, registerCommand);
YICAD_ABI_FIELD_FOLLOWS(YiCadHostApi, currentDocument, registerRibbonButton);
YICAD_ABI_FIELD_FOLLOWS(YiCadHostApi, documentAddLine, currentDocument);
YICAD_ABI_FIELD_FOLLOWS(YiCadHostApi, documentAddCircle, documentAddLine);
YICAD_ABI_FIELD_FOLLOWS(YiCadHostApi, documentRegen, documentAddCircle);
YICAD_ABI_FIELD_FOLLOWS(YiCadHostApi, documentZoomAuto, documentRegen);
YICAD_ABI_FIELD_FOLLOWS(YiCadHostApi, registerImportFilter, documentZoomAuto);
YICAD_ABI_FIELD_FOLLOWS(
    YiCadHostApi,
    registerExportFilter,
    registerImportFilter);
YICAD_ABI_FIELD_FOLLOWS(
    YiCadHostApi,
    documentBeginTransaction,
    registerExportFilter);
YICAD_ABI_FIELD_FOLLOWS(
    YiCadHostApi,
    documentCommitTransaction,
    documentBeginTransaction);
YICAD_ABI_FIELD_FOLLOWS(
    YiCadHostApi,
    documentRollbackTransaction,
    documentCommitTransaction);
YICAD_ABI_FIELD_FOLLOWS(
    YiCadHostApi,
    documentCreateEntityIterator,
    documentRollbackTransaction);
YICAD_ABI_FIELD_FOLLOWS(
    YiCadHostApi,
    entityIteratorNext,
    documentCreateEntityIterator);
YICAD_ABI_FIELD_FOLLOWS(
    YiCadHostApi,
    entityIteratorGetLine,
    entityIteratorNext);
YICAD_ABI_FIELD_FOLLOWS(
    YiCadHostApi,
    entityIteratorGetCircle,
    entityIteratorGetLine);
YICAD_ABI_FIELD_FOLLOWS(
    YiCadHostApi,
    entityIteratorDestroy,
    entityIteratorGetCircle);
YICAD_ABI_FIELD_FOLLOWS(YiCadHostApi, importApi, entityIteratorDestroy);
YICAD_ABI_FIELD_FOLLOWS(YiCadHostApi, readApi, importApi);
YICAD_ABI_FIELD_FOLLOWS(YiCadHostApi, entityApi, readApi);
YICAD_ABI_STATIC_ASSERT(
    offsetof(YiCadHostApi, importApi) == YICAD_HOST_API_V2_SIZE,
    "ABI v3 must append only one aligned host-table pointer");
YICAD_ABI_STATIC_ASSERT(
    YICAD_HOST_API_V3_SIZE ==
    YICAD_HOST_API_V2_SIZE + 2 * sizeof(void*),
    "unexpected ABI v3 host-table growth");
YICAD_ABI_STATIC_ASSERT(
    YICAD_HOST_API_V4_SIZE ==
    YICAD_HOST_API_V3_SIZE + sizeof(void*),
    "unexpected ABI v4 host-table growth");
YICAD_ABI_STATIC_ASSERT(
    sizeof(YiCadImportApi) == YICAD_IMPORT_API_V4_SIZE,
    "ABI v4 import table has unexpected tail padding");
YICAD_ABI_STATIC_ASSERT(
    sizeof(YiCadHostApi) == YICAD_HOST_API_V4_SIZE,
    "YiCadHostApi must match the ABI v4 snapshot");

YICAD_ABI_STATIC_ASSERT(
    offsetof(YiCadPluginApi, structSize) == 0,
    "YiCadPluginApi.structSize must be first");
YICAD_ABI_STATIC_ASSERT(
    offsetof(YiCadPluginApi, abiVersion) == sizeof(uint32_t),
    "YiCadPluginApi.abiVersion must be second");
YICAD_ABI_FIELD_FOLLOWS(YiCadPluginApi, pluginId, abiVersion);
YICAD_ABI_FIELD_FOLLOWS(YiCadPluginApi, pluginName, pluginId);
YICAD_ABI_FIELD_FOLLOWS(YiCadPluginApi, pluginVersion, pluginName);
YICAD_ABI_STATIC_ASSERT(
    sizeof(YiCadPluginApi) >=
        offsetof(YiCadPluginApi, pluginVersion) +
            sizeof(((YiCadPluginApi*)0)->pluginVersion),
    "YiCadPluginApi must contain its final field");

#if defined(_WIN32) && UINTPTR_MAX == UINT64_MAX
YICAD_ABI_STATIC_ASSERT(
    sizeof(YiCadHostApi) >= YICAD_HOST_API_V1_SIZE,
    "Win64 host ABI lost its v1 prefix");
YICAD_ABI_STATIC_ASSERT(
    YICAD_HOST_API_V1_SIZE == 88,
    "unexpected Win64 host ABI v1 size");
YICAD_ABI_STATIC_ASSERT(
    YICAD_HOST_API_V2_SIZE == 152,
    "unexpected Win64 host ABI v2 size");
YICAD_ABI_STATIC_ASSERT(
    YICAD_ABI_ALIGNOF(YiCadHostApi) == 8,
    "unexpected Win64 host ABI alignment");
YICAD_ABI_FIELD_AT(YiCadHostApi, message, 8);
YICAD_ABI_FIELD_AT(YiCadHostApi, registerCommand, 16);
YICAD_ABI_FIELD_AT(YiCadHostApi, registerRibbonButton, 24);
YICAD_ABI_FIELD_AT(YiCadHostApi, currentDocument, 32);
YICAD_ABI_FIELD_AT(YiCadHostApi, documentAddLine, 40);
YICAD_ABI_FIELD_AT(YiCadHostApi, documentAddCircle, 48);
YICAD_ABI_FIELD_AT(YiCadHostApi, documentRegen, 56);
YICAD_ABI_FIELD_AT(YiCadHostApi, documentZoomAuto, 64);
YICAD_ABI_FIELD_AT(YiCadHostApi, registerImportFilter, 72);
YICAD_ABI_FIELD_AT(YiCadHostApi, registerExportFilter, 80);
YICAD_ABI_FIELD_AT(YiCadHostApi, documentBeginTransaction, 88);
YICAD_ABI_FIELD_AT(YiCadHostApi, documentCommitTransaction, 96);
YICAD_ABI_FIELD_AT(YiCadHostApi, documentRollbackTransaction, 104);
YICAD_ABI_FIELD_AT(YiCadHostApi, documentCreateEntityIterator, 112);
YICAD_ABI_FIELD_AT(YiCadHostApi, entityIteratorNext, 120);
YICAD_ABI_FIELD_AT(YiCadHostApi, entityIteratorGetLine, 128);
YICAD_ABI_FIELD_AT(YiCadHostApi, entityIteratorGetCircle, 136);
YICAD_ABI_FIELD_AT(YiCadHostApi, entityIteratorDestroy, 144);
YICAD_ABI_STATIC_ASSERT(
    YICAD_HOST_API_V3_SIZE == 168,
    "unexpected Win64 host ABI v3 size");
YICAD_ABI_FIELD_AT(YiCadHostApi, importApi, 152);
YICAD_ABI_FIELD_AT(YiCadHostApi, readApi, 160);
YICAD_ABI_STATIC_ASSERT(
    YICAD_HOST_API_V4_SIZE == 176,
    "unexpected Win64 host ABI v4 size");
YICAD_ABI_FIELD_AT(YiCadHostApi, entityApi, 168);
YICAD_ABI_STATIC_ASSERT(
    YICAD_IMPORT_API_V3_SIZE == 256,
    "unexpected Win64 import ABI v3 size");
YICAD_ABI_STATIC_ASSERT(
    sizeof(YiCadImportApi) == 272,
    "unexpected Win64 import ABI v4 size");
YICAD_ABI_STATIC_ASSERT(
    YICAD_ABI_ALIGNOF(YiCadImportApi) == 8,
    "unexpected Win64 import ABI v3 alignment");
YICAD_ABI_STATIC_ASSERT(
    sizeof(YiCadPluginApi) >= YICAD_PLUGIN_API_V1_SIZE,
    "Win64 plugin ABI lost its v1 prefix");
YICAD_ABI_STATIC_ASSERT(
    YICAD_PLUGIN_API_V1_SIZE == 32,
    "unexpected Win64 plugin ABI v1 size");
YICAD_ABI_STATIC_ASSERT(
    YICAD_ABI_ALIGNOF(YiCadPluginApi) == 8,
    "unexpected Win64 plugin ABI alignment");
YICAD_ABI_FIELD_AT(YiCadPluginApi, pluginId, 8);
YICAD_ABI_FIELD_AT(YiCadPluginApi, pluginName, 16);
YICAD_ABI_FIELD_AT(YiCadPluginApi, pluginVersion, 24);
#elif defined(_WIN32) && UINTPTR_MAX == UINT32_MAX
YICAD_ABI_STATIC_ASSERT(
    sizeof(YiCadHostApi) >= YICAD_HOST_API_V1_SIZE,
    "Win32 host ABI lost its v1 prefix");
YICAD_ABI_STATIC_ASSERT(
    YICAD_HOST_API_V1_SIZE == 48,
    "unexpected Win32 host ABI v1 size");
YICAD_ABI_STATIC_ASSERT(
    YICAD_HOST_API_V2_SIZE == 80,
    "unexpected Win32 host ABI v2 size");
YICAD_ABI_STATIC_ASSERT(
    YICAD_ABI_ALIGNOF(YiCadHostApi) == 4,
    "unexpected Win32 host ABI alignment");
YICAD_ABI_FIELD_AT(YiCadHostApi, message, 8);
YICAD_ABI_FIELD_AT(YiCadHostApi, registerCommand, 12);
YICAD_ABI_FIELD_AT(YiCadHostApi, registerRibbonButton, 16);
YICAD_ABI_FIELD_AT(YiCadHostApi, currentDocument, 20);
YICAD_ABI_FIELD_AT(YiCadHostApi, documentAddLine, 24);
YICAD_ABI_FIELD_AT(YiCadHostApi, documentAddCircle, 28);
YICAD_ABI_FIELD_AT(YiCadHostApi, documentRegen, 32);
YICAD_ABI_FIELD_AT(YiCadHostApi, documentZoomAuto, 36);
YICAD_ABI_FIELD_AT(YiCadHostApi, registerImportFilter, 40);
YICAD_ABI_FIELD_AT(YiCadHostApi, registerExportFilter, 44);
YICAD_ABI_FIELD_AT(YiCadHostApi, documentBeginTransaction, 48);
YICAD_ABI_FIELD_AT(YiCadHostApi, documentCommitTransaction, 52);
YICAD_ABI_FIELD_AT(YiCadHostApi, documentRollbackTransaction, 56);
YICAD_ABI_FIELD_AT(YiCadHostApi, documentCreateEntityIterator, 60);
YICAD_ABI_FIELD_AT(YiCadHostApi, entityIteratorNext, 64);
YICAD_ABI_FIELD_AT(YiCadHostApi, entityIteratorGetLine, 68);
YICAD_ABI_FIELD_AT(YiCadHostApi, entityIteratorGetCircle, 72);
YICAD_ABI_FIELD_AT(YiCadHostApi, entityIteratorDestroy, 76);
YICAD_ABI_STATIC_ASSERT(
    YICAD_HOST_API_V3_SIZE == 88,
    "unexpected Win32 host ABI v3 size");
YICAD_ABI_FIELD_AT(YiCadHostApi, importApi, 80);
YICAD_ABI_FIELD_AT(YiCadHostApi, readApi, 84);
YICAD_ABI_STATIC_ASSERT(
    YICAD_HOST_API_V4_SIZE == 92,
    "unexpected Win32 host ABI v4 size");
YICAD_ABI_FIELD_AT(YiCadHostApi, entityApi, 88);
YICAD_ABI_STATIC_ASSERT(
    YICAD_IMPORT_API_V3_SIZE == 132,
    "unexpected Win32 import ABI v3 size");
YICAD_ABI_STATIC_ASSERT(
    sizeof(YiCadImportApi) == 140,
    "unexpected Win32 import ABI v4 size");
YICAD_ABI_STATIC_ASSERT(
    YICAD_ABI_ALIGNOF(YiCadImportApi) == 4,
    "unexpected Win32 import ABI v3 alignment");
YICAD_ABI_STATIC_ASSERT(
    sizeof(YiCadPluginApi) >= YICAD_PLUGIN_API_V1_SIZE,
    "Win32 plugin ABI lost its v1 prefix");
YICAD_ABI_STATIC_ASSERT(
    YICAD_PLUGIN_API_V1_SIZE == 20,
    "unexpected Win32 plugin ABI v1 size");
YICAD_ABI_STATIC_ASSERT(
    YICAD_ABI_ALIGNOF(YiCadPluginApi) == 4,
    "unexpected Win32 plugin ABI alignment");
YICAD_ABI_FIELD_AT(YiCadPluginApi, pluginId, 8);
YICAD_ABI_FIELD_AT(YiCadPluginApi, pluginName, 12);
YICAD_ABI_FIELD_AT(YiCadPluginApi, pluginVersion, 16);
#endif

#if defined(__cplusplus)
namespace yicad_plugin_abi_detail
{

template<typename Left, typename Right>
struct IsSame
{
    enum
    {
        value = 0
    };
};

template<typename Type>
struct IsSame<Type, Type>
{
    enum
    {
        value = 1
    };
};

} // namespace yicad_plugin_abi_detail

#define YICAD_ABI_FUNCTION_FIELD_TYPE(type, field, functionType)          \
    YICAD_ABI_STATIC_ASSERT(                                               \
        (yicad_plugin_abi_detail::IsSame<                                  \
            decltype(((type*)0)->field), functionType>::value),            \
        #type "." #field " function type changed")

YICAD_ABI_STATIC_ASSERT(
    __is_standard_layout(YiCadHostApi),
    "YiCadHostApi must have standard layout");
YICAD_ABI_STATIC_ASSERT(
    __is_standard_layout(YiCadPluginApi),
    "YiCadPluginApi must have standard layout");
YICAD_ABI_STATIC_ASSERT(
    __is_standard_layout(YiCadImportApi),
    "YiCadImportApi must have standard layout");
YICAD_ABI_FUNCTION_FIELD_TYPE(YiCadHostApi, message, YiCadMessageFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadHostApi, registerCommand, YiCadRegisterCommandFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadHostApi, registerRibbonButton, YiCadRegisterRibbonButtonFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadHostApi, currentDocument, YiCadCurrentDocumentFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadHostApi, documentAddLine, YiCadDocumentAddLineFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadHostApi, documentAddCircle, YiCadDocumentAddCircleFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadHostApi, documentRegen, YiCadDocumentRegenFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadHostApi, documentZoomAuto, YiCadDocumentZoomAutoFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadHostApi, registerImportFilter, YiCadRegisterImportFilterFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadHostApi, registerExportFilter, YiCadRegisterExportFilterFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadHostApi, documentBeginTransaction,
    YiCadDocumentBeginTransactionFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadHostApi, documentCommitTransaction,
    YiCadDocumentCommitTransactionFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadHostApi, documentRollbackTransaction,
    YiCadDocumentRollbackTransactionFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadHostApi, documentCreateEntityIterator,
    YiCadDocumentCreateEntityIteratorFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadHostApi, entityIteratorNext, YiCadEntityIteratorNextFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadHostApi, entityIteratorGetLine, YiCadEntityIteratorGetLineFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadHostApi, entityIteratorGetCircle, YiCadEntityIteratorGetCircleFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadHostApi, entityIteratorDestroy, YiCadEntityIteratorDestroyFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(YiCadImportApi, beginImport, YiCadImportBeginFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(YiCadImportApi, commitImport, YiCadImportCommitFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, rollbackImport, YiCadImportRollbackFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, getLastError, YiCadImportGetLastErrorFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, setDocumentSettings, YiCadImportSetDocumentSettingsFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createLineType, YiCadImportCreateLineTypeFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createLayer, YiCadImportCreateLayerFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createTextStyle, YiCadImportCreateTextStyleFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createDimensionStyle,
    YiCadImportCreateDimensionStyleFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, getModelSpace, YiCadImportGetModelSpaceFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(YiCadImportApi, createPoint, YiCadImportCreatePointFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(YiCadImportApi, createLine, YiCadImportCreateLineFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(YiCadImportApi, createRay, YiCadImportCreateRayFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(YiCadImportApi, createXLine, YiCadImportCreateXLineFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(YiCadImportApi, createArc, YiCadImportCreateArcFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createCircle, YiCadImportCreateCircleFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createEllipse, YiCadImportCreateEllipseFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createPolyline, YiCadImportCreatePolylineFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createSpline, YiCadImportCreateSplineFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createSolid, YiCadImportCreateSolidFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(YiCadImportApi, createText, YiCadImportCreateTextFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createMText, YiCadImportCreateMTextFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(YiCadImportApi, beginBlock, YiCadImportBeginBlockFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(YiCadImportApi, endBlock, YiCadImportEndBlockFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createInsert, YiCadImportCreateInsertFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createAttributeDefinition,
    YiCadImportCreateAttributeDefinitionFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createAttribute, YiCadImportCreateAttributeFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createDimension, YiCadImportCreateDimensionFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createLeader, YiCadImportCreateLeaderFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createHatch, YiCadImportCreateHatchFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createImage, YiCadImportCreateImageFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createCustomEntity, YiCadImportCreateCustomEntityFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, beginProxyGraphics, YiCadImportBeginProxyGraphicsFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadReadApi, entityGraphics, YiCadReadEntityGraphicsFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadEntityApiV4, registerEntityClass, YiCadRegisterEntityClassFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadEntityApiV4, createCustomEntity, YiCadEntityCreateCustomFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadEntityApiV4, setCustomEntityData, YiCadEntitySetCustomDataFn);
YICAD_ABI_STATIC_ASSERT(
    (yicad_plugin_abi_detail::IsSame<
        decltype(&yicad_plugin_get_abi_version),
        YiCadPluginGetAbiVersionFn>::value),
    "ABI version entry point signature changed");
YICAD_ABI_STATIC_ASSERT(
    (yicad_plugin_abi_detail::IsSame<
        decltype(&yicad_plugin_init),
        YiCadPluginInitFn>::value),
    "plugin init entry point signature changed");
YICAD_ABI_STATIC_ASSERT(
    (yicad_plugin_abi_detail::IsSame<
        decltype(&yicad_plugin_shutdown),
        YiCadPluginShutdownFn>::value),
    "plugin shutdown entry point signature changed");
#undef YICAD_ABI_FUNCTION_FIELD_TYPE
#else
#define YICAD_ABI_FUNCTION_FIELD_TYPE(type, field, functionType)          \
    YICAD_ABI_STATIC_ASSERT(                                               \
        _Generic(((type*)0)->field, functionType: 1, default: 0),          \
        #type "." #field " function type changed")
YICAD_ABI_FUNCTION_FIELD_TYPE(YiCadHostApi, message, YiCadMessageFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadHostApi, registerCommand, YiCadRegisterCommandFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadHostApi, registerRibbonButton, YiCadRegisterRibbonButtonFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadHostApi, currentDocument, YiCadCurrentDocumentFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadHostApi, documentAddLine, YiCadDocumentAddLineFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadHostApi, documentAddCircle, YiCadDocumentAddCircleFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadHostApi, documentRegen, YiCadDocumentRegenFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadHostApi, documentZoomAuto, YiCadDocumentZoomAutoFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadHostApi, registerImportFilter, YiCadRegisterImportFilterFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadHostApi, registerExportFilter, YiCadRegisterExportFilterFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadHostApi, documentBeginTransaction,
    YiCadDocumentBeginTransactionFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadHostApi, documentCommitTransaction,
    YiCadDocumentCommitTransactionFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadHostApi, documentRollbackTransaction,
    YiCadDocumentRollbackTransactionFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadHostApi, documentCreateEntityIterator,
    YiCadDocumentCreateEntityIteratorFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadHostApi, entityIteratorNext, YiCadEntityIteratorNextFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadHostApi, entityIteratorGetLine, YiCadEntityIteratorGetLineFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadHostApi, entityIteratorGetCircle, YiCadEntityIteratorGetCircleFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadHostApi, entityIteratorDestroy, YiCadEntityIteratorDestroyFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(YiCadImportApi, beginImport, YiCadImportBeginFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(YiCadImportApi, commitImport, YiCadImportCommitFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, rollbackImport, YiCadImportRollbackFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, getLastError, YiCadImportGetLastErrorFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, setDocumentSettings, YiCadImportSetDocumentSettingsFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createLineType, YiCadImportCreateLineTypeFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createLayer, YiCadImportCreateLayerFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createTextStyle, YiCadImportCreateTextStyleFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createDimensionStyle,
    YiCadImportCreateDimensionStyleFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, getModelSpace, YiCadImportGetModelSpaceFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(YiCadImportApi, createPoint, YiCadImportCreatePointFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(YiCadImportApi, createLine, YiCadImportCreateLineFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(YiCadImportApi, createRay, YiCadImportCreateRayFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(YiCadImportApi, createXLine, YiCadImportCreateXLineFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(YiCadImportApi, createArc, YiCadImportCreateArcFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createCircle, YiCadImportCreateCircleFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createEllipse, YiCadImportCreateEllipseFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createPolyline, YiCadImportCreatePolylineFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createSpline, YiCadImportCreateSplineFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createSolid, YiCadImportCreateSolidFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(YiCadImportApi, createText, YiCadImportCreateTextFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createMText, YiCadImportCreateMTextFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(YiCadImportApi, beginBlock, YiCadImportBeginBlockFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(YiCadImportApi, endBlock, YiCadImportEndBlockFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createInsert, YiCadImportCreateInsertFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createAttributeDefinition,
    YiCadImportCreateAttributeDefinitionFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createAttribute, YiCadImportCreateAttributeFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createDimension, YiCadImportCreateDimensionFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createLeader, YiCadImportCreateLeaderFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createHatch, YiCadImportCreateHatchFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createImage, YiCadImportCreateImageFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, createCustomEntity, YiCadImportCreateCustomEntityFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadImportApi, beginProxyGraphics, YiCadImportBeginProxyGraphicsFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadReadApi, entityGraphics, YiCadReadEntityGraphicsFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadEntityApiV4, registerEntityClass, YiCadRegisterEntityClassFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadEntityApiV4, createCustomEntity, YiCadEntityCreateCustomFn);
YICAD_ABI_FUNCTION_FIELD_TYPE(
    YiCadEntityApiV4, setCustomEntityData, YiCadEntitySetCustomDataFn);
YICAD_ABI_STATIC_ASSERT(
    _Generic(
        &yicad_plugin_get_abi_version,
        YiCadPluginGetAbiVersionFn: 1,
        default: 0),
    "ABI version entry point signature changed");
YICAD_ABI_STATIC_ASSERT(
    _Generic(&yicad_plugin_init, YiCadPluginInitFn: 1, default: 0),
    "plugin init entry point signature changed");
YICAD_ABI_STATIC_ASSERT(
    _Generic(&yicad_plugin_shutdown, YiCadPluginShutdownFn: 1, default: 0),
    "plugin shutdown entry point signature changed");
#undef YICAD_ABI_FUNCTION_FIELD_TYPE
#endif

#undef YICAD_ABI_FIELD_FOLLOWS
#undef YICAD_ABI_FIELD_AT
#undef YICAD_ABI_MIN_SIZE_IS
#undef YICAD_ABI_ALIGNOF
#undef YICAD_ABI_STATIC_ASSERT

#endif
