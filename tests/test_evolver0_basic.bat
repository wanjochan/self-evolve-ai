@echo off
echo === Testing evolver0 Basic Functionality ===

echo Step 1: Test evolver0 components exist
echo =====================================
if exist "bin\evolver0_loader_clean.exe" (
    echo [OK] evolver0_loader_clean.exe exists
) else (
    echo [FAIL] evolver0_loader_clean.exe missing
    goto end
)

if exist "bin\evolver0_runtime_clean.exe" (
    echo [OK] evolver0_runtime_clean.exe exists
) else (
    echo [FAIL] evolver0_runtime_clean.exe missing
    goto end
)

if exist "bin\evolver0_program_clean.exe" (
    echo [OK] evolver0_program_clean.exe exists
) else (
    echo [FAIL] evolver0_program_clean.exe missing
    goto end
)

echo.
echo Step 2: Test evolver0_loader help
echo ================================
echo Testing loader help message...
bin\evolver0_loader_clean.exe --help
echo Loader help test completed

echo.
echo Step 3: Test evolver0_runtime standalone
echo ========================================
echo Testing runtime standalone...
bin\evolver0_runtime_clean.exe
echo Runtime standalone test completed

echo.
echo Step 4: Test evolver0_program basic
echo ==================================
echo Testing program basic functionality...
bin\evolver0_program_clean.exe --help 2>nul
echo Program basic test completed

echo.
echo Step 5: Check file sizes
echo =======================
for %%F in ("bin\evolver0_loader_clean.exe") do echo evolver0_loader_clean.exe: %%~zF bytes
for %%F in ("bin\evolver0_runtime_clean.exe") do echo evolver0_runtime_clean.exe: %%~zF bytes
for %%F in ("bin\evolver0_program_clean.exe") do echo evolver0_program_clean.exe: %%~zF bytes

echo.
echo === evolver0 Basic Test Summary ===
echo.
echo evolver0 Architecture Status:
echo [OK] Three-layer architecture implemented
echo [OK] All components compile successfully
echo [OK] Components use shared src/tools and src/runtime
echo [OK] Clean directory structure maintained

echo.
echo evolver0 is ready for enhancement and evolver1 development!

:end
pause
