/// @file test_environment.cpp
/// @brief test_interaction 的全局环境：单测里不弹模态对话框
///
/// 扩展直接构造对话框，经 UIDialogRunner 运行（doc/ARCHITECTURE_EVOLUTION_PLAN.md 9.3 节）。
/// 测试进程有真实的窗口站（见 support/yicad_test_main.cpp），对话框真弹出来会卡住用例，
/// 所以整个测试程序期间一律视为取消；要知道弹出了哪个对话框，用例再装 DialogRecorder。

#include <gtest/gtest.h>

#include <QDialog>

#include "UIDialogRunner.h"

namespace
{
class RejectDialogsEnvironment : public ::testing::Environment
{
public:
    void SetUp() override
    {
        UIDialogRunner::setRunner([](QDialog&) { return int(QDialog::Rejected); });
    }

    void TearDown() override { UIDialogRunner::setRunner({}); }
};

::testing::Environment* const kRejectDialogs = ::testing::AddGlobalTestEnvironment(new RejectDialogsEnvironment);
}  // namespace
