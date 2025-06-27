@echo off
echo === Building evolver0 System ===

set TCC=external\tcc-win\tcc\tcc.exe

echo Building tool_c2astc...
%TCC% -o bin\tool_c2astc.exe src\tools\tool_c2astc.c src\tools\c2astc.c -Isrc\runtime

echo Building tool_astc2bin...
%TCC% -o bin\tool_astc2bin.exe src\tools\tool_astc2bin.c src\tools\c2astc.c -Isrc\runtime -Isrc\tools

echo Building evolver0_loader...
%TCC% -o bin\evolver0_loader.exe src\evolver0\evolver0_loader.c src\runtime\platform.c -Isrc\runtime -Iexternal\tcc\build\host\win32\include\winapi

echo Generating runtime...
bin\tool_c2astc.exe src\evolver0\evolver0_runtime.c bin\evolver0_runtime.astc
bin\tool_astc2bin.exe bin\evolver0_runtime.astc bin\evolver0_runtime.bin

echo Generating program...
bin\tool_c2astc.exe src\evolver0\evolver0_program.c bin\evolver0_program.astc

echo Building AI tests...
%TCC% -o bin\test_ai_adaptive_framework.exe tests\test_ai_adaptive_framework.c src\ai.todelete\ai_adaptive_framework.c src\ai.todelete\ai_optimizer.c src\ai.todelete\ai_learning.c src\ai.todelete\ai_evolution.c src\tools\c2astc.c -Isrc\ai.todelete -Isrc\runtime -Isrc\tools

%TCC% -o bin\test_complete_evolver0_system.exe tests\test_complete_evolver0_system.c src\ai.todelete\ai_adaptive_framework.c src\ai.todelete\ai_optimizer.c src\ai.todelete\ai_learning.c src\ai.todelete\ai_evolution.c src\tools\c2astc.c -Isrc\ai.todelete -Isrc\runtime -Isrc\tools

echo Testing self-bootstrap...
bin\evolver0_loader.exe bin\evolver0_runtime.bin bin\evolver0_program.astc src\evolver0\evolver0_program.c bin\evolver1_program.astc

echo Build complete!
echo Run: bin\test_complete_evolver0_system.exe
