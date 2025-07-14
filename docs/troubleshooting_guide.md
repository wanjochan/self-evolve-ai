# Self-Evolve AI Troubleshooting Guide

## Quick Diagnosis

### System Health Check
```bash
# Check if core components exist
ls -la bin/simple_loader bin/c2astc_direct

# Test basic functionality
./bin/simple_loader --arch
./bin/c2astc_direct --help 2>/dev/null || echo "c2astc_direct needs input files"

# Run quick test
echo 'int main() { return 42; }' > /tmp/test.c
./bin/c2astc_direct /tmp/test.c /tmp/test.astc
./bin/simple_loader /tmp/test.astc
```

## Common Issues and Solutions

### 1. Compilation Issues

#### Problem: "Command not found" errors
```bash
# Symptoms
bash: ./bin/simple_loader: No such file or directory
```

**Solution:**
```bash
# Check if files exist
ls -la bin/

# If missing, rebuild
make clean && make all

# Check permissions
chmod +x bin/*
```

#### Problem: GCC compilation errors
```bash
# Symptoms
gcc: error: unrecognized command line option '-std=c99'
```

**Solution:**
```bash
# Update GCC
sudo apt-get update
sudo apt-get install gcc

# Check GCC version (need 4.9+)
gcc --version

# Use alternative compiler if needed
export CC=clang
```

#### Problem: Missing header files
```bash
# Symptoms
fatal error: 'stdio.h' file not found
```

**Solution:**
```bash
# Install development packages
sudo apt-get install build-essential
sudo apt-get install libc6-dev

# For CentOS/RHEL
sudo yum groupinstall "Development Tools"
```

### 2. Module Loading Issues

#### Problem: Module not found
```bash
# Symptoms
Module: 警告: 无法打开模块文件 ./bin/pipeline_x64_64.native: No such file or directory
```

**Solution:**
```bash
# Check module files
ls -la bin/*_x64_64.native

# Rebuild modules if missing
./build_core.sh

# Check architecture detection
./bin/simple_loader --arch

# Use direct-linked version as workaround
./bin/c2astc_direct instead of ./bin/c2astc
```

#### Problem: Module loading segfault
```bash
# Symptoms
Segmentation fault (core dumped)
```

**Solution:**
```bash
# Use direct-linked compiler (recommended)
./bin/c2astc_direct input.c output.astc

# Enable debug mode
export ASTC_DEBUG=1
./bin/simple_loader program.astc

# Check with GDB
gdb ./bin/simple_loader
(gdb) run program.astc
(gdb) bt
```

### 3. ASTC File Issues

#### Problem: Invalid ASTC format
```bash
# Symptoms
Loader: 错误: 无效的ASTC文件格式
```

**Solution:**
```bash
# Check file format
file program.astc
hexdump -C program.astc | head -2

# Regenerate ASTC file
./bin/c2astc_direct source.c program.astc

# Use working example
cp tests/test_minimal.astc program.astc
./bin/simple_loader program.astc
```

#### Problem: Empty or corrupted ASTC file
```bash
# Symptoms
VM: ASTC文件版本: 0
VM: 字节码大小: 0 字节
```

**Solution:**
```bash
# Check file size
ls -la program.astc

# Regenerate with direct compiler
./bin/c2astc_direct source.c program.astc

# Verify file content
od -c program.astc | head -5
```

### 4. Runtime Execution Issues

#### Problem: Program crashes during execution
```bash
# Symptoms
=== 执行ASTC程序 ===
Segmentation fault
```

**Solution:**
```bash
# Use built-in VM (safer)
# This happens automatically when pipeline module fails

# Check program logic
cat source.c  # Review original C code

# Test with minimal program
echo 'int main() { return 0; }' > minimal.c
./bin/c2astc_direct minimal.c minimal.astc
./bin/simple_loader minimal.astc
```

#### Problem: Unexpected return values
```bash
# Symptoms
Program returned unexpected value
```

**Solution:**
```bash
# Check C program logic
cat source.c

# Verify compilation
./bin/c2astc_direct -v source.c program.astc

# Test step by step
echo 'int main() { return 42; }' > test.c
./bin/c2astc_direct test.c test.astc
./bin/simple_loader test.astc
echo "Exit code: $?"
```

### 5. Performance Issues

#### Problem: Slow compilation
```bash
# Symptoms
Compilation takes very long time
```

**Solution:**
```bash
# Use direct-linked compiler
./bin/c2astc_direct instead of ./bin/c2astc

# Check system resources
top
df -h

# Simplify source code
# Remove complex features temporarily
```

#### Problem: High memory usage
```bash
# Symptoms
Out of memory errors
```

