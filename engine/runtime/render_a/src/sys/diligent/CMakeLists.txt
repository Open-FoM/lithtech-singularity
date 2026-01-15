cmake_minimum_required (VERSION 3.24.4 FATAL_ERROR)
project (ltjs_lib_diligent_render VERSION 0.0.1 LANGUAGES CXX)

include (ltjs_common)

add_library (${PROJECT_NAME} STATIC)

ltjs_add_defaults (${PROJECT_NAME})

target_compile_definitions (
	${PROJECT_NAME}
	PRIVATE
		LTJS_DILIGENT_RENDER
		LTJS_USE_DILIGENT_RENDER
		MODEL_SUPPORT_ABC
)

target_include_directories (
	${PROJECT_NAME}
	PRIVATE
		.
		../../../../../../libs/diligent/DiligentCore
		../../../../../../libs/diligent/DiligentCore/Common/interface
		../../../../../../libs/diligent/DiligentCore/Graphics/GraphicsEngine/interface
		../../../../../../libs/diligent/DiligentCore/Graphics/GraphicsEngineVulkan/interface
		../../../../../../libs/stdlith
		../../../../../sdk/inc
		../../../../../sdk/inc/physics
		../../../../client/src
		../../../../kernel/io/src
		../../../../kernel/mem/src
		../../../../kernel/src
		../../../../kernel/src/sys/win
		../../../../lithtemplate
		../../../../model/src
		../../../../shared/src
		../../../../shared/src/sys/win
		../../../../world/src
)

target_sources (
	${PROJECT_NAME}
	PRIVATE
		diligent_render.cpp
)

target_link_libraries (
	${PROJECT_NAME}
	PRIVATE
		Diligent-GraphicsEngine
		Diligent-GraphicsEngineVkInterface
		Diligent-GraphicsEngineVk-static
)
