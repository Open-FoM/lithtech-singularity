cmake_minimum_required (VERSION 3.24.4 FATAL_ERROR)
project (slikenet_lib VERSION 0.0.1 LANGUAGES CXX)

set (SLIKENET_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")

file (GLOB SLIKENET_SOURCES CONFIGURE_DEPENDS
	"${SLIKENET_ROOT}/Source/src/*.cpp"
)

add_library (${PROJECT_NAME} STATIC)

set_target_properties (${PROJECT_NAME} PROPERTIES
	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
)

target_compile_definitions (
	${PROJECT_NAME}
	PRIVATE
		OPEN_SSL_CLIENT_SUPPORT=0
)

target_include_directories (
	${PROJECT_NAME}
	PUBLIC
		"${SLIKENET_ROOT}/Source/include"
)

target_sources (
	${PROJECT_NAME}
	PRIVATE
		${SLIKENET_SOURCES}
)

if (WIN32)
	target_link_libraries (
		${PROJECT_NAME}
		PUBLIC
			ws2_32
	)
else ()
	target_link_libraries (
		${PROJECT_NAME}
		PUBLIC
			pthread
	)
endif ()
