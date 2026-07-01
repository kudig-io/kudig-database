#!/usr/bin/env python3
"""
扫描全库 broken wikilinks 并自动修复。
"""

import re
import difflib
from pathlib import Path
from collections import defaultdict


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
    )
    return rel.startswith(excluded)


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
    matches = difflib.get_close_matches(target_lower, keys, n=3, cutoff=0.85)
    if matches:
        best = matches[0]
        if len(matches) == 1:
            return lookup[best], 'fuzzy'
        score1 = difflib.SequenceMatcher(None, target_lower, best).ratio()
        score2 = difflib.SequenceMatcher(None, target_lower, matches[1]).ratio()
        if score1 - score2 > 0.1:
            return lookup[best], 'fuzzy'

    return None, None


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

            # TOML 数组跳过
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
                basename = target_lower.split('/')[-1]
                exists = basename in lookup

            if not exists:
                broken.append((rel, target))

    return broken


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

    print("Building page lookup...")
    lookup = build_lookup(vault)
    print(f"  Lookup entries: {len(lookup)}")

    print("\nScanning broken links...")
    broken = scan_broken_links(vault, lookup)
    print(f"  Broken links found: {len(broken)}")

    fixed = []
    converted = []
    failed = []

    for src, target in broken:
        src_path = vault / src
        if not src_path.exists():
            failed.append((src, target, 'source not found'))
            continue

        matched, confidence = find_best_match(target, lookup)
        if matched:
            success = fix_link_in_file(src_path, target, matched, 'link')
            if success:
                fixed.append((src, target, matched, confidence))
            else:
                failed.append((src, target, 'pattern not found'))
        else:
            success = fix_link_in_file(src_path, target, '', 'text')
            if success:
                converted.append((src, target))
            else:
                failed.append((src, target, 'pattern not found'))

    print(f"\nFixed: {len(fixed)}")
    print(f"Converted to text: {len(converted)}")
    print(f"Failed: {len(failed)}")

    # 生成报告
    report_out = vault / '_reports/broken-links-full-fix-2026-06-26.md'
    lines = [
        "---",
        "title: 全库 Broken Wikilinks 修复报告（2026-06-26）",
        "description: 扫描全库并自动修复 broken wikilinks",
        "category: reports",
        "tags:",
        "- wiki-lint",
        "- broken-links",
        "- maintenance",
        "created: \"2026-06-26\"",
        "updated: \"2026-06-26\"",
        "---",
        "",
        "# 全库 Broken Wikilinks 修复报告",
        "",
        f"- 总 broken links: {len(broken)}",
        f"- 成功修复: {len(fixed)}",
        f"- 转纯文本: {len(converted)}",
        f"- 失败/跳过: {len(failed)}",
        "",
        "## Fixed Links",
        "",
        "| Source | Original | Replacement | Confidence |",
        "|---|---|---|---|",
    ]
    for src, target, matched, confidence in fixed[:100]:
        lines.append(f"| `{src}` | `[[{target}]]` | `[[{matched}]]` | {confidence} |")
    if len(fixed) > 100:
        lines.append(f"\n*还有 {len(fixed) - 100} 个修复未列出*")

    lines.extend([
        "",
        "## Converted to Plain Text",
        "",
        "| Source | Original |",
        "|---|---|",
    ])
    for src, target in converted[:50]:
        lines.append(f"| `{src}` | `[[{target}]]` |")
    if len(converted) > 50:
        lines.append(f"\n*还有 {len(converted) - 50} 个未列出*")

    lines.extend([
        "",
        "## Failed/Skipped",
        "",
        "| Source | Original | Reason |",
        "|---|---|---|",
    ])
    for src, target, reason in failed[:30]:
        lines.append(f"| `{src}` | `[[{target}]]` | {reason} |")

    report_out.write_text('\n'.join(lines), encoding='utf-8')
    print(f"\nReport written: {report_out}")


if __name__ == "__main__":
    main()
