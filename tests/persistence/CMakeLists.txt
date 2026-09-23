# tests/persistence —— 对应阶段 3 的 YiCadPersistence
#
# 方案 3.2 第 2 条把往返测试列为投入产出比最高的一类：它一次性锁住整个
# 数据模型的序列化语义，也是阶段 5（Qt 6 迁移）字符编码验证的唯一依靠。

yicad_add_test(persistence
    SOURCES
        test_persistence_roundtrip.cpp
        test_persistence_primitives.cpp
    LINK YiCadPersistence
    LABELS "persistence;unit;roundtrip")
