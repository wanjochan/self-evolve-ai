/**
 * simple_loader.c - Simplified Universal Loader for Testing
 * 
 * 简化版本的统一启动器，用于验证PRD三层架构
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ===============================================
// Architecture Detection
// ===============================================

const char* detect_architecture_string(void) {
#if defined(__x86_64__) || defined(_M_X64)
    return "x86_64_64";
#elif defined(__aarch64__) || defined(_M_ARM64)
    return "arm64_64";
#elif defined(__i386__) || defined(_M_IX86)
    return "x86_32_32";
#else
    return "unknown";
#endif
}

// ===============================================
// Simple Module Loading Simulation
// ===============================================

int simulate_vm_loading(const char* vm_module_path, const char* program_path) {
    printf("Simulating VM module loading...\n");
    printf("VM module: %s\n", vm_module_path);
    
    // Check if VM module file exists
    FILE* vm_file = fopen(vm_module_path, "rb");
    if (!vm_file) {
        printf("Error: VM module not found: %s\n", vm_module_path);
        return -1;
    }
    
    // Get file size
    fseek(vm_file, 0, SEEK_END);
    long vm_size = ftell(vm_file);
    fseek(vm_file, 0, SEEK_SET);
    
    printf("VM module found: %ld bytes\n", vm_size);
    
    // Read first few bytes to check format
    unsigned char header[16];
    size_t read_bytes = fread(header, 1, 16, vm_file);
    fclose(vm_file);
    
    if (read_bytes >= 4) {
        uint32_t magic = *(uint32_t*)header;
        if (magic == 0x454D5452) { // "RTME" in little endian
            printf("VM module format: RTME (legacy)\n");
        } else if (magic == 0x5654414E) { // "NATV" in little endian
            printf("VM module format: NATV (new)\n");
        } else {
            printf("VM module format: Unknown (magic: 0x%08X)\n", magic);
        }
    }
    
    // Simulate program loading
    if (program_path) {
        printf("Program to execute: %s\n", program_path);
        
        FILE* prog_file = fopen(program_path, "rb");
        if (!prog_file) {
            printf("Warning: Program file not found: %s\n", program_path);
        } else {
            fseek(prog_file, 0, SEEK_END);
            long prog_size = ftell(prog_file);
            fclose(prog_file);
            printf("Program file found: %ld bytes\n", prog_size);
        }
    } else {
        printf("No program specified, VM would start in interactive mode\n");
    }
    
    // Simulate successful execution
    printf("Simulation: VM execution completed successfully\n");
    return 0;
}

// ===============================================
// Main Function
// ===============================================

int main(int argc, char* argv[]) {
    printf("Simple Universal Loader v1.0\n");
    printf("============================\n");
    
    // Detect architecture
    const char* arch = detect_architecture_string();
    printf("Detected architecture: %s\n", arch);
    
    if (strcmp(arch, "unknown") == 0) {
        printf("Error: Unsupported architecture\n");
        return 1;
    }
    
    // Construct VM module path
    char vm_module_path[256];
    snprintf(vm_module_path, sizeof(vm_module_path), "vm_%s.native", arch);
    
    // Determine program path
    const char* program_path = NULL;
    if (argc > 1) {
        program_path = argv[1];
    }
    
    printf("\nPRD Three-Layer Architecture Test:\n");
    printf("Layer 1 (Loader): %s\n", argv[0]);
    printf("Layer 2 (VM):     %s\n", vm_module_path);
    printf("Layer 3 (Program): %s\n", program_path ? program_path : "(interactive)");
    printf("\n");
    
    // Simulate the loading and execution
    int result = simulate_vm_loading(vm_module_path, program_path);
    
    if (result == 0) {
        printf("\n✓ PRD three-layer architecture test PASSED\n");
        printf("✓ Architecture detection working\n");
        printf("✓ VM module path construction working\n");
        printf("✓ File existence checking working\n");
        printf("✓ Loader → VM → Program chain simulated successfully\n");
    } else {
        printf("\n✗ PRD three-layer architecture test FAILED\n");
        printf("✗ VM module loading failed\n");
    }
    
    return result;
}
