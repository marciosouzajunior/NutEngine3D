@echo off
setlocal

set "TOOL_DIR=%~dp0"
set "PROJECT_ROOT=%TOOL_DIR%..\.."
set "BUILD_DIR=%PROJECT_ROOT%\build\scene_compiler"

echo =========================================
echo  Building NutSceneCompiler...
echo =========================================
echo.

cmake -S "%PROJECT_ROOT%\tools\scene_compiler" -B "%BUILD_DIR%"
if errorlevel 1 (
    echo.
    pause
    exit /b 1
)

cmake --build "%BUILD_DIR%"

echo.
pause
