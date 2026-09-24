# tests/geometry —— 对应阶段 3 的 YiCadModel 中的几何实体
#
# 覆盖方案 3.2 第 2 条点名的 DmArc / DmCircle / DmEllipse 的求交、偏移、
# 包围盒，以及它们共同依赖的 Information::getIntersection；另覆盖走空间
# 搜索树的框选与虚拟交点查询（方案 9.1 节，P10）。

yicad_add_test(geometry
    SOURCES
        test_geometry_bounds.cpp
        test_geometry_arc.cpp
        test_geometry_intersection.cpp
        test_geometry_spatial_query.cpp
    LINK YiCadModel
    LABELS "geometry;unit")
