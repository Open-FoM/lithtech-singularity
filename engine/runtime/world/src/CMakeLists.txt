cmake_minimum_required (VERSION 3.24.4 FATAL_ERROR)
project (ltjs_lib_world VERSION 0.0.1 LANGUAGES CXX)

include (ltjs_common)

add_library (${PROJECT_NAME} STATIC)

ltjs_add_defaults (${PROJECT_NAME})

target_include_directories (
	${PROJECT_NAME}
	PUBLIC
		.
		${CMAKE_CURRENT_SOURCE_DIR}/../../shared/src
		${CMAKE_CURRENT_SOURCE_DIR}/../../shared/src/sys/linux
		${CMAKE_CURRENT_SOURCE_DIR}/../../shared/src/sys/win
		${CMAKE_CURRENT_SOURCE_DIR}/../../kernel/src
		${CMAKE_CURRENT_SOURCE_DIR}/../../kernel/src/sys/linux
		${CMAKE_CURRENT_SOURCE_DIR}/../../kernel/mem/src
		${CMAKE_CURRENT_SOURCE_DIR}/../../kernel/net/src
		${CMAKE_CURRENT_SOURCE_DIR}/../../kernel/toport/src
		${CMAKE_CURRENT_SOURCE_DIR}/../../lithtemplate
		${CMAKE_CURRENT_SOURCE_DIR}/../../model/src
		${CMAKE_CURRENT_SOURCE_DIR}/../../server/src
		${CMAKE_CURRENT_SOURCE_DIR}/../../../sdk/inc
		${CMAKE_CURRENT_SOURCE_DIR}/../../../../libs/stdlith
)

target_sources (
	${PROJECT_NAME}
	PRIVATE
		de_mainworld.cpp
		de_nodes.cpp
		de_objects.cpp
		fullintersectline.cpp
		intersect_line.cpp
		intersectsweptsphere.cpp
		light_table.cpp
		world_blind_object_data.cpp
		world_blocker_data.cpp
		world_blocker_math.cpp
		world_particle_blocker_data.cpp
		world_shared_bsp.cpp
		world_tree.cpp
		../../shared/src/conparse.cpp
		../../shared/src/geomroutines.cpp
		../../shared/src/lightmap_planes.cpp
		../../shared/src/parse_world_info.cpp
		../../shared/src/strtools.cpp
		../../kernel/mem/src/sys/linux/de_memory.cpp
		../../kernel/src/sys/linux/counter.cpp
		../../kernel/toport/src/dutil.cpp
)

target_link_libraries (
	${PROJECT_NAME}
	PRIVATE
		ltjs_lib_lith
		ltjs_lib_std_lith
		ltjs_lib_lt_mem
		ltjs_lib_rez_mgr
		ltjs_lib_info
)
