cmake_minimum_required (VERSION 3.24.4 FATAL_ERROR)

set (_DEDIT2_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")

add_executable (
	ltjs_dedit2_tests
	${_DEDIT2_ROOT}/path_utils.cpp
	${_DEDIT2_ROOT}/app/string_utils.cpp
	${_DEDIT2_ROOT}/app/project.cpp
	${_DEDIT2_ROOT}/app/node_props.cpp
	${_DEDIT2_ROOT}/app/world_core.cpp
	${CMAKE_CURRENT_LIST_DIR}/path_utils_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/string_utils_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/document_state_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/project_tests.cpp
	${CMAKE_CURRENT_LIST_DIR}/world_tests.cpp
)

set_target_properties (
	ltjs_dedit2_tests
	PROPERTIES
		CXX_STANDARD 20
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF
)

target_include_directories (
	ltjs_dedit2_tests
	PRIVATE
		${_DEDIT2_ROOT}
)

target_link_libraries (
	ltjs_dedit2_tests
	PRIVATE
		gtest_main
)

include (GoogleTest)
gtest_discover_tests (ltjs_dedit2_tests)
