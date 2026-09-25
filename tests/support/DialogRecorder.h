/// @file DialogRecorder.h
/// @brief 单测替身：扩展经 UIDialogRunner 运行的对话框不弹出，只记下类名并给出预设结果
///
/// test_interaction 的全局环境（interaction/test_environment.cpp）已让所有对话框视为取消，
/// 这里在此之上记录弹出的是哪个对话框；析构时恢复原先的运行方式。

#ifndef YICAD_TEST_DIALOG_RECORDER_H
#define YICAD_TEST_DIALOG_RECORDER_H

#include <functional>
#include <utility>
#include <vector>

#include <QDialog>
#include <QString>

#include "UIDialogRunner.h"

namespace yicad_test
{
/// @brief 装入时记录弹出的对话框；每个对话框在"弹出"时先交给 onShow（可填写控件），
///        再返回 result
class DialogRecorder
{
public:
    std::vector<QString> shown;             ///< 弹出过的对话框的类名，按顺序
    int result = QDialog::Rejected;         ///< 替身返回的结果
    std::function<void(QDialog&)> onShow;   ///< 返回结果前调用（可为空）

    DialogRecorder()
        : m_previous(UIDialogRunner::setRunner(
              [this](QDialog& dialog)
              {
                  shown.push_back(QString::fromLatin1(dialog.metaObject()->className()));
                  if (onShow)
                  {
                      onShow(dialog);
                  }
                  return result;
              }))
    {
    }

    ~DialogRecorder() { UIDialogRunner::setRunner(std::move(m_previous)); }

    DialogRecorder(const DialogRecorder&) = delete;
    DialogRecorder& operator=(const DialogRecorder&) = delete;

    /// @brief 最近弹出的对话框的类名；没有弹出过时为空
    QString last() const { return shown.empty() ? QString() : shown.back(); }

private:
    UIDialogRunner::Runner m_previous; ///< 装入前的运行方式
};
}  // namespace yicad_test

#endif  // YICAD_TEST_DIALOG_RECORDER_H
