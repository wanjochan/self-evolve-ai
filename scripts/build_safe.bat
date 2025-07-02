@echo off
chcp 65001 >nul
REM build_safe.bat - Safe build script to avoid antivirus false positives

echo ========================================
echo Self-Evolve AI Safe Build Script
echo Avoiding Antivirus False Positives
echo ========================================

REM Set build directory
set BUILD_DIR=build_safe
set SOURCE_DIR=%~dp0..

REM Clean old build
if exist %BUILD_DIR% (
    echo Cleaning old build directory...
    rmdir /s /q %BUILD_DIR%
)

REM Create build directory
mkdir %BUILD_DIR%
cd %BUILD_DIR%

echo.
echo Configuring CMake project...
cmake -G "Visual Studio 17 2022" ^
      -A x64 ^
      -DCMAKE_BUILD_TYPE=Release ^
      -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL ^
      -DCMAKE_INSTALL_PREFIX=../install ^
      %SOURCE_DIR%

if %ERRORLEVEL% neq 0 (
    echo CMake配置失败！
    pause
    exit /b 1
)

echo.
echo 开始编译...
cmake --build . --config Release --parallel

if %ERRORLEVEL% neq 0 (
    echo 编译失败！
    pause
    exit /b 1
)

echo.
echo 安装文件...
cmake --install . --config Release

echo.
echo ========================================
echo 构建完成！
echo ========================================
echo.
echo 生成的文件位置：
echo - 可执行文件: %BUILD_DIR%\bin\Release\loader.exe
echo - 模块文件: %BUILD_DIR%\lib\Release\*.native
echo.

REM 检查是否有代码签名证书
if defined CODESIGN_CERTIFICATE (
    echo 检测到代码签名证书，正在签名...
    signtool sign /f "%CODESIGN_CERTIFICATE%" /p "%CODESIGN_PASSWORD%" /t "http://timestamp.digicert.com" /d "Self-Evolve AI System" bin\Release\loader.exe
    if %ERRORLEVEL% equ 0 (
        echo 代码签名成功！
    ) else (
        echo 代码签名失败，但程序仍可使用
    )
) else (
    echo.
    echo 注意：未检测到代码签名证书
    echo 建议获取代码签名证书以避免杀毒软件误报
    echo.
    echo 临时解决方案：
    echo 1. 将生成的exe添加到杀毒软件白名单
    echo 2. 暂时关闭实时保护进行测试
    echo 3. 向杀毒软件厂商报告误报
)

echo.
echo 构建完成！按任意键退出...
pause > nul
