@echo off
setlocal EnableDelayedExpansion

set CONFIG=%~1
if /I not "%CONFIG%"=="debug" if /I not "%CONFIG%"=="release" (
    set CONFIG=debug
)

set OUT_DIR=bin
set BUILD_DIR=build

set EXE_NAME=cdxr

set INCLUDE_DIRS=/Isrc
set SOURCE_FILES=src\main.cpp src\Renderer.cpp
set LIBS=user32.lib dxgi.lib d3d12.lib

if /I "%CONFIG%"=="debug" (
    echo Building in debug...
    set COMPILER_FLAGS=/EHsc /W4 /Z7 /std:c++17 /fsanitize=address
    set LINKER_FLAGS=/DEBUG:FULL /MANIFEST:EMBED
) else (
    echo Building in release...
    set COMPILER_FLAGS=/O2 /EHsc /std:c++17
    set LINKER_FLAGS=/DEBUG:NONE
)

if not exist %OUT_DIR% mkdir %OUT_DIR%
if not exist %BUILD_DIR% mkdir %BUILD_DIR%

REM Sets environment variable "VCROOT" to  Drive:\Path\To\VS\VC.
call tools\locateMSVC.bat
if errorlevel 1 exit /B %ERRORLEVEL%

REM Call vcvarsall.bat to set up environment variables
@call "%VCROOT%\Auxiliary\Build\vcvarsall.bat" x64 >nul

@cl %COMPILER_FLAGS% %INCLUDE_DIRS% /c ^
    %SOURCE_FILES% /Fo:build\ 2>&1 | findstr /i "error warning"

REM An empty string to hold the list of .obj files
set OBJ_FILES=

REM Add each .obj file to the list
for /R %BUILD_DIR% %%G in (*.obj) do (
    set OBJ_FILES=!OBJ_FILES! %%G
)

@link %LINKER_FLAGS% /OUT:%OUT_DIR%/%EXE_NAME%.exe ^
      %OBJ_FILES% %LIBS% 2>&1 | findstr /i "error warning"

if exist "%~dp0bin\%EXE_NAME%.ilk" del "%~dp0bin\%EXE_NAME%.ilk"
if exist "%~dp0bin\%EXE_NAME%.rdi" del "%~dp0bin\%EXE_NAME%.rdi"

if exist "%~dp0build" @RD /S /Q "%~dp0build"

endlocal
