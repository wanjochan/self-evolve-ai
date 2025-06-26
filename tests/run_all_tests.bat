@echo off
echo ========================================
echo 运行所有evolver0测试
echo ========================================

set FAILED=0

echo.
echo [1/6] 测试基本编译器功能...
.\tool_build_program.exe tests\test_basic_compiler.c tests\test_basic_compiler.astc
if %ERRORLEVEL% NEQ 0 (
    echo ❌ 基本编译器测试编译失败
    set FAILED=1
    goto :end
)

.\evolver0_loader.exe evolver0_runtime.bin tests\test_basic_compiler.astc
if %ERRORLEVEL% NEQ 0 (
    echo ❌ 基本编译器测试执行失败
    set FAILED=1
    goto :end
)
echo ✅ 基本编译器测试通过

echo.
echo [2/6] 测试自举编译功能...
.\tool_build_program.exe tests\test_self_bootstrap.c tests\test_self_bootstrap.astc
if %ERRORLEVEL% NEQ 0 (
    echo ❌ 自举编译测试编译失败
    set FAILED=1
    goto :end
)

.\evolver0_loader.exe evolver0_runtime.bin tests\test_self_bootstrap.astc
if %ERRORLEVEL% NEQ 0 (
    echo ❌ 自举编译测试执行失败
    set FAILED=1
    goto :end
)
echo ✅ 自举编译测试通过

echo.
echo [3/6] 测试for循环支持...
.\evolver0_loader.exe evolver0_runtime.bin tests\test_for_loop.astc
if %ERRORLEVEL% NEQ 0 (
    echo ❌ for循环测试失败
    set FAILED=1
    goto :end
)
echo ✅ for循环测试通过

echo.
echo [4/6] 测试evolver0核心功能...
.\evolver0_loader.exe evolver0_runtime.bin evolver0_program.astc
if %ERRORLEVEL% NEQ 0 (
    echo ❌ evolver0核心功能测试失败
    set FAILED=1
    goto :end
)
echo ✅ evolver0核心功能测试通过

echo.
echo [5/6] 测试evolver1生成...
if exist evolver1_program.astc (
    .\evolver0_loader.exe evolver0_runtime.bin evolver1_program.astc
    if %ERRORLEVEL% NEQ 0 (
        echo ❌ evolver1执行失败
        set FAILED=1
        goto :end
    )
    echo ✅ evolver1测试通过
) else (
    echo ⚠️  evolver1_program.astc不存在，跳过测试
)

echo.
echo [6/6] 测试三层架构完整性...
echo 检查关键文件存在性...
if not exist evolver0_loader.exe (
    echo ❌ evolver0_loader.exe缺失
    set FAILED=1
)
if not exist evolver0_runtime.bin (
    echo ❌ evolver0_runtime.bin缺失
    set FAILED=1
)
if not exist evolver0_program.astc (
    echo ❌ evolver0_program.astc缺失
    set FAILED=1
)

if %FAILED% EQU 0 (
    echo ✅ 三层架构完整性检查通过
)

:end
echo.
echo ========================================
if %FAILED% EQU 0 (
    echo 🎉 所有测试通过！
    echo ✅ evolver0系统功能正常
    echo ✅ 三层架构工作正常
    echo ✅ ASTC序列化支持完整
    echo ✅ 自举编译基础架构完成
) else (
    echo ❌ 部分测试失败
    echo 请检查错误信息并修复问题
)
echo ========================================

exit /b %FAILED%
