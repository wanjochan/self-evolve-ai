#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
修复UI Automation类型库问题的脚本
"""

import sys
import os
import shutil

def fix_ui_automation():
    """修复UI Automation类型库问题"""
    print("开始修复UI Automation类型库...")
    
    try:
        import comtypes
        print(f"comtypes版本: {comtypes.__version__}")
        
        # 清理现有的生成文件
        gen_dir = os.path.join(os.path.dirname(comtypes.__file__), 'gen')
        if os.path.exists(gen_dir):
            print(f"清理现有的gen目录: {gen_dir}")
            try:
                shutil.rmtree(gen_dir)
                print("gen目录已清理")
            except Exception as e:
                print(f"清理gen目录失败: {e}")
        
        # 重新创建gen目录
        os.makedirs(gen_dir, exist_ok=True)
        
        # 创建__init__.py文件
        init_file = os.path.join(gen_dir, '__init__.py')
        with open(init_file, 'w') as f:
            f.write('# comtypes.gen package\n')
        
        print("重新生成UI Automation类型库...")
        
        # 方法1：使用UIAutomationCore.dll
        try:
            import comtypes.client
            comtypes.client.GetModule("UIAutomationCore.dll")
            print("✓ 成功生成UIAutomationCore类型库")
        except Exception as e:
            print(f"✗ 生成UIAutomationCore失败: {e}")
        
        # 方法2：使用oleacc.dll (备用)
        try:
            comtypes.client.GetModule("oleacc.dll")
            print("✓ 成功生成oleacc类型库")
        except Exception as e:
            print(f"✗ 生成oleacc失败: {e}")
        
        # 测试UI Automation是否可用
        print("\n测试UI Automation...")
        test_ui_automation()
        
    except ImportError:
        print("错误: comtypes未安装，请运行: pip install comtypes")
        return False
    except Exception as e:
        print(f"修复过程中出错: {e}")
        return False
    
    return True

def test_ui_automation():
    """测试UI Automation是否工作"""
    try:
        import comtypes
        import comtypes.client
        
        # 初始化COM
        comtypes.CoInitialize()
        
        # 尝试创建UI Automation对象
        try:
            # 方法1：直接使用CLSID
            automation = comtypes.client.CreateObject(
                "{ff48dba4-60ef-4201-aa87-54103eef594e}"
            )
            print("✓ 方法1成功：直接CLSID创建")
            return True
        except Exception as e1:
            print(f"✗ 方法1失败: {e1}")
        
        try:
            # 方法2：使用ProgID
            automation = comtypes.client.CreateObject("CUIAutomation")
            print("✓ 方法2成功：ProgID创建")
            return True
        except Exception as e2:
            print(f"✗ 方法2失败: {e2}")
        
        try:
            # 方法3：使用生成的类型库
            from comtypes.gen import UIAutomationClient
            automation = comtypes.client.CreateObject(
                UIAutomationClient.CUIAutomation,
                interface=UIAutomationClient.IUIAutomation
            )
            print("✓ 方法3成功：生成的类型库")
            return True
        except Exception as e3:
            print(f"✗ 方法3失败: {e3}")
        
        print("✗ 所有方法都失败了")
        return False
        
    except Exception as e:
        print(f"测试失败: {e}")
        return False
    finally:
        try:
            comtypes.CoUninitialize()
        except:
            pass

if __name__ == "__main__":
    fix_ui_automation() 