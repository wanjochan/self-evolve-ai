@echo off
REM cleanup_files.bat - 整理和清理源码文件结构
echo === File Cleanup and Organization Script ===
echo.

REM 创建备份目录
echo Creating backup and archive directories...
mkdir archive 2>nul
mkdir archive\old_builds 2>nul
mkdir archive\old_tests 2>nul
mkdir archive\deprecated 2>nul

REM 1. 清理bin目录中的重复和过时文件
echo === Cleaning bin directory ===

REM 移动标记为删除的文件
echo Moving .todelete files to archive...
move bin\*.todelete archive\deprecated\ 2>nul

REM 移动过时的编译器版本
echo Moving old compiler versions...
move bin\tool_c2astc_complete.exe archive\old_builds\ 2>nul
move bin\tool_c2astc_enhanced.exe archive\old_builds\ 2>nul
move bin\tool_c2astc_improved.exe archive\old_builds\ 2>nul
move bin\tool_c2astc_stdlib.exe archive\old_builds\ 2>nul

REM 移动过时的runtime版本
echo Moving old runtime versions...
move bin\c99_runtime_*.astc archive\old_builds\ 2>nul
move bin\evolver0_runtime_*.astc archive\old_builds\ 2>nul
move bin\evolver0_runtime_*.bin archive\old_builds\ 2>nul
move bin\evolver1_runtime_*.astc archive\old_builds\ 2>nul

REM 移动过时的loader版本
move bin\c99_loader_new.exe archive\old_builds\ 2>nul
move bin\enhanced_loader.exe archive\old_builds\ 2>nul

REM 移动过时的program文件
move bin\c99_program_*.astc archive\old_builds\ 2>nul
move bin\evolver0_program_*.astc archive\old_builds\ 2>nul
move bin\evolver1_program_*.astc archive\old_builds\ 2>nul

REM 2. 清理tests目录中的重复测试文件
echo === Cleaning tests directory ===

REM 移动临时测试文件
echo Moving temporary test files...
move tests\temp_*.* archive\old_tests\ 2>nul
move tests\test_*_enhanced.* archive\old_tests\ 2>nul
move tests\test_*_final.* archive\old_tests\ 2>nul
move tests\debug_*.* archive\old_tests\ 2>nul

REM 移动重复的自举测试文件
move tests\*_self_hosted*.* archive\old_tests\ 2>nul
move tests\*_self_hosting*.* archive\old_tests\ 2>nul
move tests\simple_self_*.* archive\old_tests\ 2>nul

REM 移动过时的测试文件
move tests\test_0*.* archive\old_tests\ 2>nul
move tests\test_c99_*.* archive\old_tests\ 2>nul
move tests\test_enhanced_*.* archive\old_tests\ 2>nul

REM 3. 清理根目录中的重复构建脚本
echo === Cleaning root directory build scripts ===

REM 移动过时的构建脚本
move build*_*.bat archive\deprecated\ 2>nul
move *_independent.bat archive\deprecated\ 2>nul
move *_self_hosted.bat archive\deprecated\ 2>nul

REM 4. 清理src目录中的重复文件
echo === Cleaning src directory ===

REM 移动标记为删除的目录
move src\ai.todelete archive\deprecated\ 2>nul

REM 移动重复的runtime文件
move src\c99_runtime.c archive\deprecated\ 2>nul
move src\evolver0_runtime.c archive\deprecated\ 2>nul

REM 5. 整理当前活跃的文件
echo === Organizing active files ===

REM 确保核心工具在正确位置
echo Core tools status:
if exist "bin\tool_c2astc.exe" (
    echo ✅ tool_c2astc.exe - Active
) else (
    echo ❌ tool_c2astc.exe - Missing
)

if exist "bin\tool_astc2rt.exe" (
    echo ✅ tool_astc2rt.exe - Active
) else (
    echo ❌ tool_astc2rt.exe - Missing
)

if exist "bin\enhanced_runtime_with_libc_v2.exe" (
    echo ✅ enhanced_runtime_with_libc_v2.exe - Active
) else (
    echo ❌ enhanced_runtime_with_libc_v2.exe - Missing
)

REM 6. 创建文件清单
echo === Creating file inventory ===

echo === Active Core Files === > file_inventory.txt
echo. >> file_inventory.txt
echo Core Tools: >> file_inventory.txt
dir /b bin\tool_*.exe >> file_inventory.txt 2>nul
echo. >> file_inventory.txt
echo Core Runtime: >> file_inventory.txt
dir /b bin\*runtime*.exe >> file_inventory.txt 2>nul
echo. >> file_inventory.txt
echo Core Sources: >> file_inventory.txt
dir /b src\tool_*.c >> file_inventory.txt 2>nul
dir /b src\runtime\*.c >> file_inventory.txt 2>nul
echo. >> file_inventory.txt
echo Active Tests: >> file_inventory.txt
dir /b tests\*test*.c >> file_inventory.txt 2>nul
echo. >> file_inventory.txt

echo === Archived Files === >> file_inventory.txt
echo. >> file_inventory.txt
echo Deprecated: >> file_inventory.txt
dir /b archive\deprecated\ >> file_inventory.txt 2>nul
echo. >> file_inventory.txt
echo Old Builds: >> file_inventory.txt
dir /b archive\old_builds\ >> file_inventory.txt 2>nul
echo. >> file_inventory.txt
echo Old Tests: >> file_inventory.txt
dir /b archive\old_tests\ >> file_inventory.txt 2>nul

REM 7. 统计清理结果
echo === Cleanup Summary ===
echo.

set /a deprecated_count=0
set /a old_builds_count=0
set /a old_tests_count=0

for /f %%i in ('dir /b archive\deprecated\ 2^>nul ^| find /c /v ""') do set deprecated_count=%%i
for /f %%i in ('dir /b archive\old_builds\ 2^>nul ^| find /c /v ""') do set old_builds_count=%%i
for /f %%i in ('dir /b archive\old_tests\ 2^>nul ^| find /c /v ""') do set old_tests_count=%%i

echo Moved to archive\deprecated\: %deprecated_count% files
echo Moved to archive\old_builds\: %old_builds_count% files
echo Moved to archive\old_tests\: %old_tests_count% files
echo.
echo Total files archived: %deprecated_count% + %old_builds_count% + %old_tests_count%
echo.

echo ✅ File cleanup completed!
echo 📋 File inventory saved to: file_inventory.txt
echo 📁 Archived files moved to: archive\
echo.
echo === Recommended Next Steps ===
echo 1. Review file_inventory.txt to verify cleanup
echo 2. Test core functionality with cleaned files
echo 3. Delete archive\ directory if everything works
echo 4. Commit cleaned codebase to version control
echo.
