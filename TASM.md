# TASM (Token-ASM) 规范

## 1. 概述

TASM (Token-ASM) 是自进化AI系统的统一中间表示，设计目标：
- Runtime: 容易转换为各种CPU架构(x86/ARM等)的本地机器码
- Program: 能作为平台无关的可执行格式，支持解释执行和JIT
- Loader: 便于生成各平台的本地加载器

本文档规范并不固化！当前工作重点为：尽快定义和实现最小子集实现（示意如下）：
python tasm_compiler_to_exe.py compile0.tasm compile0.exe
compile0.exe compile0.tasm compile1.exe
compile1.exe compile1.tasm compile2.exe

## 2. TASM 二进制格式

### 文件头 (16字节)
```
+0  4B  Magic     "TASM"
+4  2B  Version   Major.Minor
+6  2B  Flags     特性标记
+8  4B  Entry     入口点偏移
+12 4B  Sections  段表数量
```

### 段表 (每项16字节)
```
+0  4B  Type      段类型 (1=CODE, 2=DATA, 3=RELOC, 4=DEBUG)
+4  4B  Offset    文件偏移
+8  4B  Size      段大小
+12 4B  Flags     段属性 (R=1,W=2,X=4)
```

## 3. 类型系统

### 基本类型
```
void    - 空类型
i8      - 8位有符号整数
u8      - 8位无符号整数
i16     - 16位有符号整数
u16     - 16位无符号整数
i32     - 32位有符号整数
u32     - 32位无符号整数
i64     - 64位有符号整数
u64     - 64位无符号整数
f32     - 32位浮点数
f64     - 64位浮点数
ptr     - 指针类型
```

### 复合类型
```
array<T>     - 数组类型，T为元素类型
struct{...}  - 结构体类型
union{...}   - 联合体类型
func(...)    - 函数类型
```

## 4. IR结构

### SSA形式
所有变量只能被赋值一次，使用φ(phi)节点处理控制流汇合。

示例：
```
entry:
    v1 = 1
    v2 = 2
    br cond, then, else

then:
    v3 = v1 + v2
    br merge

else:
    v4 = v1 * v2
    br merge

merge:
    v5 = phi [v3, then], [v4, else]
    ret v5
```

### 控制流图 (CFG)
基本块和跳转关系的有向图表示。

基本块格式：
```
label:
    [phi节点...]
    [指令...]
    [终结指令]
```

终结指令类型：
- br label           ; 无条件跳转
- br cond, t, f     ; 条件跳转
- ret [value]       ; 返回
- unreachable       ; 不可达

## 5. TASM指令集

### 数据移动
```
// 栈操作
push.i <type> <imm>   ; 压入立即数
pop                   ; 弹出并丢弃
dup                   ; 复制栈顶

// 寄存器操作
mov.r <rd> <rs>       ; 寄存器移动
ldi.r <rd> <imm>      ; 加载立即数到寄存器

// 内存操作
load <type> <addr>    ; 从内存加载
store <type> <addr>   ; 存储到内存
alloca <type>         ; 在栈上分配
```

### 算术运算
```
// 整数运算
add/sub/mul/div/rem   ; 加减乘除余
and/or/xor/not        ; 位运算
shl/shr/sar           ; 移位
cmp                   ; 比较

// 浮点运算
fadd/fsub/fmul/fdiv   ; 浮点运算
fcmp                  ; 浮点比较
```

### 控制流
```
// 跳转
br label              ; 无条件跳转
br cond, t, f        ; 条件跳转

// 函数调用
call <func> <args>    ; 调用函数
ret [value]           ; 返回值

// 异常处理
invoke                ; 可能抛异常的调用
landingpad           ; 异常处理块
resume               ; 继续抛出异常
```

### SIMD/向量操作
```
vadd/vsub/vmul/vdiv   ; 向量运算
vload/vstore          ; 向量内存操作
shuffle               ; 向量重排
extract/insert        ; 向量元素访问
```

