@echo off
rem script to minimally setup msvc (cl.exe) like vcvars does but much faster.
rem it will only work for the target x64 (you can change the `set CL_TARGET` bellow)
rem and with the latest visual studio installation (2017 or newer) that vswhere.exe can find.
rem
rem it's also possible to customize this script somewhat by previously defining some of the env vars that it uses:
rem - `vs_dir`: the directory containing the visual studio installation you want to use
rem             NOTE: defining this will entirely skip running `vswhere.exe`!
rem - `msvc_tools_dir`: the specific MSVC directory inside your visual studio installation directory
rem - `wsdk_dir`: the windows sdk directory
rem - `wsdk_include_dir`: the 'include' directory inside the windows sdk installation
rem - `wsdk_lib_dir`: the 'lib' directory inside the windows sdk installation
rem
rem finally, after running this script, you can invoke `cl.exe` by using the env var `CL_BIN`.
rem so you can, for instance, use it like this: `%CL_BIN% main.c`

set CL_TARGET=x64

if not defined vs_dir (
    for /f "tokens=*" %%d in ('"%programfiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.VisualStudio.Workload.NativeDesktop -property installationPath') do set vs_dir=%%d
)
if not defined vs_dir (
    echo could not find visual studio installation. set `vs_dir` env var
    exit /b
)

if not defined msvc_tools_dir (
    for /d %%d in ("%vs_dir%\VC\Tools\MSVC\*") do set msvc_tools_dir=%%d
)
if not defined msvc_tools_dir (
    echo could not find msvc tools dir. set `msvc_tools_dir` env var
    exit /b
)

if not defined wsdk_dir (
    for /d %%d in ("%programfiles(x86)%\Windows Kits\*") do if exist "%%d\Include" (set wsdk_dir=%%d)
)
if not defined wsdk_dir (
    echo could not find windows sdk dir. set `wsdk_dir` env var
    exit /b
)

if not defined wsdk_include_dir (
    for /d %%d in ("%wsdk_dir%\Include\*") do set wsdk_include_dir=%%d
)
if not defined wsdk_include_dir (
    echo could not find windows sdk include dir. set `wsdk_include_dir` env var
    exit /b
)

if not defined wsdk_lib_dir (
    for /d %%d in ("%wsdk_dir%\Lib\*") do set wsdk_lib_dir=%%d
)
if not defined wsdk_lib_dir (
    echo could not find windows sdk lib dir. set `wsdk_lib_dir` env var
    exit /b
)

setlocal EnableDelayedExpansion

set INCLUDE=%msvc_tools_dir%\include
for /d %%d in ("%wsdk_include_dir%\*") do set INCLUDE=!INCLUDE!;%%d
set LIB=%msvc_tools_dir%\lib\%CL_TARGET%
for /d %%d in ("%wsdk_lib_dir%\*") do set LIB=!LIB!;%%d\%CL_TARGET%

endlocal & set INCLUDE=%INCLUDE% & set LIB=%LIB%

set CL_BIN="%msvc_tools_dir%\bin\HostX64\%CL_TARGET%\cl.exe"
