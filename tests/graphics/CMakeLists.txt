# tests/graphics —— GI：实体经 worldDraw 输出的图元、GI 流的记录与重放及序列化、仿射变换
# （doc/RENDER_PLAN.md 第 2 阶段）。
#
# GI 与实体都在 YiCadModel 里，本二进制只链接它；GiTextDump 把 GI 的输出写成文本，用例按文本比对。

yicad_add_test(graphics
    SOURCES
        GiTextDump.cpp
        test_graphics_transform.cpp
        test_graphics_stream.cpp
        test_graphics_entities.cpp
        test_graphics_text.cpp
    LINK YiCadModel
    LABELS "graphics;unit")
