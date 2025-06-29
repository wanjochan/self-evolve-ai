import requests
import os
import sys
from PIL import Image, ImageEnhance
import pytesseract
import numpy as np
import cv2

# 确保 pytesseract 可以找到 tesseract 可执行文件
# 如果 tesseract 不在 PATH 中，请取消注释下面一行并设置正确的路径
# pytesseract.pytesseract.tesseract_cmd = r'C:\Program Files\Tesseract-OCR\tesseract.exe'

def get_window_screenshot(window_id):
    """获取窗口截图"""
    try:
        response = requests.get(f'http://127.0.0.1:9091/windows/{window_id}/screenshot')
        if response.status_code == 200:
            # 保存截图
            with open('window_screenshot.png', 'wb') as f:
                f.write(response.content)
            print("截图已保存为 window_screenshot.png")
            return 'window_screenshot.png'
        else:
            print(f"获取截图失败: {response.text}")
            return None
    except Exception as e:
        print(f"错误: {e}")
        return None

def preprocess_image(image_path):
    """预处理图像以提高OCR准确性"""
    # 读取图像
    img = cv2.imread(image_path)
    
    # 转换为灰度图
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    
    # 应用自适应阈值
    thresh = cv2.adaptiveThreshold(
        gray, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C, 
        cv2.THRESH_BINARY, 11, 2
    )
    
    # 保存预处理后的图像
    processed_path = 'processed_' + os.path.basename(image_path)
    cv2.imwrite(processed_path, thresh)
    
    return processed_path

def extract_text_from_image(image_path):
    """从图像中提取文本"""
    # 预处理图像
    processed_image = preprocess_image(image_path)
    
    # 使用pytesseract进行OCR
    try:
        text = pytesseract.image_to_string(Image.open(processed_image), lang='chi_sim+eng')
        return text
    except Exception as e:
        print(f"OCR错误: {e}")
        return None

def analyze_dialog_structure(image_path):
    """分析对话框结构"""
    # 读取图像
    img = cv2.imread(image_path)
    
    # 转换为灰度图
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    
    # 边缘检测
    edges = cv2.Canny(gray, 50, 150)
    
    # 查找轮廓
    contours, _ = cv2.findContours(edges, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    
    # 过滤小轮廓
    min_area = 1000  # 最小面积阈值
    significant_contours = [cnt for cnt in contours if cv2.contourArea(cnt) > min_area]
    
    # 在原图上绘制轮廓
    img_with_contours = img.copy()
    cv2.drawContours(img_with_contours, significant_contours, -1, (0, 255, 0), 2)
    
    # 保存带轮廓的图像
    contour_image_path = 'contours_' + os.path.basename(image_path)
    cv2.imwrite(contour_image_path, img_with_contours)
    
    # 分析每个主要区域
    regions = []
    for i, contour in enumerate(significant_contours):
        # 获取边界矩形
        x, y, w, h = cv2.boundingRect(contour)
        
        # 提取区域
        roi = img[y:y+h, x:x+w]
        
        # 保存区域图像
        region_path = f'region_{i}_{os.path.basename(image_path)}'
        cv2.imwrite(region_path, roi)
        
        # OCR识别区域文本
        region_text = pytesseract.image_to_string(Image.open(region_path), lang='chi_sim+eng')
        
        regions.append({
            'id': i,
            'position': (x, y),
            'size': (w, h),
            'text': region_text.strip()
        })
    
    return {
        'contour_image': contour_image_path,
        'regions': regions
    }

def main():
    # Cursor窗口ID
    window_id = "52564458"
    
    # 获取截图
    screenshot_path = get_window_screenshot(window_id)
    if not screenshot_path:
        print("无法获取截图")
        return
    
    # 提取文本
    print("\n提取文本...")
    text = extract_text_from_image(screenshot_path)
    if text:
        print("\n提取的文本:")
        print("-" * 50)
        print(text)
        print("-" * 50)
    
    # 分析对话结构
    print("\n分析对话结构...")
    try:
        structure = analyze_dialog_structure(screenshot_path)
        print(f"\n找到 {len(structure['regions'])} 个主要区域")
        print(f"轮廓图像保存为: {structure['contour_image']}")
        
        print("\n区域详情:")
        for region in structure['regions']:
            print(f"\n区域 {region['id']}:")
            print(f"位置: {region['position']}")
            print(f"大小: {region['size']}")
            print(f"文本: {region['text'][:100]}..." if len(region['text']) > 100 else f"文本: {region['text']}")
    except Exception as e:
        print(f"分析结构时出错: {e}")

if __name__ == "__main__":
    main()
