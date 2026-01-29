cmake_minimum_required (VERSION 3.24.4 FATAL_ERROR)
project (ltjs VERSION 0.0.1 LANGUAGES C CXX)

# Standardize runtime output to a top-level bin directory.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

if (NOT WIN32 AND NOT APPLE)
	message (FATAL_ERROR "WIN32 or APPLE only")
endif ()

set (CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

#
# Options
#
option (LTJS_SDL_BACKEND "Use SDL backend." ON)
option (LTJS_USE_PCH "Use precompiled headers." ON)
option (LTJS_USE_CCACHE "Use ccache if available." ON)
option (LTJS_BUILD_DEDIT "Build DEdit map editor." OFF)
option (LTJS_BUILD_DEDIT2 "Build DEdit2 map editor." OFF)
option (LTJS_BUILD_RENDERDEMO "Build RenderDemo game." OFF)
option (LTJS_BUILD_TESTS "Build unit tests." OFF)

if (LTJS_BUILD_TESTS)
	include (CTest)
	enable_testing ()
endif ()

if (LTJS_USE_CCACHE)
	find_program (CCACHE_PROGRAM ccache)
	if (CCACHE_PROGRAM)
		set (CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
		set (CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
	endif ()
endif ()

#
# SDL3
#
find_package (SDL3 3.2.0 REQUIRED)

#
# SDL3_ttf
#
set (SDLTTF_VENDORED ON CACHE BOOL "Use vendored dependencies for SDL3_ttf." FORCE)
set (SDLTTF_HARFBUZZ OFF CACHE BOOL "Disable harfbuzz in SDL3_ttf." FORCE)
set (SDLTTF_SAMPLES OFF CACHE BOOL "Disable SDL3_ttf samples." FORCE)

# FreeType (vendored) shared across SDL_ttf and engine UI.
set (FT_DISABLE_ZLIB ON CACHE BOOL "freetype zlib option" FORCE)
set (FT_DISABLE_BZIP2 ON CACHE BOOL "freetype bzip2 option" FORCE)
set (FT_DISABLE_PNG ON CACHE BOOL "freetype png option" FORCE)
set (FT_DISABLE_BROTLI ON CACHE BOOL "freetype brotli option" FORCE)
if (SDLTTF_HARFBUZZ)
	set (FT_DISABLE_HARFBUZZ OFF CACHE BOOL "freetype harfbuzz option" FORCE)
	set (FT_REQUIRE_HARFBUZZ ON CACHE BOOL "freetype harfbuzz option" FORCE)
else ()
	set (FT_DISABLE_HARFBUZZ ON CACHE BOOL "freetype harfbuzz option" FORCE)
	set (FT_REQUIRE_HARFBUZZ OFF CACHE BOOL "freetype harfbuzz option" FORCE)
endif ()
add_subdirectory (libs/freetype)
add_subdirectory (libs/sdl_ttf)

# Diligent Engine (core only)
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
add_subdirectory (libs/diligent/DiligentCore)
if (_LTJS_BUILD_SHARED_LIBS STREQUAL "__UNDEFINED__")
	unset (BUILD_SHARED_LIBS)
else ()
	set (BUILD_SHARED_LIBS "${_LTJS_BUILD_SHARED_LIBS}")
endif ()

if (NOT DILIGENT_DEAR_IMGUI_PATH)
	set (DILIGENT_DEAR_IMGUI_PATH "${CMAKE_SOURCE_DIR}/libs/imgui" CACHE PATH "imgui source directory")
endif ()
add_subdirectory (libs/diligent/DiligentTools)
add_subdirectory (libs/diligent/DiligentFX)

if (TARGET Diligent-Imgui)
	target_compile_definitions(
		Diligent-Imgui
		PRIVATE
			ImGuiKey_ModCtrl=ImGuiKey_LeftCtrl
			ImGuiKey_ModShift=ImGuiKey_LeftShift
			ImGuiKey_ModAlt=ImGuiKey_LeftAlt
			ImGuiKey_ModSuper=ImGuiKey_LeftSuper
	)
endif ()

if (APPLE)
	if (TARGET Diligent-GraphicsEngineVk-shared)
		target_link_libraries (Diligent-GraphicsEngineVk-shared PRIVATE Diligent-ApplePlatform)
	endif ()
endif ()

# Break cyclic link interface between ShaderTools and HLSL2GLSL to avoid duplicate libs in link line.
if (TARGET Diligent-HLSL2GLSLConverterLib)
	get_target_property (_ltjs_hlsl2glsl_iface Diligent-HLSL2GLSLConverterLib INTERFACE_LINK_LIBRARIES)
	if (_ltjs_hlsl2glsl_iface)
		list (REMOVE_ITEM _ltjs_hlsl2glsl_iface Diligent-ShaderTools)
		set_target_properties (
			Diligent-HLSL2GLSLConverterLib
			PROPERTIES INTERFACE_LINK_LIBRARIES "${_ltjs_hlsl2glsl_iface}"
		)
	endif ()
	unset (_ltjs_hlsl2glsl_iface)
endif ()

add_subdirectory (libs/bibendovsky_spul_lib)
add_subdirectory (libs/ffmpeg)
add_subdirectory (libs/ltjs)
add_subdirectory (libs/ltjs_audio)
if (WIN32)
	add_subdirectory (libs/butemgr)
endif ()
add_subdirectory (libs/cryptmgr)
add_subdirectory (libs/lith)
if (WIN32)
	add_subdirectory (libs/mfcstub)
endif ()
if (WIN32)
	add_subdirectory (libs/regmgr)
endif ()
add_subdirectory (libs/stdlith)
add_subdirectory (libs/slikenet/ltjs)

add_subdirectory (engine/libs/ltmem)
add_subdirectory (engine/libs/rezmgr)
add_subdirectory (engine/runtime/info/src)
add_subdirectory (engine/runtime/render_a/src/sys/diligent)
add_subdirectory (engine/runtime/sound/src)
add_subdirectory (engine/runtime/sound/src/sys)
add_subdirectory (engine/runtime/ui/src)
if (WIN32)
	add_subdirectory (engine/runtime/winbuild/lithtech)
	add_subdirectory (engine/runtime/winbuild/ltmsg)
	add_subdirectory (engine/runtime/winbuild/server)
elseif (APPLE)
	add_subdirectory (engine/runtime/build/lithtech)
	add_subdirectory (engine/runtime/build/ltmsg)
	add_subdirectory (engine/runtime/build/server)
endif ()

if (LTJS_BUILD_DEDIT)
	add_subdirectory (tools/DEdit)
endif ()

if (LTJS_BUILD_DEDIT2)
	add_subdirectory (tools/DEdit2)
endif ()

if (LTJS_BUILD_RENDERDEMO)
	add_subdirectory (game/renderdemo)
endif ()

install (
	DIRECTORY
		"ltjs"
	DESTINATION
		"."
)
