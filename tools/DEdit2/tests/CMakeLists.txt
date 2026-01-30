cmake_minimum_required (VERSION 3.24.4 FATAL_ERROR)

add_executable (
	ltjs_dedit2_tests
	${CMAKE_CURRENT_LIST_DIR}/brush_primitive_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/grid_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/path_utils_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/string_utils_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/document_state_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/project_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/world_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/selection_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/grouping_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/transform_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/scene_ui_tests.cpp
	# CSG operation tests (EPIC-08)
	${CMAKE_CURRENT_LIST_DIR}/csg_types_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/csg_polygon_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/csg_triangulate_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/csg_split_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/csg_hollow_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/csg_carve_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/csg_join_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/csg_dialog_helpers_tests.cpp
	# Geometry editing tests (EPIC-09)
	${CMAKE_CURRENT_LIST_DIR}/geometry_mode_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/subobject_picking_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/geometry_ops_tests.cpp
	# Texturing system tests (EPIC-10)
	${CMAKE_CURRENT_LIST_DIR}/uv_types_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/texture_browser_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/texture_applicator_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/uv_transform_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/uv_projection_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/uv_fit_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/texture_replace_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/surface_flags_tests.cpp
)

set_target_properties (
	ltjs_dedit2_tests
	PROPERTIES
		CXX_STANDARD 20
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF
)

target_link_libraries (
	ltjs_dedit2_tests
	PRIVATE
		ltjs_dedit2_core
		gtest_main
)

include (GoogleTest)
gtest_discover_tests (ltjs_dedit2_tests)
