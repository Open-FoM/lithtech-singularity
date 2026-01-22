cmake_minimum_required (VERSION 3.24.4 FATAL_ERROR)
project (ltjs_lib_sdk VERSION 0.0.1 LANGUAGES CXX)

include (ltjs_common)

add_library (${PROJECT_NAME} STATIC)

ltjs_add_defaults (${PROJECT_NAME})

target_sources (
	${PROJECT_NAME}
	PRIVATE
		lteulerangles.cpp
		ltmodule.cpp
		lterror.cpp
		ltobjref.cpp
		ltquatbase.cpp
)

target_include_directories (
	${PROJECT_NAME}
	PRIVATE
		.
)
