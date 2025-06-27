@echo off
echo Building and running HOMEWORK 2.cpp...

:: Try to find Visual Studio developer command prompt
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat"
) else (
    echo Visual Studio developer command prompt not found.
    echo Please open Visual Studio Developer Command Prompt manually and run:
    echo   cl "HOMEWORK 2.cpp" /Fe:program.exe
    echo   program.exe
    pause
    exit /b 1
)

:: Compile the program
cl "HOMEWORK 2.cpp" /Fe:program.exe

:: Run the program if compilation succeeded
if %ERRORLEVEL% EQU 0 (
    echo Running program...
    program.exe
    pause
) else (
    echo Compilation failed.
    pause
) 