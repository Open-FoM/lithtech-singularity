cmake_minimum_required (VERSION 3.24.4 FATAL_ERROR)
project (ltjs_lib_model VERSION 0.0.1 LANGUAGES CXX)

include (ltjs_common)

add_library (${PROJECT_NAME} STATIC)

ltjs_add_defaults (${PROJECT_NAME})

target_sources (
	${PROJECT_NAME}
	PRIVATE
		animtracker.cpp
		model.cpp
		model_load.cpp
		modelallocations.cpp
		model_render_object.cpp
		transformmaker.cpp
)

target_include_directories (
	${PROJECT_NAME}
	PRIVATE
		.
		${LTJS_ROOT}/engine/sdk/inc
		${LTJS_ROOT}/engine/sdk/inc/physics
		${LTJS_ROOT}/libs/stdlith
		${LTJS_ROOT}/engine/runtime/kernel/src
		${LTJS_ROOT}/engine/runtime/kernel/src/sys/linux
		${LTJS_ROOT}/engine/runtime/kernel/mem/src
		${LTJS_ROOT}/engine/runtime/kernel/net/src
		${LTJS_ROOT}/engine/runtime/kernel/toport/src
		${LTJS_ROOT}/engine/runtime/lithtemplate
		${LTJS_ROOT}/engine/runtime/shared/src
		${LTJS_ROOT}/engine/runtime/shared/src/sys/win
		${LTJS_ROOT}/engine/runtime/shared/src/sys/linux
		${LTJS_ROOT}/engine/runtime/world/src
)

target_link_libraries (
	${PROJECT_NAME}
	PRIVATE
		ltjs_lib_lt_mem
		ltjs_lib_lith
		ltjs_lib_std_lith
		ltjs_lib_rez_mgr
		ltjs_lib_info
)
