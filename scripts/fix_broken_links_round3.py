#!/usr/bin/env python3
"""
第三轮修复 broken wikilinks。
处理指向 _reports/_meta/journal 的链接和概念链接。
"""

import re
import difflib
from pathlib import Path


def normalize(s: str) -> str:
    return s.lower().strip().replace(' ', '-').replace('_', '-')


def build_lookup(vault: Path) -> dict:
    lookup = {}
    for p in vault.rglob('*.md'):
        rel = str(p.relative_to(vault))
        text = p.read_text(encoding='utf-8', errors='ignore')

        lookup[rel.lower()] = rel
        lookup[rel.lower()[:-3]] = rel
        lookup[Path(rel).stem.lower()] = rel
        lookup[Path(rel).name.lower()] = rel
        lookup[normalize(Path(rel).stem)] = rel

        m = re.search(r'^title:\s*["\']?(.+?)["\']?$', text, re.MULTILINE)
        if m:
            title = m.group(1).strip()
            lookup[title.lower()] = rel
            lookup[normalize(title)] = rel

    return lookup


def is_excluded(rel: str) -> bool:
    excluded = (
        '.git/', '.venv/', '.ruff_cache/', '.obsidian/',
        '_archives/', '_raw/', '_staging/',
        '.comate/', '.claude/', '.codebuddy/', '.qoder/',
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

        seen_in_file = set()
        for link in links:
            target = link.split('#')[0].split('?')[0].strip()

            if re.fullmatch(r"[a-zA-Z0-9_.-]+", target):
                continue
            if target.startswith('http'):
                continue
            if target in seen_in_file:
                continue
            seen_in_file.add(target)

            target_lower = target.lower()
            exists = target_lower in lookup
            if not exists and '/' in target_lower:
                exists = target_lower.split('/')[-1] in lookup

            if not exists:
                broken.append((rel, target))

    return broken


def find_best_match(target: str, lookup: dict) -> tuple:
    target_lower = target.lower().strip()
    target_norm = normalize(target)

    if target_lower in lookup:
        return lookup[target_lower], 'exact'
    if target_norm in lookup:
        return lookup[target_norm], 'exact'

    if '/' in target_lower:
        basename = target_lower.split('/')[-1]
        if basename in lookup:
            return lookup[basename], 'basename'
        basename_norm = normalize(basename)
        if basename_norm in lookup:
            return lookup[basename_norm], 'basename'

    if len(target_lower) < 3:
        return None, None

    keys = list(lookup.keys())
    matches = difflib.get_close_matches(target_lower, keys, n=3, cutoff=0.75)
    if matches:
        best = matches[0]
        if len(matches) == 1:
            return lookup[best], 'fuzzy'
        score1 = difflib.SequenceMatcher(None, target_lower, best).ratio()
        score2 = difflib.SequenceMatcher(None, target_lower, matches[1]).ratio()
        if score1 - score2 > 0.05:
            return lookup[best], 'fuzzy'

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

    print("Building lookup...")
    lookup = build_lookup(vault)

    print("\nScanning broken links...")
    broken = scan_broken_links(vault, lookup)
    print(f"  Found: {len(broken)}")

    fixed = []
    converted = []
    failed = []

    for src, target in broken:
        src_path = vault / src

        # 指向 _reports/ 和 _meta/journal/ 的链接转为纯文本（这些是报告/日志，不属于核心知识图）
        if target.lower().startswith('_reports/') or target.lower().startswith('_meta/journal/'):
            if fix_link_in_file(src_path, target, '', 'text'):
                converted.append((src, target, 'report/journal link'))
            else:
                failed.append((src, target, 'pattern not found'))
            continue

        matched, confidence = find_best_match(target, lookup)
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

    remaining = scan_broken_links(vault, lookup)

    print(f"Fixed: {len(fixed)}")
    print(f"Converted: {len(converted)}")
    print(f"Failed: {len(failed)}")
    print(f"Remaining: {len(remaining)}")

    # 报告
    report_out = vault / '_reports/broken-links-round3-fix-2026-06-26.md'
    lines = [
        "---",
        "title: 第三轮 Broken Wikilinks 修复报告（2026-06-26）",
        "description: 修复剩余 147 个 broken wikilinks",
        "category: reports",
        "tags:",
        "- wiki-lint",
        "- broken-links",
        "created: \"2026-06-26\"",
        "updated: \"2026-06-26\"",
        "---",
        "",
        "# 第三轮 Broken Wikilinks 修复报告",
        "",
        f"- 初始剩余: {len(broken)}",
        f"- 成功修复: {len(fixed)}",
        f"- 转纯文本: {len(converted)}",
        f"- 失败: {len(failed)}",
        f"- 修复后剩余: {len(remaining)}",
        "",
        "## Fixed",
        "",
        "| Source | Original | Replacement | Confidence |",
        "|---|---|---|---|",
    ]
    for src, target, matched, confidence in fixed[:100]:
        lines.append(f"| `{src}` | `[[{target}]]` | `[[{matched}]]` | {confidence} |")

    lines.extend([
        "",
        "## Converted",
        "",
        "| Source | Original | Reason |",
        "|---|---|---|",
    ])
    for src, target, reason in converted[:50]:
        lines.append(f"| `{src}` | `[[{target}]]` | {reason} |")

    lines.extend([
        "",
        "## Remaining",
        "",
        "| Source | Original |",
        "|---|---|",
    ])
    for src, target in remaining:
        lines.append(f"| `{src}` | `[[{target}]]` |")

    report_out.write_text('\n'.join(lines), encoding='utf-8')
    print(f"\nReport written: {report_out}")


if __name__ == "__main__":
    main()
