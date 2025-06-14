@echo off
setlocal enabledelayedexpansion

set COMPILER=..\compiler0.exe
set TOTAL_TESTS=0
set PASSED_TESTS=0
set FAILED_TESTS=0

echo 运行TASM编译器测试...
echo.

rem 遍历所有测试文件
for /r %%f in (*.tasm) do (
    set /a TOTAL_TESTS+=1
    echo 测试: %%f
    
    rem 编译测试文件
    %COMPILER% "%%f" "%%~dpnf.exe"
    if !errorlevel! neq 0 (
        echo 编译失败
        set /a FAILED_TESTS+=1
        goto :continue
    )
    
    rem 运行测试程序
    "%%~dpnf.exe"
    if !errorlevel! equ 0 (
        echo 测试通过
        set /a PASSED_TESTS+=1
    ) else (
        echo 测试失败 ^(返回值: !errorlevel!^)
        set /a FAILED_TESTS+=1
    )
    
    :continue
    echo.
)

echo 测试总结:
echo 总计: %TOTAL_TESTS%
echo 通过: %PASSED_TESTS%
echo 失败: %FAILED_TESTS%

if %FAILED_TESTS% gtr 0 (
    exit /b 1
) else (
    exit /b 0
) 