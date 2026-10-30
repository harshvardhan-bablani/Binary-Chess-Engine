@echo off
rem Build and run the test suites: perft (move generation) and rules
rem (notation and game-end rules).
rem Pass a depth to cap the perft run, e.g.  run_tests.bat 3
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

echo Building build\rules.exe ...
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Isrc -o build\rules.exe ^
    tests\rules.cpp src\board.cpp src\movegen.cpp src\notation.cpp src\gamestate.cpp

if errorlevel 1 (
    echo.
    echo [FAILED] Test build failed.
    exit /b 1
)

echo Building build\searchtest.exe ...
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Isrc -o build\searchtest.exe ^
    tests\search.cpp src\board.cpp src\movegen.cpp src\eval.cpp src\search.cpp

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
build\rules.exe
if errorlevel 1 (
    echo.
    echo TESTS FAILED
    exit /b 1
)

echo.
build\searchtest.exe
if errorlevel 1 (
    echo.
    echo TESTS FAILED
    exit /b 1
)

echo.
echo ALL TESTS PASSED
