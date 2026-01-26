cmake_minimum_required (VERSION 3.24.4 FATAL_ERROR)
project (ltjs_lib_shared VERSION 0.0.1 LANGUAGES CXX)

include (ltjs_common)

add_library (${PROJECT_NAME} STATIC)

ltjs_add_defaults (${PROJECT_NAME})

target_sources (
	${PROJECT_NAME}
	PRIVATE
		bdefs.cpp
		classbind.cpp
		collision.cpp
		compress.cpp
		concommand.cpp
		conparse.cpp
		debuggeometry.cpp
		dhashtable.cpp
		dtxmgr.cpp
		engine_vars.cpp
		findobj.cpp
		ftclient.cpp
		ftserv.cpp
		gamemath.cpp
		genericprop_setup.cpp
		genltstream.cpp
		geometry.cpp
		geomroutines.cpp
		impl_common.cpp
		interface_linkage.cpp
		leech.cpp
		lightmap_compress.cpp
		lightmap_planes.cpp
		ltmessage.cpp
		lttimer.cpp
		modellt_impl.cpp
		motion.cpp
		moveobject.cpp
		moveplayer.cpp
		nexus.cpp
		objectmgr.cpp
		parse_world_info.cpp
		pixelformat.cpp
		ratetracker.cpp
		shared_iltcommon.cpp
		shared_iltphysics.cpp
		spritecontrolimpl.cpp
		stacktrace.cpp
		stdlterror.cpp
		strtools.cpp
		transformlt_impl.cpp
		version_info.cpp
)

target_include_directories (
	${PROJECT_NAME}
	PRIVATE
		.
		${LTJS_ROOT}/libs/lith
		${LTJS_ROOT}/libs/stdlith
		${LTJS_ROOT}/engine/libs/rezmgr
		${LTJS_ROOT}/engine/sdk/inc
		${LTJS_ROOT}/engine/sdk/inc/compat
		${LTJS_ROOT}/engine/sdk/inc/physics
		${LTJS_ROOT}/engine/runtime/kernel/src
		${LTJS_ROOT}/engine/runtime/kernel/src/sys/linux
		${LTJS_ROOT}/engine/runtime/kernel/io/src
		${LTJS_ROOT}/engine/runtime/kernel/mem/src
		${LTJS_ROOT}/engine/runtime/kernel/net/src
		${LTJS_ROOT}/engine/runtime/kernel/toport/src
		${LTJS_ROOT}/engine/runtime/lithtemplate
		${LTJS_ROOT}/engine/runtime/client/src
		${LTJS_ROOT}/engine/runtime/model/src
		${LTJS_ROOT}/engine/runtime/server/src
		${LTJS_ROOT}/engine/runtime/ui/src
		${LTJS_ROOT}/engine/runtime/world/src
		${LTJS_ROOT}/engine/runtime/shared/src
		${LTJS_ROOT}/engine/runtime/shared/src/sys/linux
		${LTJS_ROOT}/engine/runtime/shared/src/sys/win
)

target_link_libraries (
	${PROJECT_NAME}
	PRIVATE
		ltjs_lib_lt_mem
		ltjs_lib_lith
		ltjs_lib_std_lith
		ltjs_lib_rez_mgr
		ltjs_lib_info
		slikenet_lib
)
