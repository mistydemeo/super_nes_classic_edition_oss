cmake_minimum_required(VERSION 2.8.11)

project(LuaJIT C ASM)

find_package(NerdCMakeModules REQUIRED)
include(NerdPackageHelper)

add_package(LuaJIT)

# LuaJIT options
option(LUAJIT_DISABLE_FFI "Disable FFI extension" OFF)
option(LUAJIT_DISABLE_JIT "Disable JIT compiler" OFF)
option(LUAJIT_ENABLE_LUA52COMPAT "Enable potentially breaking Lua 5.2 features" OFF)
option(LUAJIT_DISABLE_FPU "Disable FPU support" OFF)
option(LUAJIT_USE_ABI_SOFTFP "Use SOFTFP float ABI" OFF)

# LuaJIT debugging options
option(LUAJIT_USE_SYSMALLOC "Use system allocator (realloc) instead of the bundled one" OFF)
option(LUAJIT_USE_VALGRIND "Required to run LuaJIT under Valgrind" OFF)
option(LUAJIT_USE_GDBJIT "Enable GDB JIT API support" OFF)
option(LUA_USE_APICHECK "Enable assertions for the Lua/C API" OFF)
option(LUA_USE_ASSERT "Enable assertions for the whole LuaJIT VM" OFF)


# Set LuaJIT target architecture
if (NERD_ARCH_IS_X86)
	set(LUAJIT_TARGET "LUAJIT_ARCH_X86")
elseif(NERD_ARCH_IS_X86_64)
	set(LUAJIT_TARGET "LUAJIT_ARCH_X64")
elseif(NERD_ARCH_IS_ARM)
	set(LUAJIT_TARGET "LUAJIT_ARCH_ARM")
elseif(NERD_ARCH_IS_PPC)
	set(LUAJIT_TARGET "LUAJIT_ARCH_PPC")
else()
	message(FATAL_ERROR "Unsupported architecture")
endif()

# Set LuaJIT target OS
if (NERD_PLATFORM_IS_WINDOWS)
	set(LUAJIT_OS "LUAJIT_OS_WINDOWS")
elseif(NERD_PLATFORM_IS_LINUX)
	set(LUAJIT_OS "LUAJIT_OS_LINUX")
elseif(NERD_PLATFORM_IS_OSX OR NERD_PLATFORM_IS_IOS)
	set(LUAJIT_OS "LUAJIT_OS_OSX")
elseif(NERD_PLATFORM_IS_BSD)
	set(LUAJIT_OS "LUAJIT_OS_BSD")
elseif(NERD_PLATFORM_IS_POSIX)
	set(LUAJIT_OS "LUAJIT_OS_POSIX")
else()
	set(LUAJIT_OS "LUAJIT_OS_OTHER")
endif()

# Set compiler flags
set(LUAJIT_CFLAGS -DLUAJIT_TARGET=${LUAJIT_TARGET} -DLUAJIT_OS=${LUAJIT_OS})

if (LUAJIT_DISABLE_FFI)
	list(APPEND LUAJIT_CFLAGS -DLUAJIT_DISABLE_FFI)
endif()

if (LUAJIT_DISABLE_JIT)
	list(APPEND LUAJIT_CFLAGS -DLUAJIT_DISABLE_JIT)
endif()

if (LUAJIT_ENABLE_LUA52COMPAT)
	list(APPEND LUAJIT_CFLAGS -DLUAJIT_ENABLE_LUA52COMPAT)
endif()

if (LUAJIT_USE_SYSMALLOC)
	list(APPEND LUAJIT_CFLAGS -DLUAJIT_USE_SYSMALLOC)
endif()

if (LUAJIT_USE_VALGRIND)
	list(APPEND LUAJIT_CFLAGS -DLUAJIT_USE_VALGRIND)
endif()

if (LUAJIT_USE_GDBJIT)
	list(APPEND LUAJIT_CFLAGS -DLUAJIT_USE_GDBJIT)
endif()

if (LUA_USE_APICHECK)
	list(APPEND LUAJIT_CFLAGS -DLUA_USE_APICHECK)
endif()

if (LUA_USE_ASSERT)
	list(APPEND LUAJIT_CFLAGS -DLUA_USE_ASSERT)
endif()

if (LUAJIT_DISABLE_FPU)
	list(APPEND LUAJIT_CFLAGS -DLJ_ARCH_HASFPU=0)
else()
	list(APPEND LUAJIT_CFLAGS -DLJ_ARCH_HASFPU=1)
endif()

if (LUAJIT_USE_ABI_SOFTFP)
	list(APPEND LUAJIT_CFLAGS -DLJ_ABI_SOFTFP=1)
else()
	list(APPEND LUAJIT_CFLAGS -DLJ_ABI_SOFTFP=0)
endif()

# Set DynASM flags
if (NOT LUAJIT_DISABLE_FFI)
	list(APPEND DYNASM_FLAGS -D FFI)
endif()

if (NOT LUAJIT_DISABLE_JIT)
	list(APPEND DYNASM_FLAGS -D JIT)
