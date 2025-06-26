/**
 * debug_node_types.c - 调试AST节点类型
 */

#include <stdio.h>
#include "../astc.h"

int main() {
    printf("AST Node Type Values:\n");
    printf("ASTC_TRANSLATION_UNIT = %d\n", ASTC_TRANSLATION_UNIT);
    printf("ASTC_FUNC_DECL = %d\n", ASTC_FUNC_DECL);
    printf("ASTC_VAR_DECL = %d\n", ASTC_VAR_DECL);
    printf("ASTC_PARAM_DECL = %d\n", ASTC_PARAM_DECL);
    printf("ASTC_TYPE_SPECIFIER = %d\n", ASTC_TYPE_SPECIFIER);
    printf("ASTC_COMPOUND_STMT = %d\n", ASTC_COMPOUND_STMT);
    printf("ASTC_IF_STMT = %d\n", ASTC_IF_STMT);
    printf("ASTC_WHILE_STMT = %d\n", ASTC_WHILE_STMT);
    printf("ASTC_FOR_STMT = %d\n", ASTC_FOR_STMT);
    printf("ASTC_RETURN_STMT = %d\n", ASTC_RETURN_STMT);
    printf("ASTC_BREAK_STMT = %d\n", ASTC_BREAK_STMT);
    printf("ASTC_CONTINUE_STMT = %d\n", ASTC_CONTINUE_STMT);
    printf("ASTC_EXPR_STMT = %d\n", ASTC_EXPR_STMT);
    printf("ASTC_EXPR_IDENTIFIER = %d\n", ASTC_EXPR_IDENTIFIER);
    printf("ASTC_EXPR_CONSTANT = %d\n", ASTC_EXPR_CONSTANT);
    printf("ASTC_EXPR_STRING_LITERAL = %d\n", ASTC_EXPR_STRING_LITERAL);
    printf("ASTC_UNARY_OP = %d\n", ASTC_UNARY_OP);
    printf("ASTC_BINARY_OP = %d\n", ASTC_BINARY_OP);
    printf("ASTC_CALL_EXPR = %d\n", ASTC_CALL_EXPR);
    printf("ASTC_OP_UNKNOWN = %d\n", ASTC_OP_UNKNOWN);
    printf("ASTC_OP_ADD = %d\n", ASTC_OP_ADD);
    printf("ASTC_OP_SUB = %d\n", ASTC_OP_SUB);
    printf("ASTC_OP_MUL = %d\n", ASTC_OP_MUL);
    printf("ASTC_OP_DIV = %d\n", ASTC_OP_DIV);
    printf("ASTC_OP_MOD = %d\n", ASTC_OP_MOD);
    printf("ASTC_OP_EQ = %d\n", ASTC_OP_EQ);
    printf("ASTC_OP_NE = %d\n", ASTC_OP_NE);
    printf("ASTC_OP_LT = %d\n", ASTC_OP_LT);
    printf("ASTC_OP_LE = %d\n", ASTC_OP_LE);
    printf("ASTC_OP_GT = %d\n", ASTC_OP_GT);
    printf("ASTC_OP_GE = %d\n", ASTC_OP_GE);
    printf("ASTC_OP_AND = %d\n", ASTC_OP_AND);
    printf("ASTC_OP_OR = %d\n", ASTC_OP_OR);
    printf("ASTC_OP_XOR = %d\n", ASTC_OP_XOR);
    printf("ASTC_OP_NOT = %d\n", ASTC_OP_NOT);
    printf("ASTC_OP_BITWISE_NOT = %d\n", ASTC_OP_BITWISE_NOT);
    printf("ASTC_OP_LOGICAL_AND = %d\n", ASTC_OP_LOGICAL_AND);
    printf("ASTC_OP_LOGICAL_OR = %d\n", ASTC_OP_LOGICAL_OR);
    printf("ASTC_OP_ASSIGN = %d\n", ASTC_OP_ASSIGN);
    printf("ASTC_OP_NEG = %d\n", ASTC_OP_NEG);
    printf("ASTC_OP_POS = %d\n", ASTC_OP_POS);
    
    printf("\nTarget: 64573\n");
    
    return 0;
}
