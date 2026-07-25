#!/usr/bin/env python3
"""
最终轮 broken wikilinks 修复。
使用更宽松的匹配：页面标题包含目标、目标包含页面标题、模糊匹配。
"""

import re
from pathlib import Path


def normalize(s: str) -> str:
    return s.lower().strip().replace(' ', '-').replace('_', '-')


def build_title_index(vault: Path) -> dict:
    """构建标题 → rel 索引。"""
    titles = {}
    for p in vault.rglob('*.md'):
        rel = str(p.relative_to(vault))
        text = p.read_text(encoding='utf-8', errors='ignore')

        stem = Path(rel).stem
        titles[stem.lower()] = rel
        titles[normalize(stem)] = rel

        m = re.search(r'^title:\s*["\']?(.+?)["\']?$', text, re.MULTILINE)
        if m:
            title = m.group(1).strip()
            titles[title.lower()] = rel
            titles[normalize(title)] = rel

            # 简化标题
            simple = re.sub(r'\s+in\s+kubernetes$', '', title.lower()).strip()
            simple = re.sub(r'^kubernetes\s+', '', simple).strip()
            if simple and simple != title.lower():
                titles[simple] = rel
                titles[normalize(simple)] = rel

    return titles


def is_excluded(rel: str) -> bool:
    excluded = (
        '.git/', '.venv/', '.ruff_cache/', '.obsidian/',
        '_archives/', '_raw/', '_staging/',
        '.comate/', '.claude/', '.codebuddy/', '.qoder/',
        '.understand-anything/', '.zread/',
        'web/node_modules/', 'node_modules/',
    )
    return rel.startswith(excluded)


def scan_broken_links(vault: Path, valid: set):
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
            exists = target_lower in valid
            if not exists and '/' in target_lower:
                exists = target_lower.split('/')[-1] in valid

            if not exists:
                broken.append((rel, target))

    return broken


def find_match(target: str, titles: dict) -> tuple:
    target_lower = target.lower().strip()
    target_norm = normalize(target)

    # 精确匹配
    if target_lower in titles:
        return titles[target_lower], 'exact'
    if target_norm in titles:
        return titles[target_norm], 'exact'

    # basename 精确匹配
    if '/' in target_lower:
        basename = target_lower.split('/')[-1]
        if basename in titles:
            return titles[basename], 'basename-exact'
        bn = normalize(basename)
        if bn in titles:
            return titles[bn], 'basename-exact'

    # 包含匹配：页面标题包含 target
    best_match = None
    best_score = 0
    for key, rel in titles.items():
        if target_lower in key:
            score = len(target_lower) / len(key) if key else 0
            if score > best_score:
                best_score = score
                best_match = rel
        elif key in target_lower:
            score = len(key) / len(target_lower) if target_lower else 0
            if score > best_score:
                best_score = score
                best_match = rel

    if best_match and best_score >= 0.5:
        return best_match, 'contains'

    # 字符集匹配（Jaccard-like）
    target_set = set(target_lower)
    best_jaccard = 0
    best_jaccard_rel = None
    for key, rel in titles.items():
        key_set = set(key)
        if not key_set:
            continue
        intersection = len(target_set & key_set)
        union = len(target_set | key_set)
        jaccard = intersection / union if union > 0 else 0
        if jaccard > best_jaccard:
            best_jaccard = jaccard
            best_jaccard_rel = rel

    if best_jaccard_rel and best_jaccard >= 0.85:
        return best_jaccard_rel, 'jaccard'

    return None, None


def fix_link_in_file(src_path: Path, target: str, replacement: str, mode: str) -> bool:
    text = src_path.read_text(encoding='utf-8')
    original = text

    pattern = re.compile(rf'\[\[{re.escape(target)}(?:\|([^\]]*))?\]\]')

    def repl(match):
        display = match.group(1)
        if mode == 'text':
            return display if display else target
        else:
            if display:
                return f'[[{replacement}|{display}]]'
            else:
                if '/' in replacement:
                    display = Path(replacement).stem.replace('-', ' ').replace('_', ' ')
                    return f'[[{replacement}|{display}]]'
                return f'[[{replacement}]]'

    text = pattern.sub(repl, text)
    if text != original:
        src_path.write_text(text, encoding='utf-8')
        return True
    return False


def main():
    vault = Path('/Users/allengaller/Documents/GitHub/kudig-io/kudig-database')

    print("Building title index...")
    titles = build_title_index(vault)
    valid = set(titles.keys())
    print(f"  Titles: {len(titles)}")

    # 迭代修复直到稳定
    round_num = 0
    total_fixed = 0
    total_converted = 0

    while True:
        round_num += 1
        broken = scan_broken_links(vault, valid)
        print(f"\nRound {round_num}: {len(broken)} broken links")

        if not broken:
            break

        fixed = []
        converted = []
        failed = []

        for src, target in broken:
            src_path = vault / src

            # _meta/ 链接转文本
            if target.lower().startswith('_meta/'):
                if fix_link_in_file(src_path, target, '', 'text'):
                    converted.append((src, target, '_meta'))
                else:
                    failed.append((src, target, 'pattern not found'))
                continue

            matched, confidence = find_match(target, titles)
            if matched:
                if fix_link_in_file(src_path, target, matched, 'link'):
                    fixed.append((src, target, matched, confidence))
                else:
                    failed.append((src, target, 'pattern not found'))
            else:
                if fix_link_in_file(src_path, target, '', 'text'):
                    converted.append((src, target, 'no match'))
                else:
                    failed.append((src, target, 'pattern not found'))

        print(f"  Fixed: {len(fixed)}, Converted: {len(converted)}, Failed: {len(failed)}")
        total_fixed += len(fixed)
        total_converted += len(converted)

        if len(fixed) == 0 and len(converted) == 0:
            break

    remaining = scan_broken_links(vault, valid)
    print(f"\nTotal fixed: {total_fixed}")
    print(f"Total converted: {total_converted}")
    print(f"Remaining: {len(remaining)}")

    # 报告
    report_out = vault / '_reports/broken-links-final-fix-2026-06-26.md'
    lines = [
        "---",
        "title: 最终轮 Broken Wikilinks 修复报告（2026-06-26）",
        "description: 迭代修复核心内容中的 broken wikilinks",
        "category: reports",
        "tags:",
        "- wiki-lint",
        "- broken-links",
        "created: \"2026-06-26\"",
        "updated: \"2026-06-26\"",
        "---",
        "",
        "# 最终轮 Broken Wikilinks 修复报告",
        "",
        f"- 总修复: {total_fixed}",
        f"- 总转纯文本: {total_converted}",
        f"- 最终剩余: {len(remaining)}",
        "",
        "## Remaining Broken Links",
        "",
        "| Source | Original |",
        "|---|---|",
    ]
    for src, target in remaining:
        lines.append(f"| `{src}` | `[[{target}]]` |")

    report_out.write_text('\n'.join(lines), encoding='utf-8')
    print(f"\nReport written: {report_out}")


if __name__ == "__main__":
    main()
