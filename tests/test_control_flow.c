int main() {
    int i = 0;
    
    // Test if statement
    if (i == 0) {
        i = 1;
    } else {
        i = 2;
    }
    
    // Test while loop
    while (i < 5) {
        i = i + 1;
    }
    
    // Test for loop
    for (int j = 0; j < 3; j = j + 1) {
        if (j == 1) {
            continue;
        }
        if (j == 2) {
            break;
        }
    }
    
    return 0;
}
