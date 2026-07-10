@echo off
setlocal

set "TARGET_DIR=%~dp0"
set "PROJECT_ROOT=%TARGET_DIR%..\.."

echo =========================================
echo  Building and compiling demo scene...
echo =========================================
echo.

call "%PROJECT_ROOT%\tools\scene_compiler\build.bat"
if errorlevel 1 (
    echo.
    echo Scene compiler build failed.
    pause
    exit /b 1
)

call "%PROJECT_ROOT%\tools\scene_compiler\run.bat"
if errorlevel 1 (
    echo.
    echo Demo scene compilation failed.
    pause
    exit /b 1
)

echo.
echo Demo scene header is ready in build\assets\nano\demo_scene.h
echo.
pause
