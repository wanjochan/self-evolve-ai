#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
UIA性能测试脚本
测试dump_tree和dump_vscode_tree的性能
"""

import time
import json
from uia_module import UIAModule, UIAElement

def test_dump_tree():
    """测试dump_tree性能"""
    print("\n=== 测试dump_tree性能 ===")
    
    # 初始化UIA模块
    uia = UIAModule()
    
    # 测试不同配置
    configs = [
        {'parallel': False, 'max_depth': 3},
        {'parallel': True, 'max_depth': 3, 'min_children_for_parallel': 5}
    ]
    
    for config in configs:
        print(f"\n测试配置: {config}")
        
        # 执行dump_tree
        start_time = time.time()
        uia.dump_tree(**config)
        end_time = time.time()
        
        # 输出结果
        print(f"耗时: {end_time - start_time:.2f}秒")

def test_vscode_dump_performance():
    """测试VSCode dump性能"""
    print("\n测试VSCode dump性能...")
    
    uia = UIAModule(verbose=True)
    if not uia.initialize():
        print("初始化UIA失败")
        return
        
    try:
        # 运行多次取平均值
        times = []
        for i in range(3):
            start_time = time.time()
            result = uia.dump_vscode_tree()
            end_time = time.time()
            
            if "error" in result:
                print(f"  错误: {result['error']}")
                continue
                
            dump_time = end_time - start_time
            times.append(dump_time)
            
            total_elements = result["stats"]["total_elements"]
            print(f"  运行 #{i+1}: 耗时 {dump_time:.2f}秒, 元素数量: {total_elements}")
            
            # 保存第一次的结果用于分析
            if i == 0:
                with open("vscode_tree_dump.json", "w", encoding="utf-8") as f:
                    json.dump(result, f, ensure_ascii=False, indent=2)
                print("  UI树结构已保存到 vscode_tree_dump.json")
        
        if times:
            avg_time = sum(times) / len(times)
            print(f"  平均耗时: {avg_time:.2f}秒")
            
    except Exception as e:
        print(f"测试出错: {e}")
    finally:
        uia.cleanup()

def test_click_try_again():
    """测试点击try again功能"""
    print("\n=== 测试点击try again ===")
    
    # 初始化UIA模块
    uia = UIAModule()
    
    # 执行点击
    start_time = time.time()
    success = uia.click_try_again()
    end_time = time.time()
    
    # 输出结果
    print(f"点击结果: {'成功' if success else '失败'}")
    print(f"耗时: {end_time - start_time:.2f}秒")
    
    return success

if __name__ == "__main__":
    test_dump_tree()
    test_click_try_again() 