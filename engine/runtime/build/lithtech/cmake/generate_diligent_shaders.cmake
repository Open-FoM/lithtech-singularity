cmake_minimum_required(VERSION 3.10)

if (NOT DEFINED OUTPUT_CPP OR NOT DEFINED OUTPUT_H OR NOT DEFINED SHADERS)
	message(FATAL_ERROR "OUTPUT_CPP, OUTPUT_H, and SHADERS must be set.")
endif ()

get_filename_component(output_cpp_dir "${OUTPUT_CPP}" DIRECTORY)
get_filename_component(output_h_dir "${OUTPUT_H}" DIRECTORY)
file(MAKE_DIRECTORY "${output_cpp_dir}")
file(MAKE_DIRECTORY "${output_h_dir}")

set(header_content "/* Generated file. Do not edit. */\n#ifndef LTJS_DILIGENT_SHADERS_GENERATED_H\n#define LTJS_DILIGENT_SHADERS_GENERATED_H\n\n")
set(source_content "/* Generated file. Do not edit. */\n#include \"diligent_shaders_generated.h\"\n\n")

foreach (item IN LISTS SHADERS)
	string(REPLACE "=" ";" parts "${item}")
	list(LENGTH parts parts_len)
	if (NOT parts_len EQUAL 2)
		message(FATAL_ERROR "Invalid shader mapping: ${item}")
	endif ()
	list(GET parts 0 func_name)
	list(GET parts 1 shader_path)

	file(READ "${shader_path}" shader_content)
	string(REPLACE "\r\n" "\n" shader_content "${shader_content}")
	string(REPLACE "\\" "\\\\" shader_content "${shader_content}")
	string(REPLACE "\"" "\\\"" shader_content "${shader_content}")
	string(REPLACE "\n" "\\n\"\n\"" shader_content "${shader_content}")

	string(REGEX REPLACE "[^A-Za-z0-9_]" "_" var_name "${func_name}")

	set(source_content "${source_content}static const char k_${var_name}[] = \"${shader_content}\";\n")
	set(source_content "${source_content}const char* ${func_name}()\n{\n\treturn k_${var_name};\n}\n\n")
	set(header_content "${header_content}const char* ${func_name}();\n")
endforeach ()

set(header_content "${header_content}\n#endif\n")

file(WRITE "${OUTPUT_CPP}" "${source_content}")
file(WRITE "${OUTPUT_H}" "${header_content}")
