#!/usr/bin/env python3
"""校准所有文档的 estimated_read_time"""

import os
import re
import yaml
from pathlib import Path

BASE_DIR = Path("/Users/allengaller/Documents/GitHub/kudig-io/kudig-database")
EXCLUDE_DIRS = {'.git', '.venv', '.ruff_cache', 'site', 'node_modules', '.obsidian', '.zread', '.claude', '.codebuddy', '.comate', '.github'}

def estimate_read_time(char_count: int) -> str:
    """按中文 1200 字符/分钟计算"""
    minutes = max(1, char_count // 1200)
    if minutes <= 5:
        return "5min"
    elif minutes <= 10:
        return "10min"
    elif minutes <= 15:
        return "15min"
    elif minutes <= 20:
        return "20min"
    elif minutes <= 25:
        return "25min"
    elif minutes <= 30:
        return "30min"
    elif minutes <= 40:
        return "40min"
    elif minutes <= 45:
        return "45min"
    elif minutes <= 60:
        return "1h"
    elif minutes <= 90:
        return "1.5h"
    else:
        return f"{(minutes // 30) * 30}min"

def fix_read_time(filepath: Path):
    try:
        content = filepath.read_text(encoding='utf-8')
    except:
        return False
    
    if not content.lstrip().startswith('---'):
        return False
    
    end = content.index('---', 3)
    yaml_str = content[3:end]
    body = content[end+3:]
    
    try:
        fm = yaml.safe_load(yaml_str) or {}
    except:
        return False
    
    if 'estimated_read_time' not in fm:
        return False
    
    # 按内容实际长度计算
    # 去掉 YAML front matter 和代码块
    clean_body = re.sub(r'```.*?```', '', body, flags=re.DOTALL)
    char_count = len(clean_body)
    new_time = estimate_read_time(char_count)
    old_time = fm['estimated_read_time']
    
    if old_time == new_time:
        return False
    
    fm['estimated_read_time'] = new_time
    new_yaml = yaml.dump(fm, allow_unicode=True, default_flow_style=False, sort_keys=False, width=120)
    new_content = f"---\n{new_yaml}---\n{body}"
    filepath.write_text(new_content, encoding='utf-8')
    return True

fixed = 0
total = 0
for root, dirs, files in os.walk(BASE_DIR):
    dirs[:] = [d for d in dirs if d not in EXCLUDE_DIRS]
    for f in files:
        if not f.endswith('.md'):
            continue
        fp = Path(root) / f
        total += 1
        if fix_read_time(fp):
            fixed += 1

print(f"校准完成: {fixed}/{total} 个文件更新了 estimated_read_time")
