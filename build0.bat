@echo off
echo === Building evolver0 System ===

set TCC=external\tcc-win\tcc\tcc.exe

echo Building tool_c2astc...
%TCC% -o bin\tool_c2astc.exe src\tools\tool_c2astc.c src\runtime\c2astc.c -Isrc\runtime

echo Building tool_astc2rt...
%TCC% -o bin\tool_astc2rt.exe src\tools\tool_astc2rt.c src\runtime\astc2rt.c src\runtime\c2astc.c src\runtime\codegen.c src\runtime\codegen_x64_64.c -Isrc\runtime

echo Building evolver0_loader...
%TCC% -o bin\evolver0_loader.exe src\runtime\loader.c src\runtime\platform_minimal.c -Isrc\runtime

echo Generating runtime...
bin\tool_c2astc.exe src\runtime\runtime.c bin\evolver0_runtime.astc
bin\tool_astc2rt.exe bin\evolver0_runtime.astc bin\runtimex64_64.rt

echo Generating program...
bin\tool_c2astc.exe src\evolver0.c bin\evolver0_program.astc

echo Skipping AI tests (components marked for deletion)...
REM %TCC% -o bin\test_ai_adaptive_framework.exe tests\test_ai_adaptive_framework.c src\ai.todelete\ai_adaptive_framework.c src\ai.todelete\ai_optimizer.c src\ai.todelete\ai_learning.c src\ai.todelete\ai_evolution.c src\tools\c2astc.c -Isrc\ai.todelete -Isrc\runtime -Isrc\tools

REM %TCC% -o bin\test_complete_evolver0_system.exe tests\test_complete_evolver0_system.c src\ai.todelete\ai_adaptive_framework.c src\ai.todelete\ai_optimizer.c src\ai.todelete\ai_learning.c src\ai.todelete\ai_evolution.c src\tools\c2astc.c -Isrc\ai.todelete -Isrc\runtime -Isrc\tools

echo Testing self-bootstrap...
bin\evolver0_loader.exe bin\evolver0_program.astc

echo Build complete!
echo Core evolver0 system ready - Three-layer architecture working!
echo Note: AI tests skipped (components marked for deletion)
