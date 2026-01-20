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
		LTJS_USE_TANGENT_AND_BINORMAL
)

target_include_directories (
	${PROJECT_NAME}
	PRIVATE
		.
		${CMAKE_CURRENT_BINARY_DIR}/generated
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
		..
	)

set (diligent_shader_sources
	diligent_get_optimized_2d_vertex_shader_source=${CMAKE_CURRENT_SOURCE_DIR}/shaders/optimized_2d_vs.hlsl
	diligent_get_optimized_2d_pixel_shader_source=${CMAKE_CURRENT_SOURCE_DIR}/shaders/optimized_2d_ps.hlsl
	diligent_get_glow_blur_vertex_shader_source=${CMAKE_CURRENT_SOURCE_DIR}/shaders/glow_blur_vs.hlsl
	diligent_get_glow_blur_pixel_shader_source=${CMAKE_CURRENT_SOURCE_DIR}/shaders/glow_blur_ps.hlsl
	diligent_get_world_vertex_shader_source=${CMAKE_CURRENT_SOURCE_DIR}/shaders/world_vs.hlsl
	diligent_get_world_pixel_shader_textured_source=${CMAKE_CURRENT_SOURCE_DIR}/shaders/world_ps_textured.hlsl
	diligent_get_world_pixel_shader_glow_source=${CMAKE_CURRENT_SOURCE_DIR}/shaders/world_ps_glow.hlsl
	diligent_get_world_pixel_shader_lightmap_source=${CMAKE_CURRENT_SOURCE_DIR}/shaders/world_ps_lightmap.hlsl
	diligent_get_world_pixel_shader_lightmap_only_source=${CMAKE_CURRENT_SOURCE_DIR}/shaders/world_ps_lightmap_only.hlsl
	diligent_get_world_pixel_shader_dual_source=${CMAKE_CURRENT_SOURCE_DIR}/shaders/world_ps_dual.hlsl
	diligent_get_world_pixel_shader_bump_source=${CMAKE_CURRENT_SOURCE_DIR}/shaders/world_ps_bump.hlsl
	diligent_get_world_pixel_shader_volume_effect_source=${CMAKE_CURRENT_SOURCE_DIR}/shaders/world_ps_volume_effect.hlsl
	diligent_get_world_pixel_shader_lightmap_dual_source=${CMAKE_CURRENT_SOURCE_DIR}/shaders/world_ps_lightmap_dual.hlsl
	diligent_get_world_pixel_shader_solid_source=${CMAKE_CURRENT_SOURCE_DIR}/shaders/world_ps_solid.hlsl
	diligent_get_world_pixel_shader_dynamic_light_source=${CMAKE_CURRENT_SOURCE_DIR}/shaders/world_ps_dynamic_light.hlsl
	diligent_get_world_pixel_shader_shadow_project_source=${CMAKE_CURRENT_SOURCE_DIR}/shaders/world_ps_shadow_project.hlsl
	diligent_get_world_pixel_shader_debug_source=${CMAKE_CURRENT_SOURCE_DIR}/shaders/world_ps_debug.hlsl
	diligent_get_model_pixel_shader_textured_source=${CMAKE_CURRENT_SOURCE_DIR}/shaders/model_ps_textured.hlsl
	diligent_get_model_pixel_shader_solid_source=${CMAKE_CURRENT_SOURCE_DIR}/shaders/model_ps_solid.hlsl
	GetVertexShaderSource=${CMAKE_CURRENT_SOURCE_DIR}/../../../../render_b/src/sys/diligent/shaders/drawprim_vs.hlsl
	GetPixelShaderSourceTextured=${CMAKE_CURRENT_SOURCE_DIR}/../../../../render_b/src/sys/diligent/shaders/drawprim_ps_textured.hlsl
	GetPixelShaderSourceSolid=${CMAKE_CURRENT_SOURCE_DIR}/../../../../render_b/src/sys/diligent/shaders/drawprim_ps_solid.hlsl
)

set (diligent_shader_files
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/optimized_2d_vs.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/optimized_2d_ps.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/glow_blur_vs.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/glow_blur_ps.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/world_vs.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/world_ps_textured.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/world_ps_glow.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/world_ps_lightmap.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/world_ps_lightmap_only.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/world_ps_dual.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/world_ps_bump.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/world_ps_volume_effect.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/world_ps_lightmap_dual.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/world_ps_solid.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/world_ps_dynamic_light.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/world_ps_shadow_project.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/world_ps_debug.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/model_ps_textured.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/model_ps_solid.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../render_b/src/sys/diligent/shaders/drawprim_vs.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../render_b/src/sys/diligent/shaders/drawprim_ps_textured.hlsl
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../render_b/src/sys/diligent/shaders/drawprim_ps_solid.hlsl
)

set (diligent_shader_generated_dir ${CMAKE_CURRENT_BINARY_DIR}/generated)
set (diligent_shader_generated_cpp ${diligent_shader_generated_dir}/diligent_shaders_generated.cpp)
set (diligent_shader_generated_h ${diligent_shader_generated_dir}/diligent_shaders_generated.h)

set (diligent_shader_generator_cmake ${CMAKE_CURRENT_SOURCE_DIR}/cmake/generate_diligent_shaders.cmake)

add_custom_command (
	OUTPUT
		${diligent_shader_generated_cpp}
		${diligent_shader_generated_h}
	COMMAND
		${CMAKE_COMMAND}
		-DOUTPUT_CPP=${diligent_shader_generated_cpp}
		-DOUTPUT_H=${diligent_shader_generated_h}
		"-DSHADERS=${diligent_shader_sources}"
		-P ${diligent_shader_generator_cmake}
	DEPENDS
		${diligent_shader_files}
		${diligent_shader_generator_cmake}
	VERBATIM
)


	target_sources (
		${PROJECT_NAME}
		PRIVATE
			diligent_render.cpp
			texturescriptmgr.cpp
			texturescriptinstance.cpp
			texturescriptinterpreter.cpp
			texturescriptvarmgr.cpp
			${diligent_shader_generated_cpp}
			../rendererconsolevars.cpp
		../renderstylemap.cpp
		../aabb.cpp
		../viewparams.cpp
		../viewfrustum.cpp
		rendertarget.cpp
		rendertargetmgr.cpp
)

target_link_libraries (
	${PROJECT_NAME}
	PRIVATE
		Diligent-GraphicsEngine
		Diligent-GraphicsEngineVkInterface
		Diligent-GraphicsEngineVk-static
)
