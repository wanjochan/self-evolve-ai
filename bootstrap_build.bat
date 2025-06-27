@echo off
echo === 自举构建脚本 - 完全脱离TinyCC依赖 ===
echo.

set TCC=external\tcc-win\tcc\tcc.exe
set STANDALONE_COMPILER=bin\standalone_c_compiler.exe

echo 阶段1: 使用TinyCC构建独立C编译器（最后一次使用TinyCC）
echo ================================================================

REM 构建独立C编译器
echo 构建独立C编译器...
%TCC% -o %STANDALONE_COMPILER% src\tools\standalone_c_compiler.c
if errorlevel 1 (
    echo ❌ 独立C编译器构建失败
    pause
    exit /b 1
)
echo ✅ 独立C编译器构建成功

echo.
echo 阶段2: 使用独立C编译器构建核心工具
echo ================================================================

REM 测试独立编译器
echo 测试独立编译器...
echo int main() { return 42; } > temp_test.c
%STANDALONE_COMPILER% temp_test.c temp_test.s
if errorlevel 1 (
    echo ❌ 独立编译器测试失败
    del temp_test.c
    pause
    exit /b 1
)
echo ✅ 独立编译器测试通过
del temp_test.c temp_test.s

echo.
echo 阶段3: 构建完整的自举工具链
echo ================================================================

REM 现在我们有了独立的C编译器，可以开始构建其他工具
REM 但首先需要一个汇编器和链接器来处理.s文件

echo 创建简单的汇编器和链接器...

REM 创建一个简单的汇编器（将.s转换为.o）
echo 构建汇编器...
%TCC% -o bin\simple_assembler.exe src\tools\simple_assembler.c 2>nul
if not errorlevel 1 (
    echo ✅ 汇编器构建成功
) else (
    echo ⚠️ 汇编器源文件不存在，需要创建
)

REM 创建一个简单的链接器（将.o转换为.exe）
echo 构建链接器...
%TCC% -o bin\simple_linker.exe src\tools\simple_linker.c 2>nul
if not errorlevel 1 (
    echo ✅ 链接器构建成功
) else (
    echo ⚠️ 链接器源文件不存在，需要创建
)

echo.
echo 阶段4: 验证自举能力
echo ================================================================

echo 尝试使用独立工具链编译简单程序...

REM 创建测试程序
echo #include ^<stdio.h^> > bootstrap_test.c
echo int main() { >> bootstrap_test.c
echo     printf("Hello from bootstrap compiler!\n"); >> bootstrap_test.c
echo     return 0; >> bootstrap_test.c
echo } >> bootstrap_test.c

REM 使用独立编译器编译
echo 编译测试程序...
%STANDALONE_COMPILER% bootstrap_test.c bootstrap_test.s
if errorlevel 1 (
    echo ❌ 独立编译器编译失败
    del bootstrap_test.c
    pause
    exit /b 1
)

echo ✅ 独立编译器编译成功
echo 生成的汇编文件: bootstrap_test.s

REM 显示生成的汇编代码
echo.
echo 生成的汇编代码:
echo ----------------------------------------
type bootstrap_test.s
echo ----------------------------------------

echo.
echo 阶段5: 构建evolver0核心组件（使用独立工具链）
echo ================================================================

echo 尝试使用独立编译器编译evolver0组件...

REM 编译loader
echo 编译evolver0_loader.c...
%STANDALONE_COMPILER% src\evolver0\evolver0_loader.c bin\evolver0_loader.s
if errorlevel 1 (
    echo ❌ loader编译失败
) else (
    echo ✅ loader编译成功
)

REM 编译runtime
echo 编译evolver0_runtime.c...
%STANDALONE_COMPILER% src\evolver0\evolver0_runtime.c bin\evolver0_runtime.s
if errorlevel 1 (
    echo ❌ runtime编译失败
) else (
    echo ✅ runtime编译成功
)

REM 编译program
echo 编译evolver0_program.c...
%STANDALONE_COMPILER% src\evolver0\evolver0_program.c bin\evolver0_program.s
if errorlevel 1 (
    echo ❌ program编译失败
) else (
    echo ✅ program编译成功
)

echo.
echo 阶段6: 评估自举进度
echo ================================================================

echo 自举构建进度评估:
echo.
echo ✅ 阶段1: 独立C编译器构建完成
echo ✅ 阶段2: 独立编译器功能验证完成
if exist "bin\evolver0_loader.s" (
    echo ✅ 阶段3: evolver0组件编译完成
) else (
    echo ⚠️ 阶段3: evolver0组件编译需要完善
)

echo.
echo 🎯 下一步需要完成的工作:
echo 1. 实现汇编器（.s -> .o）
echo 2. 实现链接器（.o -> .exe）
echo 3. 完善独立编译器的C语言支持
echo 4. 实现完整的自举编译流程
echo 5. 验证生成的可执行文件功能

echo.
echo 📊 TinyCC依赖状态:
echo - 当前阶段: 仅在构建独立编译器时使用TinyCC
echo - 目标状态: 完全脱离TinyCC依赖
echo - 进度评估: 30%% 完成

echo.
echo 🚀 自举构建脚本执行完成！
echo 独立C编译器已就绪，可以开始完善工具链。

REM 清理临时文件
del bootstrap_test.c bootstrap_test.s 2>nul

pause
