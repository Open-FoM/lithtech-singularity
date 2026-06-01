cmake_minimum_required (VERSION 3.24.4 FATAL_ERROR)

# This file works two ways:
#   * Embedded   — added via add_subdirectory() from the agent module when the
#                  parent build sets LTJS_AGENT_TESTS_EMBEDDED and provides a
#                  GoogleTest target (GTest::gtest_main).
#   * Standalone — configured directly (cmake -S engine/runtime/agent/tests) for
#                  a fast TDD loop with no engine/Diligent build. Point
#                  -DLTJS_GTEST_SRC at a populated googletest source tree to
#                  avoid a network fetch.

if (NOT LTJS_AGENT_TESTS_EMBEDDED)
	project (ltjs_agent_core_tests CXX)

	set (CMAKE_CXX_STANDARD 20)
	set (CMAKE_CXX_STANDARD_REQUIRED ON)
	set (CMAKE_CXX_EXTENSIONS OFF)

	set (LTJS_GTEST_SRC "" CACHE PATH "Path to a populated googletest source tree.")
	set (BUILD_GMOCK OFF CACHE BOOL "" FORCE)
	set (INSTALL_GTEST OFF CACHE BOOL "" FORCE)

	if (LTJS_GTEST_SRC AND EXISTS "${LTJS_GTEST_SRC}/CMakeLists.txt")
		add_subdirectory ("${LTJS_GTEST_SRC}" "${CMAKE_BINARY_DIR}/_gtest")
	else ()
		include (FetchContent)
		FetchContent_Declare (
			googletest
			GIT_REPOSITORY https://github.com/google/googletest.git
			GIT_TAG v1.14.0
		)
		FetchContent_MakeAvailable (googletest)
	endif ()

	enable_testing ()
endif ()

set (_agent_src ${CMAKE_CURRENT_LIST_DIR}/../src)
set (_json_inc ${CMAKE_CURRENT_LIST_DIR}/../../../../libs/diligent/DiligentTools/ThirdParty/json/single_include)

find_package (Threads REQUIRED)

add_executable (
	ltjs_agent_core_tests
	agent_core_tests.cpp
	agent_server_tests.cpp
	${_agent_src}/agent_registry.cpp
	${_agent_src}/agent_queue.cpp
	${_agent_src}/agent_protocol.cpp
	${_agent_src}/agent_tcp_server.cpp
	${_agent_src}/agent_control.cpp
)

set_target_properties (
	ltjs_agent_core_tests
	PROPERTIES
		CXX_STANDARD 20
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF
)

target_include_directories (
	ltjs_agent_core_tests
	PRIVATE
		${_agent_src}
		${_json_inc}
)

target_link_libraries (
	ltjs_agent_core_tests
	PRIVATE
		GTest::gtest_main
		Threads::Threads
)

include (GoogleTest)
gtest_discover_tests (ltjs_agent_core_tests)
