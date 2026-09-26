# tests/math —— 对应 YiCadBase（src/base/）
#
# 一个子系统一个测试二进制：每个测试目标都要链接对应库的目标文件，
# 按文件拆分测试目标只会让链接时间成倍增长。
#
# 本目录的用例只需要 YiCadBase；但全部测试二进制共用的
# support/yicad_test_main.cpp 要先跑 DmSystem::init（类型系统注册）与
# DmSettings::init（QSettings 组织名/应用名），这两步是 YiCadModel 的
# 符号。LINK 因此填 YiCadModel（PUBLIC 依赖链上已经带着 YiCadBase），
# 而不是 YiCadBase 本身——仍然比链接整个 YiCadCore（含 Render/
# Application/UI/Shell）小得多，是拆库对增量构建时间的主要收益来源。
yicad_add_test(math
    SOURCES
        test_math_angle.cpp
        test_math_solver.cpp
        test_math_vector.cpp
        test_math_rtree.cpp
        test_math_kdtree.cpp
        test_debug_instrumentation.cpp
        test_math_xml_reader.cpp
    LINK YiCadModel
    LABELS "math;unit")
