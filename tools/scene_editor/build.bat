@echo off
setlocal

set "EDITOR_DIR=%~dp0"
set "PROJECT_ROOT=%EDITOR_DIR%..\.."

echo =========================================
echo  Building NutSceneEditor...
echo =========================================
echo.

cmake --build "%PROJECT_ROOT%\build" --target NutSceneEditor

echo.
pause
