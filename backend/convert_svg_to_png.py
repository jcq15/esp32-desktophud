#!/usr/bin/env python3
"""将SVG图标批量转换为PNG
可以使用多种方法：
1. 如果系统有inkscape：使用 inkscape 命令行工具
2. 如果系统有rsvg-convert：使用 rsvg-convert
3. 如果系统有ImageMagick：使用 convert 命令
4. 否则提示用户使用在线工具
"""
import os
import subprocess
import sys
from pathlib import Path

def convert_with_inkscape(svg_path, png_path, size=64):
    """使用inkscape转换"""
    try:
        subprocess.run([
            'inkscape',
            '--export-type=png',
            f'--export-width={size}',
            f'--export-height={size}',
            f'--export-filename={png_path}',
            svg_path
        ], check=True, capture_output=True)
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False

def convert_with_rsvg(svg_path, png_path, size=64):
    """使用rsvg-convert转换"""
    try:
        subprocess.run([
            'rsvg-convert',
            '-w', str(size),
            '-h', str(size),
            '-o', png_path,
            svg_path
        ], check=True, capture_output=True)
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False

def convert_with_imagemagick(svg_path, png_path, size=64):
    """使用ImageMagick转换"""
    try:
        subprocess.run([
            'convert',
            '-background', 'white',
            '-resize', f'{size}x{size}',
            svg_path,
            png_path
        ], check=True, capture_output=True)
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False

def main():
    icons_dir = Path(__file__).parent / "icons"
    if not icons_dir.exists():
        print(f"错误: icons目录不存在: {icons_dir}")
        return
    
    svg_files = list(icons_dir.glob("*.svg"))
    if not svg_files:
        print(f"未找到SVG文件在: {icons_dir}")
        return
    
    print(f"找到 {len(svg_files)} 个SVG文件")
    
    # 检测可用的转换工具
    converters = []
    
    # 测试 inkscape
    try:
        result = subprocess.run(['inkscape', '--version'], 
                              capture_output=True, timeout=2)
        if result.returncode == 0:
            converters.append(('inkscape', convert_with_inkscape))
    except (FileNotFoundError, subprocess.TimeoutExpired):
        pass
    
    # 测试 rsvg-convert
    try:
        result = subprocess.run(['rsvg-convert', '--version'], 
                              capture_output=True, timeout=2)
        if result.returncode == 0:
            converters.append(('rsvg-convert', convert_with_rsvg))
    except (FileNotFoundError, subprocess.TimeoutExpired):
        pass
    
    # 测试 ImageMagick
    try:
        result = subprocess.run(['convert', '-version'], 
                              capture_output=True, timeout=2)
        if result.returncode == 0:
            converters.append(('ImageMagick', convert_with_imagemagick))
    except (FileNotFoundError, subprocess.TimeoutExpired):
        pass
    
    if not converters:
        print("错误: 未找到可用的SVG转换工具")
        print("请安装以下工具之一:")
        print("  - inkscape: sudo apt-get install inkscape")
        print("  - librsvg2-bin: sudo apt-get install librsvg2-bin")
        print("  - imagemagick: sudo apt-get install imagemagick")
        print("\n或者使用在线工具批量转换:")
        print("  https://convertio.co/zh/svg-png/")
        return
    
    print(f"使用转换工具: {converters[0][0]}")
    converter_func = converters[0][1]
    
    converted = 0
    skipped = 0
    
    for svg_path in svg_files:
        png_path = svg_path.with_suffix('.png')
        
        # 如果PNG已存在，跳过
        if png_path.exists():
            skipped += 1
            continue
        
        print(f"转换: {svg_path.name} -> {png_path.name}")
        if converter_func(str(svg_path), str(png_path), size=64):
            converted += 1
        else:
            print(f"  失败: {svg_path.name}")
    
    print(f"\n完成: 转换了 {converted} 个文件，跳过了 {skipped} 个已存在的文件")

if __name__ == "__main__":
    main()

