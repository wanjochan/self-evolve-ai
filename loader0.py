#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
TASM加载器 - 加载和执行TASM程序

设计理念：做最少的事，把复杂性交给Runtime

用法：
loader {program} [args...]

环境变量：
  RUNTIME - 指定Runtime文件（可选，默认自动检测）
  RUNTIME_DEBUG - 启用调试输出（可选，默认禁用）

示例：
conda activate base
python loader1.py test.tir
"""

import sys
import os
import ctypes
from ctypes import c_void_p, c_size_t
import platform

from loader_common import (
    LoaderError, detect_platform, load_file,
    verify_runtime, verify_program, setup_runtime
)
from loader_memory import MemoryManager

def load_and_run(program_file):
    """加载并运行程序
    
    Args:
        program_file: 程序文件路径
        
    Returns:
        程序返回值
        
    Raises:
        LoaderError: 当加载或运行失败时
    """
    debug = os.getenv('RUNTIME_DEBUG', '0').lower() in ('1', 'true', 'yes')
    
    try:
        if debug:
            print("\n=== 加载程序 ===")
            print(f"程序文件: {program_file}")
        
        # 加载Runtime
        runtime_file = detect_platform()
        if debug:
            print(f"\n加载Runtime: {runtime_file}")
            
        runtime_data = load_file(runtime_file)
        if debug:
            print(f"Runtime大小: {len(runtime_data)} 字节")
            print("Runtime头:")
            for i in range(min(16, len(runtime_data))):
                print(f"{runtime_data[i]:02X}", end=' ')
            print()
            
        runtime_data = setup_runtime(runtime_data)
        if debug:
            print("Runtime设置完成")
        
        # 加载程序
        if debug:
            print("\n加载程序文件...")
            
        program_data = load_file(program_file)
        if debug:
            print(f"程序大小: {len(program_data)} 字节")
            print("程序头:")
            for i in range(min(16, len(program_data))):
                print(f"{program_data[i]:02X}", end=' ')
            print()
            
        if not verify_program(program_data):
            raise LoaderError("无效的程序格式")
            
        # 计算所需内存大小
        program_size = len(program_data)
        runtime_size = len(runtime_data)
        total_size = runtime_size + program_size + 0x1000  # 额外预留4KB
        
        if debug:
            print(f"\n内存分配:")
            print(f"Runtime大小: {runtime_size} 字节")
            print(f"程序大小: {program_size} 字节")
            print(f"总大小: {total_size} 字节")
        
        # 创建内存管理器
        memory = MemoryManager()
        
        try:
            # 分配内存
            if debug:
                print("\n分配内存...")
                
            addr = memory.allocate(total_size)
            if debug:
                print(f"分配地址: 0x{addr:016X}")
            
            # 复制Runtime
            if debug:
                print("\n复制Runtime...")
                
            memory.copy_to(addr, runtime_data)
            if debug:
                print(f"Runtime基址: 0x{addr:016X}")
            
            # 复制程序
            if debug:
                print("\n复制程序...")
                
            program_addr = addr + len(runtime_data)
            memory.copy_to(program_addr, program_data)
            if debug:
                print(f"程序基址: 0x{program_addr:016X}")
            
            # 创建Runtime入口函数
            if debug:
                print("\n创建Runtime入口...")
                
            runtime_entry = memory.create_function(
                addr,
                restype=ctypes.c_int,
                argtypes=[c_void_p, c_size_t]
            )
            
            # 调用Runtime
            if debug:
                print("\n开始执行...")
                print(f"入口地址: 0x{addr:016X}")
                print(f"程序参数: addr=0x{program_addr:016X}, size={program_size}")
            
            result = runtime_entry(program_addr, program_size)
            
            if debug:
                print(f"\n执行完成")
                print(f"返回值: {result}")
                print("===============")
                
            return result
            
        finally:
            # 释放内存
            if debug:
                print("\n释放内存...")
                
            memory.free(addr, total_size)
            
            if debug:
                print("内存已释放")
                print("===============\n")
                
    except Exception as e:
        if debug:
            print(f"\n!!! 发生错误 !!!")
            print(f"错误: {str(e)}")
            print("===============\n")
        raise LoaderError(f"加载失败: {str(e)}")

def main():
    """主函数"""
    if len(sys.argv) < 2:
        print("用法: loader1.py <program_file>")
        sys.exit(1)
        
    program_file = sys.argv[1]
    debug = os.getenv('RUNTIME_DEBUG', '0').lower() in ('1', 'true', 'yes')
    
    try:
        if debug:
            print("\n=== TASM加载器 ===")
            print(f"平台: {platform.system()} {platform.machine()}")
            print(f"Python: {platform.python_version()}")
            print("===============\n")
            
        result = load_and_run(program_file)
        
        if debug:
            print(f"\n=== 程序结束 ===")
            print(f"返回值: {result}")
            print("===============\n")
            
        sys.exit(result)
        
    except LoaderError as e:
        print(f"错误: {e}")
        sys.exit(1)
        
if __name__ == '__main__':
    main() 