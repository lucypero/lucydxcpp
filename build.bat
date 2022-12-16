@echo off
call setup-msvc.bat

call build_shaders.bat

set debug=1==1

set compiler_flags=/W3 /WX

if %debug% (
    echo Debug mode...
    set compiler_flags=%compiler_flags% /Zi /DDEBUG /MDd
    set linker_flags=Effects11d.lib imguid.obj
) else (
    echo Release mode...
    set compiler_flags=%compiler_flags% /O2 /MD
    set linker_flags=Effects11.lib imgui.obj
)

if not exist "build" mkdir build
pushd build

%CL_BIN% ..\src\dxbook.cpp ^
  /I..\third_party\include ^
  /I..\third_party\include\imgui ^
  /std:c++20 %compiler_flags% /nologo /Felucydxcpp ^
  /link /LIBPATH:..\third_party\lib %linker_flags% ^
  /SUBSYSTEM:Windows
popd