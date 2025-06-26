/**
 * test_file_ops.c - 测试文件操作
 */

int main() {
    printf("Testing file operations...\n");
    
    // 测试文件创建
    FILE* fp = fopen("test_output.txt", "w");
    if (fp) {
        printf("File opened successfully\n");
        fwrite("Hello World", 1, 11, fp);
        fclose(fp);
        printf("File written and closed\n");
        return 42;
    } else {
        printf("Failed to open file\n");
        return 1;
    }
}
