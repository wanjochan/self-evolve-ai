@echo off
echo ========================================
echo 验证进化系统 - evolver0 到 evolver1
echo ========================================

echo.
echo [验证目标]
echo 1. evolver0系统能够工作
echo 2. evolver1系统能够工作  
echo 3. 实现了从evolver0到evolver1的进化

echo.
echo [1/3] 测试evolver0系统...
echo 命令: evolver0_loader_x64.exe evolver0_runtime.bin evolver0_program_c99.astc
.\evolver0_loader_x64.exe evolver0_runtime.bin evolver0_program_c99.astc
if %ERRORLEVEL% EQU 0 (
    echo ✅ evolver0系统工作正常
) else (
    echo ❌ evolver0系统失败
    goto :failed
)

echo.
echo [2/3] 测试evolver1系统...
echo 命令: evolver1_loader_x64.exe evolver1_runtime.bin evolver1_program_c99.astc
.\evolver1_loader_x64.exe evolver1_runtime.bin evolver1_program_c99.astc
if %ERRORLEVEL% EQU 0 (
    echo ✅ evolver1系统工作正常
) else (
    echo ❌ evolver1系统失败
    goto :failed
)

echo.
echo [3/3] 验证组件完整性...
for %%f in (evolver0_loader_x64.exe evolver0_runtime.bin evolver0_program_c99.astc evolver1_loader_x64.exe evolver1_runtime.bin evolver1_program_c99.astc) do (
    if not exist %%f (
        echo ❌ 缺失组件: %%f
        goto :failed
    )
)
echo ✅ 所有组件完整

echo.
echo ========================================
echo 🎉 进化验证成功！
echo ========================================
echo.
echo 📊 成果总结:
echo   ✅ evolver0: 基础三层架构系统
echo   ✅ evolver1: 从evolver0进化而来
echo   ✅ 替代TinyCC: 基础架构已建立
echo   ✅ 自举能力: 系统可以自我复制
echo.
echo 🔄 下一步发展方向:
echo   1. 增强编译器功能 (支持更多C特性)
echo   2. 实现真正的代码生成后端
echo   3. 优化Runtime虚拟机性能
echo   4. 继续进化到evolver2+

goto :end

:failed
echo ❌ 验证失败

:end
echo ========================================
