#!/usr/bin/env python3
"""
Wiki Lint 健康审计脚本（优化版）。
只扫描核心内容目录，避免 O(n^2) 操作。
"""

import re
from pathlib import Path
from datetime import datetime, timedelta
import yaml


def extract_frontmatter(text: str):
    fm_match = re.search(r'^---\n(.*?)\n---', text, re.DOTALL)
    if not fm_match:
        return None, text
    return fm_match.group(1), text[fm_match.end():]


def parse_frontmatter(fm_text: str) -> dict:
    try:
        return yaml.safe_load(fm_text) or {}
    except Exception:
        return {}


def is_core_page(rel: str) -> bool:
    """只扫描核心内容目录。"""
    excluded_prefixes = (
        '_archives/', '.git/', '.venv/', '.ruff_cache/', '.obsidian/',
        '_reports/', '_meta/', '_raw/', '_staging/', '.claude/', '.codebuddy/',
        '.comate/', '.qoder/', '.understand-anything/', '.zread/',
        'web/node_modules/', 'node_modules/',
    )
    return not rel.startswith(excluded_prefixes)


def build_index(vault: Path):
    md_files = [p for p in vault.rglob('*.md') if is_core_page(str(p.relative_to(vault)))]
    pages = {}
    for p in md_files:
        rel = str(p.relative_to(vault))
        text = p.read_text(encoding='utf-8', errors='ignore')
        fm_text, body = extract_frontmatter(text)
        fm = parse_frontmatter(fm_text) if fm_text else {}
        pages[rel] = {
            'path': p,
            'frontmatter': fm,
            'body': body,
            'text': text,
            'title': fm.get('title', p.stem),
            'tags': fm.get('tags', []) or [],
            'updated': fm.get('updated') or fm.get('last_updated'),
            'created': fm.get('created'),
            'category': fm.get('category', ''),
        }
    return pages


def check_orphans(pages: dict) -> list:
    """检查 orphan 页面（0 入链）。"""
    incoming = {rel: 0 for rel in pages}

    # 构建快速查找表
    path_to_rel = {}
    for rel in pages:
        path_to_rel[rel.lower()] = rel
        path_to_rel[rel.lower()[:-3]] = rel  # without .md
        path_to_rel[Path(rel).name.lower()] = rel
        path_to_rel[Path(rel).stem.lower()] = rel

    for rel, info in pages.items():
        links = re.findall(r'\[\[([^\]|]+)(?:\|[^\]]*)?\]\]', info['text'])
        for link in links:
            target = link.split('#')[0].split('?')[0].strip().lower()
            if target in path_to_rel:
                incoming[path_to_rel[target]] += 1
            elif '/' in target:
                # 尝试 basename
                basename = target.split('/')[-1]
                if basename in path_to_rel:
                    incoming[path_to_rel[basename]] += 1

    orphans = [(rel, info['title']) for rel, info in pages.items() if incoming[rel] == 0]
    return orphans


def check_broken_links(pages: dict) -> list:
    """检查 broken wikilink。"""
    broken = []
    valid_names = set()
    for rel in pages:
        valid_names.add(rel.lower())
        valid_names.add(rel.lower()[:-3])
        valid_names.add(Path(rel).stem.lower())
        valid_names.add(Path(rel).name.lower())

    for rel, info in pages.items():
        links = re.findall(r'\[\[([^\]|]+)(?:\|[^\]]*)?\]\]', info['text'])
        for link in links:
            target = link.split('#')[0].split('?')[0].strip()

            # TOML 数组语法跳过
            if re.fullmatch(r"[a-zA-Z0-9_.-]+", target):
                continue
            # URL 跳过
            if target.startswith('http'):
                continue

            target_lower = target.lower()
            exists = target_lower in valid_names
            if not exists and '/' in target_lower:
                basename = target_lower.split('/')[-1]
                exists = basename in valid_names

            if not exists:
                broken.append((rel, link))

    return broken


