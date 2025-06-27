@echo off
echo === Marking old/unused files for deletion ===

REM Mark old build scripts in root
if exist "build_evolver0.bat" ren "build_evolver0.bat" "build_evolver0.bat.todelete"
if exist "organize.bat" ren "organize.bat" "organize.bat.todelete"

REM Mark duplicate/old files in bin
if exist "bin\evolver0_loader_fixed.exe" ren "bin\evolver0_loader_fixed.exe" "evolver0_loader_fixed.exe.todelete"
if exist "bin\evolver0_loader_new.exe" ren "bin\evolver0_loader_new.exe" "evolver0_loader_new.exe.todelete"
if exist "bin\program_c99.exe" ren "bin\program_c99.exe" "program_c99.exe.todelete"

REM Mark old/debug test files
if exist "tests\debug_ast_structure.c" ren "tests\debug_ast_structure.c" "debug_ast_structure.c.todelete"
if exist "tests\debug_ast_structure.exe" ren "tests\debug_ast_structure.exe" "debug_ast_structure.exe.todelete"
if exist "tests\debug_astc_serialization.c" ren "tests\debug_astc_serialization.c" "debug_astc_serialization.c.todelete"
if exist "tests\debug_astc_serialization.exe" ren "tests\debug_astc_serialization.exe" "debug_astc_serialization.exe.todelete"
if exist "tests\debug_loader_runtime.c" ren "tests\debug_loader_runtime.c" "debug_loader_runtime.c.todelete"
if exist "tests\debug_loader_runtime.exe" ren "tests\debug_loader_runtime.exe" "debug_loader_runtime.exe.todelete"
if exist "tests\debug_machine_code.c" ren "tests\debug_machine_code.c" "debug_machine_code.c.todelete"
if exist "tests\debug_machine_code.exe" ren "tests\debug_machine_code.exe" "debug_machine_code.exe.todelete"
if exist "tests\debug_node_types.c" ren "tests\debug_node_types.c" "debug_node_types.c.todelete"
if exist "tests\debug_node_types.exe" ren "tests\debug_node_types.exe" "debug_node_types.exe.todelete"
if exist "tests\debug_runtime.c" ren "tests\debug_runtime.c" "debug_runtime.c.todelete"
if exist "tests\debug_runtime_code.c" ren "tests\debug_runtime_code.c" "debug_runtime_code.c.todelete"
if exist "tests\debug_runtime_code.exe" ren "tests\debug_runtime_code.exe" "debug_runtime_code.exe.todelete"
if exist "tests\debug_runtime_load.c" ren "tests\debug_runtime_load.c" "debug_runtime_load.c.todelete"
if exist "tests\debug_runtime_load.exe" ren "tests\debug_runtime_load.exe" "debug_runtime_load.exe.todelete"

REM Mark old test files that are superseded
if exist "tests\evolver0_program_copy.astc" ren "tests\evolver0_program_copy.astc" "evolver0_program_copy.astc.todelete"
if exist "tests\evolver0_program_simple.c" ren "tests\evolver0_program_simple.c" "evolver0_program_simple.c.todelete"
if exist "tests\evolver0_program_simple.astc" ren "tests\evolver0_program_simple.astc" "evolver0_program_simple.astc.todelete"
if exist "tests\evolver0_program_working.c" ren "tests\evolver0_program_working.c" "evolver0_program_working.c.todelete"
if exist "tests\evolver0_program_working.astc" ren "tests\evolver0_program_working.astc" "evolver0_program_working.astc.todelete"

REM Mark old simple test files
if exist "tests\simple_hello.c" ren "tests\simple_hello.c" "simple_hello.c.todelete"
if exist "tests\simple_hello.astc" ren "tests\simple_hello.astc" "simple_hello.astc.todelete"
if exist "tests\simple_loader.c" ren "tests\simple_loader.c" "simple_loader.c.todelete"
if exist "tests\simple_loader.exe" ren "tests\simple_loader.exe" "simple_loader.exe.todelete"
if exist "tests\simple_test.c" ren "tests\simple_test.c" "simple_test.c.todelete"
if exist "tests\simple_test.astc" ren "tests\simple_test.astc" "simple_test.astc.todelete"
if exist "tests\simple_test.bin" ren "tests\simple_test.bin" "simple_test.bin.todelete"
if exist "tests\simple_test_program.c" ren "tests\simple_test_program.c" "simple_test_program.c.todelete"
if exist "tests\simple_test_program.astc" ren "tests\simple_test_program.astc" "simple_test_program.astc.todelete"
if exist "tests\simple_test_program_new.astc" ren "tests\simple_test_program_new.astc" "simple_test_program_new.astc.todelete"
if exist "tests\simple_runtime.bin" ren "tests\simple_runtime.bin" "simple_runtime.bin.todelete"
if exist "tests\simple_two_funcs.c" ren "tests\simple_two_funcs.c" "simple_two_funcs.c.todelete"
if exist "tests\simple_two_funcs.astc" ren "tests\simple_two_funcs.astc" "simple_two_funcs.astc.todelete"

