/**
 * c99_complex_syntax_test.c - Complex C99 Syntax Structures Test
 * 
 * This program tests complex C99 syntax structures including:
 * - Complex declarations and definitions
 * - Nested structures and unions
 * - Function pointers and callbacks
 * - Advanced control flow
 * - Preprocessor macros
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Forward declarations
struct Node;
typedef struct Node Node;

// Complex type definitions
typedef int (*CompareFunc)(const void *a, const void *b);
typedef void (*ProcessFunc)(Node *node, void *data);

// Complex structures
struct Node {
    int data;
    struct Node *next;
    struct Node *prev;
    union {
        int int_value;
        float float_value;
        char *string_value;
    } extra_data;
};

typedef struct {
    Node *head;
    Node *tail;
    size_t count;
    CompareFunc compare;
    ProcessFunc process;
} LinkedList;

// Nested structures
typedef struct {
    struct {
        int x, y;
    } position;
    struct {
        int width, height;
    } size;
    struct {
        unsigned char r, g, b, a;
    } color;
} Rectangle;

// Complex enums
typedef enum {
    STATE_INIT = 0x01,
    STATE_RUNNING = 0x02,
    STATE_PAUSED = 0x04,
    STATE_STOPPED = 0x08,
    STATE_ERROR = 0x10,
    STATE_ALL = STATE_INIT | STATE_RUNNING | STATE_PAUSED | STATE_STOPPED | STATE_ERROR
} SystemState;

// Function prototypes
void test_complex_declarations(void);
void test_nested_structures(void);
void test_function_pointers(void);
void test_advanced_control_flow(void);
void test_preprocessor_macros(void);
void test_variable_length_arrays(void);

// Utility functions
Node* create_node(int data);
void destroy_node(Node *node);
LinkedList* create_list(CompareFunc compare);
void destroy_list(LinkedList *list);
void list_add(LinkedList *list, int data);
void list_process_all(LinkedList *list, ProcessFunc func, void *user_data);

// Comparison functions
int int_compare(const void *a, const void *b);
void print_node(Node *node, void *data);

int main(void) {
    printf("=== Complex C99 Syntax Structures Test ===\n\n");
    
    test_complex_declarations();
    test_nested_structures();
    test_function_pointers();
    test_advanced_control_flow();
    test_preprocessor_macros();
    test_variable_length_arrays();
    
    printf("\n=== All complex syntax tests completed ===\n");
    return 0;
}

void test_complex_declarations(void) {
    printf("1. Testing Complex Declarations:\n");
    
    // Complex array declarations
    int matrix[3][4] = {
        {1, 2, 3, 4},
        {5, 6, 7, 8},
        {9, 10, 11, 12}
    };
    
    // Pointer to array
    int (*ptr_to_array)[4] = matrix;
    
    // Array of pointers
    int *array_of_ptrs[3];
    for (int i = 0; i < 3; i++) {
        array_of_ptrs[i] = matrix[i];
    }
    
    // Function pointer array
    CompareFunc comparers[] = {int_compare, int_compare, int_compare};
    
    printf("   Matrix[1][2] = %d\n", matrix[1][2]);
    printf("   Via pointer to array: %d\n", ptr_to_array[1][2]);
    printf("   Via array of pointers: %d\n", array_of_ptrs[1][2]);
    printf("   Function pointer array size: %zu\n", sizeof(comparers) / sizeof(comparers[0]));
}

void test_nested_structures(void) {
    printf("\n2. Testing Nested Structures:\n");
    
    Rectangle rect = {
        .position = {10, 20},
        .size = {100, 50},
        .color = {255, 128, 64, 255}
    };
    
    printf("   Rectangle: pos(%d,%d) size(%dx%d) color(r:%d,g:%d,b:%d,a:%d)\n",
           rect.position.x, rect.position.y,
           rect.size.width, rect.size.height,
           rect.color.r, rect.color.g, rect.color.b, rect.color.a);
    
    // Test union within structure
    Node node = {0};
    node.data = 42;
    node.extra_data.int_value = 100;
    printf("   Node data: %d, extra int: %d\n", node.data, node.extra_data.int_value);
    
    node.extra_data.float_value = 3.14f;
    printf("   Node data: %d, extra float: %.2f\n", node.data, node.extra_data.float_value);
}

void test_function_pointers(void) {
    printf("\n3. Testing Function Pointers:\n");
    
    LinkedList *list = create_list(int_compare);
    if (!list) {
        printf("   Failed to create list\n");
        return;
    }
    
    // Add some data
    list_add(list, 30);
    list_add(list, 10);
    list_add(list, 20);
    
    printf("   List created with %zu elements\n", list->count);
    
    // Process all nodes using function pointer
    printf("   Processing all nodes:\n");
    list_process_all(list, print_node, "   ");
    
    destroy_list(list);
}

void test_advanced_control_flow(void) {
    printf("\n4. Testing Advanced Control Flow:\n");
    
    SystemState state = STATE_INIT;
    
    // Switch with complex expressions
    switch (state) {
        case STATE_INIT:
            printf("   System initializing...\n");
            state = STATE_RUNNING;
            break;
        case STATE_RUNNING:
            printf("   System running\n");
            break;
        case STATE_PAUSED:
            printf("   System paused\n");
            break;
        default:
            printf("   Unknown state: 0x%02X\n", state);
            break;
    }
    
    // Complex loop with multiple conditions
    for (int i = 0, j = 10; i < 5 && j > 5; i++, j--) {
        if (i == 2) continue;
        if (j == 7) break;
        printf("   Loop: i=%d, j=%d\n", i, j);
    }
    
    // Nested loops with labels (if supported)
    int found = 0;
    for (int i = 0; i < 3 && !found; i++) {
        for (int j = 0; j < 3; j++) {
            if (i == 1 && j == 1) {
                found = 1;
                printf("   Found target at (%d,%d)\n", i, j);
                break;
            }
        }
    }
}

void test_preprocessor_macros(void) {
    printf("\n5. Testing Preprocessor Macros:\n");
    
#define STRINGIFY(x) #x
#define CONCAT(a, b) a ## b
#define MAX3(a, b, c) ((a) > (b) ? ((a) > (c) ? (a) : (c)) : ((b) > (c) ? (b) : (c)))
#define SWAP(type, a, b) do { type temp = (a); (a) = (b); (b) = temp; } while(0)
    
    int x = 10, y = 20, z = 15;
    printf("   MAX3(%d, %d, %d) = %d\n", x, y, z, MAX3(x, y, z));
    
    printf("   Before swap: x=%d, y=%d\n", x, y);
    SWAP(int, x, y);
    printf("   After swap: x=%d, y=%d\n", x, y);
    
    printf("   Stringify test: %s\n", STRINGIFY(Hello World));
    
    int CONCAT(var, 123) = 456;
    printf("   Concatenation test: var123 = %d\n", var123);
    
#undef STRINGIFY
#undef CONCAT
#undef MAX3
#undef SWAP
}

void test_variable_length_arrays(void) {
    printf("\n6. Testing Variable Length Arrays (VLA):\n");
    
    int n = 5;
    int vla[n];  // Variable length array
    
    // Initialize VLA
    for (int i = 0; i < n; i++) {
        vla[i] = i * i;
    }
    
    printf("   VLA contents: ");
    for (int i = 0; i < n; i++) {
        printf("%d ", vla[i]);
    }
    printf("\n");
    
    // 2D VLA
    int rows = 3, cols = 4;
    int matrix[rows][cols];
    
    // Initialize 2D VLA
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            matrix[i][j] = i * cols + j;
        }
    }
    
    printf("   2D VLA (%dx%d):\n", rows, cols);
    for (int i = 0; i < rows; i++) {
        printf("     ");
        for (int j = 0; j < cols; j++) {
            printf("%2d ", matrix[i][j]);
        }
        printf("\n");
    }
}

// Utility function implementations
Node* create_node(int data) {
    Node *node = malloc(sizeof(Node));
    if (node) {
        node->data = data;
        node->next = NULL;
        node->prev = NULL;
        node->extra_data.int_value = 0;
    }
    return node;
}

void destroy_node(Node *node) {
    if (node) {
        free(node);
    }
}

LinkedList* create_list(CompareFunc compare) {
    LinkedList *list = malloc(sizeof(LinkedList));
    if (list) {
        list->head = NULL;
        list->tail = NULL;
        list->count = 0;
        list->compare = compare;
        list->process = NULL;
    }
    return list;
}

void destroy_list(LinkedList *list) {
    if (!list) return;
    
    Node *current = list->head;
    while (current) {
        Node *next = current->next;
        destroy_node(current);
        current = next;
    }
    
    free(list);
}

void list_add(LinkedList *list, int data) {
    if (!list) return;
    
    Node *node = create_node(data);
    if (!node) return;
    
    if (!list->head) {
        list->head = list->tail = node;
    } else {
        list->tail->next = node;
        node->prev = list->tail;
        list->tail = node;
    }
    
    list->count++;
}

void list_process_all(LinkedList *list, ProcessFunc func, void *user_data) {
    if (!list || !func) return;
    
    Node *current = list->head;
    while (current) {
        func(current, user_data);
        current = current->next;
    }
}

int int_compare(const void *a, const void *b) {
    int ia = *(const int*)a;
    int ib = *(const int*)b;
    return (ia > ib) - (ia < ib);
}

void print_node(Node *node, void *data) {
    const char *prefix = (const char*)data;
    if (node && prefix) {
        printf("%sNode: %d\n", prefix, node->data);
    }
}
