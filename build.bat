@echo off
setlocal

set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
call "%VCVARS%" >nul
if errorlevel 1 (
    echo Failed to initialize MSVC build environment.
    exit /b 1
)

set "SRC=src"
set "RES=resources"
set "OUT=build"

if not exist "%OUT%" mkdir "%OUT%"

echo Compiling resources...
rc.exe /nologo /fo "%OUT%\app.res" "%RES%\app.rc"
if errorlevel 1 exit /b 1

echo Compiling C++ (GDI+ rendering shim)...
cl.exe /nologo /c /O1 /Os /MT /EHsc /utf-8 /std:c++17 /GS ^
    /Fo"%OUT%\gfx_gdiplus.obj" "%SRC%\gfx_gdiplus.cpp"
if errorlevel 1 exit /b 1

echo Compiling C sources...
cl.exe /nologo /c /O1 /Os /MT /W3 /utf-8 /std:c17 /GS /D_CRT_SECURE_NO_WARNINGS ^
    /Fo"%OUT%\\" ^
    "%SRC%\main.c" "%SRC%\app.c" "%SRC%\theme.c" "%SRC%\layout.c" "%SRC%\numfmt.c" ^
    "%SRC%\calc_standard.c" "%SRC%\calc_expr.c" "%SRC%\expr_parser.c" "%SRC%\settings.c"
if errorlevel 1 exit /b 1

echo Linking...
rem gdiplus.dll and dwmapi.dll are delay-loaded so DLL search order can be
rem locked to System32 at process start (main.c calls
rem SetDefaultDllDirectories before either is first touched), closing off
rem DLL search-order / binary-planting hijacking (CWE-427).
rem /OPT:REF and /OPT:ICF strip unreferenced and duplicate code/data; both
rem are implied by non-incremental release linking but are made explicit
rem here since /INCREMENTAL:NO is also explicit.
link.exe /nologo /SUBSYSTEM:WINDOWS /OUT:"%OUT%\SimpleCalc.exe" /MANIFEST:NO ^
    /INCREMENTAL:NO /OPT:REF /OPT:ICF ^
    /DELAYLOAD:gdiplus.dll /DELAYLOAD:dwmapi.dll ^
    "%OUT%\main.obj" "%OUT%\app.obj" "%OUT%\theme.obj" "%OUT%\layout.obj" "%OUT%\numfmt.obj" ^
    "%OUT%\calc_standard.obj" "%OUT%\calc_expr.obj" "%OUT%\expr_parser.obj" "%OUT%\settings.obj" ^
    "%OUT%\gfx_gdiplus.obj" "%OUT%\app.res" ^
    user32.lib gdi32.lib gdiplus.lib dwmapi.lib advapi32.lib delayimp.lib
if errorlevel 1 exit /b 1

echo.
echo Build succeeded: %OUT%\SimpleCalc.exe
endlocal
