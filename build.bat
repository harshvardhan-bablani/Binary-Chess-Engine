@echo off
rem Build the chess engine into build\chess-engine.exe
setlocal

where g++ >nul 2>nul
if errorlevel 1 (
    echo [ERROR] g++ was not found on PATH.
    echo.
    echo Install a C++17 compiler, for example MSYS2:
    echo     https://www.msys2.org/
    echo Then add its bin folder to PATH, or run this from an
    echo "MSYS2 UCRT64" shell.
    exit /b 1
)

if not exist build mkdir build

echo Building build\chess-engine.exe ...
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Isrc -o build\chess-engine.exe ^
    src\main.cpp src\board.cpp src\movegen.cpp src\eval.cpp src\search.cpp ^
    src\notation.cpp src\gamestate.cpp src\play.cpp

if errorlevel 1 (
    echo.
    echo [FAILED] Build failed.
    exit /b 1
)

echo [OK] build\chess-engine.exe
