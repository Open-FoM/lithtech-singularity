cmake_minimum_required (VERSION 3.24.4 FATAL_ERROR)
project (ltjs_dedit VERSION 0.0.1 LANGUAGES CXX)

include (ltjs_common)

if (NOT APPLE)
	message (STATUS "DEdit CMake target is only available on macOS; Windows builds should use the legacy VC project.")
	return ()
endif ()

add_executable (${PROJECT_NAME})

ltjs_add_defaults (${PROJECT_NAME})

set_target_properties (
	${PROJECT_NAME}
	PROPERTIES
		OUTPUT_NAME DEdit
)

target_sources (
	${PROJECT_NAME}
	PRIVATE
		dedit_mac_stub.cpp
)
