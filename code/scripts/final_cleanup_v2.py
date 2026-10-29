#!/usr/bin/env python3
"""
最终清理 v2：
1. 修复 body 中的 YAML/TOML 伪链接和无效链接
2. 修复 relationships 字段中的 type、display text、无效 target
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


def build_lookup(vault: Path) -> dict:
    lookup = {}
    for p in vault.rglob('*.md'):
        rel = str(p.relative_to(vault))
        lookup[rel.lower()] = rel
        lookup[rel.lower()[:-3]] = rel
        lookup[Path(rel).stem.lower()] = rel
        lookup[Path(rel).name.lower()] = rel
    return lookup


def fix_body_links(vault: Path, lookup: dict):
    """修复 body 中的伪链接。"""
    fixed = 0
    for p in vault.rglob('*.md'):
        if is_excluded(str(p.relative_to(vault))):
            continue

        text = p.read_text(encoding='utf-8', errors='ignore')
        original = text

        # 1. YAML/TOML 字段伪链接 [[kind: Deployment]] -> `kind: Deployment`
        text = re.sub(r'\[\[(kind|group|apiVersion|name|namespace|labels|annotations|spec|metadata|data|rules|subjects|roleRef):\s*([^\]]+)\]\]', r'`\1: \2`', text)

        # 2. heading anchor [[# ...]] -> 纯文本
        text = re.sub(r'\[\[(#\s*[^\]]+)\]\]', r'\1', text)

        # 3. _meta/ 和 _reports/ 链接转文本
        text = re.sub(r'\[\[(_meta/[^\]|]+)(?:\|[^\]]*)?\]\]', r'\1', text)
        text = re.sub(r'\[\[(_reports/[^\]|]+)(?:\|[^\]]*)?\]\]', r'\1', text)

        # 4. 明显不是页面的普通词组链接转文本
        # 这些是从 lint 报告中识别出的无效链接
        invalid_links = [
            'metrics server', 'MOC from domain 8',
        ]
        for invalid in invalid_links:
            text = text.replace(f'[[{invalid}]]', invalid)

        if text != original:
            try:
                p.write_text(text, encoding='utf-8')
                fixed += 1
            except PermissionError:
                pass

    print(f"Fixed body links in {fixed} files")


def fix_relationships(vault: Path, lookup: dict):
    """修复 relationships 字段。"""
    fixed = 0

    for p in vault.rglob('*.md'):
        if is_excluded(str(p.relative_to(vault))):
            continue

        text = p.read_text(encoding='utf-8', errors='ignore')
        original = text

        # 找到 relationships 块
        fm_match = re.search(r'^(---\n.*?\n---)', text, re.DOTALL)
        if not fm_match:
            continue

        fm_text = fm_match.group(1)
        fm_original = fm_text

        # 1. type: related -> related_to
        fm_text = re.sub(r'^(\s*-\s*type:\s*)related\s*$', r'\1related_to', fm_text, flags=re.MULTILINE)

        # 2. 清理 target 中的 display text
        fm_text = re.sub(r'(target:\s*"?)\[\[([^\]|]+)\|[^\]]+\]\]("?)', r'\1[[\2]]\3', fm_text)

        # 3. 删除无效 target 的条目
        # 无效 target：纯文本、.comate/ 路径、不存在的路径
        lines = fm_text.splitlines()
        new_lines = []
        skip_next = False

        for i, line in enumerate(lines):
            if skip_next:
                skip_next = False
                continue

            target_match = re.match(r'^(\s*-\s*target:\s*)(.+)$', line)
            if target_match:
                target_value = target_match.group(2).strip().strip('"')

                # 提取 [[...]] 中的内容
                inner_match = re.match(r'\[\[(.+)\]\]', target_value)
                if inner_match:
                    inner = inner_match.group(1).split('|')[0].strip()
                else:
                    inner = target_value.strip('"')

                # 检查是否有效
                is_valid = False
                if inner:
                    inner_lower = inner.lower()
                    if inner_lower in lookup:
                        is_valid = True
                    elif '/' in inner_lower:
                        basename = inner_lower.split('/')[-1]
                        if basename in lookup:
                            is_valid = True
                            # 替换为完整路径
                            line = f'{target_match.group(1)}"[[{lookup[basename]}]]"'
                    elif inner_lower + '.md' in lookup:
                        is_valid = True

                # 如果无效，跳过当前行和下一行 type 行
                if not is_valid:
                    # 检查下一行是否是 type
                    if i + 1 < len(lines) and re.match(r'^\s*-\s*type:', lines[i + 1]):
                        # 这是 relationships 条目的一部分，需要看前面是否有 - target
                        # 跳过当前 target 行和下一行 type 行
                        skip_next = True
                        continue
                    else:
                        continue
                else:
                    # 确保 target 行格式正确
                    if not line.strip().endswith('"'):
                        line = f'{target_match.group(1)}"[[{inner}]]"'

            new_lines.append(line)

        fm_text = '\n'.join(new_lines)

        if fm_text != fm_original:
            text = text.replace(fm_original, fm_text)
            try:
                p.write_text(text, encoding='utf-8')
                fixed += 1
            except PermissionError:
                pass

    print(f"Fixed relationships in {fixed} files")


def main():
    vault = Path('/Users/allengaller/Documents/GitHub/kudig-io/kudig-database')
    lookup = build_lookup(vault)
    fix_body_links(vault, lookup)
    fix_relationships(vault, lookup)


if __name__ == "__main__":
    main()
