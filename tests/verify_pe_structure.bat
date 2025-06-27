@echo off
echo === Verifying PE Structure ===

echo Checking DOS header...
powershell -command "Get-Content tests\test_independence_v2.exe -Encoding Byte -TotalCount 2 | ForEach-Object { Write-Host ([char]$_) -NoNewline }"
echo.

echo Checking file size...
for %%F in ("tests\test_independence_v2.exe") do echo File size: %%~zF bytes

echo.
echo Comparing with working executable...
for %%F in ("bin\program_c99.exe") do echo program_c99.exe size: %%~zF bytes

echo.
echo The issue might be in our machine code generation
echo or PE format details. Let's focus on fixing the
echo executable format step by step.

pause
