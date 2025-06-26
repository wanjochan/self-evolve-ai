# PowerShell test script for evolver0 system
Write-Host "========================================"
Write-Host "Running all evolver0 tests"
Write-Host "========================================"

$failed = 0

Write-Host ""
Write-Host "[1/6] Testing basic compiler functionality..."
& .\tool_build_program.exe tests\test_basic_compiler.c tests\test_basic_compiler.astc
if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ Basic compiler test compilation failed"
    $failed = 1
} else {
    & .\evolver0_loader.exe evolver0_runtime.bin tests\test_basic_compiler.astc
    if ($LASTEXITCODE -ne 0) {
        Write-Host "❌ Basic compiler test execution failed"
        $failed = 1
    } else {
        Write-Host "✅ Basic compiler test passed"
    }
}

Write-Host ""
Write-Host "[2/6] Testing self-bootstrap functionality..."
& .\tool_build_program.exe tests\test_self_bootstrap.c tests\test_self_bootstrap.astc
if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ Self-bootstrap test compilation failed"
    $failed = 1
} else {
    & .\evolver0_loader.exe evolver0_runtime.bin tests\test_self_bootstrap.astc
    if ($LASTEXITCODE -ne 0) {
        Write-Host "❌ Self-bootstrap test execution failed"
        $failed = 1
    } else {
        Write-Host "✅ Self-bootstrap test passed"
    }
}

Write-Host ""
Write-Host "[3/6] Testing for-loop support..."
& .\evolver0_loader.exe evolver0_runtime.bin tests\test_for_loop.astc
if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ For-loop test failed"
    $failed = 1
} else {
    Write-Host "✅ For-loop test passed"
}

Write-Host ""
Write-Host "[4/6] Testing evolver0 core functionality..."
& .\evolver0_loader.exe evolver0_runtime.bin evolver0_program.astc
if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ Evolver0 core functionality test failed"
    $failed = 1
} else {
    Write-Host "✅ Evolver0 core functionality test passed"
}

Write-Host ""
Write-Host "[5/6] Testing evolver1 generation..."
if (Test-Path "evolver1_program.astc") {
    & .\evolver0_loader.exe evolver0_runtime.bin evolver1_program.astc
    if ($LASTEXITCODE -ne 0) {
        Write-Host "❌ Evolver1 execution failed"
        $failed = 1
    } else {
        Write-Host "✅ Evolver1 test passed"
    }
} else {
    Write-Host "⚠️  evolver1_program.astc does not exist, skipping test"
}

Write-Host ""
Write-Host "[6/6] Testing three-layer architecture integrity..."
$missing = @()
if (!(Test-Path "evolver0_loader.exe")) { $missing += "evolver0_loader.exe" }
if (!(Test-Path "evolver0_runtime.bin")) { $missing += "evolver0_runtime.bin" }
if (!(Test-Path "evolver0_program.astc")) { $missing += "evolver0_program.astc" }

if ($missing.Count -gt 0) {
    Write-Host "❌ Missing files: $($missing -join ', ')"
    $failed = 1
} else {
    Write-Host "✅ Three-layer architecture integrity check passed"
}

Write-Host ""
Write-Host "========================================"
if ($failed -eq 0) {
    Write-Host "🎉 All tests passed!"
    Write-Host "✅ Evolver0 system functioning normally"
    Write-Host "✅ Three-layer architecture working properly"
    Write-Host "✅ ASTC serialization support complete"
    Write-Host "✅ Self-bootstrap compilation infrastructure complete"
} else {
    Write-Host "❌ Some tests failed"
    Write-Host "Please check error messages and fix issues"
}
Write-Host "========================================"

exit $failed
