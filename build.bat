@echo off
call setup-msvc.bat
if not exist "build" mkdir build
pushd build
%CL_BIN% ..\dxbook.cpp /std:c++20 /Zi /DDEBUG /nologo /Felucydxcpp /link /SUBSYSTEM:Windows
popd