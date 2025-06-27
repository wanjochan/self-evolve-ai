@echo off
echo ========================================
echo 构建Runtime - 两步流程
echo ========================================

echo.
echo [步骤1/2] C源码 → ASTC
echo 输入: evolver0_runtime.c
echo 输出: evolver0_runtime.astc

.\tool_c2astc.exe evolver0_runtime.c evolver0_runtime.astc
if %ERRORLEVEL% NEQ 0 (
    echo ❌ C→ASTC转换失败
    goto :failed
)
echo ✅ C→ASTC转换完成

echo.
echo [步骤2/2] ASTC → 机器码
echo 输入: evolver0_runtime.astc  
echo 输出: evolver0_runtime.bin

.\tool_astc2bin.exe evolver0_runtime.astc evolver0_runtime.bin
if %ERRORLEVEL% NEQ 0 (
    echo ❌ ASTC→BIN转换失败
    goto :failed
)
echo ✅ ASTC→BIN转换完成

echo.
echo ========================================
echo 🎉 Runtime构建成功！
echo ========================================
echo.
echo 📁 生成的文件:
echo   evolver0_runtime.astc - ASTC中间文件
echo   evolver0_runtime.bin - x64机器码Runtime
echo.
echo 🎯 两步流程完成:
echo   evolver0_runtime.c → evolver0_runtime.astc → evolver0_runtime.bin

goto :end

:failed
echo ❌ Runtime构建失败

:end
echo ========================================
