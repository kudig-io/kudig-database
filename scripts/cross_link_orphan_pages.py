#!/usr/bin/env python3
"""
为指定目录中的 orphan 页面添加交叉链接。
默认处理 _reports/ 和 domain-11-production-operations/ticket-cases/ 中的 orphan 页面。
"""

import re
from pathlib import Path
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


def build_registry(vault: Path):
    registry = {}
    md_files = [p for p in vault.rglob('*.md') if not str(p.relative_to(vault)).startswith(('_archives/', '.git/', '.venv/', '.ruff_cache/', '.obsidian/'))]

    for p in md_files:
        rel = str(p.relative_to(vault))
        text = p.read_text(encoding='utf-8', errors='ignore')
        fm_text, _ = extract_frontmatter(text)
        fm = parse_frontmatter(fm_text) if fm_text else {}

        title = fm.get('title', p.stem)
        aliases = fm.get('aliases', []) or []
        tags = fm.get('tags', []) or []
        category = fm.get('category', '') or str(p.parent.relative_to(vault)).split('/')[0]

        names = [p.stem, title] + aliases
        for name in names:
            key = name.lower().strip()
            if key and len(key) > 2 and key not in registry:
                registry[key] = {
                    'path': rel,
                    'title': title,
                    'aliases': aliases,
                    'tags': tags,
                    'category': category,
                }
    return registry, md_files


def count_incoming_links(md_files: list, vault: Path) -> dict:
    incoming = {}
    path_map = {}

    for p in md_files:
        rel = str(p.relative_to(vault))
        incoming[rel] = 0
        path_map[rel.lower()] = rel
        path_map[rel.lower()[:-3]] = rel  # without .md

    for p in md_files:
        text = p.read_text(encoding='utf-8', errors='ignore')
        links = re.findall(r'\[\[([^\]|]+)(?:\|[^\]]*)?\]\]', text)
        for link in links:
            target = link.split('#')[0].split('?')[0].strip().lower()
            if target in path_map:
                incoming[path_map[target]] += 1
            else:
                # 尝试 basename
                for rel_lower, rel in list(path_map.items()):
                    if '/' in rel_lower and rel_lower.endswith('/' + target):
                        incoming[rel] += 1
                        break

    return incoming


def find_link_candidates(page_path: Path, title: str, tags: list, registry: dict, vault: Path, top_n: int = 8) -> list:
    title_lower = title.lower()
    candidates = []
    page_cat = str(page_path.parent.relative_to(vault)).split('/')[0]

    # 关键词扩展
    keywords = set(title_lower.replace('-', ' ').replace('_', ' ').split())
    tags_normalized = [str(t).lower() for t in (tags or [])]
    keywords.update(tags_normalized)
    keywords = {k for k in keywords if len(k) > 2}

    for key, info in registry.items():
        if info['path'] == str(page_path.relative_to(vault)):
            continue
        score = 0

        # 标题中的关键词匹配
        if key in title_lower:
            score += 4
        elif any(kw in key for kw in keywords if kw in key):
            score += 1

        # 关键词出现在目标标题中
        target_title_lower = info['title'].lower()
        for kw in keywords:
            if kw in target_title_lower:
                score += 1
                break

        # 共享标签
        shared_tags = set(tags or []) & set(info.get('tags', []) or [])
        if len(shared_tags) >= 2:
            score += 2
        elif len(shared_tags) == 1:
            score += 1

        # 同 category
        if info.get('category', '').split('/')[0] == page_cat:
            score += 1

        if score >= 3:
            candidates.append((score, info))

    # 去重排序
    seen = set()
    unique = []
    for score, info in sorted(candidates, key=lambda x: -x[0]):
        if info['path'] not in seen:
            unique.append((score, info))
            seen.add(info['path'])

    return unique[:top_n]


def determine_relationship_type(source_title: str, target_title: str, shared_tags: set) -> str:
    if shared_tags:
        return 'related_to'
    return 'related_to'


def add_related_section(page_path: Path, links: list, vault: Path):
    text = page_path.read_text(encoding='utf-8')
    fm_text, body = extract_frontmatter(text)

    related_lines = ["\n## Related\n"]
    for score, info in links:
        target_path = info['path']
        target_title = info['title']
        if '/' in target_path:
            wikilink = f"[[{target_path}|{target_title}]]"
        else:
            wikilink = f"[[{target_path[:-3]}|{target_title}]]"
        related_lines.append(f"- {wikilink}")

    if '## Related' in body:
        # 找到 Related 章节末尾追加
        parts = body.split('## Related', 1)
        existing = parts[1]
        next_section = existing.find('\n## ')
        if next_section > 0:
            new_body = parts[0] + '## Related' + existing[:next_section] + '\n'.join(related_lines[2:]) + '\n' + existing[next_section:]
        else:
            new_body = parts[0] + '## Related' + existing + '\n'.join(related_lines[2:]) + '\n'
    else:
        new_body = body.rstrip() + '\n' + '\n'.join(related_lines) + '\n'

    new_text = f"---\n{fm_text}\n---{new_body}"
    page_path.write_text(new_text, encoding='utf-8')


