@echo off
REM build_modules_windows.bat - Windows模块构建脚本
REM 
REM T2.3 Windows兼容性准备 - 模块构建脚本
REM 构建所有核心模块的Windows版本
REM

setlocal enabledelayedexpansion

REM 设置颜色输出
if not defined NO_COLOR (
    set "COLOR_INFO=echo [94m[INFO][0m"
    set "COLOR_SUCCESS=echo [92m[SUCCESS][0m"
    set "COLOR_WARNING=echo [93m[WARNING][0m"
    set "COLOR_ERROR=echo [91m[ERROR][0m"
) else (
    set "COLOR_INFO=echo [INFO]"
    set "COLOR_SUCCESS=echo [SUCCESS]"
    set "COLOR_WARNING=echo [WARNING]"
    set "COLOR_ERROR=echo [ERROR]"
)

REM 获取脚本目录
set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR%"
set "SRC_DIR=%PROJECT_ROOT%src"
set "BIN_DIR=%PROJECT_ROOT%bin"
set "BUILD_DIR=%PROJECT_ROOT%build"

REM 构建配置
set "BUILD_TYPE=Release"
set "COMPILER=auto"
set "ARCHITECTURE=auto"
set "VERBOSE=0"

REM 解析命令行参数
:parse_args
if "%~1"=="" goto :args_done
if /i "%~1"=="--debug" (
    set "BUILD_TYPE=Debug"
    shift
    goto :parse_args
)
if /i "%~1"=="--verbose" (
    set "VERBOSE=1"
    shift
    goto :parse_args
)
if /i "%~1"=="--help" (
    goto :show_help
)
shift
goto :parse_args

:args_done

:show_help
echo Windows模块构建脚本 - T2.3 Windows兼容性支持
echo.
echo 用法: %~nx0 [选项]
echo.
echo 选项:
echo   --debug          构建调试版本
echo   --verbose        详细输出
echo   --help           显示此帮助信息
echo.
echo 构建的模块:
echo   layer0           基础层模块
echo   pipeline         管道模块
echo   compiler         编译器模块
echo   module           模块管理器
echo   libc             C库模块
goto :eof

REM 主函数
:main
%COLOR_INFO% === Windows模块构建开始 ===

REM 检测构建环境
call :detect_build_environment
if errorlevel 1 goto :error

REM 创建输出目录
call :create_output_directories
if errorlevel 1 goto :error

REM 构建各个模块
call :build_layer0_module
if errorlevel 1 goto :error

call :build_pipeline_module
if errorlevel 1 goto :error

call :build_compiler_module
if errorlevel 1 goto :error

call :build_module_module
if errorlevel 1 goto :error

call :build_libc_module
if errorlevel 1 goto :error

REM 验证构建结果
call :verify_build_results
if errorlevel 1 goto :error

%COLOR_SUCCESS% === Windows模块构建完成 ===
goto :eof

REM 检测构建环境
:detect_build_environment
%COLOR_INFO% 检测构建环境...

REM 检测编译器
where gcc >nul 2>&1
if !errorlevel! equ 0 (
    set "CC=gcc"
    set "COMPILER=gcc"
) else (
    where clang >nul 2>&1
    if !errorlevel! equ 0 (
        set "CC=clang"
        set "COMPILER=clang"
    ) else (
        REM 尝试使用项目内置的TCC
        if exist "%PROJECT_ROOT%external\tcc-win\tcc\tcc.exe" (
            set "CC=%PROJECT_ROOT%external\tcc-win\tcc\tcc.exe"
            set "COMPILER=tcc"
            %COLOR_INFO% 使用内置TCC编译器
        ) else (
            %COLOR_ERROR% 未找到可用的C编译器
            exit /b 1
        )
    )
)

REM 检测架构
if "%PROCESSOR_ARCHITECTURE%"=="AMD64" (
    set "ARCH=x64"
    set "ARCH_BITS=64"
    set "PLATFORM_TARGET=x64_64"
) else if "%PROCESSOR_ARCHITECTURE%"=="ARM64" (
    set "ARCH=arm64"
    set "ARCH_BITS=64"
    set "PLATFORM_TARGET=arm64_64"
) else (
    set "ARCH=x86"
    set "ARCH_BITS=32"
    set "PLATFORM_TARGET=x86_32"
)

