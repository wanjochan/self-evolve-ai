@echo off
chcp 65001 >nul
echo Testing ASTC format compilation...

echo Building test_astc_format.exe...
echo Current directory: %CD%
if exist ..\external\tcc-win\tcc\tcc.exe (
    echo TCC found
    ..\external\tcc-win\tcc\tcc.exe -o test_astc_format.exe test_astc_format.c ..\src\core\utils.c
) else (
    echo TCC not found at ..\external\tcc-win\tcc\tcc.exe
    echo Checking alternative paths...
    if exist external\tcc-win\tcc\tcc.exe (
        echo TCC found in alternative path
        cd tests
        ..\external\tcc-win\tcc\tcc.exe -o test_astc_format.exe test_astc_format.c ..\src\core\utils.c
        cd ..
    ) else (
        echo TCC not found in any expected location
        exit /b 1
    )
)

if errorlevel 1 (
    echo ERROR: Compilation failed
    exit /b 1
) else (
    echo Compilation successful
    echo Running test...
    test_astc_format.exe
    echo Test completed
)
