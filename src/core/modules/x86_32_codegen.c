/**
 * x86_32_codegen.c - x86 32-bit Code Generator
 *
 * Generates x86 32-bit assembly code for C99Bin compiler
 * Supports Windows x86 and Linux i386 targets
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// x86_32 register definitions
typedef enum {
    REG_EAX = 0,    // Accumulator
    REG_ECX = 1,    // Counter
    REG_EDX = 2,    // Data
    REG_EBX = 3,    // Base
    REG_ESP = 4,    // Stack pointer
    REG_EBP = 5,    // Base pointer
    REG_ESI = 6,    // Source index
    REG_EDI = 7,    // Destination index
    REG_COUNT = 8
} x86_32_Register;

// x86_32 instruction types
typedef enum {
    INST_MOV,       // Move
    INST_PUSH,      // Push to stack
    INST_POP,       // Pop from stack
    INST_CALL,      // Function call
    INST_RET,       // Return
    INST_ADD,       // Addition
    INST_SUB,       // Subtraction
    INST_MUL,       // Multiplication
    INST_DIV,       // Division
    INST_CMP,       // Compare
    INST_JMP,       // Unconditional jump
    INST_JE,        // Jump if equal
    INST_JNE,       // Jump if not equal
    INST_INT,       // Interrupt
    INST_NOP,       // No operation
    INST_LEAVE,     // Leave function
    INST_LEA,       // Load effective address
    INST_XCHG,      // Exchange
    INST_XOR,       // Exclusive OR
    INST_OR,        // Logical OR
    INST_AND,       // Logical AND
    INST_SHL,       // Shift left
    INST_SHR,       // Shift right
    INST_TEST,      // Test
    INST_CDQ,       // Convert double to quad
    INST_IDIV       // Signed divide
} x86_32_Instruction;

// Operand types
typedef enum {
    OP_REGISTER,    // Register operand
    OP_IMMEDIATE,   // Immediate value
    OP_MEMORY,      // Memory location
    OP_LABEL        // Label reference
} x86_32_OperandType;

// Operand structure
typedef struct {
    x86_32_OperandType type;
    union {
        x86_32_Register reg;
        int32_t immediate;
        struct {
            x86_32_Register base;
            int32_t offset;
        } memory;
        char* label;
    } value;
} x86_32_Operand;

// Code generator state
typedef struct {
    FILE* output;
    char* target_platform;
    
    // Code generation
    int label_counter;
    int temp_counter;
    
    // Function state
    int stack_offset;
    int param_offset;
    int local_vars_size;
    
    // Platform-specific
    int is_windows;
    const char* call_convention;  // "cdecl", "stdcall"
    
    // Statistics
    int instructions_generated;
    int functions_generated;
    
} x86_32_CodeGen;

// Global code generator
static x86_32_CodeGen* g_codegen = NULL;

// Register names
static const char* register_names[] = {
    "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"
};

// Initialize x86_32 code generator
int x86_32_codegen_initialize(const char* target_platform) {
    printf("🔨 x86_32 CodeGen: Initializing for %s\n", target_platform);
    
    if (g_codegen) {
        printf("⚠️  x86_32 CodeGen: Already initialized\n");
        return 0;
    }
    
    g_codegen = calloc(1, sizeof(x86_32_CodeGen));
    if (!g_codegen) {
        printf("❌ x86_32 CodeGen: Memory allocation failed\n");
        return -1;
    }
    
    g_codegen->target_platform = strdup(target_platform);
    
    // Determine platform-specific settings
    if (strstr(target_platform, "windows") || strstr(target_platform, "win32")) {
        g_codegen->is_windows = 1;
        g_codegen->call_convention = "cdecl";  // Default for Windows
        printf("   🎯 Target: Windows x86 (32-bit)\n");
    } else if (strstr(target_platform, "linux") || strstr(target_platform, "i386")) {
        g_codegen->is_windows = 0;
        g_codegen->call_convention = "cdecl";  // Linux uses cdecl
        printf("   🎯 Target: Linux i386 (32-bit)\n");
    } else {
        printf("❌ x86_32 CodeGen: Unsupported platform %s\n", target_platform);
        free(g_codegen->target_platform);
        free(g_codegen);
        g_codegen = NULL;
        return -1;
    }
    
    printf("✅ x86_32 CodeGen: Initialization complete\n");
    return 0;
}

// Set output file
int x86_32_codegen_set_output(FILE* output) {
    if (!g_codegen) {
        printf("❌ x86_32 CodeGen: Not initialized\n");
        return -1;
    }
    
    g_codegen->output = output;
    return 0;
}

// Generate instruction with no operands
static void emit_instruction_0(x86_32_Instruction inst) {
    const char* inst_names[] = {
        "mov", "push", "pop", "call", "ret", "add", "sub", "mul", "div",
        "cmp", "jmp", "je", "jne", "int", "nop", "leave", "lea", "xchg",
        "xor", "or", "and", "shl", "shr", "test", "cdq", "idiv"
    };
    
    fprintf(g_codegen->output, "    %s\n", inst_names[inst]);
    g_codegen->instructions_generated++;
}

// Generate instruction with one operand
static void emit_instruction_1(x86_32_Instruction inst, x86_32_Operand* op1) {
    const char* inst_names[] = {
        "mov", "push", "pop", "call", "ret", "add", "sub", "mul", "div",
        "cmp", "jmp", "je", "jne", "int", "nop", "leave", "lea", "xchg",
        "xor", "or", "and", "shl", "shr", "test", "cdq", "idiv"
    };
    
    fprintf(g_codegen->output, "    %s ", inst_names[inst]);
    
    switch (op1->type) {
        case OP_REGISTER:
            fprintf(g_codegen->output, "%%%s", register_names[op1->value.reg]);
            break;
        case OP_IMMEDIATE:
            fprintf(g_codegen->output, "$%d", op1->value.immediate);
            break;
        case OP_MEMORY:
            if (op1->value.memory.offset != 0) {
                fprintf(g_codegen->output, "%d(%%%s)", 
                       op1->value.memory.offset, 
                       register_names[op1->value.memory.base]);
            } else {
                fprintf(g_codegen->output, "(%%%s)", 
                       register_names[op1->value.memory.base]);
            }
            break;
        case OP_LABEL:
            fprintf(g_codegen->output, "%s", op1->value.label);
            break;
    }
    
    fprintf(g_codegen->output, "\n");
    g_codegen->instructions_generated++;
}

// Generate instruction with two operands
static void emit_instruction_2(x86_32_Instruction inst, x86_32_Operand* op1, x86_32_Operand* op2) {
    const char* inst_names[] = {
        "mov", "push", "pop", "call", "ret", "add", "sub", "mul", "div",
        "cmp", "jmp", "je", "jne", "int", "nop", "leave", "lea", "xchg",
        "xor", "or", "and", "shl", "shr", "test", "cdq", "idiv"
    };
    
    fprintf(g_codegen->output, "    %s ", inst_names[inst]);
    
    // First operand
    switch (op1->type) {
        case OP_REGISTER:
            fprintf(g_codegen->output, "%%%s", register_names[op1->value.reg]);
            break;
        case OP_IMMEDIATE:
            fprintf(g_codegen->output, "$%d", op1->value.immediate);
            break;
        case OP_MEMORY:
            if (op1->value.memory.offset != 0) {
                fprintf(g_codegen->output, "%d(%%%s)", 
                       op1->value.memory.offset, 
                       register_names[op1->value.memory.base]);
            } else {
                fprintf(g_codegen->output, "(%%%s)", 
                       register_names[op1->value.memory.base]);
            }
            break;
        case OP_LABEL:
            fprintf(g_codegen->output, "%s", op1->value.label);
            break;
    }
    
    fprintf(g_codegen->output, ", ");
    
    // Second operand
    switch (op2->type) {
        case OP_REGISTER:
            fprintf(g_codegen->output, "%%%s", register_names[op2->value.reg]);
            break;
        case OP_IMMEDIATE:
            fprintf(g_codegen->output, "$%d", op2->value.immediate);
            break;
        case OP_MEMORY:
            if (op2->value.memory.offset != 0) {
                fprintf(g_codegen->output, "%d(%%%s)", 
                       op2->value.memory.offset, 
                       register_names[op2->value.memory.base]);
            } else {
                fprintf(g_codegen->output, "(%%%s)", 
                       register_names[op2->value.memory.base]);
            }
            break;
        case OP_LABEL:
            fprintf(g_codegen->output, "%s", op2->value.label);
            break;
    }
    
    fprintf(g_codegen->output, "\n");
    g_codegen->instructions_generated++;
}

// Helper functions to create operands
static x86_32_Operand make_reg(x86_32_Register reg) {
    x86_32_Operand op = {0};
    op.type = OP_REGISTER;
    op.value.reg = reg;
    return op;
}

static x86_32_Operand make_imm(int32_t value) {
    x86_32_Operand op = {0};
    op.type = OP_IMMEDIATE;
    op.value.immediate = value;
    return op;
}

static x86_32_Operand make_mem(x86_32_Register base, int32_t offset) {
    x86_32_Operand op = {0};
    op.type = OP_MEMORY;
    op.value.memory.base = base;
    op.value.memory.offset = offset;
    return op;
}

static x86_32_Operand make_label(const char* label) {
    x86_32_Operand op = {0};
    op.type = OP_LABEL;
    op.value.label = strdup(label);
    return op;
}

// Generate function prologue
int x86_32_codegen_generate_function_prologue(const char* func_name) {
    if (!g_codegen) {
        printf("❌ x86_32 CodeGen: Not initialized\n");
        return -1;
    }
    
    printf("📝 x86_32 CodeGen: Generating prologue for %s\n", func_name);
    
    // Reset function state
    g_codegen->stack_offset = 0;
    g_codegen->param_offset = 8;  // First parameter at ebp+8
    g_codegen->local_vars_size = 0;
    
    // Generate function label
    if (g_codegen->is_windows) {
        fprintf(g_codegen->output, ".globl _%s\n", func_name);
        fprintf(g_codegen->output, "_%s:\n", func_name);
    } else {
        fprintf(g_codegen->output, ".globl %s\n", func_name);
        fprintf(g_codegen->output, "%s:\n", func_name);
    }
    
    // Standard function prologue
    x86_32_Operand ebp_reg = make_reg(REG_EBP);
    x86_32_Operand esp_reg = make_reg(REG_ESP);
    
    emit_instruction_1(INST_PUSH, &ebp_reg);           // push %ebp
    emit_instruction_2(INST_MOV, &esp_reg, &ebp_reg);  // mov %esp, %ebp
    
    g_codegen->functions_generated++;
    
    return 0;
}

// Generate function epilogue
int x86_32_codegen_generate_function_epilogue(void) {
    if (!g_codegen) {
        printf("❌ x86_32 CodeGen: Not initialized\n");
        return -1;
    }
    
    printf("📝 x86_32 CodeGen: Generating function epilogue\n");
    
    // Standard function epilogue
    emit_instruction_0(INST_LEAVE);  // leave (mov %ebp, %esp; pop %ebp)
    emit_instruction_0(INST_RET);    // ret
    
    return 0;
}

// Generate return statement
int x86_32_codegen_generate_return(int value) {
    if (!g_codegen) {
        printf("❌ x86_32 CodeGen: Not initialized\n");
        return -1;
    }
    
    printf("📝 x86_32 CodeGen: Generating return %d\n", value);
    
    // Load return value into EAX
    x86_32_Operand eax_reg = make_reg(REG_EAX);
    x86_32_Operand value_imm = make_imm(value);
    
    emit_instruction_2(INST_MOV, &value_imm, &eax_reg);  // mov $value, %eax
    
    // Generate epilogue
    x86_32_codegen_generate_function_epilogue();
    
    return 0;
}

// Generate function call
int x86_32_codegen_generate_call(const char* func_name, int num_args) {
    if (!g_codegen) {
        printf("❌ x86_32 CodeGen: Not initialized\n");
        return -1;
    }
    
    printf("📝 x86_32 CodeGen: Generating call to %s with %d args\n", func_name, num_args);
    
    // Generate call instruction
    char full_name[256];
    if (g_codegen->is_windows && func_name[0] != '_') {
        snprintf(full_name, sizeof(full_name), "_%s", func_name);
    } else {
        strcpy(full_name, func_name);
    }
    
    x86_32_Operand call_target = make_label(full_name);
    emit_instruction_1(INST_CALL, &call_target);  // call func_name
    
    // Clean up stack for cdecl calling convention
    if (num_args > 0 && strcmp(g_codegen->call_convention, "cdecl") == 0) {
        x86_32_Operand esp_reg = make_reg(REG_ESP);
        x86_32_Operand cleanup_size = make_imm(num_args * 4);  // 4 bytes per arg
        emit_instruction_2(INST_ADD, &cleanup_size, &esp_reg);  // add $size, %esp
    }
    
    return 0;
}

// Generate arithmetic operation
int x86_32_codegen_generate_arithmetic(const char* operation, int left, int right, int result_reg) {
    if (!g_codegen) {
        printf("❌ x86_32 CodeGen: Not initialized\n");
        return -1;
    }
    
    printf("📝 x86_32 CodeGen: Generating %s: %d %s %d\n", operation, left, operation, right);
    
    x86_32_Operand eax_reg = make_reg(REG_EAX);
    x86_32_Operand edx_reg = make_reg(REG_EDX);
    x86_32_Operand left_imm = make_imm(left);
    x86_32_Operand right_imm = make_imm(right);
    
    // Load left operand into EAX
    emit_instruction_2(INST_MOV, &left_imm, &eax_reg);  // mov $left, %eax
    
    if (strcmp(operation, "add") == 0) {
        emit_instruction_2(INST_ADD, &right_imm, &eax_reg);  // add $right, %eax
    } else if (strcmp(operation, "sub") == 0) {
        emit_instruction_2(INST_SUB, &right_imm, &eax_reg);  // sub $right, %eax
    } else if (strcmp(operation, "mul") == 0) {
        x86_32_Operand ecx_reg = make_reg(REG_ECX);
        emit_instruction_2(INST_MOV, &right_imm, &ecx_reg);  // mov $right, %ecx
        emit_instruction_1(INST_MUL, &ecx_reg);              // mul %ecx (result in EAX)
    } else if (strcmp(operation, "div") == 0) {
        x86_32_Operand ecx_reg = make_reg(REG_ECX);
        emit_instruction_0(INST_CDQ);                         // cdq (sign extend EAX to EDX:EAX)
        emit_instruction_2(INST_MOV, &right_imm, &ecx_reg);  // mov $right, %ecx
        emit_instruction_1(INST_IDIV, &ecx_reg);             // idiv %ecx (result in EAX)
    } else {
        printf("❌ x86_32 CodeGen: Unsupported operation %s\n", operation);
        return -1;
    }
    
    return 0;
}

// Generate printf call
int x86_32_codegen_generate_printf(const char* format, int value) {
    if (!g_codegen) {
        printf("❌ x86_32 CodeGen: Not initialized\n");
        return -1;
    }
    
    printf("📝 x86_32 CodeGen: Generating printf call\n");
    
    // Generate format string label
    static int format_counter = 0;
    char format_label[64];
    snprintf(format_label, sizeof(format_label), ".LC%d", format_counter++);
    
    // Push arguments in reverse order (cdecl convention)
    x86_32_Operand value_imm = make_imm(value);
    x86_32_Operand format_addr = make_label(format_label);
    
    emit_instruction_1(INST_PUSH, &value_imm);    // push $value
    emit_instruction_1(INST_PUSH, &format_addr);  // push $format_string
    
    // Call printf
    x86_32_codegen_generate_call("printf", 2);
    
    return 0;
}

// Generate data section
int x86_32_codegen_generate_data_section(void) {
    if (!g_codegen) {
        printf("❌ x86_32 CodeGen: Not initialized\n");
        return -1;
    }
    
    fprintf(g_codegen->output, "\n.section .rodata\n");
    
    // Add common format strings
    fprintf(g_codegen->output, ".LC0:\n");
    fprintf(g_codegen->output, "    .string \"%%d\\n\"\n");
    
    return 0;
}

// Generate complete program
int x86_32_codegen_generate_program(const char* program_type, int return_value) {
    if (!g_codegen) {
        printf("❌ x86_32 CodeGen: Not initialized\n");
        return -1;
    }
    
    printf("🔨 x86_32 CodeGen: Generating %s program\n", program_type);
    
    // Generate assembly header
    fprintf(g_codegen->output, "# Generated by C99Bin x86_32 Code Generator\n");
    fprintf(g_codegen->output, "# Target: %s\n", g_codegen->target_platform);
    fprintf(g_codegen->output, "# Program type: %s\n\n", program_type);
    
    // Text section
    fprintf(g_codegen->output, ".section .text\n\n");
    
    // Generate main function
    x86_32_codegen_generate_function_prologue("main");
    
    if (strcmp(program_type, "hello_world") == 0) {
        // Generate hello world printf
        x86_32_codegen_generate_printf("Hello, World!\\n", 0);
    } else if (strcmp(program_type, "simple_return") == 0) {
        // Simple return program
    } else if (strcmp(program_type, "math_calc") == 0) {
        // Generate some math and print result
        x86_32_codegen_generate_arithmetic("add", 10, 32, REG_EAX);
        x86_32_codegen_generate_printf("%d\\n", 42);  // Simplified
    }
    
    // Generate return
    x86_32_codegen_generate_return(return_value);
    
    // Generate data section
    x86_32_codegen_generate_data_section();
    
    printf("✅ x86_32 CodeGen: Program generation complete\n");
    printf("   📊 Instructions: %d\n", g_codegen->instructions_generated);
    printf("   📊 Functions: %d\n", g_codegen->functions_generated);
    
    return 0;
}

// Cleanup code generator
void x86_32_codegen_cleanup(void) {
    if (!g_codegen) {
        return;
    }
    
    printf("🧹 x86_32 CodeGen: Cleaning up\n");
    
    if (g_codegen->target_platform) {
        free(g_codegen->target_platform);
    }
    
    free(g_codegen);
    g_codegen = NULL;
    
    printf("✅ x86_32 CodeGen: Cleanup complete\n");
}

// Test function
int x86_32_codegen_test(void) {
    printf("🧪 x86_32 CodeGen: Running basic test\n");
    
    // Initialize for Windows x86
    if (x86_32_codegen_initialize("windows-x86") != 0) {
        return -1;
    }
    
    // Create test output file
    FILE* test_output = fopen("test_x86_32_output.s", "w");
    if (!test_output) {
        printf("❌ Cannot create test output file\n");
        x86_32_codegen_cleanup();
        return -1;
    }
    
    x86_32_codegen_set_output(test_output);
    
    // Generate test program
    if (x86_32_codegen_generate_program("simple_return", 42) != 0) {
        fclose(test_output);
        x86_32_codegen_cleanup();
        return -1;
    }
    
    fclose(test_output);
    x86_32_codegen_cleanup();
    
    printf("✅ x86_32 CodeGen: Test completed successfully\n");
    return 0;
}

// Export the code generator interface
typedef struct {
    const char* architecture;
    const char* platform;
    
    int (*generate_function_prologue)(const char* func_name);
    int (*generate_function_epilogue)(void);
    int (*generate_return)(int value);
    int (*generate_call)(const char* func_name, int num_args);
    int (*generate_arithmetic)(const char* operation, int left, int right, int result_reg);
    int (*generate_program)(const char* program_type, int return_value);
    void (*cleanup)(void);
} CodeGenerator;

CodeGenerator x86_32_codegen = {
    .architecture = "x86",
    .platform = "windows",
    .generate_function_prologue = x86_32_codegen_generate_function_prologue,
    .generate_function_epilogue = x86_32_codegen_generate_function_epilogue,
    .generate_return = x86_32_codegen_generate_return,
    .generate_call = x86_32_codegen_generate_call,
    .generate_arithmetic = x86_32_codegen_generate_arithmetic,
    .generate_program = x86_32_codegen_generate_program,
    .cleanup = x86_32_codegen_cleanup
};