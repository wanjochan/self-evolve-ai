#include <stdio.h>
#include "../src/runtime/core_astc.h"

int main() {
    printf("ASTC_TRANSLATION_UNIT = %d (0x%X)\n", ASTC_TRANSLATION_UNIT, ASTC_TRANSLATION_UNIT);
    printf("ASTC_FUNC_DECL = %d (0x%X)\n", ASTC_FUNC_DECL, ASTC_FUNC_DECL);
    printf("ASTC_VAR_DECL = %d (0x%X)\n", ASTC_VAR_DECL, ASTC_VAR_DECL);
    printf("ASTC_COMPOUND_STMT = %d (0x%X)\n", ASTC_COMPOUND_STMT, ASTC_COMPOUND_STMT);
    printf("ASTC_IF_STMT = %d (0x%X)\n", ASTC_IF_STMT, ASTC_IF_STMT);
    printf("ASTC_WHILE_STMT = %d (0x%X)\n", ASTC_WHILE_STMT, ASTC_WHILE_STMT);
    printf("ASTC_BINARY_OP = %d (0x%X)\n", ASTC_BINARY_OP, ASTC_BINARY_OP);
    printf("ASTC_EXPR_IDENTIFIER = %d (0x%X)\n", ASTC_EXPR_IDENTIFIER, ASTC_EXPR_IDENTIFIER);
    printf("ASTC_EXPR_CONSTANT = %d (0x%X)\n", ASTC_EXPR_CONSTANT, ASTC_EXPR_CONSTANT);

    // Check more AST node types
    printf("ASTC_SWITCH_STMT = %d (0x%X)\n", ASTC_SWITCH_STMT, ASTC_SWITCH_STMT);
    printf("ASTC_CASE_STMT = %d (0x%X)\n", ASTC_CASE_STMT, ASTC_CASE_STMT);
    printf("ASTC_DEFAULT_STMT = %d (0x%X)\n", ASTC_DEFAULT_STMT, ASTC_DEFAULT_STMT);
    printf("ASTC_BREAK_STMT = %d (0x%X)\n", ASTC_BREAK_STMT, ASTC_BREAK_STMT);
    printf("ASTC_CONTINUE_STMT = %d (0x%X)\n", ASTC_CONTINUE_STMT, ASTC_CONTINUE_STMT);
    printf("ASTC_FOR_STMT = %d (0x%X)\n", ASTC_FOR_STMT, ASTC_FOR_STMT);
    printf("ASTC_EXPR_ARRAY_SUBSCRIPT = %d (0x%X)\n", ASTC_EXPR_ARRAY_SUBSCRIPT, ASTC_EXPR_ARRAY_SUBSCRIPT);
    printf("ASTC_EXPR_MEMBER_ACCESS = %d (0x%X)\n", ASTC_EXPR_MEMBER_ACCESS, ASTC_EXPR_MEMBER_ACCESS);
    printf("ASTC_EXPR_PTR_MEMBER_ACCESS = %d (0x%X)\n", ASTC_EXPR_PTR_MEMBER_ACCESS, ASTC_EXPR_PTR_MEMBER_ACCESS);
    printf("ASTC_EXPR_CAST_EXPR = %d (0x%X)\n", ASTC_EXPR_CAST_EXPR, ASTC_EXPR_CAST_EXPR);

    printf("\nRemaining unknown node types:\n");
    printf("64573 = 0x%X\n", 64573);
    printf("64591 = 0x%X\n", 64591);
    printf("64540 = 0x%X\n", 64540);
    printf("64589 = 0x%X\n", 64589);

    return 0;
}