def update_relationships_frontmatter(page_path: Path, links: list, vault: Path):
    text = page_path.read_text(encoding='utf-8')
    fm_text, body = extract_frontmatter(text)
    if not fm_text:
        return

    fm = parse_frontmatter(fm_text)
    source_tags = set(fm.get('tags', []) or [])

    relationships = fm.get('relationships', []) or []
    existing_targets = {r.get('target', '') for r in relationships}

    for score, info in links:
        target_wikilink = f"[[{info['path']}]]"
        if target_wikilink in existing_targets:
            continue
        shared_tags = source_tags & set(info.get('tags', []) or [])
        rel_type = determine_relationship_type(fm.get('title', ''), info['title'], shared_tags)
        relationships.append({
            'target': target_wikilink,
            'type': rel_type
        })
        existing_targets.add(target_wikilink)

    if relationships:
        fm['relationships'] = relationships
        new_fm = yaml.safe_dump(fm, allow_unicode=True, sort_keys=False, default_flow_style=False)
        new_text = f"---\n{new_fm}---{body}"
        page_path.write_text(new_text, encoding='utf-8')


def main():
    vault = Path('/Users/allengaller/Documents/GitHub/kudig-io/kudig-database')

    target_dirs = [
        vault / '_reports',
        vault / 'domain-11-production-operations/ticket-cases',
    ]

    print("Building page registry...")
    registry, md_files = build_registry(vault)
    print(f"  Registry size: {len(registry)}")
    print(f"  Total pages: {len(md_files)}")

    print("\nCounting incoming links...")
    incoming = count_incoming_links(md_files, vault)

    target_orphans = []
    for d in target_dirs:
        if not d.exists():
            continue
        for p in d.rglob('*.md'):
            rel = str(p.relative_to(vault))
            if incoming.get(rel, 0) == 0:
                target_orphans.append(p)

    print(f"\nTarget orphan pages: {len(target_orphans)}")

    links_added = 0
    pages_modified = 0
    details = []

    for p in target_orphans:
        text = p.read_text(encoding='utf-8', errors='ignore')
        fm_text, _ = extract_frontmatter(text)
        fm = parse_frontmatter(fm_text) if fm_text else {}
        title = fm.get('title', p.stem)
        tags = fm.get('tags', []) or []

        candidates = find_link_candidates(p, title, tags, registry, vault)
        extracted = [(s, i) for s, i in candidates if s >= 6]
        inferred = [(s, i) for s, i in candidates if 3 <= s < 6]
        selected = extracted + inferred[:3]

        if selected:
            add_related_section(p, selected, vault)
            update_relationships_frontmatter(p, selected, vault)
            links_added += len(selected)
            pages_modified += 1
            confidence = 'EXTRACTED' if extracted else 'INFERRED'
            details.append((str(p.relative_to(vault)), len(selected), confidence, [i['path'] for _, i in selected]))

    print(f"\nLinks added: {links_added}")
    print(f"Pages modified: {pages_modified}")

    # 写入报告
    report_path = vault / '_reports/cross-linker-targeted-2026-06-26.md'
    lines = [
        "---",
        "title: 目标目录 Orphan 页面 Cross-Link 报告（2026-06-26）",
        "description: 为 _reports 和 ticket-cases 中的 orphan 页面添加交叉链接",
        "category: reports",
        "tags:",
        "- cross-linker",
        "- wiki-maintenance",
        "created: \"2026-06-26\"",
        "updated: \"2026-06-26\"",
        "---",
        "",
        "# 目标目录 Orphan 页面 Cross-Link 报告",
        "",
        f"- 扫描页面总数: {len(md_files)}",
        f"- 目标 orphan 页面: {len(target_orphans)}",
        f"- 修改页面数: {pages_modified}",
        f"- 新增链接数: {links_added}",
        "",
        "## Details",
        "",
        "| Page | Links Added | Confidence | Targets |",
        "|---|---|---|---|",
    ]
    for rel, count, conf, targets in details:
        target_str = ', '.join(targets[:3]) + ('...' if len(targets) > 3 else '')
        lines.append(f"| `{rel}` | {count} | {conf} | {target_str} |")
    report_path.write_text('\n'.join(lines), encoding='utf-8')
    print(f"\nReport written: {report_path}")


if __name__ == "__main__":
    main()
