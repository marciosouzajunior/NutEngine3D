@echo off
setlocal

set "EDITOR_DIR=%~dp0"
set "PROJECT_ROOT=%EDITOR_DIR%..\.."
set "EDITOR_EXE=%PROJECT_ROOT%\build\NutSceneEditor.exe"

echo =========================================
echo  Starting NutSceneEditor...
echo =========================================
echo.

if not exist "%EDITOR_EXE%" (
    echo NutSceneEditor.exe was not found.
    echo Run tools\scene_editor\build.bat first.
    echo.
    pause
    exit /b 1
)

"%EDITOR_EXE%"
