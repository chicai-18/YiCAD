# tests/math —— 对应阶段 3 的 YiCadMath
#
# 一个子系统一个测试二进制：每个测试目标都要链接整个 YiCadCore 的目标文件，
# 按文件拆分测试目标只会让链接时间成倍增长。

yicad_add_test(math
    SOURCES
        test_math_angle.cpp
        test_math_solver.cpp
        test_math_vector.cpp
        test_math_rtree.cpp
        test_math_kdtree.cpp
        test_debug_instrumentation.cpp
    LABELS "math;unit")