**Solution:**
```bash
# Check available memory
free -h

# Monitor memory usage
valgrind --tool=massif ./bin/simple_loader program.astc

# Reduce program complexity
# Split large programs into smaller modules
```

### 6. Cross-Platform Issues

#### Problem: Architecture detection fails
```bash
# Symptoms
Unknown architecture detected
```

**Solution:**
```bash
# Check current architecture
uname -m
./bin/simple_loader --arch

# Force architecture if needed
export ASTC_ARCH=x64_64

# Use architecture-specific binaries
ls bin/*_x64_64*
```

#### Problem: Different Linux distribution issues
```bash
# Symptoms
Various compatibility errors
```

**Solution:**
```bash
# Check distribution
cat /etc/os-release

# Install missing dependencies
# Ubuntu/Debian:
sudo apt-get install build-essential

# CentOS/RHEL:
sudo yum groupinstall "Development Tools"

# Check GCC version compatibility
gcc --version
```

## Debug Mode and Logging

### Enable Debug Output
```bash
# Enable all debug output
export ASTC_DEBUG=1

# Run with debug
./bin/simple_loader program.astc

# Save debug output
./bin/simple_loader program.astc 2> debug.log
```

### Debug Information Levels
```bash
# Level 1: Basic info
export ASTC_DEBUG=1

# Level 2: Detailed info (if implemented)
export ASTC_DEBUG=2

# Level 3: Verbose info (if implemented)
export ASTC_DEBUG=3
```

### Log File Locations
```bash
# Check for log files
ls -la logs/ 2>/dev/null || echo "No logs directory"

# Create logs directory if needed
mkdir -p logs

# Redirect output to logs
./bin/simple_loader program.astc > logs/execution.log 2>&1
```

## Testing and Verification

### Run Test Suites
```bash
# Quick test
cd tests && ./test_layer1_loader.sh

# Comprehensive test
./test_stability_enhanced.sh

# All tests
./run_all_tests.sh
```

### Manual Testing Steps
```bash
# 1. Test basic compilation
echo 'int main() { return 0; }' > test.c
./bin/c2astc_direct test.c test.astc

# 2. Test execution
./bin/simple_loader test.astc
echo "Exit code: $?"

# 3. Test with return value
echo 'int main() { return 42; }' > test42.c
./bin/c2astc_direct test42.c test42.astc
./bin/simple_loader test42.astc
echo "Exit code: $?"

# 4. Test error handling
./bin/simple_loader nonexistent.astc
echo "Exit code: $?"
```

## Recovery Procedures

### Complete System Reset
```bash
# 1. Clean everything
make clean
rm -rf bin/* build/*

# 2. Rebuild from scratch
make all

# 3. Test basic functionality
cd tests && ./test_layer1_loader.sh
```

### Partial Recovery
```bash
# Rebuild only core modules
./build_core.sh

# Rebuild only tools
make tools

# Rebuild specific component
make simple_loader
```

### Backup and Restore
```bash
# Create backup of working system
tar -czf self-evolve-backup.tar.gz bin/ src/ tests/

# Restore from backup
tar -xzf self-evolve-backup.tar.gz
```

## Getting Help

### Information Gathering
```bash
# System information
uname -a
gcc --version
make --version

# Project status
ls -la bin/
./bin/simple_loader --arch
cd tests && ./test_layer1_loader.sh
```

### Reporting Issues
When reporting issues, include:
1. System information (OS, architecture, GCC version)
2. Error messages (full output)
3. Steps to reproduce
4. Expected vs actual behavior
5. Debug output (with ASTC_DEBUG=1)

### Community Resources
- GitHub Issues: https://github.com/wanjochan/self-evolve-ai/issues
- Documentation: docs/ directory
- Test Examples: tests/ directory

## Frequently Asked Questions

### Q: Why use c2astc_direct instead of c2astc?
**A:** The direct-linked version is more stable and doesn't depend on the module loading system, which can have compatibility issues.

### Q: What's the difference between the three layers?
**A:** Layer 1 (simple_loader) loads and executes programs, Layer 2 (modules) provides functionality, Layer 3 (ASTC programs) are the actual compiled programs.

### Q: Can I run this on macOS or Windows?
**A:** Currently optimized for Linux. macOS support is theoretically possible with minor modifications. Windows support would require significant work.

### Q: How do I add support for new C features?
**A:** Modify the lexer (tokenization), parser (AST generation), and code generator (C output) in the pipeline module.

### Q: Why do some tests show "expected failure but succeeded"?
**A:** These are tests designed to fail (like invalid input handling) that are working correctly by failing as expected.
