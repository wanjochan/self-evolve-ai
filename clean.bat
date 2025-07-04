@echo off
chcp 65001 >nul

echo ========================================
echo Self-Evolve AI Build Cleanup
echo Removing All Build Artifacts
echo ========================================

echo Cleaning build artifacts...
echo.

REM Clean Layer 1 (Loader) artifacts
echo Cleaning Layer 1 (Loader) artifacts...
if exist "bin\layer1" (
    echo Removing bin\layer1\*.exe...
    del /q "bin\layer1\*.exe" 2>nul
    echo Removing bin\layer1\*.o..." 
    del /q "bin\layer1\*.o" 2>nul
    echo Layer 1 artifacts cleaned
) else (
    echo Layer 1 directory not found (already clean)
)

echo.

REM Clean Layer 2 (Runtime Modules) artifacts
echo Cleaning Layer 2 (Runtime Modules) artifacts...
if exist "bin\layer2" (
    echo Removing bin\layer2\*.native...
    del /q "bin\layer2\*.native" 2>nul
    echo Removing bin\layer2\*.def...
    del /q "bin\layer2\*.def" 2>nul
    echo Removing bin\layer2\*.o...
    del /q "bin\layer2\*.o" 2>nul
    echo Removing bin\layer2\*_temp.exe...
    del /q "bin\layer2\*_temp.exe" 2>nul
    echo Layer 2 artifacts cleaned
) else (
    echo Layer 2 directory not found (already clean)
)

echo.

REM Clean Layer 3 (ASTC Programs) artifacts
echo Cleaning Layer 3 (ASTC Programs) artifacts...
if exist "bin\layer3" (
    echo Removing bin\layer3\*.astc...
    del /q "bin\layer3\*.astc" 2>nul
    echo Removing bin\layer3\*.o...
    del /q "bin\layer3\*.o" 2>nul
    echo Layer 3 artifacts cleaned
) else (
    echo Layer 3 directory not found (already clean)
)

echo.

REM Clean tools artifacts
echo Cleaning tools artifacts...
if exist "tools" (
    echo Removing tools\*.exe...
    del /q "tools\*.exe" 2>nul
    echo Removing tools\*.o...
    del /q "tools\*.o" 2>nul
    echo Tools artifacts cleaned
) else (
    echo Tools directory not found (already clean)
)

echo.

REM Clean test artifacts
echo Cleaning test artifacts...
echo Removing test_*.exe...
del /q "test_*.exe" 2>nul
echo Removing *.o...
del /q "*.o" 2>nul
echo Removing temporary files...
del /q "*.tmp" 2>nul
del /q "*.temp" 2>nul

echo.

REM Clean tests directory artifacts
if exist "tests" (
    echo Cleaning tests directory...
    echo Removing tests\*.exe...
    del /q "tests\*.exe" 2>nul
    echo Removing tests\*.o...
    del /q "tests\*.o" 2>nul
    echo Tests artifacts cleaned
)

echo.

REM Optional: Remove empty directories
echo Checking for empty directories...
if exist "bin\layer1" (
    dir /b "bin\layer1" | findstr . >nul || (
        echo Removing empty bin\layer1 directory...
        rmdir "bin\layer1" 2>nul
    )
)

if exist "bin\layer2" (
    dir /b "bin\layer2" | findstr . >nul || (
        echo Removing empty bin\layer2 directory...
        rmdir "bin\layer2" 2>nul
    )
)

if exist "bin\layer3" (
    dir /b "bin\layer3" | findstr . >nul || (
        echo Removing empty bin\layer3 directory...
        rmdir "bin\layer3" 2>nul
    )
)

if exist "bin" (
    dir /b "bin" | findstr . >nul || (
        echo Removing empty bin directory...
        rmdir "bin" 2>nul
    )
)

echo.
echo ========================================
echo Cleanup Summary
echo ========================================

echo Cleaned artifacts:
echo   ✓ Layer 1 executables (*.exe)
echo   ✓ Layer 2 native modules (*.native, *.def)
echo   ✓ Layer 3 ASTC programs (*.astc)
echo   ✓ Tools executables
echo   ✓ Object files (*.o)
echo   ✓ Temporary files (*.tmp, *.temp)
echo   ✓ Test executables

echo.
echo Remaining files:
if exist "bin" (
    echo bin directory contents:
    dir /s /b "bin" 2>nul || echo   (empty)
) else (
    echo   bin directory: (removed)
)

echo.
echo ========================================
echo Cleanup Completed Successfully!
echo ========================================

echo.
echo The project is now clean and ready for a fresh build.
echo Run 'build_all.bat' to rebuild the complete three-layer architecture.

exit /b 0
