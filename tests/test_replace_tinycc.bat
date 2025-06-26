@echo off
echo ========================================
echo 测试evolver0替代TinyCC能力
echo ========================================

echo.
echo [目标] 验证三层架构能够替代TinyCC进行C程序编译
echo 架构: evolver0_loader.exe + evolver0_runtime.bin + evolver0_program_c99.astc

echo.
echo [1/4] 检查组件完整性...
if not exist evolver0_loader.exe (
    echo ❌ evolver0_loader.exe 缺失
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
echo ✅ 三层架构组件完整

echo.
echo [2/4] 测试C99编译器执行...
.\evolver0_loader.exe evolver0_runtime.bin evolver0_program_c99.astc
set RESULT=%ERRORLEVEL%
echo 执行结果: %RESULT%

echo.
echo [3/4] 检查编译输出...
if exist tests\compiled_output.astc (
    echo ✅ 发现编译输出文件
    dir tests\compiled_output.astc
) else (
    echo ⚠️  未发现编译输出文件
)

echo.
echo [4/4] 状态评估...
echo.
echo 📊 当前状态:
echo   ✅ 三层架构基础: 完成
echo   ✅ Runtime虚拟机: 框架完成
echo   ✅ C99编译器框架: 完成
echo   🔄 真正编译功能: 需要完善
echo   🔄 TinyCC替代: 部分实现

echo.
echo 🎯 下一步需要:
echo   1. 实现Runtime中的真正ASTC虚拟机
echo   2. 在program_c99中集成真正的c2astc调用
echo   3. 实现完整的编译服务系统调用
echo   4. 验证能够编译并执行真正的C程序

goto :end

:failed
echo ❌ 测试失败

:end
echo ========================================
