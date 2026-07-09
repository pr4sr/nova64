@echo off
cd /d "%~dp0"
if not exist build\Debug\nova.exe (
    echo Building Nova64...
    cmake -S . -B build >nul
    cmake --build build --config Debug >nul
)
echo Starting Nova64 CLI...
echo.
build\Debug\nova.exe