REM 设置编译标志
if /i "%COMPILER%"=="gcc" (
    set "CFLAGS=-std=c99 -Wall -Wextra -fPIC -D_WIN32_WINNT=0x0600"
    set "LDFLAGS=-shared -lws2_32"
) else if /i "%COMPILER%"=="clang" (
    set "CFLAGS=-std=c99 -Wall -Wextra -fPIC -D_WIN32_WINNT=0x0600"
    set "LDFLAGS=-shared -lws2_32"
) else if /i "%COMPILER%"=="tcc" (
    set "CFLAGS=-std=c99 -D_WIN32_WINNT=0x0600"
    set "LDFLAGS=-shared -lws2_32"
)

if /i "%BUILD_TYPE%"=="Debug" (
    set "CFLAGS=%CFLAGS% -g -O0 -DDEBUG"
) else (
    set "CFLAGS=%CFLAGS% -O2 -DNDEBUG"
)

%COLOR_SUCCESS% 构建环境: %COMPILER% (%ARCH%)
exit /b 0

REM 创建输出目录
:create_output_directories
%COLOR_INFO% 创建输出目录...

if not exist "%BIN_DIR%" mkdir "%BIN_DIR%"
if not exist "%BIN_DIR%\layer2" mkdir "%BIN_DIR%\layer2"
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
if not exist "%BUILD_DIR%\modules" mkdir "%BUILD_DIR%\modules"

%COLOR_SUCCESS% 输出目录创建完成
exit /b 0

REM 构建layer0模块
:build_layer0_module
%COLOR_INFO% 构建layer0模块...

set "MODULE_NAME=layer0"
set "SOURCE_FILE=%SRC_DIR%\core\modules\layer0_module.c"
set "OUTPUT_FILE=%BIN_DIR%\layer2\%MODULE_NAME%_%PLATFORM_TARGET%.native"

if not exist "%SOURCE_FILE%" (
    %COLOR_WARNING% 源文件不存在: %SOURCE_FILE%
    exit /b 0
)

%COLOR_INFO% 编译: %MODULE_NAME%
if "%VERBOSE%"=="1" (
    echo %CC% %CFLAGS% "%SOURCE_FILE%" %LDFLAGS% -o "%OUTPUT_FILE%"
)

%CC% %CFLAGS% "%SOURCE_FILE%" %LDFLAGS% -o "%OUTPUT_FILE%"
if errorlevel 1 (
    %COLOR_ERROR% %MODULE_NAME% 模块编译失败
    exit /b 1
)

%COLOR_SUCCESS% %MODULE_NAME% 模块构建完成
exit /b 0

REM 构建pipeline模块
:build_pipeline_module
%COLOR_INFO% 构建pipeline模块...

set "MODULE_NAME=pipeline"
set "SOURCE_FILE=%SRC_DIR%\core\modules\pipeline_module.c"
set "OUTPUT_FILE=%BIN_DIR%\layer2\%MODULE_NAME%_%PLATFORM_TARGET%.native"

if not exist "%SOURCE_FILE%" (
    %COLOR_WARNING% 源文件不存在: %SOURCE_FILE%
    exit /b 0
)

%COLOR_INFO% 编译: %MODULE_NAME%
if "%VERBOSE%"=="1" (
    echo %CC% %CFLAGS% "%SOURCE_FILE%" %LDFLAGS% -o "%OUTPUT_FILE%"
)

%CC% %CFLAGS% "%SOURCE_FILE%" %LDFLAGS% -o "%OUTPUT_FILE%"
if errorlevel 1 (
    %COLOR_ERROR% %MODULE_NAME% 模块编译失败
    exit /b 1
)

%COLOR_SUCCESS% %MODULE_NAME% 模块构建完成
exit /b 0

REM 构建compiler模块
:build_compiler_module
%COLOR_INFO% 构建compiler模块...

set "MODULE_NAME=compiler"
set "SOURCE_FILE=%SRC_DIR%\core\modules\compiler_module.c"
set "OUTPUT_FILE=%BIN_DIR%\layer2\%MODULE_NAME%_%PLATFORM_TARGET%.native"

if not exist "%SOURCE_FILE%" (
    %COLOR_WARNING% 源文件不存在: %SOURCE_FILE%
    exit /b 0
)