## 6. Runtime API

### 内存管理 (0x00-0x0F)
```
0x00 malloc(size: u64) -> ptr              ; 分配内存
0x01 free(ptr: ptr)                        ; 释放内存
0x02 realloc(ptr: ptr, size: u64) -> ptr   ; 重新分配
0x03 memcpy(dst: ptr, src: ptr, n: u64)    ; 内存复制
0x04 memset(dst: ptr, val: i32, n: u64)    ; 内存设置
0x05 mmap(addr: ptr, len: u64, prot: i32) -> ptr  ; 内存映射
0x06 munmap(addr: ptr, len: u64)           ; 解除映射
```

### I/O操作 (0x10-0x1F)
```
0x10 print_i64(val: i64)                   ; 打印整数
0x11 print_str(ptr: ptr, len: u64)         ; 打印字符串
0x12 read_stdin(buf: ptr, n: u64) -> i64   ; 读标准输入
0x13 write_stdout(buf: ptr, n: u64) -> i64 ; 写标准输出
0x14 open(path: ptr, flags: i32) -> i32    ; 打开文件
0x15 close(fd: i32)                        ; 关闭文件
0x16 read(fd: i32, buf: ptr, n: u64) -> i64  ; 读文件
0x17 write(fd: i32, buf: ptr, n: u64) -> i64 ; 写文件
0x18 seek(fd: i32, offset: i64, whence: i32) -> i64  ; 文件定位
```

### 进程/线程 (0x20-0x2F)
```
0x20 exit(code: i32)                       ; 退出进程
0x21 abort()                               ; 异常终止
0x22 thread_create(func: ptr) -> i64       ; 创建线程
0x23 thread_exit(ret: ptr)                 ; 线程退出
0x24 thread_join(id: i64) -> ptr          ; 等待线程
0x25 mutex_init(mutex: ptr)                ; 初始化互斥锁
0x26 mutex_lock(mutex: ptr)                ; 加锁
0x27 mutex_unlock(mutex: ptr)              ; 解锁
```

### 系统信息 (0x30-0x3F)
```
0x30 clock() -> i64                        ; 获取时钟
0x31 time() -> i64                         ; 获取时间
0x32 random() -> i64                       ; 随机数
0x33 getenv(name: ptr) -> ptr              ; 获取环境变量
0x34 getcwd(buf: ptr, size: u64) -> ptr    ; 获取当前目录
```

## 7. 示例程序

### 基本运算 (SSA形式)
```tasm
define i64 @calc(i64 %a, i64 %b, i64 %c) {
entry:
    %sum = add i64 %a, %b    ; sum = a + b
    %mul = mul i64 %sum, %c  ; mul = sum * c
    ret i64 %mul             ; return mul
}
```

### 条件分支
```tasm
define i64 @max(i64 %a, i64 %b) {
entry:
    %cond = cmp gt i64 %a, %b  ; a > b ?
    br %cond, then, else

then:
    br merge(%a)

else:
    br merge(%b)

merge(%result):
    ret i64 %result
}
```

### 循环
```tasm
define i64 @sum(i64 %n) {
entry:
    br loop(0, 0)    ; i=0, sum=0

loop(%i, %sum):
    %cond = cmp lt i64 %i, %n  ; i < n ?
    br %cond, body, done

body:
    %i.next = add i64 %i, 1    ; i++
    %sum.next = add i64 %sum, %i  ; sum += i
    br loop(%i.next, %sum.next)

done:
    ret i64 %sum
}
```

### 使用Runtime API
```tasm
define void @hello() {
entry:
    ; 分配内存
    %str = call ptr @malloc(i64 14)
    
    ; 写入字符串
    call void @memcpy(%str, "Hello, World!\n", i64 14)
    
    ; 打印字符串
    call void @print_str(%str, i64 14)
    
    ; 释放内存
    call void @free(%str)
    ret void
}
``` 