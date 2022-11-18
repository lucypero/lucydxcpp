@echo off
call setup-msvc.bat
if not exist "build" mkdir build
pushd build
%CL_BIN% ..\main.cpp /Zi /DDEBUG /nologo /Felucydxcpp /link /SUBSYSTEM:Windows
popd
