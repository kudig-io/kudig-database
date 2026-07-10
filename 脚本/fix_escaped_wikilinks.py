#!/usr/bin/env python3
"""
修复被错误转义的 wikilinks。
将 [[target\|display\]] 转换为 [[target|display]]
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
    text = p.read_text(encoding='utf-8', errors='ignore')
    original = text

    # 修复 [[target\|display]] -> [[target|display]]
    text = text.replace('\\|', '|')

    # 修复 [[target\]] -> [[target]]
    text = text.replace('\\]]', ']]')

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

    print(f"Fixed escaped wikilinks in {fixed} files")


if __name__ == "__main__":
    main()
