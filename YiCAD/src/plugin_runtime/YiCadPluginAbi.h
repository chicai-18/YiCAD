#ifndef YICAD_PLUGIN_ABI_H
#define YICAD_PLUGIN_ABI_H

#include <stddef.h>
#include <stdint.h>

#if !defined(__cplusplus) && \
    (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 201112L)
#error "YiCadPluginAbi.h requires C11 or later"
#endif

/** @brief 冻结的 C ABI v1 版本号。 */
#define YICAD_PLUGIN_ABI_V1 UINT32_C(1)
/** @brief 文档事务与只读实体枚举 C ABI 版本号。 */
#define YICAD_PLUGIN_ABI_V2 UINT32_C(2)
#if defined(YICAD_ENABLE_PLUGIN_ABI_V3_DRAFT)
/** @brief 未发布的导入会话 C ABI v3 草案版本号。 */
#define YICAD_PLUGIN_ABI_V3_DRAFT UINT32_C(3)
#endif
/** @brief 当前 SDK 支持的最低 C ABI 版本。 */
#define YICAD_PLUGIN_ABI_MIN_VERSION YICAD_PLUGIN_ABI_V1
/** @brief 当前 SDK 支持的最高 C ABI 版本。 */
#define YICAD_PLUGIN_ABI_MAX_VERSION YICAD_PLUGIN_ABI_V2
/** @brief 当前 C ABI 版本；保留该名称以兼容 ABI v1 插件源码。 */
#define YICAD_PLUGIN_ABI_VERSION YICAD_PLUGIN_ABI_MAX_VERSION

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

#if defined(YICAD_ENABLE_PLUGIN_ABI_V3_DRAFT)
/** @brief 宿主持有的导入会话句柄，提交或回滚后立即失效。 */
typedef void* YiCadImportSessionHandle;
/** @brief 所属导入会话内有效的容器句柄。 */
typedef void* YiCadImportContainerHandle;
/** @brief 所属导入会话内有效的资源、块定义或块引用句柄。 */
typedef void* YiCadImportResourceHandle;

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
 * @brief ABI v3 草案输入结构的共同约定。
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
 * globalLineTypeScale 必须大于 0；sourceCodePage 可为空。
 */
typedef struct YiCadDocumentSettings
{
    uint32_t structSize;
    int32_t insertionUnits;
    int32_t measurement;
    double globalLineTypeScale;
    YiCadStringView sourceCodePage;
} YiCadDocumentSettings;

/**
 * @brief 简单线型定义；名称非空，elements 至少包含一个有限值。
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
 * ByLayer；visible 为 0 或 1；当前二维模型要求 lineTypeScale 为 1，normal 为
 * 正 Z 轴。
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
 * @brief ABI v3 草案可扩展输入的当前最小必需前缀。
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
#endif

typedef int32_t YiCadEntityType;

#define YICAD_ENTITY_UNKNOWN ((YiCadEntityType)0)
#define YICAD_ENTITY_LINE ((YiCadEntityType)1)
#define YICAD_ENTITY_CIRCLE ((YiCadEntityType)2)

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

typedef void (YICAD_PLUGIN_CALL *YiCadCommandCallback)(void* userData);
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadImportCallback)(
    YiCadDocumentHandle document,
    const char* filePath,
    void* userData);
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadExportCallback)(
    YiCadDocumentHandle document,
    const char* filePath,
    void* userData);

typedef void (YICAD_PLUGIN_CALL *YiCadMessageFn)(const char* text);
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadRegisterCommandFn)(
    const char* pluginId,
    const char* commandId,
    const char* displayName,
    YiCadCommandCallback callback,
    void* userData);
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadRegisterRibbonButtonFn)(
    const char* pluginId,
    const char* tab,
    const char* group,
    const char* commandId,
    const char* iconPath);
typedef YiCadDocumentHandle (YICAD_PLUGIN_CALL *YiCadCurrentDocumentFn)(void);
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadDocumentAddLineFn)(
    YiCadDocumentHandle document,
    double x1,
    double y1,
    double x2,
    double y2);
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadDocumentAddCircleFn)(
    YiCadDocumentHandle document,
    double centerX,
    double centerY,
    double radius);
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadDocumentRegenFn)(
    YiCadDocumentHandle document);
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadDocumentZoomAutoFn)(
    YiCadDocumentHandle document);
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadRegisterImportFilterFn)(
    const char* pluginId,
    const char* formatId,
    const char* displayName,
    const char* extension,
    YiCadImportCallback callback,
    void* userData);
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadRegisterExportFilterFn)(
    const char* pluginId,
    const char* formatId,
    const char* displayName,
    const char* extension,
    YiCadExportCallback callback,
    void* userData);
typedef YiCadTransactionHandle (YICAD_PLUGIN_CALL *YiCadDocumentBeginTransactionFn)(
    YiCadDocumentHandle document,
    const char* name);
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadDocumentCommitTransactionFn)(
    YiCadTransactionHandle transaction);
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadDocumentRollbackTransactionFn)(
    YiCadTransactionHandle transaction);
typedef YiCadEntityIteratorHandle (YICAD_PLUGIN_CALL *YiCadDocumentCreateEntityIteratorFn)(
    YiCadDocumentHandle document);
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadEntityIteratorNextFn)(
    YiCadEntityIteratorHandle iterator,
    YiCadEntityType* entityType);
/* next 返回失败表示已到末尾或句柄/参数无效，之后没有当前实体。 */
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadEntityIteratorGetLineFn)(
    YiCadEntityIteratorHandle iterator,
    YiCadLineData* line);
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadEntityIteratorGetCircleFn)(
    YiCadEntityIteratorHandle iterator,
    YiCadCircleData* circle);
