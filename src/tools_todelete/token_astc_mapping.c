/**
 * token_astc_mapping.c - Token到ASTC的映射实现
 * 
 * 这个文件实现了词法token到语义ASTC节点的映射
 * 避免了token.h和astc.h之间的重复定义
 */

#include "token.h"
#include "astc.h"
#include <stdbool.h>

// Token到ASTC操作符的映射
ASTNodeType token_to_astc_op(TokenType token) {
    switch (token) {
        // 算术运算符
        case TOKEN_PLUS:        return ASTC_OP_ADD;
        case TOKEN_MINUS:       return ASTC_OP_SUB;
        case TOKEN_STAR:        return ASTC_OP_MUL;
        case TOKEN_SLASH:       return ASTC_OP_DIV;
        case TOKEN_PERCENT:     return ASTC_OP_MOD;
        
        // 比较运算符
        case TOKEN_EQ:          return ASTC_OP_EQ;
        case TOKEN_NE:          return ASTC_OP_NE;
        case TOKEN_LT:          return ASTC_OP_LT;
        case TOKEN_LE:          return ASTC_OP_LE;
        case TOKEN_GT:          return ASTC_OP_GT;
        case TOKEN_GE:          return ASTC_OP_GE;
        
        // 位运算符
        case TOKEN_AMPERSAND:   return ASTC_OP_AND;
        case TOKEN_PIPE:        return ASTC_OP_OR;
        case TOKEN_CARET:       return ASTC_OP_XOR;
        case TOKEN_TILDE:       return ASTC_OP_BITWISE_NOT;
        
        // 逻辑运算符
        case TOKEN_LOGICAL_AND: return ASTC_OP_LOGICAL_AND;
        case TOKEN_LOGICAL_OR:  return ASTC_OP_LOGICAL_OR;
        case TOKEN_BANG:        return ASTC_OP_NOT;
        
        // 赋值运算符
        case TOKEN_ASSIGN:      return ASTC_OP_ASSIGN;
        
        default:
            return ASTC_OP_UNKNOWN;
    }
}

// 检查token是否为操作符
bool is_operator_token(TokenType token) {
    switch (token) {
        case TOKEN_PLUS:
        case TOKEN_MINUS:
        case TOKEN_STAR:
        case TOKEN_SLASH:
        case TOKEN_PERCENT:
        case TOKEN_AMPERSAND:
        case TOKEN_PIPE:
        case TOKEN_CARET:
        case TOKEN_TILDE:
        case TOKEN_BANG:
        case TOKEN_ASSIGN:
        case TOKEN_EQ:
        case TOKEN_NE:
        case TOKEN_LT:
        case TOKEN_LE:
        case TOKEN_GT:
        case TOKEN_GE:
        case TOKEN_LOGICAL_AND:
        case TOKEN_LOGICAL_OR:
        case TOKEN_INC:
        case TOKEN_DEC:
        case TOKEN_ADD_ASSIGN:
        case TOKEN_SUB_ASSIGN:
        case TOKEN_MUL_ASSIGN:
        case TOKEN_DIV_ASSIGN:
        case TOKEN_MOD_ASSIGN:
        case TOKEN_AND_ASSIGN:
        case TOKEN_OR_ASSIGN:
        case TOKEN_XOR_ASSIGN:
            return true;
        default:
            return false;
    }
}

// 获取操作符优先级
int get_operator_precedence(TokenType token) {
    switch (token) {
        // 最高优先级：后缀运算符
        case TOKEN_INC:
        case TOKEN_DEC:
            return 15;
            
        // 一元运算符
        case TOKEN_BANG:
        case TOKEN_TILDE:
            return 14;
            
        // 乘法、除法、取模
        case TOKEN_STAR:
        case TOKEN_SLASH:
        case TOKEN_PERCENT:
            return 13;
            
        // 加法、减法
        case TOKEN_PLUS:
        case TOKEN_MINUS:
            return 12;
            
        // 位移运算符
        case TOKEN_SHL:
        case TOKEN_SHR:
            return 11;
            
        // 关系运算符
        case TOKEN_LT:
        case TOKEN_LE:
        case TOKEN_GT:
        case TOKEN_GE:
            return 10;
            
        // 相等运算符
        case TOKEN_EQ:
        case TOKEN_NE:
            return 9;
            
        // 位与
        case TOKEN_AMPERSAND:
            return 8;
            
        // 位异或
        case TOKEN_CARET:
            return 7;
            
        // 位或
        case TOKEN_PIPE:
            return 6;
            
        // 逻辑与
        case TOKEN_LOGICAL_AND:
            return 5;
            
        // 逻辑或
        case TOKEN_LOGICAL_OR:
            return 4;
            
        // 赋值运算符
        case TOKEN_ASSIGN:
        case TOKEN_ADD_ASSIGN:
        case TOKEN_SUB_ASSIGN:
        case TOKEN_MUL_ASSIGN:
        case TOKEN_DIV_ASSIGN:
        case TOKEN_MOD_ASSIGN:
        case TOKEN_AND_ASSIGN:
        case TOKEN_OR_ASSIGN:
        case TOKEN_XOR_ASSIGN:
            return 2;
            
        default:
            return 0;  // 不是运算符
    }
}

// 检查token是否为一元运算符
bool is_unary_operator(TokenType token) {
    switch (token) {
        case TOKEN_PLUS:    // 正号
        case TOKEN_MINUS:   // 负号
        case TOKEN_BANG:    // 逻辑非
        case TOKEN_TILDE:   // 按位取反
        case TOKEN_STAR:    // 解引用
        case TOKEN_AMPERSAND: // 取地址
        case TOKEN_INC:     // 前缀递增
        case TOKEN_DEC:     // 前缀递减
            return true;
        default:
            return false;
    }
}

// 检查token是否为二元运算符
bool is_binary_operator(TokenType token) {
    return is_operator_token(token) && !is_unary_operator(token);
}

// 检查运算符是否为右结合
bool is_right_associative(TokenType token) {
    switch (token) {
        case TOKEN_ASSIGN:
        case TOKEN_ADD_ASSIGN:
        case TOKEN_SUB_ASSIGN:
        case TOKEN_MUL_ASSIGN:
        case TOKEN_DIV_ASSIGN:
        case TOKEN_MOD_ASSIGN:
        case TOKEN_AND_ASSIGN:
        case TOKEN_OR_ASSIGN:
        case TOKEN_XOR_ASSIGN:
            return true;  // 赋值运算符是右结合的
        default:
            return false; // 其他运算符都是左结合的
    }
}
