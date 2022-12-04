@echo off

set FXC_BIN="C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64\fxc.exe"

if not exist "build" mkdir build
pushd build
%FXC_BIN% /T vs_5_0 /E vs_main /nologo /Fo vert.compiled_shader ..\shaders.hlsl
%FXC_BIN% /T ps_5_0 /E ps_main /nologo /Fo pixel.compiled_shader ..\shaders.hlsl
popd
