#!/usr/bin/env python3
"""
修复核心内容页面中剩余的 broken wikilinks。
这些目标文件存在，但链接格式可能不匹配（如 .md 后缀、显示文本差异）。
"""

import re
from pathlib import Path


def build_lookup(vault: Path) -> dict:
    lookup = {}
    for p in vault.rglob('*.md'):
        rel = str(p.relative_to(vault))
        lookup[rel.lower()] = rel
        lookup[rel.lower()[:-3]] = rel  # without .md
        lookup[Path(rel).stem.lower()] = rel
        lookup[Path(rel).name.lower()] = rel

        text = p.read_text(encoding='utf-8', errors='ignore')
        m = re.search(r'^title:\s*["\']?(.+?)["\']?$', text, re.MULTILINE)
        if m:
            title = m.group(1).strip().lower()
            lookup[title] = rel
            # 去掉 "kubernetes" 前缀等
            simple = re.sub(r'^kubernetes\s+', '', title).strip()
            if simple and simple != title:
                lookup[simple] = rel
    return lookup


def is_excluded(rel: str) -> bool:
    excluded = (
        '.git/', '.venv/', '.ruff_cache/', '.obsidian/',
        '_archives/', '_raw/', '_staging/',
        '.comate/', '.claude/', '.codebuddy/', '.qoder/',
        '.understand-anything/', '.zread/',
        'web/node_modules/', 'node_modules/',
    )
    return rel.startswith(excluded)


def scan_broken_links(vault: Path, lookup: dict):
    broken = []
    md_files = [p for p in vault.rglob('*.md') if not is_excluded(str(p.relative_to(vault)))]

    for p in md_files:
        rel = str(p.relative_to(vault))
        text = p.read_text(encoding='utf-8', errors='ignore')
        links = re.findall(r'\[\[([^\]|]+)(?:\|[^\]]*)?\]\]', text)

        seen = set()
        for link in links:
            target = link.split('#')[0].split('?')[0].strip()

            if re.fullmatch(r"[a-zA-Z0-9_.-]+", target):
                continue
            if target.startswith('http'):
                continue
            if target in seen:
                continue
            seen.add(target)

            target_lower = target.lower()
            exists = target_lower in lookup
            if not exists and '/' in target_lower:
                exists = target_lower.split('/')[-1] in lookup

            if not exists:
                broken.append((rel, target))

    return broken


def find_match(target: str, lookup: dict) -> str:
    target_lower = target.lower().strip()

    if target_lower in lookup:
        return lookup[target_lower]

    if '/' in target_lower:
        basename = target_lower.split('/')[-1]
        if basename in lookup:
            return lookup[basename]
        if basename + '.md' in lookup:
            return lookup[basename + '.md']

    # 尝试加 .md
    if target_lower + '.md' in lookup:
        return lookup[target_lower + '.md']

    return None


def fix_link_in_file(src_path: Path, target: str, replacement: str) -> bool:
    text = src_path.read_text(encoding='utf-8')
    original = text

    pattern = re.compile(rf'\[\[{re.escape(target)}(?:\|([^\]]*))?\]\]')

    def repl(match):
        display = match.group(1)
        if display:
            return f'[[{replacement}|{display}]]'
        else:
            return f'[[{replacement}]]'

    text = pattern.sub(repl, text)
    if text != original:
        src_path.write_text(text, encoding='utf-8')
        return True
    return False


def main():
    vault = Path('/Users/allengaller/Documents/GitHub/kudig-io/kudig-database')

    lookup = build_lookup(vault)
    broken = scan_broken_links(vault, lookup)

    fixed = []
    failed = []

    for src, target in broken:
        if src.startswith('_reports/'):
            continue  # 跳过报告文件

        src_path = vault / src
        matched = find_match(target, lookup)
        if matched:
            if fix_link_in_file(src_path, target, matched):
                fixed.append((src, target, matched))
            else:
                failed.append((src, target, 'pattern not found'))
        else:
            failed.append((src, target, 'no match'))

    print(f"Core broken links: {len([b for b in broken if not b[0].startswith('_reports/')])}")
    print(f"Fixed: {len(fixed)}")
    print(f"Failed: {len(failed)}")

    for src, target, reason in failed:
        print(f"  {src} -> [[{target}]] ({reason})")


if __name__ == "__main__":
    main()
