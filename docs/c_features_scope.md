# C Language Feature Scope for `program_c99`

This document outlines the minimal set of C language features required by the `program_c99` compiler to successfully compile `evolver0_loader.c` and `runtime.c`. This scope is defined to focus development on the essential features for achieving self-compilation of the loader and runtime.

## 1. Preprocessor Directives

- `#include <header.h>`: Support for standard library headers (`stdio.h`, `stdlib.h`, `string.h`, `stdint.h`, `stdbool.h`, `stddef.h`).
- `#define NAME value`: Macro definition for constants.
- `#define NAME(args) expansion`: Macro definition for simple functions (though not heavily used, good to have).
- `#ifdef`, `#ifndef`, `#else`, `#endif`: Conditional compilation.

## 2. Data Types and Declarations

### Basic Types
- `void`
- `char`
- `int`
- `float`
- `double`

### Type Qualifiers
- `const`
- `signed`, `unsigned`
- `short`, `long`

### Standard Library Types
- `size_t` (`stddef.h`)
- `FILE` (`stdio.h`)
- `va_list` (`stdarg.h`)
- Integer types from `stdint.h`: `uint8_t`, `uint32_t`, `int32_t`, `int64_t`, `intptr_t`.
- `bool` (`stdbool.h`)

### Aggregate Types
- `struct`: Structure definition and usage.
- `union`: Union definition and usage.
- `enum`: Enumeration definition and usage.

### Derived Types
- Pointers: To all basic, aggregate, and function types. Includes `void*`.
- Arrays: Of basic types and pointers.
- `typedef`: Creating aliases for types.

### Keywords
- `typedef`, `struct`, `enum`, `union`, `extern`, `static`, `const`.

## 3. Statements

- **Expression Statements**: `a = b;`, `func();`
- **Selection Statements**: `if-else`, `switch-case-default`.
- **Iteration Statements**: `for`, `while`.
- **Jump Statements**: `return`, `break`.
- **Compound Statements**: `{ ... }`

## 4. Expressions and Operators

### Arithmetic Operators
- `+`, `-`, `*`, `/`, `%`

### Relational and Equality Operators
- `==`, `!=`, `>`, `<`, `>=`, `<=`

### Logical Operators
- `&&`, `||`, `!`

### Assignment Operators
- `=`

### Member Access Operators
- `.` (for structs/unions)
- `->` (for pointers to structs/unions)

### Unary Operators
- `&` (address-of)
- `*` (dereference)
- `-` (negation)
- `sizeof`

### Other
- Function calls: `()`
- Type casting: `(type)expr`

## 5. Functions

- **Function Definition**: With return types and parameters.
- **Function Declaration**: Prototypes.
- **Function Pointers**: Declaration and usage.
- **Variadic Functions**: Handling of `...` and `va_list` for functions like `printf`.

## 6. Required Standard Library Functions

The compiler does not need to implement these, but it must correctly handle the calling conventions for them.

### from `stdio.h`
- `printf`, `fprintf`, `vsnprintf`
- `fopen`, `fclose`, `fread`, `fwrite`, `fseek`, `ftell`, `fflush`

### from `stdlib.h`
- `malloc`, `free`, `realloc`

### from `string.h`
- `strcmp`, `memcpy`, `memcmp`, `memset`, `strdup`

## 7. Platform-Specific Code

The loader contains platform-specific code for Windows (`windows.h`, `VirtualAlloc`, `VirtualFree`). The backend will initially target one platform (e.g., Windows x86-64), and these calls will need to be translated to the appropriate system calls or library functions for that target.