/// @file GiTextDump.h
/// @brief 把 GI 的输出写成文本，test_graphics 用它比对实体的 worldDraw 与 GI 流的重放
///
/// 每个调用一行；嵌套绘制写成 "draw {" 与 "}" 包住的一段，段里先是属性，"---" 之后是图元。
/// 数值保留 6 位有效数字，角度写成度。drawShared 与 glyphRun 不展开共享对象与字形，只写引用与变换。

#ifndef GITEXTDUMP_H
#define GITEXTDUMP_H

#include <string>

#include "IGiDrawable.h"

/// @brief 可绘制对象的属性与图元，写成文本
/// @details 顶层先写 setAttributes 设的属性，一行 "---"，再写 worldDraw 的输出
std::string giDump(const IGiDrawable& drawable);

/// @brief 只写 worldDraw 的输出（不含属性），便于只关心图元的用例
std::string giDumpGeometry(const IGiDrawable& drawable);

#endif // GITEXTDUMP_H
