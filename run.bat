@echo off
setlocal EnableDelayedExpansion

REM Sets environment variable "VCROOT" to  Drive:\Path\To\VS\VC.
call tools\locateMSVC.bat
if errorlevel 1 exit /B %ERRORLEVEL%

set /p MSVC_VERSION=<"%VCROOT%\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt"

pushd bin

REM Needed for ASan
robocopy "%VCROOT%\Tools\MSVC\%MSVC_VERSION%\bin\Hostx64\x64" "%~dp0bin" ^
          clang_rt.asan_dynamic-x86_64.dll /mt 2>&1 | findstr /i "ERROR"

cdxr.exe

popd
endlocal
