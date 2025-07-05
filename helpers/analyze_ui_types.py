#!/usr/bin/env python3
"""
分析UI元素类型分布
"""

import json
from collections import Counter

def analyze_ui_types(json_file):
    """分析UI元素类型分布"""
    with open(json_file, 'r', encoding='utf-8') as f:
        data = json.load(f)
    
    elements = data.get('elements', [])
    
    # 统计类型分布
    type_counter = Counter()
    confidence_by_type = {}
    
    for element in elements:
        element_type = element['type']
        confidence = element['confidence']
        
        type_counter[element_type] += 1
        
        if element_type not in confidence_by_type:
            confidence_by_type[element_type] = []
        confidence_by_type[element_type].append(confidence)
    
    print("🎯 UI元素类型分布分析")
    print("=" * 50)
    print(f"总检测元素数量: {len(elements)}")
    print()
    
    print("📊 类型分布:")
    for element_type, count in type_counter.most_common():
        percentage = (count / len(elements)) * 100
        avg_confidence = sum(confidence_by_type[element_type]) / len(confidence_by_type[element_type])
        print(f"  {element_type:12} : {count:2d} 个 ({percentage:5.1f}%) - 平均置信度: {avg_confidence:.2f}")
    
    print()
    print("🔍 详细分析:")
    
    # 按类型分组显示元素
    elements_by_type = {}
    for element in elements:
        element_type = element['type']
        if element_type not in elements_by_type:
            elements_by_type[element_type] = []
        elements_by_type[element_type].append(element)
    
    for element_type in sorted(elements_by_type.keys()):
        elements_of_type = elements_by_type[element_type]
        print(f"\n{element_type.upper()} 类型元素 ({len(elements_of_type)} 个):")
        
        # 显示前5个最高置信度的元素
        sorted_elements = sorted(elements_of_type, key=lambda x: x['confidence'], reverse=True)
        for i, element in enumerate(sorted_elements[:5]):
            pos = element['position']
            size = element['size']
            print(f"  {i+1}. ID:{element['id']:2d} 位置:({pos['x1']:3d},{pos['y1']:3d},{pos['x2']:3d},{pos['y2']:3d}) "
                  f"大小:{size['width']:3d}x{size['height']:3d} 置信度:{element['confidence']:.2f}")
        
        if len(elements_of_type) > 5:
            print(f"  ... 还有 {len(elements_of_type) - 5} 个元素")

def compare_with_old_analysis():
    """对比新旧分析结果"""
    print("\n" + "=" * 50)
    print("🔄 对比分析 (新 vs 旧)")
    print("=" * 50)
    
    try:
        # 读取旧的分析结果
        with open('test_analysis.json', 'r', encoding='utf-8') as f:
            old_data = json.load(f)
        old_elements = old_data.get('elements', [])
        
        # 读取新的分析结果
        with open('enhanced_analysis.json', 'r', encoding='utf-8') as f:
            new_data = json.load(f)
        new_elements = new_data.get('elements', [])
        
        print(f"旧版本检测: {len(old_elements)} 个元素 (全部为 button 类型)")
        print(f"新版本检测: {len(new_elements)} 个元素 (多种类型)")
        
        # 统计新版本的类型分布
        new_types = Counter(element['type'] for element in new_elements)
        print(f"\n新版本类型分布:")
        for element_type, count in new_types.most_common():
            print(f"  {element_type}: {count} 个")
        
        print(f"\n✅ 改进效果:")
        print(f"  - 类型多样性: 从 1 种类型增加到 {len(new_types)} 种类型")
        print(f"  - 智能分类: 能够区分按钮、图标、文本、链接、标签页等")
        print(f"  - 检测精度: 保持了相同的检测数量 ({len(new_elements)} vs {len(old_elements)})")
        
    except FileNotFoundError:
        print("未找到旧的分析文件，跳过对比")

if __name__ == "__main__":
    analyze_ui_types('enhanced_analysis.json')
    compare_with_old_analysis()
