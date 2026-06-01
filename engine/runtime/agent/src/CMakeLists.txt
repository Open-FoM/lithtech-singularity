cmake_minimum_required (VERSION 3.24.4 FATAL_ERROR)
project (ltjs_lib_agent VERSION 0.0.1 LANGUAGES CXX)

include (ltjs_common)

find_package (Threads REQUIRED)

add_library (${PROJECT_NAME} STATIC)

ltjs_add_defaults (${PROJECT_NAME})

# PUBLIC so the singularity exe (and the client-side engine tools) inherit the
# agent headers and the vendored nlohmann/json include simply by linking this
# library.
target_include_directories (
	${PROJECT_NAME}
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_SOURCE_DIR}/libs/diligent/DiligentTools/ThirdParty/json/single_include
)

target_link_libraries (
	${PROJECT_NAME}
	PRIVATE
		Threads::Threads
)

target_sources (
	${PROJECT_NAME}
	PRIVATE
		agent_tool.h
		agent_registry.h
		agent_registry.cpp
		agent_queue.h
		agent_queue.cpp
		agent_protocol.h
		agent_protocol.cpp
		agent_tcp_server.h
		agent_tcp_server.cpp
		agent_control.h
		agent_control.cpp
)

# Optional: build the engine-free core unit tests as part of the main build when
# LTJS_BUILD_TESTS is enabled. Dormant otherwise, so it cannot affect normal
# engine builds. Reuses GoogleTest via FetchContent (shared with other suites).
if (LTJS_BUILD_TESTS)
	if (NOT TARGET gtest)
		include (FetchContent)
		set (BUILD_GMOCK OFF CACHE BOOL "Disable gmock." FORCE)
		set (INSTALL_GTEST OFF CACHE BOOL "Disable GoogleTest install." FORCE)
		FetchContent_Declare (
			googletest
			GIT_REPOSITORY https://github.com/google/googletest.git
			GIT_TAG v1.14.0
		)
		FetchContent_MakeAvailable (googletest)
	endif ()
	include (CTest)
	enable_testing ()
	set (LTJS_AGENT_TESTS_EMBEDDED ON)
	add_subdirectory (${CMAKE_CURRENT_SOURCE_DIR}/../tests ${CMAKE_CURRENT_BINARY_DIR}/tests)
endif ()
