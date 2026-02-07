import os
import sys
import glob

def count_non_space_chars(filepath):
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            text = f.read()
        return len([c for c in text if not c.isspace()])
    except Exception as e:
        print(f"警告：无法读取文件 {filepath} - {e}")
        return 0

if __name__ == "__main__":
    # 获取目标文件夹（默认当前目录）
    folder = sys.argv[1] if len(sys.argv) > 1 else "."
    
    # 查找所有 .md 文件（包括子目录）
    md_files = glob.glob(os.path.join(folder, "**", "*.md"), recursive=True)
    
    if not md_files:
        print("未找到任何 .md 文件。")
        sys.exit(0)
    
    total = 0
    for file in md_files:
        count = count_non_space_chars(file)
        print(f"{file}: {count} 字符")
        total += count
    
    print(f"\n总字符数（不含空白）: {total}")
