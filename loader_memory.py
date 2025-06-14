#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
TASM加载器 - 内存管理
"""

import os
import platform
import ctypes
from ctypes import c_void_p, c_size_t, c_ubyte, POINTER, cast
from ctypes import wintypes
from loader_common import LoaderError

class MemoryManager:
    """内存管理器"""
    
    def __init__(self):
        """初始化内存管理器"""
        self.os_name = platform.system().lower()
        
        if self.os_name == 'windows':
            # Windows内存管理常量
            self.MEM_COMMIT = 0x1000
            self.MEM_RESERVE = 0x2000
            self.MEM_RELEASE = 0x8000
            self.PAGE_EXECUTE_READWRITE = 0x40
            
            # 加载kernel32
            self.kernel32 = ctypes.WinDLL('kernel32')
            
            # 设置VirtualAlloc的参数类型
            self.kernel32.VirtualAlloc.argtypes = [
                wintypes.LPVOID,    # lpAddress
                ctypes.c_size_t,    # dwSize
                wintypes.DWORD,     # flAllocationType
                wintypes.DWORD      # flProtect
            ]
            self.kernel32.VirtualAlloc.restype = wintypes.LPVOID
            
            # 设置VirtualFree的参数类型
            self.kernel32.VirtualFree.argtypes = [
                wintypes.LPVOID,    # lpAddress
                ctypes.c_size_t,    # dwSize
                wintypes.DWORD      # dwFreeType
            ]
            self.kernel32.VirtualFree.restype = wintypes.BOOL
            
            # 设置FlushInstructionCache的参数类型
            self.kernel32.FlushInstructionCache.argtypes = [
                wintypes.HANDLE,    # hProcess
                wintypes.LPVOID,    # lpBaseAddress
                ctypes.c_size_t     # dwSize
            ]
            self.kernel32.FlushInstructionCache.restype = wintypes.BOOL
            
        else:
            # Unix内存管理常量
            self.PROT_READ = 0x1
            self.PROT_WRITE = 0x2
            self.PROT_EXEC = 0x4
            self.MAP_PRIVATE = 0x2
            self.MAP_ANONYMOUS = 0x20
            
            # 加载libc
            self.libc = ctypes.CDLL(ctypes.util.find_library('c'))
    
    def allocate(self, size):
        """分配内存
        
        Args:
            size: 要分配的内存大小
            
        Returns:
            分配的内存地址
            
        Raises:
            LoaderError: 当内存分配失败时
        """
        try:
            if self.os_name == 'windows':
                # Windows内存分配
                addr = self.kernel32.VirtualAlloc(
                    None,
                    size,
                    self.MEM_COMMIT | self.MEM_RESERVE,
                    self.PAGE_EXECUTE_READWRITE
                )
                if not addr:
                    raise LoaderError("内存分配失败")
                    
            else:
                # Unix内存分配
                addr = self.libc.mmap(
                    0,
                    size,
                    self.PROT_READ | self.PROT_WRITE | self.PROT_EXEC,
                    self.MAP_PRIVATE | self.MAP_ANONYMOUS,
                    -1,
                    0
                )
                if addr == -1:
                    raise LoaderError("内存分配失败")
                    
            return addr
            
        except Exception as e:
            raise LoaderError(f"内存分配失败: {str(e)}")
    
    def free(self, addr, size):
        """释放内存
        
        Args:
            addr: 要释放的内存地址
            size: 内存大小
            
        Raises:
            LoaderError: 当内存释放失败时
        """
        try:
            if self.os_name == 'windows':
                # Windows内存释放
                if not self.kernel32.VirtualFree(addr, 0, self.MEM_RELEASE):
                    raise LoaderError("内存释放失败")
                    
            else:
                # Unix内存释放
                if self.libc.munmap(addr, size) == -1:
                    raise LoaderError("内存释放失败")
                    
        except Exception as e:
            raise LoaderError(f"内存释放失败: {str(e)}")
    
    def copy_to(self, addr, data):
        """复制数据到内存
        
        Args:
            addr: 目标内存地址
            data: 要复制的数据
            
        Raises:
            LoaderError: 当内存复制失败时
        """
        try:
            # 复制数据
            dst_buf = (c_ubyte * len(data)).from_address(addr)
            for i, byte in enumerate(data):
                dst_buf[i] = byte
                
            # 刷新指令缓存（Windows）
            if self.os_name == 'windows':
                if not self.kernel32.FlushInstructionCache(
                    self.kernel32.GetCurrentProcess(),
                    addr,
                    len(data)
                ):
                    raise LoaderError("刷新指令缓存失败")
                    
        except Exception as e:
            raise LoaderError(f"内存复制失败: {str(e)}")
    
    def create_function(self, addr, restype=ctypes.c_int, argtypes=None):
        """创建函数指针
        
        Args:
            addr: 函数地址
            restype: 返回值类型
            argtypes: 参数类型列表
            
        Returns:
            函数对象
        """
        # 创建函数类型
        if argtypes is None:
            argtypes = []
        CFUNCTYPE = ctypes.CFUNCTYPE(restype, *argtypes)
        
        # 创建函数对象
        return CFUNCTYPE(addr) 