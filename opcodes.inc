; 指令操作码定义
%define OP_NOP   0x00    ; 空操作
%define OP_MOV   0x01    ; 数据传输
%define OP_ADD   0x02    ; 加法
%define OP_SUB   0x03    ; 减法
%define OP_MUL   0x04    ; 乘法
%define OP_DIV   0x05    ; 除法
%define OP_MOD   0x06    ; 取模
%define OP_INC   0x07    ; 自增
%define OP_DEC   0x08    ; 自减
%define OP_AND   0x09    ; 与
%define OP_OR    0x0A    ; 或
%define OP_XOR   0x0B    ; 异或
%define OP_NOT   0x0C    ; 非
%define OP_SHL   0x0D    ; 左移
%define OP_SHR   0x0E    ; 右移
%define OP_CMP   0x0F    ; 比较
%define OP_JMP   0x10    ; 无条件跳转
%define OP_JE    0x11    ; 等于跳转
%define OP_JNE   0x12    ; 不等于跳转
%define OP_JG    0x13    ; 大于跳转
%define OP_JL    0x14    ; 小于跳转
%define OP_JGE   0x15    ; 大于等于跳转
%define OP_JLE   0x16    ; 小于等于跳转
%define OP_CALL  0x17    ; 调用
%define OP_RET   0x18    ; 返回
%define OP_PUSH  0x19    ; 压栈
%define OP_POP   0x1A    ; 出栈
%define OP_LOAD  0x1B    ; 加载内存
%define OP_STORE 0x1C    ; 存储到内存
%define OP_IN    0x1D    ; 输入
%define OP_OUT   0x1E    ; 输出
%define OP_HLT   0xFF    ; 停机

; 寄存器定义
%define REG_EAX 0
%define REG_EBX 1
%define REG_ECX 2
%define REG_EDX 3
%define REG_ESI 4
%define REG_EDI 5
%define REG_ESP 6
%define REG_EBP 7
%define REG_COUNT 8

; 条件码标志位
%define FLAG_ZF  0x01    ; 零标志
%define FLAG_SF  0x02    ; 符号标志
%define FLAG_OF  0x04    ; 溢出标志
%define FLAG_CF  0x08    ; 进位标志
