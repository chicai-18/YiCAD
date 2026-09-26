/// @file test_persistence_filter_registry.cpp
/// @brief 文件格式注册表 FilterRegistry（分层重组 S4b）
///
/// 注册表取代了原 FileIO 的格式分派：原生格式在 DmSystem::init 时登记（测试入口
/// yicad_test_main.cpp 调了它），插件格式由壳层在插件加载后登记（test_dxf_encoding 覆盖）。
/// 这里核对原生格式的登记、登记顺序决定的优先级，以及登记与注销。

#include <gtest/gtest.h>

#include <memory>
#include <utility>
#include <vector>

#include "DmDocument.h"
#include "FilterInterface.h"
#include "FilterOcdIO.h"
#include "FilterRegistry.h"

namespace
{
const QString kOcdImportFilter = QStringLiteral("Drawing Exchange YCD (*.ycd)");
const QString kOcdFormat = QString::fromLatin1(DOCDEFAULTFORMAT);

/// @brief 测试用过滤器：认后缀为 suffix 的文件与格式名 format，不读写
class FakeFilter : public FilterInterface
{
public:
    FakeFilter(QString suffix, QString format)
        : m_suffix(std::move(suffix))
        , m_format(std::move(format))
    {
    }

    bool canImport(const QString& file) const override { return file.endsWith(QStringLiteral(".") + m_suffix); }
    bool canExport(const QString& type) const override { return type == m_format; }
    bool fileImport(DmDocument&, const QString&) override { return false; }
    bool fileExport(DmDocument&, const QString&, const QString&) override { return false; }

private:
    QString m_suffix;
    QString m_format;
};

/// @brief 用例期间的登记，析构时注销，不影响别的用例
struct Registrations
{
    std::vector<int> ids;

    ~Registrations()
    {
        for (int id : ids)
        {
            FilterRegistry::instance().remove(id);
        }
    }

    void addImport(const QString& nameFilter, const QString& suffix)
    {
        ids.push_back(FilterRegistry::instance().addImport(
            nameFilter, [suffix]() -> std::unique_ptr<FilterInterface> {
                return std::make_unique<FakeFilter>(suffix, QString());
            }));
    }

    void addExport(const QString& format, const QString& nameFilter)
    {
        ids.push_back(FilterRegistry::instance().addExport(
            format, nameFilter, [format]() -> std::unique_ptr<FilterInterface> {
                return std::make_unique<FakeFilter>(QString(), format);
            }));
    }
};

bool isOcd(const std::unique_ptr<FilterInterface>& filter)
{
    return dynamic_cast<FilterOcdIO*>(filter.get()) != nullptr;
}
}  // namespace

TEST(FilterRegistry, 原生格式在系统初始化时登记)
{
    const FilterRegistry& registry = FilterRegistry::instance();
    EXPECT_TRUE(isOcd(registry.importFilter(QStringLiteral("C:/tmp/drawing.ycd"))));
    EXPECT_TRUE(isOcd(registry.exportFilter(kOcdFormat)));
    EXPECT_EQ(registry.importNameFilters(), QStringList{kOcdImportFilter});
    EXPECT_EQ(registry.exportNameFilters(), QStringList{kOcdFormat});
    // 原生格式的过滤串就是格式名
    EXPECT_EQ(registry.exportFormatType(kOcdFormat), kOcdFormat);
}

TEST(FilterRegistry, 找不到时返回空且过滤串原样返回)
{
    const FilterRegistry& registry = FilterRegistry::instance();
    EXPECT_EQ(registry.importFilter(QStringLiteral("drawing.dxf")), nullptr);
    EXPECT_EQ(registry.exportFilter(QStringLiteral("AutoCAD DXF (*.dxf)")), nullptr);
    EXPECT_EQ(registry.exportFormatType(QStringLiteral("AutoCAD DXF (*.dxf)")), QStringLiteral("AutoCAD DXF (*.dxf)"));
    // 是否接得住由过滤器判断：FilterOcdIO::canImport 只认小写的 "ycd"，注册表不改变这一点
    EXPECT_EQ(registry.importFilter(QStringLiteral("drawing.YCD")), nullptr);
}

TEST(FilterRegistry, 登记的格式排在原生格式之后注销后消失)
{
    const QString importFilter = QStringLiteral("Fake drawing (*.fake)");
    const QString exportFilter = QStringLiteral("Fake drawing 2 (*.fake)");
    const QString format = QStringLiteral("com.example.fake/drawing");
    {
        Registrations registrations;
        registrations.addImport(importFilter, QStringLiteral("fake"));
        registrations.addExport(format, exportFilter);

        const FilterRegistry& registry = FilterRegistry::instance();
        EXPECT_EQ(registry.importNameFilters(), (QStringList{kOcdImportFilter, importFilter}));
        EXPECT_EQ(registry.exportNameFilters(), (QStringList{kOcdFormat, exportFilter}));
        EXPECT_NE(dynamic_cast<FakeFilter*>(registry.importFilter(QStringLiteral("a.fake")).get()), nullptr);
        EXPECT_NE(dynamic_cast<FakeFilter*>(registry.exportFilter(format).get()), nullptr);
        // 保存对话框选中的过滤串换成格式名
        EXPECT_EQ(registry.exportFormatType(exportFilter), format);
    }
    const FilterRegistry& registry = FilterRegistry::instance();
    EXPECT_EQ(registry.importFilter(QStringLiteral("a.fake")), nullptr);
    EXPECT_EQ(registry.exportFilter(format), nullptr);
    EXPECT_EQ(registry.importNameFilters(), QStringList{kOcdImportFilter});
    EXPECT_EQ(registry.exportNameFilters(), QStringList{kOcdFormat});
}

TEST(FilterRegistry, 同一后缀先登记的优先)
{
    // 插件声明 .ycd 也接不走原生格式：原生格式先登记（原 FileIO 也是先查内置格式）
    Registrations registrations;
    registrations.addImport(QStringLiteral("Another YCD (*.ycd)"), QStringLiteral("ycd"));
    EXPECT_TRUE(isOcd(FilterRegistry::instance().importFilter(QStringLiteral("drawing.ycd"))));
}

TEST(FilterRegistry, 注销不存在的登记号什么也不做)
{
    FilterRegistry::instance().remove(-1);
    EXPECT_EQ(FilterRegistry::instance().importNameFilters(), QStringList{kOcdImportFilter});
}
