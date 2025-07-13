#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ITEMS 1000
#define MAX_NAME_LEN 64

typedef struct {
    int id;
    char name[MAX_NAME_LEN];
    float price;
    int quantity;
} Item;

typedef struct {
    Item items[MAX_ITEMS];
    int count;
} Inventory;

void init_inventory(Inventory* inv) {
    inv->count = 0;
    memset(inv->items, 0, sizeof(inv->items));
}

int add_item(Inventory* inv, int id, const char* name, float price, int quantity) {
    if (inv->count >= MAX_ITEMS) {
        return -1;
    }
    
    Item* item = &inv->items[inv->count];
    item->id = id;
    strncpy(item->name, name, MAX_NAME_LEN - 1);
    item->name[MAX_NAME_LEN - 1] = '\0';
    item->price = price;
    item->quantity = quantity;
    
    inv->count++;
    return 0;
}

Item* find_item(Inventory* inv, int id) {
    for (int i = 0; i < inv->count; i++) {
        if (inv->items[i].id == id) {
            return &inv->items[i];
        }
    }
    return NULL;
}

float calculate_total_value(Inventory* inv) {
    float total = 0.0;
    for (int i = 0; i < inv->count; i++) {
        total += inv->items[i].price * inv->items[i].quantity;
    }
    return total;
}

void print_inventory(Inventory* inv) {
    printf("Inventory (%d items):\n", inv->count);
    for (int i = 0; i < inv->count; i++) {
        Item* item = &inv->items[i];
        printf("  %d: %s - $%.2f x %d = $%.2f\n",
               item->id, item->name, item->price, item->quantity,
               item->price * item->quantity);
    }
    printf("Total value: $%.2f\n", calculate_total_value(inv));
}

int main() {
    Inventory inventory;
    init_inventory(&inventory);
    
    // 添加一些测试数据
    for (int i = 0; i < 50; i++) {
        char name[MAX_NAME_LEN];
        snprintf(name, sizeof(name), "Item_%d", i);
        add_item(&inventory, i, name, 10.0 + i * 0.5, i + 1);
    }
    
    print_inventory(&inventory);
    
    // 查找测试
    for (int i = 0; i < 10; i++) {
        Item* item = find_item(&inventory, i * 5);
        if (item) {
            printf("Found item %d: %s\n", item->id, item->name);
        }
    }
    
    return 0;
}
