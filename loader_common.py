#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
TASM加载器 - 共享工具函数和类
"""

import os
import platform
import ctypes
from ctypes import c_void_p, c_size_t, c_ubyte, POINTER, cast
from ctypes import wintypes

class LoaderError(Exception):
    """加载器错误"""
    pass

def detect_platform():
    """检测平台并返回对应的Runtime文件名"""
    # 如果设置了RUNTIME环境变量，直接使用
    runtime = os.getenv('RUNTIME')
    if runtime:
        return runtime
        
    # 否则根据平台选择默认文件名
    os_name = platform.system().lower()
    arch = platform.machine().lower()
    
    # 支持的平台组合
    supported_platforms = {
        ('windows', 'amd64'): 'Runtime-win-x64.bin',
        ('windows', 'x86_64'): 'Runtime-win-x64.bin',
        ('linux', 'x86_64'): 'Runtime-linux-x64.bin',
        ('linux', 'aarch64'): 'Runtime-linux-arm64.bin',
        ('darwin', 'x86_64'): 'Runtime-macos-x64.bin',
        ('darwin', 'arm64'): 'Runtime-macos-arm64.bin'
    }
    
    platform_key = (os_name, arch)
    if platform_key not in supported_platforms:
        raise LoaderError(
            f"不支持的平台: {os_name}-{arch}\n"
            f"支持的平台: {', '.join(f'{os}-{arch}' for os, arch in supported_platforms.keys())}"
        )
    
    return supported_platforms[platform_key]

def load_file(filename):
    """加载文件并返回字节数组"""
    if not os.path.exists(filename):
        raise LoaderError(f"文件不存在: {filename}")
    
    with open(filename, 'rb') as f:
        return f.read()

def verify_runtime(runtime_data):
    """验证Runtime格式
    
    Args:
        runtime_data: Runtime二进制数据
        
    Returns:
        bool: 是否是有效的Runtime
        
    Raises:
        LoaderError: 当Runtime格式无效时
    """
    try:
        # 检查最小长度
        if len(runtime_data) < 16:
            raise LoaderError("无效的Runtime：文件太小")
            
        # 检查魔数（前4字节应该是'RUNT'）
        magic = runtime_data[:4]
        if magic != b'RUNT':
            raise LoaderError(f"无效的Runtime：魔数错误 {magic}")
            
        # 检查版本（第4-5字节）
        version_major = runtime_data[4]
        version_minor = runtime_data[5]
        if version_major != 1 or version_minor != 0:
            raise LoaderError(f"不支持的Runtime版本: {version_major}.{version_minor}")
            
        # 检查平台标识（第6-7字节）
        platform_id = runtime_data[6:8]
        os_name = platform.system().lower()
        if os_name == 'windows' and platform_id != b'WN':
            raise LoaderError("Runtime平台不匹配：需要Windows版本")
        elif os_name == 'linux' and platform_id != b'LX':
            raise LoaderError("Runtime平台不匹配：需要Linux版本")
        elif os_name == 'darwin' and platform_id != b'MC':
            raise LoaderError("Runtime平台不匹配：需要macOS版本")
            
        # 检查入口点（第8-15字节）
        entry_point = int.from_bytes(runtime_data[8:16], 'little')
        if entry_point >= len(runtime_data):
            raise LoaderError("无效的Runtime：入口点超出范围")
            
        # 检查API占位符
        if os_name == 'windows':
            required_markers = [
                b'API_WIN_GETSTDHANDLE',
                b'API_WIN_WRITECONSOLEA',
                b'API_WIN_READCONSOLEA',
                b'API_WIN_EXITPROCESS'
            ]
        else:
            required_markers = [
                b'API_UNIX_WRITE',
                b'API_UNIX_READ',
                b'API_UNIX_EXIT'
            ]
            
        for marker in required_markers:
            if runtime_data.find(marker) == -1:
                raise LoaderError(f"Runtime缺少必需的API占位符: {marker}")
                
        return True
        
    except LoaderError:
        raise
    except Exception as e:
        raise LoaderError(f"Runtime格式验证失败: {str(e)}")

def verify_program(program_data):
    """验证程序格式
    
    Args:
        program_data: 程序二进制数据
        
    Returns:
        bool: 是否是有效的程序
        
    Raises:
        LoaderError: 当程序格式无效时
    """
    try:
        # 检查魔数
        if len(program_data) < 16:
            raise LoaderError("无效的程序：文件太小")
            
        magic = program_data[:4]
        if magic != b'TASM':
            raise LoaderError(f"无效的程序：魔数错误 {magic}")
            
        # 检查版本
        version_major = program_data[4]
        version_minor = program_data[5]
        if version_major != 1 or version_minor != 0:
            raise LoaderError(f"不支持的程序版本: {version_major}.{version_minor}")
            
        # 检查段表
        num_sections = int.from_bytes(program_data[12:16], 'little')
        if len(program_data) < 16 + num_sections * 16:
            raise LoaderError("无效的程序：段表不完整")
            
        return True
        
    except Exception as e:
        raise LoaderError(f"程序格式验证失败: {str(e)}")

def setup_runtime(runtime_data):
    """设置Runtime
    
    Args:
        runtime_data: Runtime二进制数据
        
    Returns:
        设置后的Runtime数据
        
    Raises:
        LoaderError: 当Runtime设置失败时
    """
    try:
        # 验证Runtime格式
        if not verify_runtime(runtime_data):
            raise LoaderError("无效的Runtime格式")
            
        # 注入API地址
        os_name = platform.system().lower()
        if os_name == 'windows':
            # Windows API注入
            kernel32 = ctypes.WinDLL('kernel32')
            apis = {
                'GetStdHandle': kernel32.GetStdHandle,
                'WriteConsoleA': kernel32.WriteConsoleA,
                'ReadConsoleA': kernel32.ReadConsoleA,
                'ExitProcess': kernel32.ExitProcess
            }
            
            # API占位符标记
            markers = {
                'GetStdHandle': b'API_WIN_GETSTDHANDLE',
                'WriteConsoleA': b'API_WIN_WRITECONSOLEA',
                'ReadConsoleA': b'API_WIN_READCONSOLEA',
                'ExitProcess': b'API_WIN_EXITPROCESS'
            }
            
        elif os_name in ('linux', 'darwin'):
            # Unix API注入
            libc = ctypes.CDLL(ctypes.util.find_library('c'))
            apis = {
                'write': libc.write,
                'read': libc.read,
                'exit': libc.exit
            }
            
            # API占位符标记
            markers = {
                'write': b'API_UNIX_WRITE',
                'read': b'API_UNIX_READ',
                'exit': b'API_UNIX_EXIT'
            }
            
        else:
            raise LoaderError(f"不支持的操作系统: {os_name}")
            
        # 注入API地址
        runtime_data = bytearray(runtime_data)
        for api_name, api_func in apis.items():
            marker = markers[api_name]
            pos = runtime_data.find(marker)
            
            if pos == -1:
                if os.getenv('RUNTIME_DEBUG'):
                    print(f"警告: 未找到API {api_name} 的占位符")
                continue
                
            api_addr = ctypes.cast(api_func, ctypes.c_void_p).value
            if os.getenv('RUNTIME_DEBUG'):
                print(f"注入 {api_name} 地址: {hex(api_addr)} 到位置: {hex(pos)}")
                
            # 替换占位符为实际地址
            addr_bytes = api_addr.to_bytes(8, 'little')
            runtime_data[pos:pos+8] = addr_bytes
            
        return bytes(runtime_data)
        
    except Exception as e:
        raise LoaderError(f"Runtime设置失败: {str(e)}") 