@echo off
REM forward a precompiled product to cami

setlocal enabledelayedexpansion

@REM set the directory of cami(default to '${PROJECT_ROOT}/build/src/Release' which is the path produced by MSVC)
set cami_dir=%~dp0\..\..\build\src\Release

set input=%1
set test_suite_dir=%2
set out_dir=%3

if "%input%" == "" (
    echo missing input
    echo usage: %0 ^<input_name^> ^<test_suit_dir^> ^<out_dir^>
    exit /b 1
)
if "%test_suite_dir%" == "" (
    echo missing test_suit_dir
    echo usage: %0 ^<input_name^> ^<test_suit_dir^> ^<out_dir^>
    exit /b 1
)
if "%out_dir%" == "" (
    echo missing out_dir
    echo usage: %0 ^<input_name^> ^<test_suit_dir^> ^<out_dir^>
    exit /b 1
)

for /f "tokens=*" %%a in ('echo !input!') do (
    set "input=%%a"
    set "output=!input:%test_suite_dir%\=!"
    set "output=!output:.c=.tbc!"
    set "output=%out_dir%\!output!"
)

%cami_dir%\cami run %output%