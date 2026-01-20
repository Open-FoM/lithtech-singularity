cmake_minimum_required (VERSION 3.24.4 FATAL_ERROR)

################
# Common stuff #
################

function (ltjs_add_defaults)
	if (NOT (${ARGC} EQUAL 1))
		message (FATAL_ERROR "Usage: ltjs_add_defaults <target_name>")
	endif ()

	set_target_properties (${ARGV0} PROPERTIES
		CXX_STANDARD 20
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF
	)

	set_target_properties (${ARGV0} PROPERTIES
		C_STANDARD 99
		C_STANDARD_REQUIRED ON
		C_EXTENSIONS OFF
	)

	target_compile_definitions (
		${ARGV0}
		PRIVATE
			NOPS2
			$<$<NOT:$<CONFIG:DEBUG>>:_FINAL>
			$<$<CONFIG:DEBUG>:_DEBUG>
			$<$<BOOL:${LTJS_SDL_BACKEND}>:LTJS_SDL_BACKEND>
			$<$<BOOL:${LTJS_USE_D3DX9}>:LTJS_USE_D3DX9>
	)

	target_include_directories (
		${ARGV0}
		PRIVATE
			$<$<BOOL:${LTJS_ROOT}>:${LTJS_ROOT}/libs/ltjs/include>
			$<$<NOT:$<BOOL:${LTJS_ROOT}>>:${CMAKE_SOURCE_DIR}/libs/ltjs/include>
	)

	if (MSVC)
		target_compile_definitions (
			${ARGV0}
			PRIVATE
				$<$<CONFIG:DEBUG>:_CRT_SECURE_NO_WARNINGS>
				$<$<CONFIG:DEBUG>:_ITERATOR_DEBUG_LEVEL=0>
				_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS
		)
	endif ()

	if (WIN32)
		target_compile_definitions (
			${ARGV0}
			PRIVATE
				NOMINMAX
		)
	endif ()

	if (APPLE)
		target_compile_definitions (
			${ARGV0}
			PRIVATE
				__LINUX
				PLATFORM_MACOS=1
		)
	endif ()

	if (MSVC)
		target_compile_options (
			${ARGV0}
			PRIVATE
			# Warning Level
				-W4
			# Multi-processor Compilation
				-MP
			# Suppress "unreferenced formal parameter" warning
				-wd4100
			# Suppress "The POSIX name for this item is deprecated" warning
				-wd4996
			# Runtime Library (Multi-threaded Debug)
				$<$<CONFIG:DEBUG>:-MTd>
		)

		if (CMAKE_SIZEOF_VOID_P EQUAL 4)
			target_compile_options (
				${ARGV0}
				PRIVATE
				# No Enhanced Instructions (prevents overflow of the x87 FPU stack)
					-arch:IA32
			)
		endif ()
	endif ()

	if (MINGW)
		target_compile_options (
			${ARGV0}
			# Warning Level
			PRIVATE
				-Wfatal-errors
		)
	endif ()

	target_link_libraries (
		${ARGV0}
		PRIVATE
			SDL3::SDL3
	)

	if (NOT ${PROJECT_NAME} STREQUAL "ltjs_lib")
		get_target_property(_ltjs_target_type ${ARGV0} TYPE)
		if (NOT _ltjs_target_type STREQUAL "STATIC_LIBRARY")
			target_link_libraries (
				${ARGV0}
				PRIVATE
					ltjs_lib
			)
		endif ()
	endif ()
endfunction (ltjs_add_defaults)
