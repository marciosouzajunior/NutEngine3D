@echo off
setlocal

set "TOOL_DIR=%~dp0"
set "PROJECT_ROOT=%TOOL_DIR%..\.."
set "COMPILER_EXE=%PROJECT_ROOT%\build\scene_compiler\NutSceneCompiler.exe"
set "INPUT_SCENE=%~1"
set "OUTPUT_HEADER=%PROJECT_ROOT%\build\assets\nano\demo_scene.h"

if "%INPUT_SCENE%"=="" (
    echo Usage: tools\scene_compiler\run_scene.bat assets\scenes\file.nutscene
    echo.
    exit /b 1
)

if not exist "%COMPILER_EXE%" (
    echo NutSceneCompiler.exe was not found.
    echo Run tools\scene_compiler\build.bat first.
    echo.
    exit /b 1
)

if not exist "%PROJECT_ROOT%\build\assets\nano" (
    mkdir "%PROJECT_ROOT%\build\assets\nano"
)

echo =========================================
echo  Compiling scene for Nano...
echo =========================================
echo Input : %INPUT_SCENE%
echo Output: %OUTPUT_HEADER%
echo.

"%COMPILER_EXE%" "%PROJECT_ROOT%\%INPUT_SCENE%" "%OUTPUT_HEADER%"
if errorlevel 1 (
    echo.
    exit /b 1
)

echo.
echo Scene header updated.
