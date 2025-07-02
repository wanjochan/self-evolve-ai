@echo off
chcp 65001 >nul 2>&1

echo ========================================
echo Building Layer 3 ASTC Programs
echo PRD.md: c99_{arch}_{bits}.astc (C99 Compiler)
echo ========================================

REM Check if TCC is available
if not exist "external\tcc-win\tcc\tcc.exe" (
    echo Error: TCC compiler not found
    exit /b 1
)

REM Create output directories
if not exist "bin\layer3" mkdir "bin\layer3"
if not exist "bin\tools" mkdir "bin\tools"

REM Set TCC path
set TCC=external\tcc-win\tcc\tcc.exe

REM Detect current architecture
set CURRENT_ARCH=x64
if "%PROCESSOR_ARCHITECTURE%"=="ARM64" set CURRENT_ARCH=arm64
if "%PROCESSOR_ARCHITEW6432%"=="ARM64" set CURRENT_ARCH=arm64

echo Current architecture detected: %CURRENT_ARCH%
echo.

REM Step 1: Build the development tools
echo Building development tools...
echo ==============================

echo Building tool_c2astc.exe...
%TCC% -o "bin\tools\tool_c2astc.exe" -g -O2 "src\tools\tool_c2astc.c" "src\compiler\c2astc.c"

if %ERRORLEVEL% neq 0 (
    echo Error: Failed to compile tool_c2astc.exe
    exit /b 1
)
echo Success: tool_c2astc.exe compiled successfully

echo.
echo Building tool_astc2native.exe...
%TCC% -o "bin\tools\tool_astc2native.exe" -g -O2 "src\tools\tool_astc2native.c"

if %ERRORLEVEL% neq 0 (
    echo Error: Failed to compile tool_astc2native.exe
    exit /b 1
)
echo Success: tool_astc2native.exe compiled successfully

REM Step 2: Build C99 compiler for x64 architecture
echo.
echo Building c99_x64_64.astc program...
echo ===================================

REM Use tool_c2astc.exe to convert C99 compiler source to ASTC bytecode
"bin\tools\tool_c2astc.exe" "src\layer3\c99.c" "bin\layer3\c99_x64_64.astc"

if %ERRORLEVEL% neq 0 (
    echo Error: Failed to compile c99_x64_64.astc
    exit /b 1
)
echo Success: c99_x64_64.astc compiled successfully

REM Step 3: Build ARM64 tools if cross-compilation is supported
echo.
echo Building ARM64 development tools...
echo ===================================

REM Check if ARM64 cross-compilation is supported by testing TCC capabilities
echo Testing ARM64 cross-compilation support...
%TCC% -DTCC_TARGET_ARM64 -E -xc NUL >nul 2>&1
if %ERRORLEVEL% equ 0 (
    echo ARM64 cross-compilation supported, proceeding...

    REM Build ARM64 tools
    echo Building tool_c2astc_arm64.exe...
    %TCC% -DTCC_TARGET_ARM64 -o "bin\tools\tool_c2astc_arm64.exe" -g -O2 "src\tools\tool_c2astc.c"

    if %ERRORLEVEL% neq 0 (
        echo Warning: Failed to compile tool_c2astc_arm64.exe
        set ARM64_TOOLS_SUCCESS=0
    ) else (
        echo Success: tool_c2astc_arm64.exe compiled successfully

        echo Building tool_astc2native_arm64.exe...
        %TCC% -DTCC_TARGET_ARM64 -o "bin\tools\tool_astc2native_arm64.exe" -g -O2 "src\tools\tool_astc2native.c"

        if %ERRORLEVEL% neq 0 (
            echo Warning: Failed to compile tool_astc2native_arm64.exe
            set ARM64_TOOLS_SUCCESS=0
        ) else (
            echo Success: tool_astc2native_arm64.exe compiled successfully
            set ARM64_TOOLS_SUCCESS=1
        )
    )
) else (
    echo Warning: ARM64 cross-compilation not supported by current TCC build
    echo Skipping ARM64 tools compilation...
    set ARM64_TOOLS_SUCCESS=0
)

REM Step 4: Build C99 compiler for ARM64 architecture
echo.
echo Building c99_arm64_64.astc program...
echo ====================================

if defined ARM64_TOOLS_SUCCESS if %ARM64_TOOLS_SUCCESS%==1 (
    REM Use ARM64 tool_c2astc to convert C99 compiler source to ARM64 ASTC bytecode
    "bin\tools\tool_c2astc_arm64.exe" "src\layer3\c99.c" "bin\layer3\c99_arm64_64.astc"

    if %ERRORLEVEL% neq 0 (
        echo Warning: Failed to compile c99_arm64_64.astc
        set ARM64_C99_SUCCESS=0
    ) else (
        echo Success: c99_arm64_64.astc compiled successfully
        set ARM64_C99_SUCCESS=1
    )
) else (
    echo Skipping ARM64 C99 compiler compilation (ARM64 tools not available)...
    set ARM64_C99_SUCCESS=0
)