endif()

if (NERD_PLATFORM_IS_WINDOWS)
	list(APPEND DYNASM_FLAGS -D WIN)
endif()

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
	list(APPEND DYNASM_FLAGS -D P64)
endif()

if (NOT LUAJIT_DISABLE_FPU)
	list(APPEND DYNASM_FLAGS -D FPU)
endif()

if (NOT LUAJIT_USE_ABI_SOFTFP)
	list(APPEND DYNASM_FLAGS -D HFABI)
endif()

# Set DynASM architecture
if (NERD_ARCH_IS_X86 OR NERD_ARCH_IS_X86_64)
	set(DYNASM_ARCH "x86")
elseif(NERD_ARCH_IS_ARM)
	set(DYNASM_ARCH "arm")
elseif(NERD_ARCH_IS_PPC)
	set(DYNASM_ARCH "ppc")
else()
	message(FATAL_ERROR "Unsupported architecture")
endif()

set(DYNASM_DASC "${CMAKE_CURRENT_SOURCE_DIR}/src/vm_${DYNASM_ARCH}.dasc")
set(DYNASM_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/dynasm/dynasm.lua")

set(TARGET_SIZEOF_VOID_P ${CMAKE_SIZEOF_VOID_P})

if (CMAKE_CROSSCOMPILING)
	# Convert lists to strings, allowing them to be passed to LuaJIT-host
	string(REPLACE ";" "^^" LUAJIT_CFLAGS_STRING "${LUAJIT_CFLAGS}")
	string(REPLACE ";" "^^" DYNASM_FLAGS_STRING "${DYNASM_FLAGS}")
	string(REPLACE ";" "^^" CMAKE_FIND_ROOT_PATH_STRING "${CMAKE_FIND_ROOT_PATH}")
	
	# Compile LuaJIT-host as an external project to use host toolchain instead of target's one. This is required when cross-compiling.
	include(ExternalProject)
	ExternalProject_Add(LuaJIT-host
		PREFIX LuaJIT-host
		LIST_SEPARATOR "^^"
		SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/host
		CMAKE_ARGS -DCMAKE_FIND_ROOT_PATH=${CMAKE_FIND_ROOT_PATH_STRING} -DTARGET_SIZEOF_VOID_P=${TARGET_SIZEOF_VOID_P} -DLUAJIT_CFLAGS=${LUAJIT_CFLAGS_STRING} -DDYNASM_SCRIPT=${DYNASM_SCRIPT} -DDYNASM_DASC=${DYNASM_DASC} -DDYNASM_FLAGS=${DYNASM_FLAGS_STRING}
		INSTALL_COMMAND ""
	)

	ExternalProject_Get_Property(LuaJIT-host BINARY_DIR)
	set(BUILDVM_PATH "${BINARY_DIR}/buildvm")

	add_executable(buildvm IMPORTED)
	set_target_properties(buildvm PROPERTIES IMPORTED_LOCATION ${BUILDVM_PATH})
	add_dependencies(buildvm LuaJIT-host)
else()
	add_subdirectory("src/host")
endif()

set(LUAJIT_LIB_FILES
	"src/lib_base.c"
	"src/lib_math.c"
	"src/lib_bit.c"
	"src/lib_string.c"
	"src/lib_table.c"
	"src/lib_io.c"
	"src/lib_os.c"
	"src/lib_package.c"
	"src/lib_debug.c"
	"src/lib_ffi.c"
	"src/lib_jit.c"
)

macro(add_buildvm_output output mode)
	add_custom_command(OUTPUT ${output}
		COMMAND buildvm -m ${mode} -o ${CMAKE_CURRENT_BINARY_DIR}/${output} ${ARGN}
		DEPENDS buildvm ${ARGN}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMENT "Generating ${output}"
		VERBATIM
	)
endmacro()

if (NERD_PLATFORM_IS_WINDOWS)
	set(LJ_VM_NAME "lj_vm.obj")
	set(LJ_VM_MODE "peobj")
elseif(NERD_PLATFORM_IS_OSX OR NERD_PLATFORM_IS_IOS)
	set(LJ_VM_NAME "lj_vm.s")
	set(LJ_VM_MODE "machasm")
else()
	set(LJ_VM_NAME "lj_vm.s")
	set(LJ_VM_MODE "elfasm")
endif()

add_buildvm_output(${LJ_VM_NAME} ${LJ_VM_MODE}                    )
add_buildvm_output(lj_bcdef.h    bcdef         ${LUAJIT_LIB_FILES})
add_buildvm_output(lj_ffdef.h    ffdef         ${LUAJIT_LIB_FILES})
add_buildvm_output(lj_libdef.h   libdef        ${LUAJIT_LIB_FILES})
add_buildvm_output(lj_recdef.h   recdef        ${LUAJIT_LIB_FILES})
add_buildvm_output(jit/vmdef.lua vmdef         ${LUAJIT_LIB_FILES})
add_buildvm_output(lj_folddef.h  folddef       src/lj_opt_fold.c  )

set(LUAJIT_INCLUDE_FILES
	src/lua.h
	src/luaconf.h
	src/lualib.h
	src/lauxlib.h
	src/lua.hpp
	src/luajit.h
)

set(LUAJIT_SRC_FILES
	${LUAJIT_LIB_FILES}
	${LUAJIT_INCLUDE_FILES}
	"src/lib_aux.c"
	"src/lib_init.c"
#	"src/ljamalg.c"
	"src/lj_alloc.c"
	"src/lj_alloc.h"
	"src/lj_api.c"
	"src/lj_arch.h"
	"src/lj_asm.c"
	"src/lj_asm.h"
	"src/lj_asm_arm.h"
	"src/lj_asm_mips.h"
	"src/lj_asm_ppc.h"
	"src/lj_asm_x86.h"
	"src/lj_bc.c"
	"src/lj_bc.h"
	"src/lj_bcdump.h"
	"src/lj_bcread.c"
	"src/lj_bcwrite.c"
	"src/lj_carith.c"
	"src/lj_carith.h"
	"src/lj_ccall.c"
	"src/lj_ccall.h"
	"src/lj_ccallback.c"
	"src/lj_ccallback.h"
	"src/lj_cconv.c"
	"src/lj_cconv.h"
	"src/lj_cdata.c"
	"src/lj_cdata.h"
	"src/lj_char.c"
	"src/lj_char.h"
	"src/lj_clib.c"
	"src/lj_clib.h"
	"src/lj_cparse.c"
	"src/lj_cparse.h"
	"src/lj_crecord.c"
	"src/lj_crecord.h"
	"src/lj_ctype.c"
	"src/lj_ctype.h"
	"src/lj_debug.c"
	"src/lj_debug.h"
	"src/lj_def.h"
	"src/lj_dispatch.c"
	"src/lj_dispatch.h"
	"src/lj_emit_arm.h"
	"src/lj_emit_mips.h"
	"src/lj_emit_ppc.h"
	"src/lj_emit_x86.h"
	"src/lj_err.c"
	"src/lj_err.h"
	"src/lj_errmsg.h"
	"src/lj_ff.h"
	"src/lj_ffrecord.c"
	"src/lj_ffrecord.h"
	"src/lj_frame.h"
	"src/lj_func.c"
	"src/lj_func.h"
	"src/lj_gc.c"
	"src/lj_gc.h"
	"src/lj_gdbjit.c"
	"src/lj_gdbjit.h"
	"src/lj_ir.c"
	"src/lj_ir.h"
	"src/lj_ircall.h"
	"src/lj_iropt.h"
	"src/lj_jit.h"
	"src/lj_lex.c"
	"src/lj_lex.h"
	"src/lj_lib.c"
	"src/lj_lib.h"
	"src/lj_load.c"
	"src/lj_mcode.c"
	"src/lj_mcode.h"
	"src/lj_meta.c"
	"src/lj_meta.h"
	"src/lj_obj.c"
	"src/lj_obj.h"
	"src/lj_opt_dce.c"
	"src/lj_opt_fold.c"
	"src/lj_opt_loop.c"
	"src/lj_opt_mem.c"
	"src/lj_opt_narrow.c"
	"src/lj_opt_sink.c"
	"src/lj_opt_split.c"
	"src/lj_parse.c"
	"src/lj_parse.h"
	"src/lj_record.c"
	"src/lj_record.h"
	"src/lj_snap.c"
	"src/lj_snap.h"
	"src/lj_state.c"
	"src/lj_state.h"
	"src/lj_str.c"
	"src/lj_str.h"
	"src/lj_strscan.c"
	"src/lj_strscan.h"
	"src/lj_tab.c"
	"src/lj_tab.h"
	"src/lj_target.h"
	"src/lj_target_arm.h"
	"src/lj_target_mips.h"
	"src/lj_target_ppc.h"
	"src/lj_target_x86.h"
	"src/lj_trace.c"
	"src/lj_trace.h"
	"src/lj_traceerr.h"
	"src/lj_udata.c"
	"src/lj_udata.h"
	"src/lj_vm.h"
	"src/lj_vmevent.c"
	"src/lj_vmevent.h"
	"src/lj_vmmath.c"
#	"src/luajit.c"

	# Generated files
	"${LJ_VM_NAME}"
	"lj_bcdef.h"
	"lj_ffdef.h"
	"lj_folddef.h"
	"lj_libdef.h"
	"lj_recdef.h"
)

add_library(LuaJIT ${LUAJIT_SRC_FILES})

target_compile_definitions(LuaJIT PUBLIC ${LUAJIT_CFLAGS})

target_include_directories(LuaJIT PUBLIC
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
	"$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/LuaJIT>"
)

set(LUA_LIBRARIES "LuaJIT::LuaJIT" CACHE STRING "Lua Libraries" FORCE)

package_library(LuaJIT)
install(FILES ${LUAJIT_INCLUDE_FILES} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/LuaJIT)
