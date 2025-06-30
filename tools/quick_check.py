#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
快速检查工具 - 简单快速地检查当前VSCode状态
"""

import sys
import os

# 添加当前目录到路径
sys.path.append(os.path.dirname(__file__))

def quick_check():
    """快速检查当前状态"""
    try:
        print("🔍 快速检查VSCode状态...")
        
        # 导入模块
        from uia_module import UIAModule
        
        module = UIAModule(verbose=False)
        
        # 查找VSCode窗口
        vscode_window = module.find_vscode_window()
        if not vscode_window:
            print("❌ 未找到VSCode窗口")
            return
        
        print(f"✅ 找到VSCode窗口: {vscode_window['title']}")
        
        # 获取根元素
        root_element = module.get_element_from_hwnd(int(vscode_window['id']))
        if not root_element:
            print("❌ 无法获取UI根元素")
            return
        
        print("✅ 获取UI根元素成功")
        
        # 快速搜索几个关键词
        keywords = ["going", "keep", "would", "augment"]
        
        for keyword in keywords:
            try:
                print(f"  搜索 '{keyword}'...", end=' ')
                
                results = module.search_text_in_tree(
                    root_element,
                    [keyword],
                    max_depth=6,  # 减少深度以提高速度
                    case_sensitive=False
                )
                
                if results:
                    print(f"✅ {len(results)} 个结果")
                    
                    # 检查是否有完整的"keep going"文本
                    for result in results:
                        text = result.get('text_content', '').lower()
                        if 'would' in text and 'keep' in text and 'going' in text:
                            print(f"    🎯 找到完整匹配: '{result['text_content'][:60]}...'")
                            return True
                        elif 'keep' in text and 'going' in text:
                            print(f"    📝 找到部分匹配: '{result['text_content'][:60]}...'")
                else:
                    print("❌")
                    
            except Exception as e:
                print(f"❌ 错误: {e}")
        
        print("ℹ️  当前未检测到'Would you like me to keep going'提示")
        return False
        
    except Exception as e:
        print(f"❌ 检查过程出错: {e}")
        return False
    
    finally:
        try:
            UIAModule.cleanup()
        except:
            pass

if __name__ == "__main__":
    quick_check() 