REM Mark old working test files
if exist "tests\working_compiler.c" ren "tests\working_compiler.c" "working_compiler.c.todelete"
if exist "tests\working_compiler.astc" ren "tests\working_compiler.astc" "working_compiler.astc.todelete"
if exist "tests\minimal_working.c" ren "tests\minimal_working.c" "minimal_working.c.todelete"
if exist "tests\minimal_working.astc" ren "tests\minimal_working.astc" "minimal_working.astc.todelete"

REM Mark old individual test files that are now covered by comprehensive tests
if exist "tests\test_simple_function.c" ren "tests\test_simple_function.c" "test_simple_function.c.todelete"
if exist "tests\test_simple_main.c" ren "tests\test_simple_main.c" "test_simple_main.c.todelete"
if exist "tests\test_simple_program.c" ren "tests\test_simple_program.c" "test_simple_program.c.todelete"
if exist "tests\test_simple_program.astc" ren "tests\test_simple_program.astc" "test_simple_program.astc.todelete"
if exist "tests\test_simple_runtime.c" ren "tests\test_simple_runtime.c" "test_simple_runtime.c.todelete"
if exist "tests\test_simple_runtime.exe" ren "tests\test_simple_runtime.exe" "test_simple_runtime.exe.todelete"

REM Mark old utility files
if exist "tests\hex_dump.c" ren "tests\hex_dump.c" "hex_dump.c.todelete"
if exist "tests\hex_dump.exe" ren "tests\hex_dump.exe" "hex_dump.exe.todelete"
if exist "tests\safe_loader.c" ren "tests\safe_loader.c" "safe_loader.c.todelete"
if exist "tests\safe_loader.exe" ren "tests\safe_loader.exe" "safe_loader.exe.todelete"

REM Mark old compile test files
if exist "tests\test_compile_function.c" ren "tests\test_compile_function.c" "test_compile_function.c.todelete"
if exist "tests\test_c2astc_debug.c" ren "tests\test_c2astc_debug.c" "test_c2astc_debug.c.todelete"

REM Mark old machine code test files
if exist "tests\test_machine_code.c" ren "tests\test_machine_code.c" "test_machine_code.c.todelete"
if exist "tests\test_machine_code.exe" ren "tests\test_machine_code.exe" "test_machine_code.exe.todelete"
if exist "tests\test_machine_code_call.c" ren "tests\test_machine_code_call.c" "test_machine_code_call.c.todelete"
if exist "tests\test_machine_code_call.exe" ren "tests\test_machine_code_call.exe" "test_machine_code_call.exe.todelete"

REM Mark old loader test files
if exist "tests\test_loader_args.c" ren "tests\test_loader_args.c" "test_loader_args.c.todelete"
if exist "tests\test_loader_args.exe" ren "tests\test_loader_args.exe" "test_loader_args.exe.todelete"
if exist "tests\test_loader_simple.c" ren "tests\test_loader_simple.c" "test_loader_simple.c.todelete"
if exist "tests\test_loader_simple.exe" ren "tests\test_loader_simple.exe" "test_loader_simple.exe.todelete"

REM Mark old runtime test files
if exist "tests\test_our_runtime.c" ren "tests\test_our_runtime.c" "test_our_runtime.c.todelete"
if exist "tests\test_our_runtime.exe" ren "tests\test_our_runtime.exe" "test_our_runtime.exe.todelete"

REM Mark old bootstrap test files (superseded by complete bootstrap test)
if exist "tests\test_complete_bootstrap.c" ren "tests\test_complete_bootstrap.c" "test_complete_bootstrap.c.todelete"
if exist "tests\test_complete_bootstrap.exe" ren "tests\test_complete_bootstrap.exe" "test_complete_bootstrap.exe.todelete"

REM Mark old individual AI test files (superseded by comprehensive AI tests)
if exist "tests\test_ai_evolution.c" ren "tests\test_ai_evolution.c" "test_ai_evolution.c.todelete"
if exist "tests\test_ai_evolution.exe" ren "tests\test_ai_evolution.exe" "test_ai_evolution.exe.todelete"
if exist "tests\test_ai_learning.c" ren "tests\test_ai_learning.c" "test_ai_learning.c.todelete"
if exist "tests\test_ai_learning.exe" ren "tests\test_ai_learning.exe" "test_ai_learning.exe.todelete"
if exist "tests\test_ai_optimizer.c" ren "tests\test_ai_optimizer.c" "test_ai_optimizer.c.todelete"
if exist "tests\test_ai_optimizer.exe" ren "tests\test_ai_optimizer.exe" "test_ai_optimizer.exe.todelete"

REM Mark old output files
if exist "tests\output.txt" ren "tests\output.txt" "output.txt.todelete"
if exist "tests\debug_runtime.astc" ren "tests\debug_runtime.astc" "debug_runtime.astc.todelete"

REM Mark old test executable
if exist "tests\test_simple.exe" ren "tests\test_simple.exe" "test_simple.exe.todelete"

echo.
echo Old files marked for deletion with .todelete suffix
echo These files can be safely deleted later after verification
echo.
echo Remaining important test files:
echo   - test_complete_evolver0_system.* (comprehensive system test)
echo   - test_ai_adaptive_framework.* (complete AI framework test)
echo   - Individual feature tests (basic compiler, language features)
echo   - run_all_tests.* (test runners)
echo.
