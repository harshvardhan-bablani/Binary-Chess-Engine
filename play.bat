@echo off
rem Play a game against the engine.
rem   play.bat          you play White
rem   play.bat black    you play Black
setlocal

if not exist "%~dp0build\chess-engine.exe" (
    echo Building the engine first...
    call "%~dp0build.bat"
    if errorlevel 1 (
        echo.
        echo [FAILED] Could not build the engine, so there is nothing to play.
        exit /b 1
    )
)

"%~dp0build\chess-engine.exe" play %*
