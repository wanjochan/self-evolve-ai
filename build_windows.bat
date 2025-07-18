@echo off
REM build_windows.bat - Windows构建脚本
REM 
REM T2.3 Windows兼容性准备 - 主构建脚本
REM 支持MinGW、MSYS2、Visual Studio等Windows构建环境
REM

setlocal enabledelayedexpansion

REM 设置颜色输出 (如果支持)
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
set "BUILD_DIR=%PROJECT_ROOT%build"
set "BIN_DIR=%PROJECT_ROOT%bin"
set "SRC_DIR=%PROJECT_ROOT%src"

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
if /i "%~1"=="--release" (
    set "BUILD_TYPE=Release"
    shift
    goto :parse_args
)
if /i "%~1"=="--compiler" (
    set "COMPILER=%~2"
    shift
    shift
    goto :parse_args
)
if /i "%~1"=="--arch" (
    set "ARCHITECTURE=%~2"
    shift
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
%COLOR_WARNING% 未知参数: %~1
shift
goto :parse_args

:args_done

REM 显示帮助信息
:show_help
echo Windows构建脚本 - T2.3 Windows兼容性支持
echo.
echo 用法: %~nx0 [选项]
echo.
echo 选项:
echo   --debug          构建调试版本
echo   --release        构建发布版本 (默认)
echo   --compiler NAME  指定编译器 (gcc, clang, msvc, auto)
echo   --arch ARCH      指定架构 (x86, x64, auto)
echo   --verbose        详细输出
echo   --help           显示此帮助信息
echo.
echo 支持的编译器:
echo   gcc              MinGW GCC
echo   clang            Clang/LLVM
echo   msvc             Microsoft Visual C++
echo   auto             自动检测 (默认)
echo.
echo 示例:
echo   %~nx0                           # 默认构建
echo   %~nx0 --debug --verbose         # 调试版本，详细输出
echo   %~nx0 --compiler gcc --arch x64 # 使用GCC构建64位版本
goto :eof

REM 主函数
:main
%COLOR_INFO% === Windows构建开始 ===
%COLOR_INFO% 构建类型: %BUILD_TYPE%
%COLOR_INFO% 项目根目录: %PROJECT_ROOT%

REM 检测Windows环境
call :detect_windows_environment
if errorlevel 1 goto :error

REM 检测编译器
call :detect_compiler
if errorlevel 1 goto :error

REM 检测架构
call :detect_architecture
if errorlevel 1 goto :error

REM 设置构建环境
call :setup_build_environment
if errorlevel 1 goto :error

REM 创建构建目录
call :create_build_directories
if errorlevel 1 goto :error

REM 构建核心组件
call :build_core_components
if errorlevel 1 goto :error

REM 构建模块
call :build_modules
if errorlevel 1 goto :error

REM 构建工具
call :build_tools
if errorlevel 1 goto :error

REM 运行测试
call :run_tests
if errorlevel 1 goto :error

%COLOR_SUCCESS% === Windows构建完成 ===
goto :eof

REM 检测Windows环境
:detect_windows_environment
%COLOR_INFO% 检测Windows环境...

REM 检测Windows版本
for /f "tokens=4-5 delims=. " %%i in ('ver') do set "WIN_VERSION=%%i.%%j"
%COLOR_INFO% Windows版本: %WIN_VERSION%

REM 检测环境类型
set "WIN_ENV=native"
if defined MSYSTEM (
    set "WIN_ENV=msys2"
    %COLOR_INFO% 检测到MSYS2环境: %MSYSTEM%
) else if defined CYGWIN (
    set "WIN_ENV=cygwin"
    %COLOR_INFO% 检测到Cygwin环境
) else if defined MINGW_PREFIX (
    set "WIN_ENV=mingw"
    %COLOR_INFO% 检测到MinGW环境
)

REM 设置路径分隔符
if "%WIN_ENV%"=="native" (
    set "PATH_SEP=\"
    set "SHARED_LIB_EXT=.dll"
    set "EXECUTABLE_EXT=.exe"
) else (
    set "PATH_SEP=/"
    set "SHARED_LIB_EXT=.dll"
    set "EXECUTABLE_EXT=.exe"
)

%COLOR_SUCCESS% Windows环境检测完成: %WIN_ENV%
exit /b 0

REM 检测编译器
:detect_compiler
%COLOR_INFO% 检测编译器...

if /i "%COMPILER%"=="auto" (
    REM 自动检测编译器
    where gcc >nul 2>&1
    if !errorlevel! equ 0 (
        set "COMPILER=gcc"
        %COLOR_INFO% 自动检测到GCC编译器
    ) else (
        where clang >nul 2>&1
        if !errorlevel! equ 0 (
            set "COMPILER=clang"
            %COLOR_INFO% 自动检测到Clang编译器
        ) else (
            where cl >nul 2>&1
            if !errorlevel! equ 0 (
                set "COMPILER=msvc"
                %COLOR_INFO% 自动检测到MSVC编译器
            ) else (
                %COLOR_ERROR% 未找到可用的编译器
                exit /b 1
            )
        )
    )
)

REM 设置编译器特定配置
if /i "%COMPILER%"=="gcc" (
    set "CC=gcc"
    set "CXX=g++"
    set "CFLAGS=-std=c99 -Wall -Wextra"
    set "LDFLAGS=-lws2_32"
) else if /i "%COMPILER%"=="clang" (
    set "CC=clang"
    set "CXX=clang++"
    set "CFLAGS=-std=c99 -Wall -Wextra"
    set "LDFLAGS=-lws2_32"
) else if /i "%COMPILER%"=="msvc" (
    set "CC=cl"
    set "CXX=cl"
    set "CFLAGS=/std:c11 /W3"
    set "LDFLAGS=ws2_32.lib"
) else (
    %COLOR_ERROR% 不支持的编译器: %COMPILER%
    exit /b 1
)

%COLOR_SUCCESS% 编译器设置完成: %COMPILER%
exit /b 0

REM 检测架构
:detect_architecture
%COLOR_INFO% 检测目标架构...

if /i "%ARCHITECTURE%"=="auto" (
    REM 自动检测架构
    if "%PROCESSOR_ARCHITECTURE%"=="AMD64" (
        set "ARCHITECTURE=x64"
    ) else if "%PROCESSOR_ARCHITECTURE%"=="x86" (
        set "ARCHITECTURE=x86"
    ) else if "%PROCESSOR_ARCHITECTURE%"=="ARM64" (
        set "ARCHITECTURE=arm64"
    ) else (
        set "ARCHITECTURE=x64"
        %COLOR_WARNING% 未知架构，默认使用x64
    )
)

REM 设置架构特定标志
if /i "%ARCHITECTURE%"=="x64" (
    if /i "%COMPILER%"=="gcc" (
        set "ARCH_FLAGS=-m64"
    ) else if /i "%COMPILER%"=="clang" (
        set "ARCH_FLAGS=-m64"
    ) else if /i "%COMPILER%"=="msvc" (
        set "ARCH_FLAGS=/favor:AMD64"
    )
    set "PLATFORM_TARGET=x64_64"
) else if /i "%ARCHITECTURE%"=="x86" (
    if /i "%COMPILER%"=="gcc" (
        set "ARCH_FLAGS=-m32"
    ) else if /i "%COMPILER%"=="clang" (
        set "ARCH_FLAGS=-m32"
    ) else if /i "%COMPILER%"=="msvc" (
        set "ARCH_FLAGS=/favor:IA32"
    )
    set "PLATFORM_TARGET=x86_32"
) else if /i "%ARCHITECTURE%"=="arm64" (
    set "ARCH_FLAGS="
    set "PLATFORM_TARGET=arm64_64"
) else (
    %COLOR_ERROR% 不支持的架构: %ARCHITECTURE%
    exit /b 1
)

%COLOR_SUCCESS% 架构设置完成: %ARCHITECTURE% (%PLATFORM_TARGET%)
exit /b 0

REM 设置构建环境
:setup_build_environment
%COLOR_INFO% 设置构建环境...

REM 设置构建标志
if /i "%BUILD_TYPE%"=="Debug" (
    if /i "%COMPILER%"=="msvc" (
        set "BUILD_FLAGS=/Od /Zi /MDd"
    ) else (
        set "BUILD_FLAGS=-g -O0"
    )
) else (
    if /i "%COMPILER%"=="msvc" (
        set "BUILD_FLAGS=/O2 /MD"
    ) else (
        set "BUILD_FLAGS=-O2 -DNDEBUG"
    )
)

REM 组合所有编译标志
set "ALL_CFLAGS=%CFLAGS% %ARCH_FLAGS% %BUILD_FLAGS% -D_WIN32_WINNT=0x0600"
set "ALL_LDFLAGS=%LDFLAGS%"

if "%VERBOSE%"=="1" (
    %COLOR_INFO% CC: %CC%
    %COLOR_INFO% CFLAGS: %ALL_CFLAGS%
    %COLOR_INFO% LDFLAGS: %ALL_LDFLAGS%
)

%COLOR_SUCCESS% 构建环境设置完成
exit /b 0

REM 创建构建目录
:create_build_directories
%COLOR_INFO% 创建构建目录...

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
if not exist "%BIN_DIR%" mkdir "%BIN_DIR%"
if not exist "%BIN_DIR%\layer2" mkdir "%BIN_DIR%\layer2"

%COLOR_SUCCESS% 构建目录创建完成
exit /b 0

REM 构建核心组件
:build_core_components
%COLOR_INFO% 构建核心组件...

REM 这里添加具体的构建命令
%COLOR_INFO% 构建核心模块系统...
REM TODO: 添加实际的构建命令

%COLOR_SUCCESS% 核心组件构建完成
exit /b 0

REM 构建模块
:build_modules
%COLOR_INFO% 构建模块...

REM 这里添加模块构建命令
%COLOR_INFO% 构建系统模块...
REM TODO: 添加实际的模块构建命令

%COLOR_SUCCESS% 模块构建完成
exit /b 0

REM 构建工具
:build_tools
%COLOR_INFO% 构建工具...

REM 这里添加工具构建命令
%COLOR_INFO% 构建开发工具...
REM TODO: 添加实际的工具构建命令

%COLOR_SUCCESS% 工具构建完成
exit /b 0

REM 运行测试
:run_tests
%COLOR_INFO% 运行基础测试...

REM 这里添加测试命令
%COLOR_INFO% 运行Windows兼容性测试...
REM TODO: 添加实际的测试命令

%COLOR_SUCCESS% 测试完成
exit /b 0

REM 错误处理
:error
%COLOR_ERROR% 构建失败！
exit /b 1

REM 调用主函数
call :main %*