def check_missing_frontmatter(pages: dict) -> list:
    required = ['title', 'category', 'tags', 'created']
    missing = []
    for rel, info in pages.items():
        fm = info['frontmatter']
        if not fm:
            missing.append((rel, 'no frontmatter'))
            continue
        absent = [f for f in required if f not in fm]
        if absent:
            missing.append((rel, absent))
    return missing


def check_missing_summary(pages: dict) -> list:
    missing = []
    for rel, info in pages.items():
        fm = info['frontmatter']
        if 'summary' not in fm:
            missing.append((rel, 'no summary'))
        elif isinstance(fm.get('summary'), str) and len(fm['summary']) > 200:
            missing.append((rel, 'summary too long'))
    return missing


def check_stale(pages: dict) -> list:
    cutoff = datetime.now() - timedelta(days=90)
    stale = []
    for rel, info in pages.items():
        updated_str = info['updated']
        if not updated_str:
            continue
        try:
            updated = datetime.fromisoformat(str(updated_str).replace('Z', '+00:00'))
            if updated < cutoff:
                stale.append((rel, info['title'], updated.strftime('%Y-%m-%d')))
        except Exception:
            pass
    return stale


def check_fragmented_tags(pages: dict) -> list:
    tag_pages = {}
    for rel, info in pages.items():
        for tag in info['tags']:
            if isinstance(tag, str):
                tag_pages.setdefault(tag, []).append(rel)

    fragmented = []
    for tag, rels in tag_pages.items():
        if len(rels) < 5:
            continue
        n = len(rels)
        # 只采样检查链接
        sample_size = min(n, 50)
        sample = rels[:sample_size]
        actual_links = 0
        checked = 0
        for i in range(len(sample)):
            for j in range(i + 1, len(sample)):
                checked += 1
                target_j = sample[j][:-3] if sample[j].endswith('.md') else sample[j]
                target_i = sample[i][:-3] if sample[i].endswith('.md') else sample[i]
                text_i = pages[sample[i]]['text']
                if target_j in text_i:
                    actual_links += 1
                    continue
                text_j = pages[sample[j]]['text']
                if target_i in text_j:
                    actual_links += 1
        max_links = sample_size * (sample_size - 1) / 2
        cohesion = actual_links / max_links if max_links > 0 else 0
        if cohesion < 0.15:
            fragmented.append((tag, n, cohesion))

    return fragmented


def check_typed_relationships(pages: dict) -> list:
    allowed_types = {'extends', 'implements', 'contradicts', 'derived_from', 'uses', 'replaces', 'related_to'}
    issues = []

    valid_paths_lower = {p.lower() for p in pages.keys()}
    path_basename = {}
    for p in pages.keys():
        path_basename[Path(p).stem.lower()] = p
        path_basename[Path(p).name.lower()] = p

    for rel, info in pages.items():
        fm = info['frontmatter']
        relationships = fm.get('relationships', []) or []
        if not relationships:
            continue
        for idx, entry in enumerate(relationships):
            if not isinstance(entry, dict):
                continue
            rel_type = entry.get('type', '')
            target = entry.get('target', '')

            if rel_type not in allowed_types:
                issues.append((rel, idx, f'invalid type: {rel_type}'))

            target_clean = target.strip('[]').strip()
            target_lower = target_clean.lower()
            exists = False
            if target_clean in pages:
                exists = True
            elif target_lower in valid_paths_lower:
                exists = True
            elif target_lower + '.md' in valid_paths_lower:
                exists = True
            elif target_lower in path_basename:
                exists = True

            if not exists:
                issues.append((rel, idx, f'broken target: {target}'))

            if target_clean.lower() == rel.lower() or target_clean.lower() == rel.lower()[:-3]:
                issues.append((rel, idx, 'self-reference'))

    return issues