typedef void (YICAD_PLUGIN_CALL *YiCadEntityIteratorDestroyFn)(
    YiCadEntityIteratorHandle iterator);

#if defined(YICAD_ENABLE_PLUGIN_ABI_V3_DRAFT)
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
 * @brief 未发布的 ABI v3 导入子函数表草案。
 * @note 宿主持有本表，其生命周期与 YiCadHostApi 相同。插件读取任何函数指针前
 * 必须同时检查 abiVersion、structSize 和指针；缺失尾字段表示该能力不支持。
 * 字段顺序是当前候选顺序，只能在尾部追加。
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
};
#endif

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
#if defined(YICAD_ENABLE_PLUGIN_ABI_V3_DRAFT)
    /** @brief ABI v3 草案导入子表；其生命周期与宿主表相同。 */
    const YiCadImportApi* importApi;
#endif
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
#if defined(YICAD_ENABLE_PLUGIN_ABI_V3_DRAFT)
/** @brief ABI v3 草案宿主表的当前可访问字节数。 */
#define YICAD_HOST_API_V3_DRAFT_SIZE                                      \
    ((uint32_t)(offsetof(YiCadHostApi, importApi) +                        \
                sizeof(((YiCadHostApi*)0)->importApi)))
/** @brief ABI v3 草案导入子表的当前可访问字节数。 */
#define YICAD_IMPORT_API_V3_DRAFT_SIZE                                    \
    ((uint32_t)(offsetof(YiCadImportApi, createImage) +                    \
                sizeof(((YiCadImportApi*)0)->createImage)))
#endif

typedef uint32_t (YICAD_PLUGIN_CALL *YiCadPluginGetAbiVersionFn)(void);
typedef YiCadResult (YICAD_PLUGIN_CALL *YiCadPluginInitFn)(
    const YiCadHostApi* host,
    YiCadPluginApi* plugin);
typedef void (YICAD_PLUGIN_CALL *YiCadPluginShutdownFn)(void);

/** @brief 返回插件实现的最高 C ABI 版本。 */
YICAD_PLUGIN_API uint32_t YICAD_PLUGIN_CALL
yicad_plugin_get_abi_version(void);
YICAD_PLUGIN_API YiCadResult YICAD_PLUGIN_CALL
yicad_plugin_init(const YiCadHostApi* host, YiCadPluginApi* plugin);
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
#if defined(YICAD_ENABLE_PLUGIN_ABI_V3_DRAFT)
YICAD_ABI_STATIC_ASSERT(
    YICAD_PLUGIN_ABI_V3_DRAFT == UINT32_C(3),
    "ABI v3 draft version snapshot changed");
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
YICAD_ABI_FIELD_FOLLOWS(YiCadImportApi, createText, createSpline);
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
#endif
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
#if defined(YICAD_ENABLE_PLUGIN_ABI_V3_DRAFT)
YICAD_ABI_FIELD_FOLLOWS(YiCadHostApi, importApi, entityIteratorDestroy);
YICAD_ABI_STATIC_ASSERT(
    offsetof(YiCadHostApi, importApi) == YICAD_HOST_API_V2_SIZE,
    "ABI v3 draft must append only one aligned host-table pointer");
YICAD_ABI_STATIC_ASSERT(
    YICAD_HOST_API_V3_DRAFT_SIZE ==
        YICAD_HOST_API_V2_SIZE + sizeof(void*),
    "unexpected ABI v3 draft host-table growth");
#endif
YICAD_ABI_STATIC_ASSERT(
    sizeof(YiCadHostApi) >=
        offsetof(YiCadHostApi, entityIteratorDestroy) +
            sizeof(((YiCadHostApi*)0)->entityIteratorDestroy),
    "YiCadHostApi must contain its final field");

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

YICAD_ABI_STATIC_ASSERT(
    __is_standard_layout(YiCadHostApi),
    "YiCadHostApi must have standard layout");
YICAD_ABI_STATIC_ASSERT(
    __is_standard_layout(YiCadPluginApi),
    "YiCadPluginApi must have standard layout");
#if defined(YICAD_ENABLE_PLUGIN_ABI_V3_DRAFT)
YICAD_ABI_STATIC_ASSERT(
    __is_standard_layout(YiCadImportApi),
    "YiCadImportApi must have standard layout");
#endif
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
#else
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
#endif

#undef YICAD_ABI_FIELD_FOLLOWS
#undef YICAD_ABI_FIELD_AT
#undef YICAD_ABI_MIN_SIZE_IS
#undef YICAD_ABI_ALIGNOF
#undef YICAD_ABI_STATIC_ASSERT

#endif
