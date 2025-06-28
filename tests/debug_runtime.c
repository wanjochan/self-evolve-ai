// 调试runtime - 检查是否能接收程序数据
int main(void* program_data, size_t program_size) {
    // 简单检查：如果接收到了数据，返回数据大小的低8位
    if (program_data && program_size > 0) {
        return (int)(program_size & 0xFF);
    }
    return 99; // 没有接收到数据
}
