cmake_minimum_required (VERSION 3.24.4 FATAL_ERROR)
project (ltjs_dedit2 VERSION 0.0.1 LANGUAGES CXX)

if (NOT DEFINED LTJS_ROOT)
	if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_LIST_DIR)
		get_filename_component (LTJS_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
	else ()
		set (LTJS_ROOT "${CMAKE_SOURCE_DIR}")
	endif ()
endif ()

list (APPEND CMAKE_MODULE_PATH "${LTJS_ROOT}/cmake")

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_LIST_DIR)
	option (LTJS_SDL_BACKEND "Use SDL backend." ON)
	option (LTJS_USE_PCH "Use precompiled headers." ON)
	option (LTJS_USE_CCACHE "Use ccache if available." ON)
	option (LTJS_BUILD_TESTS "Build unit tests." OFF)

	if (LTJS_USE_CCACHE)
		find_program (CCACHE_PROGRAM ccache)
		if (CCACHE_PROGRAM)
			set (CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
			set (CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
		endif ()
	endif ()
endif ()

include (ltjs_common)

if (NOT APPLE)
	message (STATUS "DEdit2 CMake target is only available on macOS; Windows builds should use the legacy VC project.")
	return ()
endif ()

if (NOT TARGET SDL3::SDL3)
	find_package (SDL3 3.2.0 REQUIRED)
endif ()

if (LTJS_BUILD_TESTS)
	include (CTest)
	enable_testing ()

	include (FetchContent)
	set (BUILD_GMOCK OFF CACHE BOOL "Disable gmock for DEdit2 tests." FORCE)
	set (INSTALL_GTEST OFF CACHE BOOL "Disable GoogleTest install for DEdit2 tests." FORCE)
	FetchContent_Declare (
		googletest
		GIT_REPOSITORY https://github.com/google/googletest.git
		GIT_TAG v1.14.0
	)
	FetchContent_MakeAvailable (googletest)
	add_subdirectory (tests)
endif ()

if (NOT TARGET Diligent-GraphicsEngineVk-static)
	set (DILIGENT_NO_DIRECT3D12 ON CACHE BOOL "Disable D3D12 backend" FORCE)
	set (DILIGENT_NO_OPENGL ON CACHE BOOL "Disable OpenGL backend" FORCE)
	set (DILIGENT_NO_ARCHIVER ON CACHE BOOL "Disable Archiver" FORCE)
	set (DILIGENT_NO_WEBGPU ON CACHE BOOL "Disable WebGPU backend" FORCE)
	if (APPLE)
		set (DILIGENT_NO_METAL OFF CACHE BOOL "Enable Metal backend on Apple" FORCE)
	else ()
		set (DILIGENT_NO_METAL ON CACHE BOOL "Disable Metal backend on non-Apple" FORCE)
	endif ()
	if (DEFINED BUILD_SHARED_LIBS)
		set (_LTJS_BUILD_SHARED_LIBS "${BUILD_SHARED_LIBS}")
	else ()
		set (_LTJS_BUILD_SHARED_LIBS "__UNDEFINED__")
	endif ()
	set (BUILD_SHARED_LIBS OFF)
	add_subdirectory ("${LTJS_ROOT}/libs/diligent/DiligentCore" "${CMAKE_BINARY_DIR}/diligent")
	if (_LTJS_BUILD_SHARED_LIBS STREQUAL "__UNDEFINED__")
		unset (BUILD_SHARED_LIBS)
	else ()
		set (BUILD_SHARED_LIBS "${_LTJS_BUILD_SHARED_LIBS}")
	endif ()
endif ()

if (NOT TARGET ltjs_lib)
	add_subdirectory ("${LTJS_ROOT}/libs/ltjs" "${CMAKE_BINARY_DIR}/ltjs_lib")
endif ()
if (NOT TARGET ltjs_lib_diligent_render)
	add_subdirectory ("${LTJS_ROOT}/engine/runtime/render_a/src/sys/diligent" "${CMAKE_BINARY_DIR}/ltjs_lib_diligent_render")
endif ()
if (NOT TARGET ltjs_lib_freetype)
	add_subdirectory ("${LTJS_ROOT}/libs/freetype" "${CMAKE_BINARY_DIR}/ltjs_lib_freetype")
endif ()
if (NOT TARGET ltjs_lib_lt_mem)
	add_subdirectory ("${LTJS_ROOT}/engine/libs/ltmem" "${CMAKE_BINARY_DIR}/ltjs_lib_lt_mem")
endif ()
if (NOT TARGET ltjs_lib_rez_mgr)
	add_subdirectory ("${LTJS_ROOT}/engine/libs/rezmgr" "${CMAKE_BINARY_DIR}/ltjs_lib_rez_mgr")
endif ()
if (NOT TARGET ltjs_lib_std_lith)
	add_subdirectory ("${LTJS_ROOT}/libs/stdlith" "${CMAKE_BINARY_DIR}/ltjs_lib_std_lith")
endif ()
if (NOT TARGET ltjs_lib_lith)
	add_subdirectory ("${LTJS_ROOT}/libs/lith" "${CMAKE_BINARY_DIR}/ltjs_lib_lith")
endif ()
if (NOT TARGET ltjs_lib_info)
	add_subdirectory ("${LTJS_ROOT}/engine/runtime/info/src" "${CMAKE_BINARY_DIR}/ltjs_lib_info")
endif ()
if (NOT TARGET ltjs_lib_sdk)
	add_subdirectory ("${LTJS_ROOT}/engine/sdk/inc" "${CMAKE_BINARY_DIR}/ltjs_lib_sdk")
endif ()
if (NOT TARGET ltjs_lib_shared)
	add_subdirectory ("${LTJS_ROOT}/engine/runtime/shared/src" "${CMAKE_BINARY_DIR}/ltjs_lib_shared")
endif ()
if (NOT TARGET ltjs_lib_model)
	add_subdirectory ("${LTJS_ROOT}/engine/runtime/model/src" "${CMAKE_BINARY_DIR}/ltjs_lib_model")
endif ()
if (NOT TARGET ltjs_lib_world)
	add_subdirectory ("${LTJS_ROOT}/engine/runtime/world/src" "${CMAKE_BINARY_DIR}/ltjs_lib_world")
endif ()
if (NOT TARGET ltjs_lib_ui)
	add_subdirectory ("${LTJS_ROOT}/engine/runtime/ui/src" "${CMAKE_BINARY_DIR}/ltjs_lib_ui")
endif ()
if (NOT TARGET ltjs_lib_model)
	add_subdirectory ("${LTJS_ROOT}/engine/runtime/model/src" "${CMAKE_BINARY_DIR}/ltjs_lib_model")
endif ()
target_compile_definitions (
	ltjs_lib_model
	PRIVATE
		LTJS_USE_DILIGENT_RENDER
)

add_executable (${PROJECT_NAME})

ltjs_add_defaults (${PROJECT_NAME})

target_compile_definitions (
	${PROJECT_NAME}
	PRIVATE
		LTJS_USE_DILIGENT_RENDER
		LTJS_USE_TANGENT_AND_BINORMAL
		LTJS_DEDIT2_FILEMGR
)

set_target_properties (
	${PROJECT_NAME}
	PROPERTIES
		OUTPUT_NAME DEdit2
)

target_sources (
	${PROJECT_NAME}
	PRIVATE
		app_main.cpp
		app/editor_loop.cpp
		brush/brush_primitive.cpp
		grouping/node_grouping.cpp
		selection/depth_cycle.cpp
		selection/marquee_selection.cpp
		selection/selection_filter.cpp
		selection/selection_query.cpp
		transform/mirror.cpp
		app/document_state.h
		app/editor_paths.cpp
		app/editor_session.h
		app/node_props.cpp
		app/project.cpp
		app/project_utils.cpp
		app/world_core.cpp
		app/recent_projects.cpp
		app/scene_loader.cpp
		app/string_utils.cpp
		app/world.cpp
		ui/viewport_panel.cpp
		viewport/diligent_viewport.cpp
		viewport/interaction.cpp
		viewport/lighting.cpp
		viewport/overlays.cpp
		viewport/render_settings.cpp
		viewport/scene_filters.cpp
		viewport/sky_models.cpp
		viewport/world_bounds.cpp
		viewport/world_settings.cpp
		bsp_view.cpp
		dedit2_concommand.cpp
		editor_state.cpp
		editor_transfer.cpp
		engine_console_stub.cpp
		engine_sprite.cpp
		engine_render.cpp
		lt_stream.cpp
		path_utils.cpp
		texture_cache.cpp
		texture_effect_group.cpp
		ui_console.cpp
		ui_dock.cpp
		ui_goto.cpp
		ui_status_bar.cpp
		ui_toolbar.cpp
		ui_tools_dock.cpp
		ui_project.cpp
		ui_worlds.cpp
		ui_scene.cpp
		ui_viewport.cpp
		multi_viewport.cpp
		viewport_picking.cpp
		viewport_gizmo.cpp
		viewport_render.cpp
		undo_stack.cpp
		ui_properties.cpp
		ui_shared.cpp
		platform_macos.mm
		${LTJS_ROOT}/engine/libs/ltamgr/ltabitfile.cpp
		${LTJS_ROOT}/engine/libs/ltamgr/ltacompressedfile.cpp
		${LTJS_ROOT}/engine/libs/ltamgr/ltaconverter.cpp
		${LTJS_ROOT}/engine/libs/ltamgr/ltafile.cpp
		${LTJS_ROOT}/engine/libs/ltamgr/ltafilebuffer.cpp
		${LTJS_ROOT}/engine/libs/ltamgr/ltahuffmantree.cpp
		${LTJS_ROOT}/engine/libs/ltamgr/ltaloadonlyalloc.cpp
		${LTJS_ROOT}/engine/libs/ltamgr/ltanode.cpp
		${LTJS_ROOT}/engine/libs/ltamgr/ltanodebuilder.cpp
		${LTJS_ROOT}/engine/libs/ltamgr/ltanodeiterator.cpp
		${LTJS_ROOT}/engine/libs/ltamgr/ltanodreader.cpp
		${LTJS_ROOT}/engine/libs/ltamgr/ltanodewriter.cpp
		${LTJS_ROOT}/engine/libs/ltamgr/ltareader.cpp
		${LTJS_ROOT}/engine/libs/ltamgr/ltautil.cpp
		${LTJS_ROOT}/engine/libs/ltamgr/ltawriter.cpp
		${LTJS_ROOT}/engine/libs/ltamgr/lzsswindow.cpp
		${LTJS_ROOT}/engine/runtime/client/src/client_filemgr.cpp
		${LTJS_ROOT}/engine/runtime/client/src/sprite.cpp
		${LTJS_ROOT}/engine/runtime/kernel/io/src/sys/linux/linuxfile.cpp
		${LTJS_ROOT}/engine/runtime/kernel/src/sys/linux/lthread.cpp
		${LTJS_ROOT}/engine/runtime/kernel/src/sys/linux/ltthread.cpp
		${LTJS_ROOT}/engine/runtime/kernel/toport/src/stringmgr.cpp
		${LTJS_ROOT}/engine/runtime/kernel/toport/src/stringhelper.cpp
		${LTJS_ROOT}/engine/runtime/kernel/toport/src/bindmgr.cpp
		${LTJS_ROOT}/engine/runtime/kernel/src/sys/linux/ltlibraryloader.cpp
		dsys_stub.cpp
		${LTJS_ROOT}/libs/imgui/imgui.cpp
		${LTJS_ROOT}/libs/imgui/imgui_draw.cpp
		${LTJS_ROOT}/libs/imgui/imgui_tables.cpp
		${LTJS_ROOT}/libs/imgui/imgui_widgets.cpp
		${LTJS_ROOT}/libs/imgui/misc/cpp/imgui_stdlib.cpp
		${LTJS_ROOT}/libs/imgui/backends/imgui_impl_sdl3.cpp
		${LTJS_ROOT}/libs/diligent/DiligentTools/Imgui/src/ImGuiDiligentRenderer.cpp
		${LTJS_ROOT}/libs/diligent/DiligentTools/Imgui/src/ImGuiImplDiligent.cpp
		${LTJS_ROOT}/libs/diligent/DiligentTools/Imgui/src/ImGuiImplSDL3.cpp
		${LTJS_ROOT}/libs/diligent/DiligentTools/Imgui/src/ImGuiUtils.cpp
)

target_include_directories (
	${PROJECT_NAME}
	PRIVATE
		${CMAKE_CURRENT_LIST_DIR}
		${LTJS_ROOT}/libs/imgui
		${LTJS_ROOT}/libs/imgui/backends
		${LTJS_ROOT}/libs/diligent
		${LTJS_ROOT}/libs/lith
		${LTJS_ROOT}/libs/stdlith
		${LTJS_ROOT}/libs/diligent/DiligentCore/Primitives/interface
		${LTJS_ROOT}/libs/diligent/DiligentCore/Common/interface
		${LTJS_ROOT}/libs/diligent/DiligentCore/Graphics/GraphicsEngine/interface
		${LTJS_ROOT}/libs/diligent/DiligentCore/Graphics/GraphicsTools/interface
		${LTJS_ROOT}/libs/diligent/DiligentCore/Graphics/GraphicsAccessories/interface
		${LTJS_ROOT}/libs/diligent/DiligentCore/Graphics/GraphicsEngineVulkan/interface
		${LTJS_ROOT}/libs/diligent/DiligentTools/Imgui/interface
		${LTJS_ROOT}/engine/libs/ltamgr
		${LTJS_ROOT}/engine/libs/rezmgr
		${LTJS_ROOT}/engine/sdk/inc
		${LTJS_ROOT}/engine/runtime/kernel/src
		${LTJS_ROOT}/engine/runtime/kernel/src/sys/linux
		${LTJS_ROOT}/engine/runtime/kernel/io/src
		${LTJS_ROOT}/engine/runtime/kernel/mem/src
		${LTJS_ROOT}/engine/runtime/kernel/net/src
		${LTJS_ROOT}/engine/runtime/kernel/toport/src
		${LTJS_ROOT}/engine/runtime/shared/src
		${LTJS_ROOT}/engine/runtime/shared/src/sys/linux
		${LTJS_ROOT}/engine/runtime/shared/src/sys/win
		${LTJS_ROOT}/engine/runtime/client/src
		${LTJS_ROOT}/engine/runtime/render_a/src/sys
		${LTJS_ROOT}/engine/runtime/render_a/src/sys/diligent
		${LTJS_ROOT}/engine/runtime/server/src
		${LTJS_ROOT}/engine/runtime/world/src
)

target_link_libraries (
	${PROJECT_NAME}
	PRIVATE
		ltjs_lib_diligent_render
		ltjs_lib_world
		ltjs_lib_model
		ltjs_lib_shared
		ltjs_lib_sdk
		ltjs_lib_ui
		ltjs_lib_lt_mem
		ltjs_lib_rez_mgr
		ltjs_lib_std_lith
		ltjs_lib_lith
		ltjs_lib_info
		Diligent-GraphicsEngineVk-static
)

if (APPLE)
	target_link_libraries (
		${PROJECT_NAME}
		PRIVATE
			"-framework Cocoa"
			"-framework Metal"
			"-framework QuartzCore"
			"-framework UniformTypeIdentifiers"
	)
endif ()
