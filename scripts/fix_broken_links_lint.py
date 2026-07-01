#!/usr/bin/env python3
"""
根据 wiki-lint 报告自动修复 broken wikilinks。
策略：
1. 精确匹配 → 直接替换路径
2. 标题匹配 → 替换为标题对应页面
3. 模糊匹配 → 编辑距离 ≤2 且唯一最佳
4. 无法匹配 → 转换为纯文本
"""

import re
import difflib
from pathlib import Path


def normalize(s: str) -> str:
    return s.lower().strip().replace(' ', '-').replace('_', '-')


def build_lookup(vault: Path) -> dict:
    """构建页面查找表：多种 key → rel。"""
    lookup = {}
    for p in vault.rglob('*.md'):
        rel = str(p.relative_to(vault))
        text = p.read_text(encoding='utf-8', errors='ignore')

        # 路径变体
        lookup[rel.lower()] = rel
        lookup[rel.lower()[:-3]] = rel
        lookup[Path(rel).stem.lower()] = rel
        lookup[Path(rel).name.lower()] = rel
        lookup[normalize(Path(rel).stem)] = rel

        # 标题
        m = re.search(r'^title:\s*["\']?(.+?)["\']?$', text, re.MULTILINE)
        if m:
            title = m.group(1).strip()
            lookup[title.lower()] = rel
            lookup[normalize(title)] = rel

    return lookup


def find_best_match(target: str, lookup: dict) -> tuple:
    """返回 (matched_rel, confidence)。"""
    target_lower = target.lower().strip()
    target_norm = normalize(target)

    # 1. 精确匹配
    if target_lower in lookup:
        return lookup[target_lower], 'exact'
    if target_norm in lookup:
        return lookup[target_norm], 'exact'

    # 2. basename 匹配
    if '/' in target_lower:
        basename = target_lower.split('/')[-1]
        if basename in lookup:
            return lookup[basename], 'basename'
        basename_norm = normalize(basename)
        if basename_norm in lookup:
            return lookup[basename_norm], 'basename'

    # 3. 模糊匹配（只针对短目标）
    if len(target_lower) < 3:
        return None, None

    keys = list(lookup.keys())
    matches = difflib.get_close_matches(target_lower, keys, n=3, cutoff=0.85)
    if matches:
        best = matches[0]
        # 检查第二佳是否明显差
        if len(matches) == 1:
            return lookup[best], 'fuzzy'
        score1 = difflib.SequenceMatcher(None, target_lower, best).ratio()
        score2 = difflib.SequenceMatcher(None, target_lower, matches[1]).ratio()
        if score1 - score2 > 0.1:
            return lookup[best], 'fuzzy'

    return None, None


def parse_lint_report(report_path: Path):
    """解析 lint 报告中的 broken links。"""
    text = report_path.read_text(encoding='utf-8')
    # 找到 Broken Wikilinks 部分
    start = text.find('## Broken Wikilinks')
    end = text.find('## Missing Frontmatter')
    if start == -1 or end == -1:
        return []

    section = text[start:end]
    broken = []
    for line in section.splitlines():
        line = line.strip()
        if not line.startswith('-'):
            continue
        # 格式: - `source` — `[[target]]`
        parts = line.split(' — `[[', 1)
        if len(parts) != 2:
            continue
        src_part = parts[0].strip('- `')
        tgt_part = parts[1].strip('`]').strip()
        broken.append((src_part, tgt_part))

    return broken


def fix_link_in_file(src_path: Path, target: str, replacement: str, mode: str) -> bool:
    """在源文件中修复一个 broken link。"""
    text = src_path.read_text(encoding='utf-8')
    original = text

    # 匹配 [[target|display]] 或 [[target]]
    pattern = re.compile(rf'\[\[{re.escape(target)}(?:\|([^\]]*))?\]\]')

    def repl(match):
        display = match.group(1)
        if mode == 'text':
            return display if display else target
        else:
            if display:
                return f'[[{replacement}|{display}]]'
            else:
                # 如果 replacement 包含斜杠且没有 display，使用 replacement 的 basename 作为 display
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
    report_path = vault / '_reports/wiki-lint-audit-2026-06-26.md'

    print("Building page lookup...")
    lookup = build_lookup(vault)
    print(f"  Lookup entries: {len(lookup)}")

    print("\nParsing broken links...")
    broken = parse_lint_report(report_path)
    print(f"  Broken links: {len(broken)}")

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
                failed.append((src, target, 'pattern not found in file'))
        else:
            success = fix_link_in_file(src_path, target, '', 'text')
            if success:
                converted.append((src, target))
            else:
                failed.append((src, target, 'pattern not found in file'))

    print(f"\nFixed: {len(fixed)}")
    print(f"Converted to text: {len(converted)}")
    print(f"Failed: {len(failed)}")

    # 生成报告
    report_out = vault / '_reports/broken-links-fix-2026-06-26.md'
    lines = [
        "---",
        "title: Broken Wikilinks 修复报告（2026-06-26）",
        "description: 根据 wiki-lint 审计自动修复 broken wikilinks",
        "category: reports",
        "tags:",
        "- wiki-lint",
        "- broken-links",
        "- maintenance",
        "created: \"2026-06-26\"",
        "updated: \"2026-06-26\"",
        "---",
        "",
        "# Broken Wikilinks 修复报告",
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
        lines.append(f"| ... | ... | ... | ... |")
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
        lines.append(f"| ... | ... |")
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
