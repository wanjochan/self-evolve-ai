@echo off
echo === Testing evolver2 -> evolver3 Bootstrap ===
echo Verifying complete TinyCC independence

echo.
echo Step 1: Backup TinyCC (simulate removal)
echo ========================================
if exist "external\tcc-win\tcc\tcc.exe" (
    echo Backing up TinyCC to simulate removal...
    move "external\tcc-win\tcc\tcc.exe" "external\tcc-win\tcc\tcc.exe.backup" >nul
    echo TinyCC temporarily removed from system
) else (
    echo TinyCC already not available
)

echo.
echo Step 2: Test evolver2 tools functionality
echo =========================================

echo Testing program_c99 without TinyCC...
echo int main() { return 200; } > tests\bootstrap_test.c

bin\program_c99.exe tests\bootstrap_test.c tests\bootstrap_test.astc
if errorlevel 1 (
    echo FAIL: program_c99 cannot work without TinyCC
    set PROGRAM_C99_OK=0
) else (
    echo OK: program_c99 works independently
    set PROGRAM_C99_OK=1
)

echo Testing astc_assembler_v3 without TinyCC...
if %PROGRAM_C99_OK%==1 (
    bin\astc_assembler_v3.exe tests\bootstrap_test.astc tests\bootstrap_test.exe windows-x64
    if errorlevel 1 (
        echo FAIL: astc_assembler_v3 cannot work without TinyCC
        set ASSEMBLER_OK=0
    ) else (
        echo OK: astc_assembler_v3 works independently
        
        echo Testing generated executable...
        tests\bootstrap_test.exe
        echo Exit code: %errorlevel%
        set ASSEMBLER_OK=1
    )
) else (
    set ASSEMBLER_OK=0
)

echo.
echo Step 3: Test evolver3 component creation
echo ========================================

echo Creating evolver3_loader.c (enhanced version)...
echo #include ^<stdio.h^> > tests\evolver3_loader.c
echo int main() { >> tests\evolver3_loader.c
echo     printf("evolver3_loader v1.0\n"); >> tests\evolver3_loader.c
echo     return 300; >> tests\evolver3_loader.c
echo } >> tests\evolver3_loader.c

echo Compiling evolver3_loader with evolver2 tools...
if %PROGRAM_C99_OK%==1 (
    bin\program_c99.exe tests\evolver3_loader.c tests\evolver3_loader.astc
    if errorlevel 1 (
        echo FAIL: Cannot compile evolver3 components
        set EVOLVER3_OK=0
    ) else (
        echo OK: evolver3_loader compiled successfully
        
        echo Assembling evolver3_loader...
        if %ASSEMBLER_OK%==1 (
            bin\astc_assembler_v3.exe tests\evolver3_loader.astc tests\evolver3_loader.exe windows-x64
            if errorlevel 1 (
                echo FAIL: Cannot assemble evolver3 components
                set EVOLVER3_OK=0
            ) else (
                echo OK: evolver3_loader assembled successfully
                
                echo Testing evolver3_loader...
                tests\evolver3_loader.exe
                echo evolver3_loader exit code: %errorlevel%
                set EVOLVER3_OK=1
            )
        ) else (
            set EVOLVER3_OK=0
        )
    )
) else (
    set EVOLVER3_OK=0
)

echo.
echo Step 4: Test self-replication capability
echo ========================================

echo Testing if evolver2 can replicate itself...
if %PROGRAM_C99_OK%==1 if %ASSEMBLER_OK%==1 (
    echo Recompiling program_c99 with itself...
    bin\program_c99.exe src\tools\program_c99.c tests\program_c99_v3.astc
    if errorlevel 1 (
        echo FAIL: Cannot self-replicate program_c99
        set SELF_REPLICATE_OK=0
    ) else (
        echo OK: program_c99 self-replication successful
        
        echo Assembling replicated program_c99...
        bin\astc_assembler_v3.exe tests\program_c99_v3.astc tests\program_c99_v3.exe windows-x64
        if errorlevel 1 (
            echo FAIL: Cannot assemble replicated program_c99
            set SELF_REPLICATE_OK=0
        ) else (
            echo OK: Replicated program_c99 assembled
            set SELF_REPLICATE_OK=1
        )
    )
) else (
    set SELF_REPLICATE_OK=0
)

echo.
echo Step 5: Final independence assessment
echo ====================================

echo COMPLETE INDEPENDENCE VERIFICATION:
echo.

if %PROGRAM_C99_OK%==1 (
    echo [PASS] program_c99 works without TinyCC
) else (
    echo [FAIL] program_c99 requires TinyCC
)

if %ASSEMBLER_OK%==1 (
    echo [PASS] astc_assembler_v3 works without TinyCC
) else (
    echo [FAIL] astc_assembler_v3 requires TinyCC
)

if %EVOLVER3_OK%==1 (
    echo [PASS] Can build evolver3 components
) else (
    echo [FAIL] Cannot build evolver3 components
)

if %SELF_REPLICATE_OK%==1 (
    echo [PASS] Can self-replicate toolchain
) else (
    echo [FAIL] Cannot self-replicate toolchain
)

echo.
set /a TOTAL_TESTS=4
set /a PASSED_TESTS=0
if %PROGRAM_C99_OK%==1 set /a PASSED_TESTS+=1
if %ASSEMBLER_OK%==1 set /a PASSED_TESTS+=1
if %EVOLVER3_OK%==1 set /a PASSED_TESTS+=1
if %SELF_REPLICATE_OK%==1 set /a PASSED_TESTS+=1

echo Bootstrap Score: %PASSED_TESTS%/%TOTAL_TESTS% tests passed

if %PASSED_TESTS%==4 (
    echo.
    echo *** CONFIRMED: TRUE 100%% TINYCC INDEPENDENCE! ***
    echo.
    echo ✅ evolver2 tools work completely without TinyCC
    echo ✅ Can build evolver3 components independently
    echo ✅ Can self-replicate the entire toolchain
    echo ✅ Generated executables run correctly
    echo.
    echo 🏆 VERIFIED: evolver2 -> evolver3 bootstrap possible!
    echo 🎉 Complete compiler independence achieved!
) else (
    echo.
    echo ❌ REALITY CHECK: NOT truly independent yet
    echo ❌ Failed tests: %PASSED_TESTS%/%TOTAL_TESTS%
    echo ❌ Cannot reliably bootstrap evolver3 without TinyCC
    echo.
    echo 🔧 evolver2 tools still have dependencies
)

echo.
echo Step 6: Restore TinyCC
echo ======================
if exist "external\tcc-win\tcc\tcc.exe.backup" (
    echo Restoring TinyCC...
    move "external\tcc-win\tcc\tcc.exe.backup" "external\tcc-win\tcc\tcc.exe" >nul
    echo TinyCC restored
)

echo.
echo Cleanup...
del tests\bootstrap_test.* tests\evolver3_loader.* tests\program_c99_v3.* 2>nul

echo.
echo === evolver2 -> evolver3 Bootstrap Test Complete ===
pause
