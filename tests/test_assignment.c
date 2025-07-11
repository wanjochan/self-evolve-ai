// Test assignment operators in C99 compiler
#include <stdio.h>

int main() {
    int a = 10;
    int b = 5;
    
    printf("Initial values: a = %d, b = %d\n", a, b);
    
    // Test basic assignment
    a = 20;
    printf("After a = 20: a = %d\n", a);
    
    // Test compound assignment operators
    a += b;  // a = a + b
    printf("After a += b: a = %d\n", a);
    
    a -= b;  // a = a - b  
    printf("After a -= b: a = %d\n", a);
    
    a *= b;  // a = a * b
    printf("After a *= b: a = %d\n", a);
    
    a /= b;  // a = a / b
    printf("After a /= b: a = %d\n", a);
    
    a %= b;  // a = a % b
    printf("After a %%= b: a = %d\n", a);
    
    // Test bitwise assignment operators
    a = 15;  // Reset a
    printf("Reset a = %d\n", a);
    
    a &= 7;  // a = a & 7
    printf("After a &= 7: a = %d\n", a);
    
    a |= 8;  // a = a | 8
    printf("After a |= 8: a = %d\n", a);
    
    a ^= 3;  // a = a ^ 3
    printf("After a ^= 3: a = %d\n", a);
    
    a <<= 1; // a = a << 1
    printf("After a <<= 1: a = %d\n", a);
    
    a >>= 2; // a = a >> 2
    printf("After a >>= 2: a = %d\n", a);
    
    return 0;
}
