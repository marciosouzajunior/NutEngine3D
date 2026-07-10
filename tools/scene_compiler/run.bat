@echo off
setlocal

set "TOOL_DIR=%~dp0"
set "PROJECT_ROOT=%TOOL_DIR%..\.."
set "COMPILER_EXE=%PROJECT_ROOT%\build\scene_compiler\NutSceneCompiler.exe"
set "INPUT_SCENE=%PROJECT_ROOT%\assets\scenes\demo.nutscene"
set "OUTPUT_HEADER=%PROJECT_ROOT%\build\assets\nano\demo_scene.h"

echo =========================================
echo  Running NutSceneCompiler...
echo =========================================
echo.

if not exist "%COMPILER_EXE%" (
    echo NutSceneCompiler.exe was not found.
    echo Run tools\scene_compiler\build.bat first.
    echo.
    pause
    exit /b 1
)

if not exist "%PROJECT_ROOT%\build\assets\nano" (
    mkdir "%PROJECT_ROOT%\build\assets\nano"
)

"%COMPILER_EXE%" "%INPUT_SCENE%" "%OUTPUT_HEADER%"

echo.
pause
