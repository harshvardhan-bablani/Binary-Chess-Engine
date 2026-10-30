@echo off
rem Build and run the perft move-generation test suite.
rem Pass a depth to cap the run, e.g.  run_tests.bat 3
setlocal

where g++ >nul 2>nul
if errorlevel 1 (
    echo [ERROR] g++ was not found on PATH. See build.bat for details.
    exit /b 1
)

if not exist build mkdir build

echo Building build\perft.exe ...
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Isrc -o build\perft.exe ^
    tests\perft.cpp src\board.cpp src\movegen.cpp

if errorlevel 1 (
    echo.
    echo [FAILED] Test build failed.
    exit /b 1
)

echo.
build\perft.exe %*
if errorlevel 1 (
    echo.
    echo TESTS FAILED
    exit /b 1
)

echo.
echo ALL TESTS PASSED
