cmake_minimum_required (VERSION 3.24.4 FATAL_ERROR)
project (ltjs_dedit VERSION 0.0.1 LANGUAGES CXX)

include (ltjs_common)

if (NOT APPLE)
	message (STATUS "DEdit CMake target is only available on macOS; Windows builds should use the legacy VC project.")
	return ()
endif ()

if (NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/dedit_app_main.cpp" OR
	NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/dedit_platform_macos.mm")
	message (STATUS "DEdit sources removed; skipping legacy DEdit target.")
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
		dedit_app_main.cpp
		dedit_platform_macos.mm
		${CMAKE_SOURCE_DIR}/engine/libs/ltamgr/ltabitfile.cpp
		${CMAKE_SOURCE_DIR}/engine/libs/ltamgr/ltacompressedfile.cpp
		${CMAKE_SOURCE_DIR}/engine/libs/ltamgr/ltaconverter.cpp
		${CMAKE_SOURCE_DIR}/engine/libs/ltamgr/ltafile.cpp
		${CMAKE_SOURCE_DIR}/engine/libs/ltamgr/ltafilebuffer.cpp
		${CMAKE_SOURCE_DIR}/engine/libs/ltamgr/ltahuffmantree.cpp
		${CMAKE_SOURCE_DIR}/engine/libs/ltamgr/ltaloadonlyalloc.cpp
		${CMAKE_SOURCE_DIR}/engine/libs/ltamgr/ltanode.cpp
		${CMAKE_SOURCE_DIR}/engine/libs/ltamgr/ltanodebuilder.cpp
		${CMAKE_SOURCE_DIR}/engine/libs/ltamgr/ltanodeiterator.cpp
		${CMAKE_SOURCE_DIR}/engine/libs/ltamgr/ltanodreader.cpp
		${CMAKE_SOURCE_DIR}/engine/libs/ltamgr/ltareader.cpp
		${CMAKE_SOURCE_DIR}/engine/libs/ltamgr/ltautil.cpp
		${CMAKE_SOURCE_DIR}/engine/libs/ltamgr/ltawriter.cpp
		${CMAKE_SOURCE_DIR}/engine/libs/ltamgr/lzsswindow.cpp
		${CMAKE_SOURCE_DIR}/libs/imgui/imgui.cpp
		${CMAKE_SOURCE_DIR}/libs/imgui/imgui_draw.cpp
		${CMAKE_SOURCE_DIR}/libs/imgui/imgui_tables.cpp
		${CMAKE_SOURCE_DIR}/libs/imgui/imgui_widgets.cpp
		${CMAKE_SOURCE_DIR}/libs/imgui/misc/cpp/imgui_stdlib.cpp
		${CMAKE_SOURCE_DIR}/libs/imgui/backends/imgui_impl_sdl3.cpp
		${CMAKE_SOURCE_DIR}/libs/diligent/DiligentTools/Imgui/src/ImGuiDiligentRenderer.cpp
		${CMAKE_SOURCE_DIR}/libs/diligent/DiligentTools/Imgui/src/ImGuiImplDiligent.cpp
		${CMAKE_SOURCE_DIR}/libs/diligent/DiligentTools/Imgui/src/ImGuiImplSDL3.cpp
		${CMAKE_SOURCE_DIR}/libs/diligent/DiligentTools/Imgui/src/ImGuiUtils.cpp
)

target_include_directories (
	${PROJECT_NAME}
	PRIVATE
		${CMAKE_SOURCE_DIR}/libs/imgui
		${CMAKE_SOURCE_DIR}/libs/imgui/backends
		${CMAKE_SOURCE_DIR}/libs/diligent
		${CMAKE_SOURCE_DIR}/libs/diligent/DiligentCore/Common/interface
		${CMAKE_SOURCE_DIR}/libs/diligent/DiligentCore/Graphics/GraphicsEngine/interface
		${CMAKE_SOURCE_DIR}/libs/diligent/DiligentCore/Graphics/GraphicsEngineVulkan/interface
		${CMAKE_SOURCE_DIR}/libs/diligent/DiligentCore/Graphics/GraphicsAccessories/interface
		${CMAKE_SOURCE_DIR}/libs/diligent/DiligentCore/Graphics/GraphicsTools/interface
		${CMAKE_SOURCE_DIR}/libs/diligent/DiligentTools/Imgui/interface
		${CMAKE_SOURCE_DIR}/engine/libs/ltamgr
		${CMAKE_SOURCE_DIR}/engine/sdk/inc
)

target_link_libraries (
	${PROJECT_NAME}
	PRIVATE
		Diligent-GraphicsEngineVk-static
)

if (APPLE)
	target_link_libraries (
		${PROJECT_NAME}
		PRIVATE
			"-framework Cocoa"
			"-framework Metal"
			"-framework QuartzCore"
	)
endif ()
