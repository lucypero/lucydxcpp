@echo off

set debug=1==0

set flags=

if %debug% (
    set flags=%flags% /Fc /Od /Zi
) else (
    set flags=%flags%
)

if not exist "build" mkdir build
pushd build
fxc %flags% /nologo /T fx_5_0 /Fo color.fxo ..\src\shaders\color.fx
fxc %flags% /nologo /T fx_5_0 /Fo color_trippy.fxo ..\src\shaders\color_trippy.fx
fxc %flags% /nologo /T fx_5_0 /Fo basic.fxo ..\src\shaders\basic.fx
fxc %flags% /nologo /T fx_5_0 /Fo toon.fxo ..\src\shaders\toon.fx
popd
