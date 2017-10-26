@rem
@rem Script to build LuaJIT on MSVC for CTR
@rem
@setlocal

@if not defined VS100COMNTOOLS goto :FAIL_NOVS100
@call "%VS100COMNTOOLS%vsvars32.bat"

pushd %~dp0

@set DEFINES=-DLJ_TARGET_CTR -DLUAJIT_DISABLE_FFI -DLUAJIT_DISABLE_JIT -DLUAJIT_TARGET=LUAJIT_ARCH_arm -DLJ_ARCH_HASFPU -D__ARM_ARCH_6__ -D__ARM_EABI__ -DLUAJIT_USE_SYSMALLOC

@set HOST_CC=cl /nologo /c /MT /O2 /W1 /D _CRT_SECURE_NO_DEPRECATE /I .
@set HOST_LINK=link /nologo
@set HOST_CFLAGS=%DEFINES%
@rem ALL_LIB=lib_base.c lib_math.c lib_bit.c lib_string.c lib_table.c lib_io.c lib_os.c lib_package.c lib_debug.c lib_jit.c lib_ffi.c
@set ALL_LIB=lib_base.c lib_math.c lib_bit.c lib_string.c lib_table.c lib_io.c lib_os.c lib_package.c lib_debug.c lib_jit.c lib_ffi.c

@%HOST_CC% %HOST_CFLAGS% host\minilua.c
@if errorlevel 1 goto :BAD
@%HOST_LINK% /out:minilua.exe minilua.obj
@if errorlevel 1 goto :BAD

@minilua ../dynasm/dynasm.lua -LN -D FPU -D HFABI -o host/buildvm_arch.h vm_arm.dasc
@if errorlevel 1 goto :BAD

@%HOST_CC% %HOST_CFLAGS% /I ..\dynasm host\buildvm*.c
@if errorlevel 1 goto :BAD
@%HOST_LINK% /out:buildvm.exe buildvm*.obj
@if errorlevel 1 goto :BAD

@buildvm -m elfasm -o lj_vm.s
@if errorlevel 1 goto :BAD
@buildvm -m ffdef -o lj_ffdef.h %ALL_LIB%
@if errorlevel 1 goto :BAD
@buildvm -m bcdef -o lj_bcdef.h %ALL_LIB%
@if errorlevel 1 goto :BAD
@buildvm -m folddef -o lj_folddef.h lj_opt_fold.c
@if errorlevel 1 goto :BAD
@buildvm -m recdef -o lj_recdef.h %ALL_LIB%
@if errorlevel 1 goto :BAD
@buildvm -m libdef -o lj_libdef.h %ALL_LIB%
@if errorlevel 1 goto :BAD
@buildvm -m vmdef -o jit\vmdef.lua %ALL_LIB%
@if errorlevel 1 goto :BAD

@goto END

:FAIL_NOVS100
@echo.
@echo *******************************************************
@echo Could not find Visual Studio install directory
@echo *******************************************************
@goto :END

:BAD
@echo.
@echo *******************************************************
@echo Build FAILED -- Please check the error messages
@echo *******************************************************
@goto :END

:END