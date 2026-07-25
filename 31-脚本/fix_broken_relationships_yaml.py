#!/usr/bin/env python3
"""
修复 relationships 字段中的 YAML 格式错误。
将 target: [['...']] 转换回 target: "[[...]]" 或删除无效条目。
"""

import re
from pathlib import Path


def is_excluded(rel: str) -> bool:
    excluded = (
        '.git/', '.venv/', '.ruff_cache/', '.obsidian/',
        '_archives/', '_raw/', '_staging/',
        '.comate/', '.claude/', '.codebuddy/', '.qoder/',
        '.understand-anything/', '.zread/',
        'web/node_modules/', 'node_modules/',
    )
    return rel.startswith(excluded)


def fix_file(p: Path) -> bool:
    text = p.read_text(encoding='utf-8')
    original = text

    # 1. 将 target: [['...']] 转换为 target: "[[...]]"
    # 处理单层嵌套： [['path']]
    text = re.sub(r"target:\s*\[\['([^']+)'\]\]", r'target: "[[\1]]"', text)
    text = re.sub(r'target:\s*\[\["([^"]+)"\]\]', r'target: "[[\1]]"', text)

    # 2. 将 target: [[path|display]] 转换为 target: "[[path]]"
    text = re.sub(r'target:\s*\[\[([^\]|]+)\|[^\]]+\]\]\]', r'target: "[[\1]]"', text)

    # 3. 将 target: [[...]]（无引号）转换为 target: "[[...]]"
    text = re.sub(r'target:\s*(\[\[[^\]]+\]\])', r'target: "\1"', text)

    # 4. 清理内层嵌套： [[[[path]]]] -> [[path]]
    text = re.sub(r'\[\[\[\[([^\]]+)\]\]\]\]', r'[[\1]]', text)
    text = re.sub(r"\[\['([^\]]+)'\]\]", r'[[\1]]', text)

    if text != original:
        try:
            p.write_text(text, encoding='utf-8')
            return True
        except PermissionError:
            return False
    return False


def main():
    vault = Path('/Users/allengaller/Documents/GitHub/kudig-io/kudig-database')

    fixed = 0
    for p in vault.rglob('*.md'):
        if is_excluded(str(p.relative_to(vault))):
            continue
        if fix_file(p):
            fixed += 1

    print(f"Fixed YAML in {fixed} files")


if __name__ == "__main__":
    main()