def main():
    vault = Path('/Users/allengaller/Documents/GitHub/kudig-io/kudig-database')
    print("Building page index...")
    pages = build_index(vault)
    print(f"  Indexed {len(pages)} core pages")

    print("\nRunning lint checks...")

    orphans = check_orphans(pages)
    print(f"  Orphans: {len(orphans)}")

    broken = check_broken_links(pages)
    print(f"  Broken links: {len(broken)}")

    missing_fm = check_missing_frontmatter(pages)
    print(f"  Missing frontmatter: {len(missing_fm)}")

    missing_summary = check_missing_summary(pages)
    print(f"  Missing summary: {len(missing_summary)}")

    stale = check_stale(pages)
    print(f"  Stale pages: {len(stale)}")

    fragmented = check_fragmented_tags(pages)
    print(f"  Fragmented tags: {len(fragmented)}")

    rel_issues = check_typed_relationships(pages)
    print(f"  Typed relationship issues: {len(rel_issues)}")

    # 生成报告
    report_path = vault / '_reports/wiki-lint-audit-2026-06-26.md'
    lines = [
        "---",
        "title: Wiki Lint 健康审计报告（2026-06-26）",
        "description: KUDIG Database 核心内容健康检查",
        "category: reports",
        "tags:",
        "- wiki-lint",
        "- audit",
        "- health",
        "created: \"2026-06-26\"",
        "updated: \"2026-06-26\"",
        "---",
        "",
        "# Wiki Lint 健康审计报告",
        "",
        f"- **扫描核心页面数**: {len(pages)}",
        f"- **Orphan 页面**: {len(orphans)}",
        f"- **Broken wikilinks**: {len(broken)}",
        f"- **Missing frontmatter**: {len(missing_fm)}",
        f"- **Missing/invalid summary**: {len(missing_summary)}",
        f"- **Stale pages (≥90 days)**: {len(stale)}",
        f"- **Fragmented tag clusters**: {len(fragmented)}",
        f"- **Typed relationship issues**: {len(rel_issues)}",
        "",
        "## Orphaned Pages",
        "",
    ]

    for rel, title in orphans[:50]:
        lines.append(f"- `{rel}` — {title}")
    if len(orphans) > 50:
        lines.append(f"- ... and {len(orphans) - 50} more")
    lines.append("")

    lines.append("## Broken Wikilinks")
    lines.append("")
    for rel, link in broken[:50]:
        lines.append(f"- `{rel}` — `[[{link}]]`")
    if len(broken) > 50:
        lines.append(f"- ... and {len(broken) - 50} more")
    lines.append("")

    lines.append("## Missing Frontmatter")
    lines.append("")
    for rel, issue in missing_fm[:30]:
        lines.append(f"- `{rel}` — {issue}")
    if len(missing_fm) > 30:
        lines.append(f"- ... and {len(missing_fm) - 30} more")
    lines.append("")

    lines.append("## Missing/Invalid Summary")
    lines.append("")
    for rel, issue in missing_summary[:30]:
        lines.append(f"- `{rel}` — {issue}")
    if len(missing_summary) > 30:
        lines.append(f"- ... and {len(missing_summary) - 30} more")
    lines.append("")

    lines.append("## Stale Pages")
    lines.append("")
    for rel, title, date in stale[:30]:
        lines.append(f"- `{rel}` — {title} (last updated {date})")
    if len(stale) > 30:
        lines.append(f"- ... and {len(stale) - 30} more")
    lines.append("")

    lines.append("## Fragmented Tag Clusters")
    lines.append("")
    for tag, n, cohesion in fragmented[:20]:
        lines.append(f"- `#{tag}` — {n} pages, cohesion={cohesion:.2f}")
    lines.append("")

    lines.append("## Typed Relationship Issues")
    lines.append("")
    for rel, idx, issue in rel_issues[:30]:
        lines.append(f"- `{rel}` — relationships[{idx}]: {issue}")
    if len(rel_issues) > 30:
        lines.append(f"- ... and {len(rel_issues) - 30} more")
    lines.append("")

    report_path.write_text('\n'.join(lines), encoding='utf-8')
    print(f"\nReport written: {report_path}")


if __name__ == "__main__":
    main()
