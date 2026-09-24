/// @file test_coordinate_input.cpp
/// @brief 命令行坐标输入解析（GuiCoordinateInput）的单元测试
///
/// 解析逻辑从 GuiEventHandler::commandEvent 原样抽出
/// （doc/COMMAND_TOOL_MIGRATION_PLAN.md 第二步第 3 项），旧版 Action 栈与命令的
/// 工具共用。这里锁定四种格式与两种非坐标情形的现有行为。

#include <gtest/gtest.h>

#include <cmath>

#include "DmVector.h"
#include "GuiCoordinateInput.h"

namespace
{
constexpr double kEps = 1e-9;

void expectPosition(const GuiCoordinateInput& input, double x, double y)
{
    ASSERT_EQ(input.status, GuiCoordinateInput::Status::Ok);
    EXPECT_NEAR(input.position.x, x, kEps);
    EXPECT_NEAR(input.position.y, y, kEps);
}
}  // namespace

TEST(CoordinateInputTest, 绝对直角坐标)
{
    expectPosition(GuiCoordinateInput::parse("10,20", DmVector(100.0, 200.0)), 10.0, 20.0);
}

TEST(CoordinateInputTest, 相对直角坐标以相对零点为基准)
{
    expectPosition(GuiCoordinateInput::parse("@10,20", DmVector(100.0, 200.0)), 110.0, 220.0);
}

TEST(CoordinateInputTest, 绝对极坐标的角度单位为度)
{
    expectPosition(GuiCoordinateInput::parse("10<90", DmVector(100.0, 200.0)), 0.0, 10.0);
}

TEST(CoordinateInputTest, 相对极坐标以相对零点为基准)
{
    expectPosition(GuiCoordinateInput::parse("@10<0", DmVector(100.0, 200.0)), 110.0, 200.0);
}

TEST(CoordinateInputTest, 分量是表达式)
{
    expectPosition(GuiCoordinateInput::parse("1+2,3*4", DmVector(0.0, 0.0)), 3.0, 12.0);
}

TEST(CoordinateInputTest, 表达式求值失败是语法错误)
{
    EXPECT_EQ(GuiCoordinateInput::parse("abc,1", DmVector(0.0, 0.0)).status,
              GuiCoordinateInput::Status::SyntaxError);
    EXPECT_EQ(GuiCoordinateInput::parse("@1<", DmVector(0.0, 0.0)).status,
              GuiCoordinateInput::Status::SyntaxError);
}

TEST(CoordinateInputTest, 同时含逗号与尖括号时按直角坐标解析)
{
    // 按原先的分支顺序先认逗号。y 分量 "2<3" 在 muParser 里是比较表达式，
    // 值为 1，所以得到 (1, 1) 而不是语法错误。
    expectPosition(GuiCoordinateInput::parse("1,2<3", DmVector(0.0, 0.0)), 1.0, 1.0);
}

TEST(CoordinateInputTest, 不含逗号与尖括号的文本不是坐标)
{
    EXPECT_EQ(GuiCoordinateInput::parse("line", DmVector(0.0, 0.0)).status,
              GuiCoordinateInput::Status::NotCoordinate);
    EXPECT_EQ(GuiCoordinateInput::parse("12", DmVector(0.0, 0.0)).status,
              GuiCoordinateInput::Status::NotCoordinate);
}
