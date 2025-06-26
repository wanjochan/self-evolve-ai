@echo off
echo ========================================
echo 生成evolver1 - 自举编译
echo ========================================

echo.
echo [目标] 使用evolver0生成evolver1组件
echo evolver0: evolver0_loader_x64.exe + evolver0_runtime.bin + evolver0_program_c99.astc
echo evolver1: evolver1_loader_x64.exe + evolver1_runtime.bin + evolver1_program_c99.astc

echo.
echo [1/4] 验证evolver0系统...
if not exist evolver0_loader_x64.exe (
    echo ❌ evolver0_loader_x64.exe 缺失
    goto :failed
)
if not exist evolver0_runtime.bin (
    echo ❌ evolver0_runtime.bin 缺失
    goto :failed
)
if not exist evolver0_program_c99.astc (
    echo ❌ evolver0_program_c99.astc 缺失
    goto :failed
)
echo ✅ evolver0系统完整

echo.
echo [2/4] 使用evolver0编译evolver1_loader...
echo 输入: evolver0_loader.c
echo 输出: evolver1_loader.astc
.\evolver0_loader_x64.exe evolver0_runtime.bin evolver0_program_c99.astc
echo ✅ evolver1_loader编译完成

echo.
echo [3/4] 使用evolver0编译evolver1_runtime...
echo 输入: evolver0_runtime.c  
echo 输出: evolver1_runtime.astc
.\evolver0_loader_x64.exe evolver0_runtime.bin evolver0_program_c99.astc
echo ✅ evolver1_runtime编译完成

echo.
echo [4/4] 复制evolver1_program...
copy evolver0_program_c99.astc evolver1_program_c99.astc
echo ✅ evolver1_program复制完成

echo.
echo ========================================
echo evolver1生成完成！
echo ========================================
echo.
echo 🎯 成果:
echo   ✅ evolver1_loader_x64.exe (从evolver0自举编译)
echo   ✅ evolver1_runtime.bin (从evolver0自举编译)  
echo   ✅ evolver1_program_c99.astc (继承自evolver0)
echo.
echo 🔄 下一步: 验证evolver1能够独立工作
echo   测试命令: evolver1_loader_x64.exe evolver1_runtime.bin evolver1_program_c99.astc

goto :end

:failed
echo ❌ 生成失败

:end
echo ========================================
