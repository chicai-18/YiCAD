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
/** @brief 所属导入会话内有效的资源句柄。 */
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

typedef struct YiCadPoint2d
{
    double x;
    double y;
} YiCadPoint2d;

typedef struct YiCadPoint3d
{
    double x;
    double y;
    double z;
} YiCadPoint3d;

typedef YiCadPoint2d YiCadVector2d;
typedef YiCadPoint3d YiCadVector3d;

typedef int32_t YiCadColorMethod;
#define YICAD_COLOR_BY_LAYER ((YiCadColorMethod)0)
#define YICAD_COLOR_BY_BLOCK ((YiCadColorMethod)1)
#define YICAD_COLOR_ACI ((YiCadColorMethod)2)
#define YICAD_COLOR_RGB ((YiCadColorMethod)3)

/** @brief 支持随层、随块和 RGB；ACI 输入由宿主转换为 RGB。 */
typedef struct YiCadColorData
{
    uint32_t structSize;
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

/** @brief 文档设置；代码页仅作为源文件元数据保存。 */
typedef struct YiCadDocumentSettings
{
    uint32_t structSize;
    int32_t insertionUnits;
    int32_t measurement;
    double globalLineTypeScale;
    YiCadStringView sourceCodePage;
} YiCadDocumentSettings;

/** @brief 简单线型定义；complex 非零时宿主必须明确拒绝。 */
typedef struct YiCadLineTypeDataV3
{
    uint32_t structSize;
    YiCadStringView name;
    YiCadStringView description;
    YiCadDoubleArrayView elements;
    uint32_t complex;
} YiCadLineTypeDataV3;

/** @brief 图层定义。frozen 非零表示不可见，lineType 为空时使用 Continuous。 */
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

/** @brief 文字样式定义；字体文件原名在字体缺失时仍会保存。 */
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

/** @brief YiCAD 当前可表达的标注样式字段。 */
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

/** @brief 后续实体创建接口共用的公共属性。 */
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
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportSetDocumentSettingsFn)(
    YiCadImportSessionHandle session,
    const YiCadDocumentSettings* settings);
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportCreateLineTypeFn)(
    YiCadImportSessionHandle session,
    const YiCadLineTypeDataV3* data,
    YiCadResourceConflictPolicy conflictPolicy,
    YiCadImportResourceHandle* resource);
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportCreateLayerFn)(
    YiCadImportSessionHandle session,
    const YiCadLayerDataV3* data,
    YiCadResourceConflictPolicy conflictPolicy,
    YiCadImportResourceHandle* resource);
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportCreateTextStyleFn)(
    YiCadImportSessionHandle session,
    const YiCadTextStyleDataV3* data,
    YiCadResourceConflictPolicy conflictPolicy,
    YiCadImportResourceHandle* resource);
typedef YiCadImportResult (YICAD_PLUGIN_CALL *YiCadImportCreateDimensionStyleFn)(
    YiCadImportSessionHandle session,
    const YiCadDimensionStyleDataV3* data,
    YiCadResourceConflictPolicy conflictPolicy,
    YiCadImportResourceHandle* resource);

/** @brief 未发布的 ABI v3 导入子函数表草案。 */
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
    ((uint32_t)(offsetof(YiCadImportApi, createDimensionStyle) +           \
                sizeof(((YiCadImportApi*)0)->createDimensionStyle)))
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
#undef YICAD_ABI_ALIGNOF
#undef YICAD_ABI_STATIC_ASSERT

#endif