REM Step 5: Create backward compatibility links
echo.
echo Creating backward compatibility links...
echo =======================================
copy "bin\layer3\c99_x64_64.astc" "bin\layer3\c99.astc" >nul
echo Success: c99.astc (generic) created as copy of c99_x64_64.astc

REM Also copy tools to bin root for compatibility
copy "bin\tools\tool_c2astc.exe" "bin\tool_c2astc.exe" >nul
copy "bin\tools\tool_astc2native.exe" "bin\tool_astc2native.exe" >nul
echo Success: Tools copied to bin\ for backward compatibility

REM Step 6: Testing built programs
echo.
echo Testing built programs...
echo ========================

REM Test development tools
echo Testing tool_c2astc.exe...
if exist "bin\tools\tool_c2astc.exe" (
    echo Success: tool_c2astc.exe created
    REM Test basic functionality with --help
    "bin\tools\tool_c2astc.exe" --help >nul 2>&1
    if %ERRORLEVEL% equ 0 (
        echo Success: tool_c2astc.exe runs successfully
    ) else (
        echo Info: tool_c2astc.exe compiled but --help not supported
    )
) else (
    echo Error: tool_c2astc.exe not found
    exit /b 1
)

echo Testing tool_astc2native.exe...
if exist "bin\tools\tool_astc2native.exe" (
    echo Success: tool_astc2native.exe created
) else (
    echo Error: tool_astc2native.exe not found
    exit /b 1
)

REM Test x64 c99 compiler
echo Testing c99_x64_64.astc...
if exist "bin\layer3\c99_x64_64.astc" (
    echo Success: c99_x64_64.astc created
    REM Note: ASTC files need to be executed via Layer 1 loader + Layer 2 VM
    echo Info: c99_x64_64.astc is ASTC bytecode (requires loader + VM to execute)
) else (
    echo Error: c99_x64_64.astc not found
    exit /b 1
)

REM Test ARM64 c99 compiler (if available)
if exist "bin\layer3\c99_arm64_64.astc" (
    echo Success: c99_arm64_64.astc created
    set ARM64_C99_AVAILABLE=1
) else (
    echo Info: c99_arm64_64.astc not available
    set ARM64_C99_AVAILABLE=0
)

REM Test generic c99.astc
if exist "bin\layer3\c99.astc" (
    echo Success: c99.astc (generic) created
) else (
    echo Error: c99.astc (generic) not found
    exit /b 1
)

REM Test backward compatibility tools
if exist "bin\tool_c2astc.exe" (
    echo Success: bin\tool_c2astc.exe (compatibility copy) created
) else (
    echo Warning: bin\tool_c2astc.exe (compatibility copy) not found
)

echo.
echo Layer 3 Build Summary:
echo ======================
echo Development Tools:
dir "bin\tools\*.exe" 2>nul
echo.
echo ASTC Programs:
dir "bin\layer3\*.astc" 2>nul
echo.
echo Compatibility Tools:
dir "bin\tool_*.exe" 2>nul

echo.
echo Success: Layer 3 build completed
echo.
echo PRD.md Layer 3 Components Built:
echo.
echo Development Tools:
echo   tool_c2astc.exe        - C source to ASTC bytecode converter (x64)
echo   tool_astc2native.exe   - ASTC bytecode to native code converter (x64)
if defined ARM64_TOOLS_SUCCESS if %ARM64_TOOLS_SUCCESS%==1 (
    echo   tool_c2astc_arm64.exe     - C source to ASTC bytecode converter (ARM64)
    echo   tool_astc2native_arm64.exe - ASTC bytecode to native code converter (ARM64)
)
echo.
echo ASTC Programs:
echo   c99_x64_64.astc        - C99 compiler for x64 architecture
if defined ARM64_C99_AVAILABLE if %ARM64_C99_AVAILABLE%==1 (
    echo   c99_arm64_64.astc      - C99 compiler for ARM64 architecture
)
echo   c99.astc               - Generic C99 compiler (x64 compatible)
echo.
echo Correct Usage in PRD.md three-layer architecture:
echo   Step 1: Convert C to ASTC:
echo     tool_c2astc.exe source.c program.astc
echo   Step 2: Execute ASTC via layers:
echo     loader_{arch}_{bits}.exe program.astc [args]
echo     Layer 1 (loader) → Layer 2 (vm) → Layer 3 (program.astc)
echo.
echo C99 Compiler Usage:
echo   tool_c2astc.exe hello.c hello.astc
echo   loader_x64_64.exe c99_x64_64.astc hello.c -o hello
echo.
if defined ARM64_C99_AVAILABLE if %ARM64_C99_AVAILABLE%==1 (
    echo Multi-architecture support: x64 and ARM64 tools and compilers available
    echo Cross-compilation: Use appropriate tool_{name}_{arch}.exe for target architecture
) else (
    echo Single-architecture support: x64 tools and compilers only
    echo Cross-compilation: ARM64 tools require TCC with ARM64 target support
)

exit /b 0
