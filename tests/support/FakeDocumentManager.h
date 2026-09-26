/// @file FakeDocumentManager.h
/// @brief 单测用的 IDocumentManager 替身：文档、视图与未命名文档的名字按预设返回，
///        文件操作只记录调用

#ifndef YICAD_TEST_FAKE_DOCUMENT_MANAGER_H
#define YICAD_TEST_FAKE_DOCUMENT_MANAGER_H

#include <map>
#include <vector>

#include <QString>
#include <QStringList>

#include "IDocumentManager.h"

namespace yicad_test
{
/// @brief IDocumentManager 的测试替身，见文件说明
class FakeDocumentManager : public IDocumentManager
{
public:
    DmDocument* current = nullptr;                        ///< currentDocument() 的返回值
    GuiDocumentView* currentView = nullptr;               ///< currentDocumentView() 的返回值
    std::vector<DmDocument*> open;                        ///< documents() 的返回值
    std::vector<GuiDocumentView*> views;                  ///< documentViews() 的返回值
    std::map<const DmDocument*, QString> untitledNames;   ///< untitledDocumentName() 按文档返回，没有时为空
    QStringList calls;                                    ///< 文件操作的调用记录，按调用顺序

    DmDocument* currentDocument() const override { return current; }
    GuiDocumentView* currentDocumentView() const override { return currentView; }
    std::vector<DmDocument*> documents() const override { return open; }
    std::vector<GuiDocumentView*> documentViews() const override { return views; }

    void newDocument() override { calls.append(QStringLiteral("new")); }
    void openDocument() override { calls.append(QStringLiteral("open")); }
    void saveDocument() override { calls.append(QStringLiteral("save")); }
    void saveDocumentAs() override { calls.append(QStringLiteral("saveAs")); }
    void exportImage() override { calls.append(QStringLiteral("exportImage")); }

    QString untitledDocumentName(const DmDocument* document) const override
    {
        const auto it = untitledNames.find(document);
        return it == untitledNames.end() ? QString() : it->second;
    }
};
}  // namespace yicad_test

#endif  // YICAD_TEST_FAKE_DOCUMENT_MANAGER_H
