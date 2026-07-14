@echo off
setlocal

set "EDITOR_DIR=%~dp0"
set "PROJECT_ROOT=%EDITOR_DIR%..\.."
set "EDITOR_EXE=%PROJECT_ROOT%\build\NutSceneEditor.exe"
set "MINGW_BIN=C:\mingw64\bin"

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

if exist "%MINGW_BIN%\libstdc++-6.dll" (
    set "PATH=%MINGW_BIN%;%PATH%"
)

"%EDITOR_EXE%"

if errorlevel 1 (
    echo.
    echo NutSceneEditor failed to start.
    echo If Windows reports a missing runtime, confirm that %MINGW_BIN% exists and contains the MinGW DLLs.
)
