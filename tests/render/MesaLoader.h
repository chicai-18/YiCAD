/// @file MesaLoader.h
/// @brief test_render 在进程启动时装入 Mesa 的软件 OpenGL（llvmpipe），见 MesaLoader.cpp

#ifndef YICAD_TEST_MESA_LOADER_H
#define YICAD_TEST_MESA_LOADER_H

#include <string>

namespace yicad_test
{

/// @brief 启动时装入 Mesa 的结果
struct MesaStatus
{
    bool loaded = false;   ///< opengl32.dll 已从 Mesa 目录装入
    std::string path;      ///< 尝试装入的 opengl32.dll 的完整路径
    std::string error;     ///< 装入失败的原因
};

/// @brief 启动时装入 Mesa 的结果（main 之前已经尝试过）
const MesaStatus& mesaStatus();

}  // namespace yicad_test

#endif  // YICAD_TEST_MESA_LOADER_H
