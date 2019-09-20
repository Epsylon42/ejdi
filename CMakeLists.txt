cmake_minimum_required(VERSION 3.6)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
include(HunterGate)
HunterGate(
   URL "https://github.com/ruslo/hunter/archive/v0.23.149.tar.gz"
   SHA1 "c9f170bd697ac6117e68137a2cffdeca679db67e"
)
project(ejdi LANGUAGES C CXX VERSION 0.0.1)

include(cppm_tool)
cppm_load()
cppm_cxx_standard(2a)
cppm_compiler_option(
	DEBUG
		CLANG "-Wall -Werror"
		GCC   "-Wall -Werror"
		MSVC  "/MP"
	RELEASE
		CLANG "-Wall -Werror"
		GCC   "-Wall -Werror"
		MSVC  "/MP"
)



cppm_target_define(ejdi BINARY 
SOURCES 
	src/lexem_groups.cpp
	src/main.cpp
	src/lexer.cpp
	src/parser.cpp
	src/span.cpp
	src/ast.cpp
)


cppm_target_dependencies(ejdi 
)

cppm_target_install(ejdi)

