#!/usr/bin/env python3
"""
修复 relationships frontmatter 中被破坏的 target 格式。
将 [[path|display]] 转换为 [[path]]。
"""

import re
from pathlib import Path


def fix_relationships_in_file(p: Path) -> bool:
    text = p.read_text(encoding='utf-8')
    original = text

    # 匹配 relationships 块中的 target: "[[path|display]]"
    pattern = re.compile(r'(target:\s*"?)\[\[([^\]|]+)\|[^\]]+\]\]("?)')

    def repl(match):
        prefix = match.group(1)
        path = match.group(2)
        suffix = match.group(3)
        return f'{prefix}[[{path}]]{suffix}'

    text = pattern.sub(repl, text)
    if text != original:
        try:
            p.write_text(text, encoding='utf-8')
            return True
        except PermissionError:
            print(f"  Permission denied: {p}")
            return False
    return False


def main():
    vault = Path('/Users/allengaller/Documents/GitHub/kudig-io/kudig-database')

    fixed_files = 0
    for p in vault.rglob('*.md'):
        if p.is_file():
            try:
                if fix_relationships_in_file(p):
                    fixed_files += 1
            except Exception as e:
                print(f"  Error processing {p}: {e}")

    print(f"Fixed relationships in {fixed_files} files")


if __name__ == "__main__":
    main()
