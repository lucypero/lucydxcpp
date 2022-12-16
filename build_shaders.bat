@echo off

set FXC_BIN="C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64\fxc.exe"

set debug=1==1

if %debug% (
    set flags=/Fc /Od /Zi
) else (
    set flags=
)

if not exist "build" mkdir build
pushd build
%FXC_BIN% %flags% /nologo /T fx_5_0 /Fo color.fxo ..\src\shaders\color.fx
popd
