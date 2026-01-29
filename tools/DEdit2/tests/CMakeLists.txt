cmake_minimum_required (VERSION 3.24.4 FATAL_ERROR)

add_executable (
	ltjs_dedit2_tests
	${CMAKE_CURRENT_LIST_DIR}/grid_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/path_utils_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/string_utils_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/document_state_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/project_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/world_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/selection_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/grouping_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/transform_tests.cpp
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
