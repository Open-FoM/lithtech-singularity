cmake_minimum_required (VERSION 3.24.4 FATAL_ERROR)
project (ltjs VERSION 0.0.1 LANGUAGES CXX)

if (NOT WIN32 AND NOT APPLE)
	message (FATAL_ERROR "WIN32 or APPLE only")
endif ()

set (CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

#
# Options
#
option (LTJS_SDL_BACKEND "Use SDL backend." ON)
option (LTJS_USE_PCH "Use precompiled headers." ON)

#
# SDL3
#
find_package (SDL3 3.2.0 REQUIRED)

# Diligent Engine (core only)
set (DILIGENT_NO_DIRECT3D12 ON CACHE BOOL "Disable D3D12 backend" FORCE)
set (DILIGENT_NO_OPENGL ON CACHE BOOL "Disable OpenGL backend" FORCE)
set (DILIGENT_NO_WEBGPU ON CACHE BOOL "Disable WebGPU backend" FORCE)
if (APPLE)
	set (DILIGENT_NO_METAL OFF CACHE BOOL "Enable Metal backend on Apple" FORCE)
else ()
	set (DILIGENT_NO_METAL ON CACHE BOOL "Disable Metal backend on non-Apple" FORCE)
endif ()
add_subdirectory (libs/diligent/DiligentCore)

add_subdirectory (libs/bibendovsky_spul_lib)
add_subdirectory (libs/ffmpeg)
add_subdirectory (libs/ltjs)
add_subdirectory (libs/ltjs_audio)
if (WIN32)
	add_subdirectory (libs/butemgr)
endif ()
add_subdirectory (libs/cryptmgr)
add_subdirectory (libs/freetype)
add_subdirectory (libs/lith)
if (WIN32)
	add_subdirectory (libs/mfcstub)
endif ()
if (WIN32)
	add_subdirectory (libs/regmgr)
endif ()
add_subdirectory (libs/stdlith)

add_subdirectory (engine/libs/ltmem)
if (WIN32)
	add_subdirectory (engine/libs/rezmgr)
endif ()
if (WIN32)
	add_subdirectory (engine/runtime/info/src)
endif ()
add_subdirectory (engine/runtime/render_a/src/sys/diligent)
add_subdirectory (engine/runtime/sound/src)
add_subdirectory (engine/runtime/sound/src/sys)
add_subdirectory (engine/runtime/ui/src)
if (WIN32)
	add_subdirectory (engine/runtime/winbuild/lithtech)
	add_subdirectory (engine/runtime/winbuild/ltmsg)
	add_subdirectory (engine/runtime/winbuild/server)
endif ()

install (
	DIRECTORY
		"ltjs"
	DESTINATION
		"."
)
