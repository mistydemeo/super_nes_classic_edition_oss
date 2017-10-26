cmake_minimum_required(VERSION 2.8.11)

project(LuaJIT-host C)

find_package(NerdCMakeModules REQUIRED)
include(NerdDetect)

add_executable(minilua "minilua.c")

if (NERD_PLATFORM_IS_POSIX)
	# Link to math library
	target_link_libraries(minilua m)
endif()

add_custom_command(OUTPUT buildvm_arch.h
	COMMAND minilua ${DYNASM_SCRIPT} -LN ${DYNASM_FLAGS} -o ${CMAKE_CURRENT_BINARY_DIR}/buildvm_arch.h ${DYNASM_DASC}
	DEPENDS minilua ${DYNASM_SCRIPT} ${DYNASM_DASC}
	COMMENT "Generating buildvm_arch.h"
	VERBATIM
)

add_executable(buildvm
	"buildvm.c"
	"buildvm.h"
	"buildvm_asm.c"
	"buildvm_fold.c"
	"buildvm_lib.c"
	"buildvm_peobj.c"

	# Generated files
	"buildvm_arch.h"
)

target_include_directories(buildvm PRIVATE
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>"
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
)

target_compile_definitions(buildvm PUBLIC ${LUAJIT_CFLAGS})

# LuaJIT only support cross compilation if both architectures have the same pointer size
if (NOT TARGET_SIZEOF_VOID_P EQUAL CMAKE_SIZEOF_VOID_P)
	if (TARGET_SIZEOF_VOID_P EQUAL 4)
		set_target_properties(buildvm PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
	else()
		message(FATAL_ERROR "Cross-compiling LuaJIT on architectures having different pointer sizes is not supported")
	endif()
endif()

add_custom_command(TARGET buildvm POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:buildvm> ${PROJECT_BINARY_DIR}
)
