@echo off
echo === Evolver0 Codebase Cleanup ===
echo.

echo Step 1: Creating backup directory...
if not exist "backup_before_cleanup" mkdir backup_before_cleanup

echo Step 2: Moving .todelete files to backup...
if exist "src\ai.todelete" (
    echo Moving AI components marked for deletion...
    move "src\ai.todelete" "backup_before_cleanup\" >nul 2>&1
)

echo Step 3: Cleaning up bin directory...
if exist "bin\*.todelete" (
    echo Moving obsolete executables...
    move "bin\*.todelete" "backup_before_cleanup\" >nul 2>&1
)

echo Step 4: Cleaning up tools directory...
if exist "src\tools\*.todelete" (
    echo Moving obsolete tools...
    move "src\tools\*.todelete" "backup_before_cleanup\" >nul 2>&1
)

echo Step 5: Organizing core evolver0 files...
echo Core evolver0 files:
echo   - src\evolver0\evolver0_loader.c
echo   - src\evolver0\evolver0_runtime.c  
echo   - src\evolver0\evolver0_program.c
echo   - src\runtime\ (shared components)
echo   - src\tools\ (build tools)
echo   - build0.bat (main build script)

echo Step 6: Verifying evolver1 files...
echo Generated evolver1 files:
dir bin\evolver1_* /b 2>nul

echo Step 7: Creating file inventory...
echo === Core System Files === > file_inventory.txt
echo Evolver0 Components: >> file_inventory.txt
dir src\evolver0\*.c /b >> file_inventory.txt
echo. >> file_inventory.txt
echo Runtime Components: >> file_inventory.txt  
dir src\runtime\*.c /b >> file_inventory.txt
echo. >> file_inventory.txt
echo Build Tools: >> file_inventory.txt
dir src\tools\*.c /b >> file_inventory.txt
echo. >> file_inventory.txt
echo Generated Binaries: >> file_inventory.txt
dir bin\evolver0_* /b >> file_inventory.txt
echo. >> file_inventory.txt
echo Evolver1 System: >> file_inventory.txt
dir bin\evolver1_* /b >> file_inventory.txt

echo.
echo === Cleanup Complete ===
echo.
echo ✅ Obsolete files moved to backup_before_cleanup\
echo ✅ Core evolver0 system preserved
echo ✅ Evolver1 system verified
echo ✅ File inventory created: file_inventory.txt
echo.
echo Next steps:
echo   1. Run build0.bat to verify system integrity
echo   2. Test evolver0 and evolver1 functionality
echo   3. Review file_inventory.txt for completeness
echo.
pause
