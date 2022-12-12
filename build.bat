@echo off

rem consider calling build_shaders here so everything gets built in one place.

set debug=1==1

if %debug% (
    echo Debug mode...
    set compiler_flags=/Zi /DDEBUG /MDd
    set linker_flags= ..\third_party\Effects11\Effects11d.lib
) else (
    echo Release mode...
    set compiler_flags=/O2 /MD
    set linker_flags= ..\third_party\Effects11\Effects11.lib
)

call setup-msvc.bat
if not exist "build" mkdir build
pushd build
%CL_BIN% ..\dxbook.cpp ^
  /I..\third_party\Effects11\include ^
  /std:c++20 %compiler_flags% /nologo /Felucydxcpp ^
  /link %linker_flags% ^
  /SUBSYSTEM:Windows
popd