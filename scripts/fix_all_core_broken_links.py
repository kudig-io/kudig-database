#!/usr/bin/env python3
"""
统一修复核心内容页面中的 broken wikilinks。
使用标题索引 + 路径索引。
"""

import re
from pathlib import Path


def normalize(s: str) -> str:
    return s.lower().strip().replace(' ', '-').replace('_', '-')


def build_index(vault: Path) -> dict:
    """构建 target -> rel 索引，包含路径、stem、标题。"""
    index = {}
    for p in vault.rglob('*.md'):
        rel = str(p.relative_to(vault))
        text = p.read_text(encoding='utf-8', errors='ignore')

        # 路径变体
        index[rel.lower()] = rel
        index[rel.lower()[:-3]] = rel
        index[Path(rel).stem.lower()] = rel
        index[Path(rel).name.lower()] = rel
        index[normalize(Path(rel).stem)] = rel

        # 标题变体
        m = re.search(r'^title:\s*["\']?(.+?)["\']?$', text, re.MULTILINE)
        if m:
            title = m.group(1).strip()
            index[title.lower()] = rel
            index[normalize(title)] = rel

            # 简化标题
            simple = re.sub(r'\s+in\s+kubernetes$', '', title.lower()).strip()
            simple = re.sub(r'^kubernetes\s+', '', simple).strip()
            if simple and simple != title.lower():
                index[simple] = rel
                index[normalize(simple)] = rel

    return index


def is_excluded(rel: str) -> bool:
    excluded = (
        '.git/', '.venv/', '.ruff_cache/', '.obsidian/',
        '_archives/', '_raw/', '_staging/',
        '.comate/', '.claude/', '.codebuddy/', '.qoder/',
        '.understand-anything/', '.zread/',
        'web/node_modules/', 'node_modules/',
    )
    return rel.startswith(excluded)


def find_match(target: str, index: dict) -> str:
    target_lower = target.lower().strip()

    if target_lower in index:
        return index[target_lower]

    target_norm = normalize(target)
    if target_norm in index:
        return index[target_norm]

    if '/' in target_lower:
        basename = target_lower.split('/')[-1]
        if basename in index:
            return index[basename]
        if basename + '.md' in index:
            return index[basename + '.md']
        bn = normalize(basename)
        if bn in index:
            return index[bn]

    if target_lower + '.md' in index:
        return index[target_lower + '.md']

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
            if '/' in replacement:
                display = Path(replacement).stem.replace('-', ' ').replace('_', ' ')
                return f'[[{replacement}|{display}]]'
            return f'[[{replacement}]]'

    text = pattern.sub(repl, text)
    if text != original:
        try:
            src_path.write_text(text, encoding='utf-8')
            return True
        except PermissionError:
            print(f"  Permission denied: {src_path}")
            return False
    return False


def main():
    vault = Path('/Users/allengaller/Documents/GitHub/kudig-io/kudig-database')

    print("Building index...")
    index = build_index(vault)
    print(f"  Index entries: {len(index)}")

    print("\nScanning and fixing core broken links...")
    md_files = [p for p in vault.rglob('*.md') if not is_excluded(str(p.relative_to(vault)))]

    fixed = 0
    converted = 0
    remaining = []

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

            matched = find_match(target, index)
            if matched:
                if fix_link_in_file(p, target, matched):
                    fixed += 1
            else:
                # _meta/ 和 _reports/ 链接转纯文本
                if target.lower().startswith('_meta/') or target.lower().startswith('_reports/'):
                    if fix_link_in_file(p, target, target):
                        # dummy call to trigger write; will convert via display logic
                        pass
                    # Actually convert
                    text_new = p.read_text(encoding='utf-8')
                    pattern = re.compile(rf'\[\[{re.escape(target)}(?:\|([^\]]*))?\]\]')
                    def repl(m):
                        return m.group(1) if m.group(1) else target
                    text_new = pattern.sub(repl, text_new)
                    if text_new != text:
                        p.write_text(text_new, encoding='utf-8')
                        converted += 1
                else:
                    remaining.append((rel, target))

    print(f"Fixed: {fixed}")
    print(f"Converted _meta/_reports links: {converted}")
    print(f"Remaining core broken links: {len(remaining)}")

    for src, target in remaining[:20]:
        print(f"  {src} -> [[{target}]]")


if __name__ == "__main__":
    main()