%COLOR_INFO% 编译: %MODULE_NAME%
if "%VERBOSE%"=="1" (
    echo %CC% %CFLAGS% "%SOURCE_FILE%" %LDFLAGS% -o "%OUTPUT_FILE%"
)

%CC% %CFLAGS% "%SOURCE_FILE%" %LDFLAGS% -o "%OUTPUT_FILE%"
if errorlevel 1 (
    %COLOR_ERROR% %MODULE_NAME% 模块编译失败
    exit /b 1
)

%COLOR_SUCCESS% %MODULE_NAME% 模块构建完成
exit /b 0

REM 构建module模块
:build_module_module
%COLOR_INFO% 构建module模块...

set "MODULE_NAME=module"
set "SOURCE_FILE=%SRC_DIR%\core\modules\module_module.c"
set "OUTPUT_FILE=%BIN_DIR%\layer2\%MODULE_NAME%_%PLATFORM_TARGET%.native"

if not exist "%SOURCE_FILE%" (
    %COLOR_WARNING% 源文件不存在: %SOURCE_FILE%
    exit /b 0
)

%COLOR_INFO% 编译: %MODULE_NAME%
if "%VERBOSE%"=="1" (
    echo %CC% %CFLAGS% -I"%SRC_DIR%\core" "%SOURCE_FILE%" %LDFLAGS% -o "%OUTPUT_FILE%"
)

%CC% %CFLAGS% -I"%SRC_DIR%\core" "%SOURCE_FILE%" %LDFLAGS% -o "%OUTPUT_FILE%"
if errorlevel 1 (
    %COLOR_ERROR% %MODULE_NAME% 模块编译失败
    exit /b 1
)

%COLOR_SUCCESS% %MODULE_NAME% 模块构建完成
exit /b 0

REM 构建libc模块
:build_libc_module
%COLOR_INFO% 构建libc模块...

set "MODULE_NAME=libc"
set "SOURCE_FILE=%SRC_DIR%\core\modules\libc_module.c"
set "OUTPUT_FILE=%BIN_DIR%\layer2\%MODULE_NAME%_%PLATFORM_TARGET%.native"

if not exist "%SOURCE_FILE%" (
    %COLOR_WARNING% 源文件不存在: %SOURCE_FILE%
    exit /b 0
)

%COLOR_INFO% 编译: %MODULE_NAME%
if "%VERBOSE%"=="1" (
    echo %CC% %CFLAGS% "%SOURCE_FILE%" %LDFLAGS% -o "%OUTPUT_FILE%"
)

%CC% %CFLAGS% "%SOURCE_FILE%" %LDFLAGS% -o "%OUTPUT_FILE%"
if errorlevel 1 (
    %COLOR_ERROR% %MODULE_NAME% 模块编译失败
    exit /b 1
)

%COLOR_SUCCESS% %MODULE_NAME% 模块构建完成
exit /b 0

REM 验证构建结果
:verify_build_results
%COLOR_INFO% 验证构建结果...

set "MODULES=layer0 pipeline compiler module libc"
set "SUCCESS_COUNT=0"
set "TOTAL_COUNT=0"

for %%m in (%MODULES%) do (
    set /a TOTAL_COUNT+=1
    set "MODULE_FILE=%BIN_DIR%\layer2\%%m_%PLATFORM_TARGET%.native"
    if exist "!MODULE_FILE!" (
        set /a SUCCESS_COUNT+=1
        %COLOR_SUCCESS% ✅ %%m 模块构建成功
    ) else (
        %COLOR_WARNING% ❌ %%m 模块构建失败或跳过
    )
)

%COLOR_INFO% 构建统计: %SUCCESS_COUNT%/%TOTAL_COUNT% 模块成功

if %SUCCESS_COUNT% equ %TOTAL_COUNT% (
    %COLOR_SUCCESS% 所有模块构建成功！
) else if %SUCCESS_COUNT% gtr 0 (
    %COLOR_WARNING% 部分模块构建成功
) else (
    %COLOR_ERROR% 没有模块构建成功
    exit /b 1
)

exit /b 0

REM 错误处理
:error
%COLOR_ERROR% 模块构建失败！
exit /b 1

REM 调用主函数
call :main %*
